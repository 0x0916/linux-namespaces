# User namespace

`User namespace`用来隔离安全相关的标识符和属性，特别是用户ID，组ID，用户根目录，keys，权能信息。

一个进程在namespace里的用户ID和组ID可以是不同的。比如，一个进程在namespace里是root用户，用户ID为0，
而在namespace外是普通用户，用户ID为非0。


## user namespace的嵌套

user namespace可以嵌套，除了初始user namespace外，每一个user namespace都有一个父user namespace。每个
user namespace可以有0个或者多个子 user namespace。

我们可以通过`unshare`或者`clone`系统调用来创建新的 user namespace。在3.11后的内核版本中，最多可以创建
32层嵌套的user namespace。如果超出了限制，这两个系统调用会返回错误`EUSER`。


