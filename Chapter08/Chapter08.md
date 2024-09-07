# 扩展实验：给XV6增加多线程支持



仿照Linux内核中对线程的实现方式，即：用task_struct的结构体来管理进程，task_struct中保存了一个进程的所有信息，包括内存地址空间、打开的文件、调度优先级、运行状态、上下文信息等；**Linux没有为支持线程而新增任何数据结构**，也就是说Linux中的进程和线程都是用task_struct这个结构体来表示；在Linux看来线程就是一个普通的进程，只不过这个进程和其它进程**共享了地址空间等资源**，所有共享同一个地址空间的task_struct组成了我们常说的进程，每个task_struct可以理解为我们常说的线程。

我们仿照 Linux 的实现，让 XV6 的内核支持 **“共享资源” 的进程，也就是 线程**。



## 一、修改内核进程管理结构，支持进程共享资源



内核进程管理结构在 **kernel/proc.h** 中定义：

[点击这里查看--代码修改](https://github.com/hitsz-ids/tutorial/commit/20e7a81d735379c05e70166a813b30ba125e1421#diff-9217aa6c10ce4a80f68f3d335a5aac4ffa587639b1e5053becbdf60db11bc9f1)

```
# 主要增加了这一行代码，用于标明当前 进程  是否为 线程

int is_thread;               // Flag to indicate whether this is a thread or process

```

[点击这里查看--原始代码](https://github.com/hitsz-ids/tutorial/blob/54228d7771fccad72096110d3d3f394c1c8e96fa/Chapter08/XV6/kernel/proc.h) 

[点击这里查看--修改后的代码](https://github.com/hitsz-ids/tutorial/blob/20e7a81d735379c05e70166a813b30ba125e1421/Chapter08/XV6/kernel/proc.h)



![](01.png)



## 二、内核增加 线程创建、线程等待  功能



内核进程操作在 **kernel/proc.c** 中，我们增加三个方法：



点击这里查看--代码修改

```
// 线程创建
int  thread_create(void (*start_routine)(void*), void *arg, void *stack)

// 线程退出
void thread_exit(void)

// 线程等待
int  thread_join(int pid)

```

[点击这里查看--原始代码](https://github.com/hitsz-ids/tutorial/blob/63696fbb703cc847f22f5125140497c3dd4cd395/Chapter08/XV6/kernel/proc.c)

点击这里查看--修改后的代码





