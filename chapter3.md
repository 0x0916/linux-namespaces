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
PID of child created by clone() is 113796
childFunc(): PID  = 1
childFunc(): PPID = 0
Mounting procfs at /proc2
```

前两行输出说明了**在不同的PID namespace中子进程具有不同的进程ID**，也就是说，子进程在clone系统调用所在的PID namespace中的进程ID为113796，而在新创建的PID namespace中进程ID为1。

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

我们的示例程序运行在shell会话中，并且挂载/proc文件系统到目录`/proc2`下，一般在实际中，/proc文件系统应该挂载到边走的目录/proc下。为了防止不同PID namespace中的进程信息都在/proc目录下，给我们的演示带来困惑，这里我们将子PID namespace的/proc文件系统，挂载到不同的目录下。

在/proc/PID目录下，存放的是父PID namespace的进程信息，在/proc2/PID目录下，存放的是子PID namespace的进程信息。需要强调的是，在shell会话中，尽管子PID namespace中的进程可以看到/proc目录下的进程信息，但这些信息对子PID namespace中的进程都是没有意义的，因为系统调用解释PID时，只从自己所处的PID namespace中寻找进程信息。

将/proc文件系统挂载到/proc目录下是很有必要的，因为一些命令。例如`ps`会依赖该目录下的信息。有两种方法将其挂载到/proc目录下，且不会与父PID namespace相冲突。

* 在clone系统调用时，同时传递flag  CLONE_NEWNS 新建一个mount namespace。
* 子进程通过系统调用 chroot() 切换其跟目录。

我们可以将上面运行的程序停止，并使用ps命令在父PID namespace中查看父进程和子进程的信息。

```bash
^Z
[1]+  Stopped                 ./pidns_init_sleep /proc2
# ps -C sleep -C pidns_init_sleep -o "pid ppid stat cmd"
   PID   PPID STAT CMD
113795  25038 T    ./pidns_init_sleep /proc2
113796 113795 S    sleep 600
```

最后一行sleep进程的PPID的值(113795)就是pidns_init_sleep进程的进程ID。我们可以通过命令readlink读取文件`/proc/PID/ns/pid`，可以看到这两个进程处于不同的PID namesapce中

```bash
# readlink /proc/113795/ns/pid 
pid:[4026531836]
# readlink /proc/113796/ns/pid 
pid:[4026532455]
```

通过查看目录`/proc2`，我们可以看到子PID namesapce中含有如下进程：

```bash
# ls -d /proc2/[1-9]*
/proc2/1
```

子PID namespace中只含有一个进程，其进程ID为1，进一步，通过查看`/proc2/PID/status`可以得到如下信息:

```bash
# cat /proc2/1/status | egrep '^(Name|PP*id)'
Name:	sleep
Pid:	1
PPid:	0
```
在子PID namespace中，子进程的PPID的值为0，跟系统调用`getppid()`返回的值一样。

## PID namespace的嵌套

PID namespace 是可以嵌套的，嵌套的PID namesapce具有parent-child层级的关系。进程只能**看到**同一个PID namespace中的进程和其子PID namespace的进程信息（包括所有后代PID namespace）。这里的**看到**的意思是：可以调用以PID为参数的系统调用，比如`kill()`来发送信号给某个进程。反过来，子PID namespace中的进程不能看到其父PID namespace中的进程信息。

进程在每一个PID namespace层级上都有一个进程ID。`getpid()`系统调用总是返回调用者所在的PID namespace中进程的进程ID。

我们通过如下的示例程序，学习和了解一下PID namesapce的嵌套：

```c
/* multi_pidns.c
 *
 * Create a series of child process in nested PID namespaces
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
#include <limits.h>


/* A simple error-handling function: print an error message based
   on the value `errno` and terminate the calling process
*/
#define bail(msg)				\
	do { perror(msg);			\
		exit(EXIT_FAILURE);		\
	} while (0)


#define STACK_SIZE	(1024 * 1024)	// stack size for cloned child

static char child_stack[STACK_SIZE];

/* Recursively create a series of child process in nested PID namespaces.
  'arg' is an integer that counts down to 0 during the recursion.
  When the counter reaches 0, recursion stops and the tail child executes
  the sleep(1) program.
  */

static int childFunc(void *arg) {
	static int first_call = 1;
	long level = (long) arg;

	if (!first_call) {
		char mount_point[PATH_MAX];
		snprintf(mount_point, PATH_MAX, "/proc%c", (char) ('0' + level));
		mkdir(mount_point, 0555);
		if (mount("proc", mount_point, "proc", 0, NULL) == -1)
			bail("mount");

		printf("Mounting procfs at %s\n", mount_point);
	}

	first_call = 0;

	if (level > 0) {
		level--;
		pid_t	child_pid;

		// create child taht has its own PID namespace;
		// child commences excution in childFunc()
		child_pid = clone(childFunc, child_stack + STACK_SIZE, CLONE_NEWPID| SIGCHLD, (void*) level);
		if (child_pid == -1)
			bail("clone");

		if (waitpid(child_pid, NULL, 0) == -1) // wait for child
			bail("waitpid");
	} else {
		printf("Final child sleeping\n");
		execlp("sleep", "sleep", "1000", (char *)NULL);
		bail("execlp");
	}

	return 0;
}

int main(int argc, char **argv) {
	long	levels;

	levels = (argc > 1) ? atoi(argv[1]) : 5;
	childFunc((void *)levels);

	exit(EXIT_SUCCESS);
}
```

该示例程序递归的创建了多个嵌套的PID namespace，并挂载不同的PID namespace中的proc文件系统到不同的目录，最后的一个PID namespace中调用sleep函数。

```bash
# ./multi_pidns 5
Mounting procfs at /proc4
Mounting procfs at /proc3
Mounting procfs at /proc2
Mounting procfs at /proc1
Mounting procfs at /proc0
Final child sleeping
```

我们将程序停下来，看一下不同层级的PID namespace中都包含了哪些进程的信息。

```bash
^Z
[2]+  Stopped                 ./multi_pidns 5
# ls -d /proc4/[1-9]*           # 最顶级的PID namespace
/proc4/1  /proc4/2  /proc4/3  /proc4/4  /proc4/5
# ls -d /proc3/[1-9]*
/proc3/1  /proc3/2  /proc3/3  /proc3/4
# ls -d /proc2/[1-9]*
/proc2/1  /proc2/2  /proc2/3
# ls -d /proc1/[1-9]*
/proc1/1  /proc1/2
# ls -d /proc0/[1-9]*
/proc0/1
```

从中可以看出，每一级的PID namespace中都包含了其PID namespace和其子PID namespace（所有的后世子孙PID namespace）中的进程信息。

```bash
# grep -H 'Name:.*sleep' /proc?/[1-9]*/status
/proc0/1/status:Name:	sleep
/proc1/2/status:Name:	sleep
/proc2/3/status:Name:	sleep
/proc3/4/status:Name:	sleep
/proc4/5/status:Name:	sleep

```

sleep在不同级别的PID namespace中都有其不同的进程ID。

运行示例程序会在系统中残留proc文件系统的挂载点，当程序退出后，我们可以使用如下命令，将其删除：

```bash
# umount /proc?
# rmdir /proc?
```

## PID namespace的init进程

## 信号和init进程

## unshare()和setns()

