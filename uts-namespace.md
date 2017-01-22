# UTS namespace

UTS namespace隔离了linux上两个系统标识符`nodename`和`domainname`，我们可以通过系统调用`uname()`查看它们，同时我们也可以通过系统调用`sethostname()`和`setdomainname()`设置它们。在容器的上下文中，UTS namespace允许不同的容器具有不同的`hostname`和`NIS domain name`


示例

```c
/* demo_uts_namespaces.c
 *
 * Demonstrate the operation of UTS namespaces
 */
#define _GNU_SOURCE
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sched.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


/* A simple error-handling function: print an error message based
   on the value `errno` and terminate the calling process
*/
#define bail(msg)				\
	do { perror(msg);			\
		exit(EXIT_FAILURE);		\
	} while (0)


/* Start function for cloned child */
static int childFunc(void *arg) {
	struct utsname uts;

	// change hostname in UTS namespace of child
	if (sethostname(arg, strlen(arg)) == -1)
		bail("sethostname");

	// retriveve and dispaly hostname
	if (uname(&uts) == -1)
		bail("uname");
	printf("uts.nodename in child: %s\n", uts.nodename);

	// keep the namespace open for a while, by sleeping
	// this is allows some experimentation: for example, another
	// process might join the namespace
	sleep(200);

	return 0;		// child terminates now
}

#define STACK_SIZE	(1024 * 1024)	// stack size for cloned child

static char child_stack[STACK_SIZE];

int main(int argc, char **argv) {
	pid_t	child_pid;
	struct utsname	uts;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <child-hostname>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// create child taht has its own UTS namespace;
	// child commences excution in childFunc()
	child_pid = clone(childFunc, child_stack + STACK_SIZE, CLONE_NEWUTS | SIGCHLD, argv[1]);
	if (child_pid == -1)
		bail("clone");

	printf("PID of child created by clone() is %ld\n", (long) child_pid);

	// Parent falls through to here

	sleep(1);		// give child time to change its hostname

	// display hostname in parent's UTS namespace, This will be different
	// from hostname in child's UTS namespace
	if (uname(&uts) == -1)
		bail("uname");
	printf("uts.nodename in parent: %s\n", uts.nodename);

	if (waitpid(child_pid, NULL, 0) == -1) // wait for child
		bail("waitpid");
	printf("child has terminated\n");

	exit(EXIT_SUCCESS);
}
```




