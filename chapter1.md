# Namespace 介绍

## Namespace是什么？

Namespace是将内核的全局资源进行封装，使得每个namespace都有一份独立的资源，因此不同的进程在各自的Namespace内对同一种资源的使用不会相互干扰。namespaces的一个用武之地就是用来实现容器（Containers）。

目前 Linux内核提供了如下七种的namespace，每种namespace实现了对不同的内核全局资源的封装。

| Namespace | 常量参数 | 隔离的资源 |
| --- | --- | --- |
| Cgroup | CLONE\_NEWCGROUP | cgroup根目录 |
| IPC | CLONE\_NEWIPC | 隔离SystemV IPC和POSIX消息队列 |
| Network | CLONE\_NEWNET | 隔离网络资源 |
| PID | CLONE\_NEWPID | 隔离进程ID |
| User | CLONE\_NEWUSER | 隔离用户ID和组ID |
| UTS | CLONE\_NEWUTS | 隔离主机名和域名 |
| Mount | CLONE\_NEWNS | 隔离文件系统挂载点 |

下面我们对不同的namespaces简单的介绍，namespace相关的API接口会在后续章节介绍。

### Mount namespace

### UTS namespace

### IPC namespace

### PID namespace

### Network namespace

### User namespace

### Cgroup namespace



