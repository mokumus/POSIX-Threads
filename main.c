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
};

//Globals
char students_file[MAX_NAME], homeworks_file[MAX_NAME];
int fd_students, fd_homeworks, money;

char *jobs;
struct Student *student_data;

// Function Prototypes

// Parsing and printing.
void print_usage(void);
int lines(char *filename); // Get number of non-empty lines in a file
char *read_line(int n);

// Workers
void *student(void *data);

// Wrappers
int s_wait(sem_t *sem);
int s_post(sem_t *sem);
int s_init(sem_t *sem, int val);

int main(int argc, char *argv[])
{
  int n_students;

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

    switch (i)
    {
    case 1:
      snprintf(students_file, MAX_NAME, "%s", argv[i]);
      break;
    case 2:
      snprintf(homeworks_file, MAX_NAME, "%s", argv[i]);
      break;
    case 3:
      money = atoi(argv[i]);
      break;
    default:
      errExit("Logic error while parsing");
      break;
    }
  }
  fd_homeworks = open(homeworks_file, O_RDONLY);

  if (fd_homeworks == -1)
    errExit("open");

  // Read student data from file
  
  // 1. Get number of students in the file
  n_students = lines(homeworks_file);
 
  // 2. Allocate Students struct array
  struct Student *students = malloc(n_students * sizeof(struct Student));
  
  // 3. Populate students array & create threads
  printf(BOLDBLUE "=================================================\n" RESET);
  printf(BOLDYELLOW "%d students-for-hire threads have been created.\n" RESET, n_students);
  printf(BOLDBLUE "=================================================\n" RESET);
  printf(BOLDBLUE "%-15s%-7s%-7s%-7s\n" RESET, "Name", "Q", "S", "C");

  for (int i = 0; i < n_students; i++)
  {

    sscanf(read_line(i), "%s %d %d %d", students[i].name, 
                                        &students[i].quality, 
                                        &students[i].speed,
                                        &students[i].price);

    printf(BOLDWHITE "%-15s%-7d%-7d%-7d\n" RESET, students[i].name, students[i].quality, students[i].speed, students[i].price);
  }
  printf(BOLDBLUE "=================================================\n" RESET);

  // Initilize Thread Data

  // Do Cheater Student Things

  // Free resources
  free(students);
  close(fd_homeworks);

  return 0;
}

void *student(void *data)
{
  printf("I am student\n");
  pthread_exit(NULL);
}

int lines(char *filename)
{
  char c, last = '\n';
  int i = 0, lines = 0;

  while (pread(fd_homeworks, &c, 1, i++))
  {
    if (c == '\n' && last != '\n')
      lines++;
    last = c;
  }
  if (last != '\n')
    lines++;

  return lines;
}

char *read_line(int n)
{
  int line = 0, i = 0, k = 0;
  char c;
  static char buffer[MAX_NAME];
  while (pread(fd_homeworks, &c, 1, i++) && line <= n)
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