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
  int price;

  //status variables
  int homeworks_done;
  int money_made;
  int is_busy;
};

//Globals
char students_file[MAX_NAME], homeworks_file[MAX_NAME];
int fd_students, fd_homeworks, money, n_students, jobs_done, jobs_given, max_jobs;

char *jobs;
struct Student *student_data;
pthread_t *thread_ids;
struct Student *students;

sig_atomic_t exit_requested = 0;
sem_t sem_printf;
sem_t sem_access;
sem_t sem_nstored;
sem_t sem_nempty;
// Function Prototypes

// Parsing and printing.
void print_usage(void);
int lines(int fd); // Get number of non-empty lines in a file
int chars(int fd);
char *read_line(int fd, int n);
void tsprintf(const char *format, ...);

// Workers
void *student(void *data);
void cheater(int fd);

// Worker helpers
int have_enough_money(void);

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
  thread_ids = malloc(n_students * sizeof(*thread_ids));
  jobs = malloc(max_jobs * sizeof(*jobs));

  // Initilize semaphores
  s_init(&sem_printf, 1);
  s_init(&sem_access, 1);
  s_init(&sem_nstored, 0);
  s_init(&sem_nempty, max_jobs);
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
           &students[i].price);

    printf(BOLDWHITE "%-15s%-7d%-7d%-7d\n" RESET, students[i].name, students[i].quality, students[i].speed, students[i].price);
  }
  printf(BOLDBLUE "=================================================\n" RESET);
  for (int i = 0; i < n_students; i++)
    pthread_create(&thread_ids[i], NULL, student, &students[i]);

  // Do Cheater Student Things
  cheater(fd_homeworks);

  // Join threads and free resources =================================
  for (int i = 0; i < n_students; i++)
    pthread_join(thread_ids[i], NULL);

  sem_destroy(&sem_printf);
  sem_destroy(&sem_access);
  sem_destroy(&sem_nstored);
  sem_destroy(&sem_nempty);

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
    tsprintf(BOLDBLUE "%s is waiting for a homework\n" RESET, student->name);
    //s_wait(&sem_nstored);
    //s_wait(&sem_access);

    if (jobs_done >= max_jobs)
    {
      tsprintf(BOLDRED "%s is exitting\n" RESET, student->name);
      s_post(&sem_nstored);
      //s_post(&sem_access);
      return NULL;
    }
    printf(BOLDBLUE "%s is solving homework Q for %d, H has %dTL left.\n" RESET, student->name, student->price, money);
    money -= student->price;
    jobs_done++;
    //s_post(&sem_access);
    s_post(&sem_nempty);
  }

  return NULL;
}

void cheater(int fd)
{
  char c;
  int i = 0;
  while (!exit_requested)
  {

    s_wait(&sem_nempty);
    //s_wait(&sem_access);

    if (jobs_given >= max_jobs)
    {
      tsprintf(BOLDMAGENTA "H has no other homeworks, terminating.\n" RESET);
      //s_post(&sem_nstored);
      s_post(&sem_nempty);
      //s_post(&sem_access);
      return;
    }

    pread(fd, &c, 1, i++);
    jobs_given++;

    // Print new homework priority
    tsprintf(BOLDYELLOW "H has a new homework %c; remaining money is %dTL\n" RESET, c, money);

    
    //s_post(&sem_nstored);
    //s_post(&sem_access);
  }
}

int have_enough_money(void)
{
  s_wait(&sem_access);

  for (int i = 0; i < n_students; i++)
  {
    if (students[i].price <= money)
      return 1;
  }
  s_post(&sem_access);
  return 0;
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
    if (c != '\n')
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