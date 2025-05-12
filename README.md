# MicroFiber

MicroFiber is a lightweight user-level threading library built in C++. It provides cooperative and preemptive
multitasking support for applications that need efficient thread management without relying on operating system threads.


## Features

- Lightweight threads with customizable priorities
- Cooperative and preemptive scheduling
- Configurable schedulers: First-Come First-Served (FCFS), Random, and Priority-based
- Thread lifecycle management: create, yield, kill, wait, sleep, and wakeup
- Spin-based busy waiting and interrupt-safe logging
- Basic synchronization with Lock and FIFO-based wait queues


## Configuration

Before using the library, initialize it with a configuration:

```cpp
Config config = {
    .scheduler_name = Config::SchedulerType::FCFS,
    .is_preemptive = true
};

MicroFiber::microfiber_start(&config);
```


## Thread Lifecycle

### Create a thread

Create a thread to run a given function with a given priority:

```cpp
int my_thread_fn(void *arg) {
    // Your code here
    return 0;
}

ThreadID tid = MicroFiber::thread_create(my_thread_fn, nullptr, 0);
```


### Yield a thread

Yield execution to another thread (or a specific thread):

```cpp
MicroFiber::thread_yield(MicroFiber::ThreadCodes::ANY);
```


### Exit and kill

Exit the current thread or kill a specific thread:

```cpp
MicroFiber::thread_exit(0);               // Exit current thread
MicroFiber::thread_kill(tid);             // Kill a specific thread
```


### Sleep and wakeup

Sleep the current thread for a specified duration or wake up a sleeping thread:

```cpp
MicroFiber::thread_sleep(queue);          // Sleep on a queue
MicroFiber::thread_wakeup(queue, true);   // Wake one or all
```


### Thread waiting

Wait for a thread to finish execution:

```cpp
int exit_code;
MicroFiber::thread_wait(tid, &exit_code);
```


### Lock example

```cpp
Lock lock;

void critical_section() {
    lock.acquire();
    // Protected section here
    lock.release();
}
```
