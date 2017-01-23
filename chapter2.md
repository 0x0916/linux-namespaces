# Namespace API

Namespace API 包括三个系统调用`clone()`、`unshare()`、`setns()`和一些`/proc/`文件。这本节中，我们将详细分析这些系统调用和proc文件，这三个系统调用都需要使用常量`CLONE_NEW*`，包括`CLONE_NEWUTS`,`CLONE_NEWIPC`,`CLONE_NEWNS`,`CLONE_NEWPID`,`CLONE_NEWUSER`,`CLONE_NEWNET`。


## 使用`clone（）`创建一个新的namespace

创建新的namespace的一种方式是使用系统调用`clone()`，该系统调用的原型如下：

```c
int clone(int (*child_func)(void *), void *child_stack, int flags, void *arg);

```

clone系统调用用来创建一个新的进程，但是它比传统的UNIX 函数 fork vfork控制粒度更精细，总过有大约20多种`CLONE_*`flags参数可以控制创建进程的方方面面，比如父进程和子进程
是否共享一些资源（虚拟内存、打开的文件描述符、信号处理方法）。如果flags中包含标记`CLONE_NEW*`，那么对应的namespace会被创建，而新创建的子进程属于对应的namespace中的进程。
flags参数中可以通知指定多个标记`CLONE_NEW*`。

我们可以通过来自lwn的示例程序[demo_uts_namespaces.c](code/demo_uts_namespaces.c)来说明如何通过clone系统调用创建一个新的UTS namespace。

该程序接收一个参数，运行时，它创建了一个子进程，并在新的UTS namespace中运行它，在新的UTS namespace中，子进程根据接收的参数，修改了hostname。

关键代码片段如下：

使用`clone`创建子进程:

```c
	// create child taht has its own UTS namespace;
	// child commences excution in childFunc()
	child_pid = clone(childFunc, child_stack + STACK_SIZE, CLONE_NEWUTS | SIGCHLD, argv[1]);
	if (child_pid == -1)
		bail("clone");

	printf("PID of child created by clone() is %ld\n", (long) child_pid);
```

子进程将会执行用户自定义的函数体`childFunc`，由于`flags`中指定了参数`CLONE_NEWUTS`，子进程将会在新的UTS namespace中执行。
创建完子进程后，父进程会进行睡眠，以这种方式保证子进程先执行，在子进程中，其在新的UTS namespace中修改了hostname。此后
父进程使用`uname()`获取父UTS namespace中的`hostname`，并打印。

```c
	sleep(1);		// give child time to change its hostname

	// display hostname in parent's UTS namespace, This will be different
	// from hostname in child's UTS namespace
	if (uname(&uts) == -1)
		bail("uname");
	printf("uts.nodename in parent: %s\n", uts.nodename);

```

于此同时，子进程修改了其hostname，并打印出其修改后的hostname。

```c
	// change hostname in UTS namespace of child
	if (sethostname(arg, strlen(arg)) == -1)
		bail("sethostname");

	// retriveve and dispaly hostname
	if (uname(&uts) == -1)
		bail("uname");
	printf("uts.nodename in child: %s\n", uts.nodename);

```

运行该程序，我们可以看到子进程和父进程处于不同的UTS namespace中：

```bash
# ./demo_uts_namespaces apple
PID of child created by clone() is 117700
uts.nodename in child: apple
uts.nodename in parent: SZX1000042009
```

## `/proc/PID/ns` 文件

每一个进程都有一个目录/proc/PID/ns，该目录包含了一系列描述进程所属namespace的文件，每一个文件都是一个特殊的符号链接。

```bash
# ls -l /proc/$$/ns/
total 0
lrwxrwxrwx 1 root root 0 Jan 23 09:34 ipc -> ipc:[4026531839]
lrwxrwxrwx 1 root root 0 Jan 23 09:34 mnt -> mnt:[4026531840]
lrwxrwxrwx 1 root root 0 Jan 23 09:34 net -> net:[4026532244]
lrwxrwxrwx 1 root root 0 Jan 23 09:34 pid -> pid:[4026531836]
lrwxrwxrwx 1 root root 0 Jan 23 09:34 user -> user:[4026531837]
lrwxrwxrwx 1 root root 0 Jan 23 09:34 uts -> uts:[4026531838]
```

这些文件的用处之一就是我们可以通过它查看两个进程是否属于同一个namesapce。如果两个进程属于同一个namespace，那么/proc/PID/ns目录中对应的文件的inode号相同。
我们可以通过系统调用stat查看文件的inode号。

然而，内核同时将该目录下的文件链接到一个按照一定规则命名的文件中，该文件名称为: `namespace type:[inode number]`，所以我们可以通过查看符号连接名称来判断两个
进程是否属于同一个namespace。为了查看符号链接，我们可以使用命令`ls -l `或者`readlink`。

我们可以示例程序`demo_uts_namespaces`中的父子进程是否处于不同的namespace中：

```bash
^Z
[1]+  Stopped                 ./demo_uts_namespaces apple
# jobs -l
[1]+ 117699 Stopped                 ./demo_uts_namespaces apple
# readlink  /proc/117699/ns/uts 
uts:[4026531838]
# readlink  /proc/117700/ns/uts 
uts:[4026532458]
```
我们可以看出，`/proc/PID/ns/uts`符号连接的inode是不同的，表明这两个进程属于不同的UTS namespace。

/proc/PID/ns 目录下的文件还有一个用途，如果打开了其中某个文件，只要该文件没有关闭，即使其对应的namespace中的进程都终止了，该namespace也会一直存在。
我们也可以通过bind mount符号链接到其他目录中来保持一个namespace的存在。

``` bash
# touch ~/apple
# mount --bind /proc/117700/ns/uts ~/apple
```

## 使用系统调用`setns()`加入到已有的namespace中

保持一个不包含任何进程的namespace，是非常有用的，比如我们可以在以后向该namespace中添加进程。系统调用`setns()`可以完成这项工作：将调用进程加入到已存在的namespace中。

```c
	int setns(int fd, int nstype)
```
精确的讲，`setns()`将调用者进程从当前namespace中移除，并将其添加到新的namespace中。参数`fd`指定了新的namespace，其是一个打开`/proc/PID/ns/`中对应namespace文件的描述符。
参数`nstype`允许调用者去检查`fd`指定的namespace类型是否正确。如果该参数为0，将不进行检查。当调用者已经知道`fd`所指向的namespace时，不需要进行检查。如果`fd`是通过
`Unix domain socket`来进行传递的，该项检查就非常有用。

接着上面的示例程序，本小节的示例程序[ns_exec](code/ns_exec.c)通过open系统调用打开`/proc/PID/ns/uts`，然后调用`setns`将其本身加入到新的UTS namespace中，并使用`exec`执行指定的程序。

```c
	fd = open(argv[1], O_RDONLY);		// get file descriptor for namespace
	if (fd == -1)
		bail("open");

	if (setns(fd, 0) == -1)			// join that namespace
		bail("setns");

	execvp(argv[2], &argv[2]);		// execute a command in namespace

```

在示例程序中，我们使用前面创建好的UTS namespace，将进程添加到该UTS namesapce中：

```bash
# ./ns_exec ~/apple /bin/bash
# hostname
apple
# readlink /proc/117700/ns/uts
uts:[4026532458]
# readlink /proc/$$/ns/uts
uts:[4026532458]
```

## 使用系统调用`unshare()`离开一个namespace

最后一个跟namespace有关的系统调用是`unshare()`：

```c
	int unshare(int flags);
```

`unshare()`跟`clone()`的功能类似，只不过`unshare()`操作于调用者进程：它根据参数`flags`创建新的namespace，并将调用这进程加入到该新的namespace中。
`unshare()`的作用就是创建新的namespace，而不需要创建新的子进程。

所以，代码段

```c
	clone(..., CLONE_NEWxxx,...);
```

从某种程度上，等价于

```c
	if (fork() == 0)
		unshare(CLONE_NEWxxx);
```

`unshare()`系统调用的一个用处就是实现`unshare`命令，其实现的关键如下：

```c

	if (unshare(flags) == -1)
		bail("unshare");

	execvp(argv[optind], &argv[optind]);

```

一个简单的`unshare`命令的实现为[unshare.c](code/unshare.c),在下面的示例结果中，我们使用`unshare.c`在新的mnt namespace中启动了一个bash程序。

```bash
# echo $$						# 显示shell的进程号
106031
# cat /proc/106031/mounts | grep lxcfs   # 显示lxcfs的挂载点
lxcfs /var/lib/lxcfs fuse.lxcfs rw,nosuid,nodev,relatime,user_id=0,group_id=0,allow_other 0 0
# readlink /proc/106031/ns/mnt           # 显示mount namespace的ID 
mnt:[4026531840]
# ./unshare -m /bin/bash			     # 在新的mount namespace中启动bash
# readlink /proc/$$/ns/mnt				 # 显示mount namespace的ID 
mnt:[4026532468]
# umount /var/lib/lxcfs                  # 在新的namespace中卸载掉 lxcfs
# cat /proc/$$/mounts | grep lxcfs
# cat /proc/106031/mounts | grep lxcfs
lxcfs /var/lib/lxcfs fuse.lxcfs rw,nosuid,nodev,relatime,user_id=0,group_id=0,allow_other 0 0

```

从上面可以看出，在新的mount namespace中卸载掉lxcfs，不会影响其他namespace中的lxcfs的挂载。



