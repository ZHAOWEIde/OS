# OS_Lab5 仿Ext2文件管理系统(Linux / C)

#### 说明
+   本实验基于哈尔滨工业大学（深圳）计算机操作系统课程指导完成。
+   [-操作系统课程指导-](https://hitsz-lab.gitee.io/os_lab/lab5/part1/)
+   代码位于code文件夹、code内代码含义
    -   disk.h / disk.c：模拟一个 4M 的磁盘，一次以512B读写；
    -   naiveExt2.h    ：文件管理系统头文件；
    -   naiveExt2.c    : 函数实现；
    
#### 使用说明

1.  在code目录下使用命令行编译指令
    gcc -g disk.h disk.c naiveExt2.c naiveExt2.h -o main
2.  在code中运行./main
3.	按说明运行即可，注意未完成相对路径，所以输入文件时，请输入绝对路径  
    ![naiveExt2菜单说明](https://github.com/ZHAOWEIde/OS/blob/master/image/show.png "菜单")
    
#### Ext2介绍

+   文件系统是操作系统用于明确存储设备（磁盘）或分区上的文件的方法和数据结构；即在存储设备上组织文件的方法。简单地说文件就是在磁盘上组织文件的方法。
+   文件系统主要作用：
    -   管理和调度文件的存储空间，提供文件的逻辑结构、物理结构和存储方法;
    -   实现文件从标识到实际地址的映射，实现文件的控制操作和存取操作，提高磁盘查找数据的效率；
    -   实现文件信息的共享并提供可靠的文件保密和保护措施，提供文件的安全措施；
    -   优化磁盘空间利用率；
+   目前在各种操作系统中存在着各种各样的文件系统，在Windows平台主流有：FAT、FAT16、FAT32、NTFS等；在Unix平台主流的有：Ext2、Ext3、Ext4等。这些文件系统在管理磁盘时都有各自的一套策略和方法，随之也带来不同的优缺点。Ext2是GNU/Linux系统中标准的文件系统，其特点是存取文件的性能较好，对于中小型的文件更显示其优势。本实验以Ext2为模板，实现一个简单的文件系统。

+   Ext2文件系统的构成
    -   一个物理磁盘可以划分为多个磁盘分区，每个磁盘分区可以从逻辑上看成是从0开始编号的大量扇区，各自可以格式化程不同类型的文件系统（如Ext2、NTFS等）。如果格式化成Ext2文件系统，则其内部按照Ext2的规范，将磁盘盘块组织成超级块、组描述符和位图、索引节点、目录等管理数据，放在分区前端称为元数据区，剩余空间用于保存文件数据。
    
#### naiveExt2实现内容介绍

1.  实现青春版Ext2文件系统
    -创建文件/文件夹（数据块可预分配）；
    -读取文件夹内容；
    -复制文件；
    -关闭系统；
    -系统关闭后，再次进入该系统还能还原出上次关闭时系统内的文件部署。
2.  为实现的文件系统实现简单的 shell 以及 shell 命令以展示实现的功能
    -ls - 展示读取文件夹内容
    -mkdir - 创建文件夹
    -touch - 创建文件
    -cp - 复制文件
    -shutdown - 关闭系统
    
#### naiveExt2内容展示

1.  基础功能：  
    ![基础功能](https://github.com/ZHAOWEIde/OS/blob/master/image/basic.png)

2.  创建文件功能：  
    ![touch创建文件](https://github.com/ZHAOWEIde/OS/blob/master/image/touch.png)
    
3.  读写文件功能：  
    ![echo/cat](https://github.com/ZHAOWEIde/OS/blob/master/image/echo%26cat.png)
    
#### 可扩展功能

1.  cd功能;
2.  fork()多进程;






