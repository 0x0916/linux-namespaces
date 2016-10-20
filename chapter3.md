# PID namespace

## 概念

PID namespace用来隔离进程ID，所以，不同的PID namespace中的进程，可以具有相同的进程ID了。

跟传统的Linux系统一样，一个进程的ID在PID namespace中是唯一的，PID namespace中的进程号从1开始分配。在传统Linux系统中，进程ID为1的进程（init 进程）比较特殊，同样的，在PID namespace中进程号为1的进程也比较特殊，它肩负这PID namespace中任务管理的角色。

在使用PID namespace时，需要内核打开配置选项`CONFIG_PID_NS`。首先我们通过下面的例子，先直观感受一下PID namesapce。

## 实例一

我们可以通过给系统调用clone传递CLONE\_NEWPID flag来创建一个新的PID namespaces。创建一个新的PID namespace的完整代码如下：

```c
/* pidns_init_sleep.c
 *
 * A simple demonstration of PID namespace
 */
#define _GNU_SOURCE
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sched.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>


/* A simple error-handling function: print an error message based
   on the value `errno` and terminate the calling process
*/
#define bail(msg)                \
    do { perror(msg);            \
        exit(EXIT_FAILURE);        \
    } while (0)


/* Start function for cloned child */
static int childFunc(void *arg) {

    printf("childFunc(): PID  = %ld\n", (long) getpid());
    printf("childFunc(): PPID = %ld\n", (long) getppid());


    char *mount_point = arg;

    if (mount_point != NULL) {
        mkdir(mount_point, 0555);        // Create directory for mount point
        if (mount("proc", mount_point, "proc", 0, NULL) == -1)
            bail("mount");
        printf("Mounting procfs at %s\n", mount_point);
    }

    execlp("sleep", "sleep", "600", (char *)NULL);
    bail("execlp");        // only reached if execlp() fails
}

#define STACK_SIZE    (1024 * 1024)    // stack size for cloned child

static char child_stack[STACK_SIZE];

int main(int argc, char **argv) {
    pid_t    child_pid;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <proc dir>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // create child taht has its own PID namespace;
    // child commences excution in childFunc()
    child_pid = clone(childFunc, child_stack + STACK_SIZE, CLONE_NEWPID| SIGCHLD, argv[1]);
    if (child_pid == -1)
        bail("clone");

    printf("PID of child created by clone() is %ld\n", (long) child_pid);

    // Parent falls through to here
    if (waitpid(child_pid, NULL, 0) == -1) // wait for child
        bail("waitpid");
    printf("child has terminated\n");

    exit(EXIT_SUCCESS);
}
```

这个程序使用`clone()`系统调用创建了一个新的PID namespace，并显示新创建子进程的ID。

``` c
 // create child taht has its own PID namespace;
 // child commences excution in childFunc()
 child_pid = clone(childFunc, child_stack + STACK_SIZE, CLONE_NEWPID| SIGCHLD, argv[1]);

 printf("PID of child created by clone() is %ld\n", (long) child_pid);

```
新创建的子进程的从childFunc开始执行，它将接收clone的最后一个参数`argv[1]`最为它的参数。childFun中，显示了该子进程的进程ID以及其父进程的进程ID，最后执行了标准的sleep函数。

```c
 printf("childFunc(): PID = %ld\n", (long) getpid());
 printf("childFunc(): PPID = %ld\n", (long) getppid());
 ...
 execlp("sleep", "sleep", "600", (char *)NULL);

```
我们来运行一下这个程序，其输出如下：

``` bash

$ gcc -o pidns_init_sleep pidns_init_sleep.c
$ su # 需要root权限才能够创建PID namespace
Password：
# ./pidns_init_sleep /proc2
PID of child created by clone() is 4069
childFunc(): PID = 1
childFunc(): PPID = 0
Mounting procfs at /proc2

```

前两行输出说明了**在不同的PID namespace中子进程具有不同的进程ID**，也就是说，子进程在clone系统调用所在的PID namespace中的进程ID为4069，而在新创建的PID namespace中进程ID为1。

接着第三行显示了在**新创建的PID namespace中，子进程的PPID为0（getppid()的返回值）**。一个进程只能看到跟自己在同一个PID namespace中或者其子PID namespace中的进程，在该示例程序中，子进程根其父进程不在同一个PID namespace中，子进程不能看到其父PID namespace中的进程，因此，getppid()返回子进程的PPID为0.

为了解释最后一行的输出内容，我们先了解一下下节基础知识。

## /proc/PID 

在linux系统中，每一个进程都会有一个目录`/proc/PID`，该目录里包含的文件描述了该进程的各种信息。**在一个PID namespace中，`/proc/PID`目录只包含了该PID namespace以及其子PID namespace 中进程的信息**。

然而，为了能够看到新创建的PID namesapce中的进程的信息，我们需要将/proc文件系统挂着到某个目录中。在PID namespace中的shell下，我们可以使用如下的命令
```
# mount -t proc proc /mount_point
```
或者，我们可以想程序中的那样，使用`mount()`系统调用挂在/proc文件系统。

```c
 mkdir(mount_point, 0555); // Create directory for mount point
 mount("proc", mount_point, "proc", 0, NULL);
 printf("Mounting procfs at %s\n", mount_point);

```
在我们的示例程序中，mount_point的值就是通过命令行参数传递进来的值`/proc2`。


## PID namespace的嵌套

