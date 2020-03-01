# ArkProtect
###平台在Win7 x86/x64<br/>
一个可以保护PC的Windows内核监视工具<br/>
可能存在一些bug，请在虚拟机中测试
# #流程模块:
1. 枚举过程。
2. 枚举进程加载的模块。
3.枚举进程的运行线程。
4. 枚举进程的打开句柄。
5. 枚举进程的打开窗口。
6. 枚举进程的用户空间内存。
7. 终止一个进程(强制终止)。
# #驱动程序模块:
1. 枚举当前加载的驱动程序。
2. 目标驱动程序卸载。
# #内核模块:
1. 列举系统回调。
2. 列举过滤驱动。
3.枚举计时器对象(IOTimer/ DpcTimer)。
# #内核钩子:
1. 现在，只支持ssdthookcheck和sssdthook检查，它将在未来支持内联钩子检查。


# ArkProtect
### Platform in Win7 x86/x64<br/>
## A Windows kernel watch tool which can protect PC somehow<br/>
#### Maybe some bugs exist, please test it in Virtual Machine

## process module:

1. Enumerate processes.

2. Enumerate process's loaded modules.

3. Enumerate process's running threads.

4. Enumerate process's openning handles.

5. Enumerate process's openning windows.

6. Enumerate process's userspace memory.

7. Terminate a process (by force).

## driver module:

1. Enumerate current loaded drivers.

2. Unload target driver.

## kernel module:

1. Enumerate system callbacks.

2. Enumerate filter drivers.

3. Enumerate timer object (IOTimer/ DpcTimer).

## kernel hook:

1. Now, just support ssdthookcheck & sssdthook check, it will support inline hook check in the future.
