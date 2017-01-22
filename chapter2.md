# Namespace API

Namespace API 包括三个系统调用`clone()`、`unshare()`、`setns()`和一些`/proc/`文件。这本节中，我们将详细分析这些系统调用和proc文件，这三个系统调用都需要使用常量`CLONE_NEW*`，包括`CLONE_NEWUTS`,`CLONE_NEWIPC`,`CLONE_NEWNS`,`CLONE_NEWPID`,`CLONE_NEWUSER`,`CLONE_NEWNET`。


## 使用`clone（）`创建一个新的namespace

创建新的namespace的一种方式是使用系统调用`clone()`，该系统调用的原型如下：

```
int clone(int (*child_func)(void *), void *child_stack, int flags, void *arg);

```


