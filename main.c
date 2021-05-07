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

// Function Prototypes

// Valdiation and printing.
void print_usage(void);

// Workers
void *student(void *data);

// Wrappers
int s_wait(sem_t *sem);
int s_post(sem_t *sem);
int s_init(sem_t *sem, int val);

int main(int argc, char *argv[]){

  // Input parsing & validation =======================
  if(argc < 4){
    print_usage();
    errExit("Missing parameters");
  }

  if(argc > 4){
    print_usage();
    errExit("Too many parameters");
  }
  
  for(int i = 1; i < 4; i++){
    
    switch (i)
    {
    case 1:
      snprintf(students_file, MAX_NAME, "%s", argv[i]); break;
    case 2:
      snprintf(homeworks_file, MAX_NAME, "%s", argv[i]); break;
    case 3:
      money = atoi(argv[i]); break;
    default:
      errExit("Logic error while parsing");
      break;
    }
  }

  // Initilize Thread Data


  // Do Cheater Student Things

  return 0;
}

void *student(void *data){  
  printf("I am student\n");
  pthread_exit(NULL);
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