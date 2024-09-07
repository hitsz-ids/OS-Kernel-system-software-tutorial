#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include <stdint.h>  // For uintptr_t

// 共享变量
volatile int count = 0;

// 线程函数
void thread_one_func(void *arg) {

  for (int i=0;i<10;i++)    
    printf("Thread 1: count = %d\n", ++count);

  thread_exit();  // 线程完成工作后退出
}

// 线程函数
void thread_two_func(void *arg) {

  for (int i=0;i<10;i++)    
    printf("Thread 2: count = %d\n", ++count);

  thread_exit();  // 线程完成工作后退出
}


// 线程函数
void thread_three_func(void *arg) {

  for (int i=0;i<10;i++)    
    printf("Thread 3: count = %d\n", ++count);

  thread_exit();  // 线程完成工作后退出
}

int main() {
  void *stack1 = malloc(4096);  // 为第一个线程分配栈
  void *stack2 = malloc(4096);  // 为第二个线程分配栈
  void *stack3 = malloc(4096);  // 为第三个线程分配栈
  int status1, status2, status3;

  if (stack1 == 0 || stack2 == 0 || stack3 == 0) {
    printf("Failed to allocate stack.\n");
    exit(1);
  }

  // 创建第一个线程
  int tid1 = thread_create(thread_one_func, (void*)(uintptr_t)1, stack1);
  if (tid1 < 0) {
    printf("Failed to create thread 1.\n");
    free(stack1);
    free(stack2);
    free(stack3);
    exit(1);
  }

  if (thread_join(tid1, (uint64)&status1) < 0) {
    printf("Failed to join thread 1.\n");
  }
  printf("Parent-Thread 1 Finish\n");
  printf("Parent-Thread 1 exit status = %d\n", status1);

  // 创建第二个线程
  int tid2 = thread_create(thread_two_func, (void*)(uintptr_t)2, stack2);
  if (tid2 < 0) {
    printf("Failed to create thread 2.\n");
    free(stack1);
    free(stack2);
    free(stack3);
    exit(1);
  }

  if (thread_join(tid2, (uint64)&status2) < 0) {
    printf("Failed to join thread 2.\n");
  }
  printf("Parent-Thread 2 Finish\n");
  printf("Parent-Thread 2 exit status = %d\n", status2);

  // 创建第三个线程
  int tid3 = thread_create(thread_three_func, (void*)(uintptr_t)3, stack3);
  if (tid3 < 0) {
    printf("Failed to create thread 3.\n");
    free(stack1);
    free(stack2);
    free(stack3);
    exit(1);
  }

  if (thread_join(tid3, (uint64)&status3) < 0) {
    printf("Failed to join thread 3.\n");
  }
  
  printf("Parent-Thread 3 Finish\n");
  printf("Parent-Thread 3 exit status = %d\n", status3);

  printf("All threads finished. Final count = %d\n", count);
  
  // 释放栈内存
  free(stack1);
  free(stack2);
  free(stack3);

  exit(0);
}
