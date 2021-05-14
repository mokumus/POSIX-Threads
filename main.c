//
//  main.c
//  Threads
//
//  Created by Muhammed Okumuş on 07.05.2021.
//  Copyright 2021 Muhammed Okumuş. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>    // getopt(), read(), close()
#include <fcntl.h>     // open(), O_CREAT, O_RDWR, O_RDONLY
#include <pthread.h>   // all the pthread stuff
#include <semaphore.h> // sem_init(), sem_wait(), sem_post(), sem_destroy()
#include <sys/wait.h>  // sig_atomic_t
#include <stdarg.h>

#define MAX_NAME 256
#define MAX_MSG 256

#define RESET "\033[0m"
#define BLACK "\033[30m"              /* Black */
#define RED "\033[31m"                /* Red */
#define GREEN "\033[32m"              /* Green */
#define YELLOW "\033[33m"             /* Yellow */
#define BLUE "\033[34m"               /* Blue */
#define MAGENTA "\033[35m"            /* Magenta */
#define CYAN "\033[36m"               /* Cyan */
#define WHITE "\033[37m"              /* White */
#define BOLDBLACK "\033[1m\033[30m"   /* Bold Black */
#define BOLDRED "\033[1m\033[31m"     /* Bold Red */
#define BOLDGREEN "\033[1m\033[32m"   /* Bold Green */
#define BOLDYELLOW "\033[1m\033[33m"  /* Bold Yellow */
#define BOLDBLUE "\033[1m\033[34m"    /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m" /* Bold Magenta */
#define BOLDCYAN "\033[1m\033[36m"    /* Bold Cyan */
#define BOLDWHITE "\033[1m\033[37m"   /* Bold White */

#define errExit(msg)    \
  do                    \
  {                     \
    perror(msg);        \
    exit(EXIT_FAILURE); \
  } while (0)

struct Student
{
  char name[64];
  int quality;
  int speed;
  int cost;

  //status variables
  int homeworks_done;
  int money_made;
  int is_busy;

  int pipe_fd[2];
};

//Globals
char students_file[MAX_NAME], homeworks_file[MAX_NAME];
int fd_students, fd_homeworks, money, n_students, max_jobs, jobs_read, jobs_assigned;

char *jobs;
struct Student *student_data;
pthread_t *thread_ids;
struct Student *students;

sig_atomic_t exit_requested = 0;
sem_t sem_printf;
sem_t sem_access;
sem_t sem_jobs_read;
// Function Prototypes

// Parsing and printing.
void print_usage(void);
int lines(int fd); // Get number of non-empty lines in a file
int chars(int fd);
char *read_line(int fd, int n);
void tsprintf(const char *format, ...);

// Workers
void *student(void *data);
void *cheater(void *data);
void manager(void);

// Worker helpers
int have_enough_money(void);
int select_student(char hw_type);
int get_value(int i, char key);

// Wrappers
int s_wait(sem_t *sem);
int s_post(sem_t *sem);
int s_init(sem_t *sem, int val);

// Signal handler
void sig_handler(int sig_no);

int main(int argc, char *argv[])
{
  // Input parsing & validation =======================
  if (argc < 4)
  {
    print_usage();
    errExit("Missing parameters");
  }

  if (argc > 4)
  {
    print_usage();
    errExit("Too many parameters");
  }

  for (int i = 1; i < 4; i++)
  {
    if (i == 1)
      snprintf(homeworks_file, MAX_NAME, "%s", argv[i]);

    if (i == 2)
      snprintf(students_file, MAX_NAME, "%s", argv[i]);

    if (i == 3)
      money = atoi(argv[i]);
  }

  setbuf(stdout, NULL);

  // Read student data from file
  fd_students = open(students_file, O_RDONLY);
  if (fd_students == -1)
    errExit("open");

  // Read homeworks data from file
  fd_homeworks = open(homeworks_file, O_RDONLY);
  if (fd_homeworks == -1)
    errExit("open");

  // 1. Get number of students in the file
  n_students = lines(fd_students);
  max_jobs = chars(fd_homeworks);

  // 2. Allocate Students struct array
  students = malloc(n_students * sizeof(*students));
  thread_ids = malloc((n_students + 1) * sizeof(*thread_ids));
  jobs = malloc(max_jobs * sizeof(*jobs));

  // Initilize semaphores
  s_init(&sem_printf, 1);
  s_init(&sem_access, 1);
  s_init(&sem_jobs_read, 0);

  // 3. Populate students array & create threads
  printf(BOLDBLUE "=================================================\n" RESET);
  printf(BOLDYELLOW "%d students-for-hire threads have been created.\n" RESET, n_students);
  printf(BOLDBLUE "=================================================\n" RESET);
  printf(BOLDBLUE "%-15s%-7s%-7s%-7s\n" RESET, "Name", "Q", "S", "C");

  for (int i = 0; i < n_students; i++)
  {
    // Initilize Thread Data
    sscanf(read_line(fd_students, i), "%s %d %d %d", students[i].name,
           &students[i].quality,
           &students[i].speed,
           &students[i].cost);

    if (pipe(students[i].pipe_fd) == -1)
      errExit("pipe");

    printf(BOLDWHITE "%-15s%-7d%-7d%-7d\n" RESET, students[i].name, students[i].quality, students[i].speed, students[i].cost);
  }
  printf(BOLDBLUE "=================================================\n" RESET);

  for (int i = 0; i < n_students; i++)
    pthread_create(&thread_ids[i], NULL, student, &students[i]);

  pthread_create(&thread_ids[n_students], NULL, cheater, NULL);

  manager();

  // Join threads and free resources =================================
  for (int i = 0; i < n_students + 1; i++)
    pthread_join(thread_ids[i], NULL);

  sem_destroy(&sem_printf);
  sem_destroy(&sem_access);
  sem_destroy(&sem_jobs_read);

  free(students);
  free(thread_ids);
  free(jobs);
  close(fd_homeworks);

  return 0;
}

void *student(void *data)
{
  struct Student *student = data;

  while (!exit_requested)
  {
    char msg[MAX_MSG];
    if (read(student->pipe_fd[0], &msg, sizeof(msg)))
    {
      if (msg[0] == 'e')
      {
        close(student->pipe_fd[0]);
        return NULL;
      }

      printf("msg to %s: %s\n", student->name, msg);
    }
  }

  return NULL;
}

void *cheater(void *data)
{
  int i = 0;
  char c;
  while (!exit_requested)
  {
    s_wait(&sem_access);
    if (jobs_read >= max_jobs || !have_enough_money())
    {
      tsprintf(BOLDMAGENTA "H has no %s, terminating.\n" RESET, !have_enough_money() ? "more money for homeworks" : "other homeworks");
      s_post(&sem_jobs_read);
      s_post(&sem_access);
      return NULL;
    }
    // Valid homeworks & byte read
    pread(fd_homeworks, &c, 1, i++);
    if (c == 'Q' || c == 'S' || c == 'C')
    {

      tsprintf(MAGENTA "H has a new homework %c; remaining money is %dTL\n" RESET, c, money);
      jobs[jobs_read++] = c;
      s_post(&sem_jobs_read);
    }
    s_post(&sem_access);
  }

  return NULL;
}

void manager(void)
{
  while (!exit_requested)
  {
    s_wait(&sem_jobs_read);
    s_wait(&sem_access);

    if (jobs_assigned >= max_jobs || !have_enough_money())
    {
      tsprintf(BOLDYELLOW "%s, closing.\n" RESET, !have_enough_money() ? "Money is over" : "No more homeworks left or coming in");
      for (int i = 0; i < n_students; i++)
      {
        char msg[MAX_MSG] = "exit message from manager\n";
        write(students[i].pipe_fd[1], msg, MAX_MSG);
        close(students[i].pipe_fd[1]);
      }
      s_post(&sem_access);
      return;
    }

    int id = select_student(jobs[jobs_assigned]);

    if (id == -1)
      errExit("Logic error");

    char msg[MAX_MSG] = "This is a message from manager\n";

    write(students[id].pipe_fd[1], msg, MAX_MSG);

    money -= students[id].cost;
    tsprintf(YELLOW "M assigning homework %c to %s, money left %dTL\n" RESET, jobs[jobs_assigned++], students[id].name, money);
    s_post(&sem_access);
  }
}

int have_enough_money(void)
{
  //s_wait(&sem_access);

  for (int i = 0; i < n_students; i++)
  {
    //printf("mone: %d\n", money);
    if (students[i].cost <= money)
      return 1;
  }
  //s_post(&sem_access);
  return 0;
}

int select_student(char hw_type)
{
  int ret = -1;
  int val = -1;
  int cost = 999999;
  for (int i = 0; i < n_students; i++)
  {
    int temp_cost = get_value(i, 'C');
    //printf("cost: %d\n", temp_cost);
    if (students[i].is_busy)
      continue;
    else if (hw_type == 'C')
    {
      if (cost > temp_cost)
      {
        cost = temp_cost;
        ret = i;
      }
    }
    else
    {
      int temp = get_value(i, hw_type);
      //printf("temp: %d\n", temp);
      if (val <= temp && cost > temp_cost && money >= temp_cost)
      {
        val = temp;
        ret = i;
      }
      else if (val < temp && money >= temp_cost)
      {
        val = temp;
        ret = i;
      }
    }
  }

  //printf("selected %d for job %c\n", ret, hw_type);
  return ret;
}

int get_value(int i, char key)
{
  if (i >= n_students)
    errExit("Index out of bounds");

  switch (key)
  {
  case 'Q':
    return students[i].quality;
  case 'S':
    return students[i].speed;
  case 'C':
    return students[i].cost;
  default:
    errExit("Invalid key");
    ;
  }
}

int lines(int fd)
{
  char c, last = '\n';
  int i = 0, lines = 0;

  while (pread(fd, &c, 1, i++))
  {

    if (c == '\n' && last != '\n')
      lines++;
    last = c;
  }
  if (last != '\n')
    lines++;

  return lines;
}

int chars(int fd)
{
  char c;
  int i = 0, chars = 0;

  while (pread(fd, &c, 1, i++))
  {
    if (c == 'Q' || c == 'S' || c == 'C')
      chars++;
  }
  return chars;
}

char *read_line(int fd, int n)
{
  int line = 0, i = 0, k = 0;
  char c;
  static char buffer[MAX_NAME];
  while (pread(fd, &c, 1, i++) && line <= n)
  {
    if (line == n)
      buffer[k++] = c;

    if (c == '\n')
    {
      buffer[k - 1] = '\0';
      line++;
    }
  }
  buffer[k] = '\0';
  return buffer;
}

void print_usage(void)
{
  printf("========================================\n");
  printf("Usage:\n"
         "./program homeworkFilePath studentsFilePath [money(int)]\n");
  printf("========================================\n");
}

int s_wait(sem_t *sem)
{
  int ret;
  ret = sem_wait(sem);
  if (ret == -1)
    errExit("s_wait");

  return ret;
}
int s_post(sem_t *sem)
{
  int ret;
  ret = sem_post(sem);
  if (ret == -1)
    errExit("s_post");

  return ret;
}

int s_init(sem_t *sem, int val)
{
  int ret;
  ret = sem_init(sem, 1, val);
  if (ret == -1)
    errExit("s_init");

  return ret;
}

void sig_handler(int sig_no)
{
  exit_requested = sig_no;
}

void tsprintf(const char *format, ...)
{
  va_list args;
  s_wait(&sem_printf);

  va_start(args, format);
  vprintf(format, args);
  va_end(args);

  s_post(&sem_printf);
}