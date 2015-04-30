该项目仅供学习研究为目的

以下内容的格式是以readme.md文件对应的Markdown语法来写的, 
使用的是 https://stackedit.io 在线编辑器进行的编辑
(因为不同网站对Markdown语法的显示效果都不完全相同, 因此如果出现
显示上的问题, 请直接查看readme.md的源文件内容, 之所以使用这种文件格式
, 完全是为了方便显示下面的截图而已!)

从v3.0.0版本开始, zenglOX开始进入桌面环境, 在内核中实现了窗口管理器, 整个桌面其实就是一个窗口。

截图如下(如果下面的图片无法显示的话, 可以直接查看源码根目录下的screenshot目录, 所有截图都放在该目录内):

![screenshot/v302_1.jpg](https://github.com/zenglong/zenglOX/raw/master/screenshot/v302_1.jpg)

![screenshot/v300_1.jpg](https://github.com/zenglong/zenglOX/raw/master/screenshot/v300_1.jpg)

![screenshot/v300_2.jpg](https://github.com/zenglong/zenglOX/raw/master/screenshot/v300_2.jpg)

![screenshot/v300_3.jpg](https://github.com/zenglong/zenglOX/raw/master/screenshot/v300_3.jpg)

![screenshot/v300_4.jpg](https://github.com/zenglong/zenglOX/raw/master/screenshot/v300_4.jpg)

```
第一幅图片是v3.0.2版本中的截图。

第二幅与第三幅图片是VirtualBox与VMware下的截图，第四幅图是在笔记本上，通过U盘启动时的截图，第五幅图是在作者的台式机上,
通过U盘启动时的截图。(将Grub2写入U盘的方法, 在下面会进行介绍, 在作者的笔记本上, 鼠标键盘都可以正常使用, 但是在
作者的台式机上, 只有键盘可以正常工作, 鼠标估计是走的USB通道, 没有通过PS/2的second port对应的IRQ来发送中断信号)

VirtualBox与VMware中运行时, 所需要的zenglOX.iso镜像可以直接从sourceforge网盘对应的v3.0.0的文件夹里下载到.

在 www.zengl.com 的官网上, zenglOX v1.0.0版本对应的文章里, 介绍过VirtualBox中新建zenglOX虚拟机的详细步骤, 
在zenglOX v1.4.0与v1.4.1版本对应的文章里, 介绍过如何在VMware中新建zenglOX虚拟机的详细步骤.
VirtualBox中请开启硬件加速功能, 在开启硬件加速功能时, 图形界面的渲染速度要比非硬件加速时要快很多, 
在新建虚拟机时, VirtualBox默认就已经打开了加速功能.

v3.0.0版本是使用Qemu来作为开发调试用的模拟器的, Bochs的渲染速度以及执行性能都不如Qemu, 不太适合进行图形化界面的开发,
当然, v3.0.0的版本也可以在Bochs上正常运行, 只要你的Bochs可以运行v2.4.0的版本, 就可以直接运行v3.0.0的版本.
下面会介绍Qemu的安装和使用方法.

这里需要特别注意的地方是: 无论是用Bochs还是用Qemu, 你都必须在真实的机子上安装了Linux发行版后, 再在这些发行版里安装Bochs或Qemu,
如果你使用VirtualBox虚拟机安装的Linux发行版, 然后再在发行版中安装Bochs或Qemu的话, Bochs与Qemu里的鼠标很可能用不了, 因为Linux发行版在
VirtualBox里运行时, 会给Bochs与Qemu传递错误的鼠标位移值, 这个值通常会是几万的很大的值, 通过调试Qemu的源代码就会发现这个问题.

作者一开始为了方便使用windows里的资源, 就将ubuntu之类的系统装在了VirtualBox里, 但是这样一来, ubuntu中安装的Qemu和Bochs的鼠标都无法正常工作,
最后作者将ubuntu装在真实的机子上, 让它和windows双启动, 再安装Bochs与Qemu, 这样一来, 鼠标就可以正常工作了.

如果读者有自己的hobby OS的话, 如果想加入鼠标驱动, 就需要注意这个问题.

系统进入桌面前, 会在一个1024x768的黑色屏幕上显示一些内核信息, 显示完信息后, 会等待3秒才进入桌面, 每过一秒都会在下面显示两个小数点,
显示出6个小数点后, 就会进入桌面.

在桌面的左上角有一个黑底白边的正方形, 用鼠标单击该框框, 可以启动term, term就是v3.0.0版本中实现的带窗口界面的终端程式。
如果鼠标用不了(一般是在真实的机子上, 通过U盘启动时, 有可能出现这种情况, 因为现在的机子上, 鼠标很多都不再走PS/2的通道, 在虚拟机上,
比如bochs, qemu, VirtualBox以及VMware中, 经过作者的测试, 都可以正常使用鼠标和键盘), 鼠标用不了时, 在出现桌面时, 按F2键
也可以启动term

启动term后, 如果你在v3.0.0之前的版本里, 已经在VirtualBox或VMware里新建过了zenglOX虚拟机的话, 估计磁盘已经分区格式化过了, 并且可能在磁盘里
残留了之前版本的应用程式, 比如ee编辑器, zengl脚本程式等, 那么你在mount iso和mount hd ...命令后, 还需要通过isoget -all命令将ee编辑器,
zengl脚本程式, 以及libc.so动态库文件进行更新, 因为libc.so库文件里将使用syscall_cmd_window_write之类的系统调用来显示数据到对应的窗口中,
原来的syscall_monitor_write系统调用, 只会将信息暂时显示到屏幕上, 这些暂时信息是可以被鼠标或窗口给擦掉的! 而且不更新数据的话, 原来的ee编辑器是
无法正常工作的, 旧版的libc.so以及ee编辑器运行时会当掉你的系统!

在桌面具有输入焦点时，按上下键可以适当的调节鼠标的scale, 你可以将scale看成鼠标移动速度的一个因子, 在鼠标驱动里, 当scale大于1时,
当鼠标位移值超过一个指定的数值时, 会将实际位移值乘以scale, 来得出最终的位移值。(只要在桌面上单击一下, 桌面对应的窗口就会具有输入焦点了,
当启动一个带窗口的应用程式时, 如term时, 这些窗口会自动获取到输入焦点, 其他窗口就需要通过单击来获取输入焦点)

调节鼠标scale时, 桌面程式会将scale信息通过syscall_monitor_write系统调用直接显示到屏幕上, 这个信息是暂时性的, 你可以用鼠标或别的窗口将它擦掉, 
或者在桌面具有输入焦点时, 按F5键来刷新整个屏幕以将这些信息给擦掉。

下面来看下, 如何搭建Qemu的开发调试环境:

首先参考官网v0.0.1里的文章搭建交叉编译环境(请使用slackware或ubuntu之类的linux系统, bochs可以安装, 也可以不安装, 因为v3.0.0版本使用的是Qemu来开发的, 
如果安装bochs的话, 还需要按照v2.0.0版本的要求加入e1000网卡的支持, 以及按照v2.4.0版本的要求, 修改bochs-2.6里的pci_ide.cc文件, 启动startBochs时,
还需要root权限

从zenglOX v3.1.0版本开始，不再提供bochs相关的启动脚本和相关配置文件，因为，v3.1.0及后续版本已经不适合在bochs下进行开发了，
zenglOX v3.1.0版本只提供qemu的启动脚本和qemu相关的虚拟磁盘。

首先从sourceforge网盘的v3.0.0文件夹里下载qemu-2.2.0.tar.bz2 , 假设我们下载到了Downloads目录中:

zengl@zengl.com:~/Downloads$ ls
qemu-2.2.0.tar.bz2 .....
zengl@zengl.com:~/Downloads$ 

接着解压qemu-2.2.0.tar.bz2压缩包:

zengl@zengl.com:~/Downloads$ tar xvf qemu-2.2.0.tar.bz2
zengl@zengl.com:~/Downloads$ ls
qemu-2.2.0  qemu-2.2.0.tar.bz2 .....
zengl@zengl.com:~/Downloads$ 

解压后, 应该在Downloads目录中会多出一个qemu-2.2.0的文件夹, 接着创建qemu-build目录(编译时的中间文件都会生成到该目录内):

zengl@zengl.com:~/Downloads$ mkdir qemu-build

cd进入qemu-build目录:

zengl@zengl.com:~/Downloads$ cd qemu-build/
zengl@zengl.com:~/Downloads/qemu-build$ 

在进行configure配置之前, 我们需要先安装qemu所需的一些依赖项(下面以ubuntu为例进行说明):

zengl@zengl.com:~/Downloads/qemu-build$ sudo apt-get install libsdl1.2-dev

=========================
libsdl1.2-dev为sdl库, 因为qemu启动时需要SDL库来显示界面, 如果没有安装sdl库的话, 启动qemu时就会出现:
VNC server running on`127.0.0.1:5900'的信息, 然后qemu就停在这条信息上, 没反应了
=========================

zengl@zengl.com:~/Downloads/qemu-build$ sudo apt-get install autoconf automake libtool

=========================
安装automake之类的工具, 防止配置时出现: ./autogen.sh: 4: autoreconf: not found 之类的错误
=========================

zengl@zengl.com:~/Downloads/qemu-build$ sudo apt-get install zlib1g-dev

=========================
安装zlib库和相关的头文件, 防止配置时出现:
   Error: zlib check failed
   Make sure to have the zlib libs and headers installed. 的错误
=========================

zengl@zengl.com:~/Downloads/qemu-build$ sudo apt-get install libglib2.0-dev

=========================
安装glib库, 防止配置时出现:
ERROR: glib-2.12 required to compile QEMU 的错误
=========================

以上是作者安装qemu时出现的一些依赖问题, 如果读者有别的依赖没安装的话, 当出现错误时, 请根据错误信息在网络中搜索解决方案. 

安装完依赖项后, 对于zenglOX v3.1.0及之后的版本，由于引入了sound blaster 16之类的声卡的驱动，并且，声音输出在qemu下执行时，它的I/O线程
会很繁忙，以至于在Ubuntu系统下，如果不开启kvm硬件加速的话，qemu的I/O线程会严重阻塞VCPU线程，从而导致qemu的CPU指令无法正常执行，也就会让
qemu里的音乐播放，图形界面渲染，鼠标操作等一系列功能都无法正常运作(主要是在播放音乐过程中)。
因此，对于zenglOX v3.1.0及之后的版本就必须开启KVM硬件加速(至少在ubuntu的系统下是这样的)。

要开启KVM(也就是处理器的虚拟化技术), 
读者可以参考 http://www.linuxfromscratch.org/blfs/view/svn/postlfs/qemu.html 该链接里的内容。

该链接中，提示读者可以先用如下命令，来查看处理器是否支持虚拟化技术:

egrep '^flags.*(vmx|svm)' /proc/cpuinfo

如果什么输出信息都没有，那么很遗憾，你的处理器不支持虚拟话技术(或者是: 你的Linux是安装在VirtualBox之类的虚拟机中的，VirtualBox不会将
处理器的虚拟化功能，传递给运行在其中的系统)，
如果该命令产生了任何输出信息的话，就说明你的处理器支持虚拟化技术，接着就可以进入BIOS中查看该虚拟化技术是否开启了，如果没开启就手动开启。

作者的台式机用的是技嘉的主板(型号GA-F2A85XM-DS2)，在该主板的BIOS里的M.I.T.页面下，
有一个Advanced Frequency Settings项目，进入该项目后，
再进入Advanced CPU Core Features项目，该项目里，有一个SVM Mode选项，该选项默认是Enabled开启状态，因此，作者的技嘉主板
默认就开启了CPU的虚拟化技术。作者的华硕笔记本默认也是开启了处理器的虚拟化技术的。对于其他主板，则需要查看主板的说明书，或者网上搜索。

如果CPU不支持虚拟化技术的话，就只能将生成的iso镜像放置到VirtualBox或VMware下进行测试了.

在开启了CPU的虚拟化功能后，还需要查看Linux内核是否将KVM编译为了内建模式，或者将KVM编译为了模块模式，在上面那个链接里
已经写的比较详细了，这里就不多说了，在ubuntu系统下(作者所使用的是ubuntu 12.04的系统)，KVM默认就被编译为了模块模式，
读者可以使用 modinfo kvm-intel 和 modinfo kvm-amd 命令来查看这两个模块的相关信息，对于Intel处理器，需要使用kvm-intel模块,
对于AMD处理器，则需要使用kvm-amd模块。要加载kvm-amd模块，可以使用sudo modprobe kvm-amd命令，要加载kvm-intel模块，
则可以使用sudo modprobe kvm-intel命令，如果你的处理器不支持某个模块的话，
modprobe命令执行时，就会抛出Operation not supported的信息，并且不会去加载该模块，因此，
你不需要担心，因加载到了错误的模块而影响到系统的正常运行。

如果想让kvm-intel或kvm-amd模块在启动时，自动被加载的话，在ubuntu系统下，可以在/etc/modules文件中加入对应的模块名.

要查看模块是否被成功加载了，可以通过 lsmod | grep kvm 命令来查看。如果显示出了kvm的相关信息，那么就说明对应的模块被加载成功了。

在kvm模块被成功加载后，qemu不需要什么额外的配置(qemu本身就支持kvm)，只需在qemu的启动命令行参数中，加入-enable-kvm参数即可，
从zenglOX v3.1.0版本开始，作者默认就在startQemu的脚本文件中，加入了-enable-kvm的参数。如果你的Linux系统环境下，不需要开启kvm
硬件加速，qemu也能正常播放音乐的话，可以将该参数去掉。不过如果能开启kvm硬件加速的话，最好还是开启，因为它可以有效的提高qemu的
执行速度。

此外，从zenglOX v3.1.0版本开始，startQemu脚本文件的开头，加入了一条export QEMU_AUDIO_DRV=alsa命令，这条命令用于强制要求
qemu使用ALSA音频驱动来播放声音。

要让qemu播放出声音，首先需要确保qemu所在的Linux系统能发出声音，如果Linux系统都不能发出声音的话，运行在Linux系统中的qemu就
更不可能发出声音了。

作者的台式机安装的是ubuntu 12.04的Linux发行版, 一开始该系统是没有声音的，alsamixer命令执行后，显示出"
This sound device does not have any controls"的信息，根本没有音量可供调节，因此，应该是alsa音频驱动的问题。
作者就参考 http://blog.csdn.net/rainysia/article/details/12907443 这篇文章将ALSA音频驱动重新安装了一下
(作者也是realtek alc887的声卡)，同时参考 http://blog.csdn.net/wangzhilife/article/details/7881722 和
http://blog.163.com/seven_7_one/blog/static/162606412201185114049121 这两个链接里的文章，将ALSA相关的
alsa-lib及alsa-utils都安装了一遍(安装时，要注意alsa-driver，alsa-lib及alsa-utils的版本号要一致)，
重启后，alsamixer就可以正常调节音量了, ubuntu也就可以正常播放声音了。

作者的笔记本上，装的也是ubuntu 12.04的系统，但是，默认就可以播放声音，因为，现在的Linux内核中，默认就集成了alsa的音频驱动，
作者笔记本上的声卡比较旧，内核集成的alsa驱动可以识别出来, 不像作者台式机上的声卡, 内核集成的alsa驱动无法识别，就只有到
alsa的官方网站(http://www.alsa-project.org/), 下载新版的驱动来安装了。当然，你也可以将ubuntu 12.04升级到14.04或者
15.04的新版本，让新版本的内核中集成的alsa驱动来识别。不过，作者不喜欢升级，一方面要等满久，另一方面，新版本是否稳定也不清楚。

对于2.6.x的linux内核，你可以从alsa官网的old drivers (alsa-driver)的ftp站点(当前地址为
ftp://ftp.alsa-project.org/pub/driver/)，下载到1.0.23，1.0.24或者1.0.25的alsa-driver来进行编译安装，
对于3.x.x或者更高版本的Linux系统，alsa驱动已经被彻底整合进Linux的内核源码中了，
alsa官方已经不再提供单独的alsa-driver的下载地址了。

在系统加入了ALSA音频驱动以及相关的alsa-lib, alsa-utils的情况下，如果系统能正常播放声音，那么，接下来，就只需在qemu的配置中
加入alsa的支持即可, 对于zenglOX v3.1.0之前的版本，你可以使用如下配置: 

zengl@zengl.com:~/Downloads/qemu-build$ ../qemu-2.2.0/configure --target-list=i386-softmmu --enable-debug --disable-pie

对于zenglOX v3.1.0之后的版本，就需要使用如下所示的配置:

zengl@zengl.com:~/Downloads/qemu-build$ ../qemu-2.2.0/configure --target-list=i386-softmmu --enable-debug --disable-pie --enable-sdl --audio-drv-list='oss alsa sdl'

增加了--enable-sdl和--audio-drv-list='oss alsa sdl'两个配置参数，--enable-sdl表示采用SDL来进行渲染，有的linux系统中会
使用gtk来渲染，gtk渲染下，播放声音时，会阻塞gtk的渲染工作。在startQemu的启动脚本中，也加入了一条-display sdl的启动参数，表示
强制使用SDL引擎来进行渲染，--audio-drv-list='oss alsa sdl'表示qemu将支持oss，alsa及sdl的音频驱动，这样在startQemu脚本文件里
加入export QEMU_AUDIO_DRV=alsa命令后，qemu就会通过alsa音频驱动来播放声音了，如果没有--audio-drv-list='oss alsa sdl'这条
配置信息的话，那么qemu就将只支持原始的oss音频驱动，当export QEMU_AUDIO_DRV=alsa命令执行后，再启动qemu时，会提示
audio: Unknown audio driver `alsa'的信息，这样qemu就无法通过alsa音频驱动来播放声音了，因此，必须加入
--audio-drv-list='oss alsa sdl'这条配置信息。

在使用configure对qemu配置完后，就可以通过make和make install命令来编译安装qemu了:

zengl@zengl.com:~/Downloads/qemu-build$ ../qemu-2.2.0/configure --target-list=i386-softmmu --enable-debug --disable-pie --enable-sdl --audio-drv-list='oss alsa sdl'
zengl@zengl.com:~/Downloads/qemu-build$ make
zengl@zengl.com:~/Downloads/qemu-build$ sudo make install
zengl@zengl.com:~/Downloads/qemu-build$ qemu-system-i386 --version
QEMU emulator version 2.2.0, Copyright (c) 2003-2008 Fabrice Bellard
zengl@zengl.com:~/Downloads/qemu-build$ 

上面configure配置时的--target-list=i386-softmmu是让make编译时, 只生成x86的qemu模拟器, 如果不指定该参数的话, 就会将arm之类的模拟器也生成一遍, 
那就会编译很久了, 而且我们也不需要arm之类的测试平台. --enable-debug参数让你可以调试qemu源代码(它会保留调试所需的符号信息), --disable-pie参数
也是用于调试的, 它会让生成的qemu的指令代码不是与位置无关的, 只有这样才能正常调试qemu的源代码, 没有这个参数的话, gdb调试qemu源代码时会出现问题.
--enable-sdl与--audio-drv-list配置参数的含义，在上面已经解释过了，这里就不多说了。

在configure配置生成Makefile文件后, 就可以通过make命令来进行编译了, 编译完后, 可以通过sudo make install来安装qemu到系统中, qemu默认会安装到
/usr/local/bin位置处, 安装时需要root权限, 因此就用到了sudo命令, 安装完后, 可以通过qemu-system-i386 --version来查看qemu的版本号信息, 如果版本
号是2.2.0的话, 就说明你已经将qemu正确的安装到系统中了.

zenglOX内核源代码的编译过程, 与之前版本是一样的, 假设v3.0.0版本的源文件位于zenglOX_v3.0.0文件夹内, 那么直接用make和make iso就可以生成zenglOX.bin,
initrd.img及zenglOX.iso文件:

zengl@zengl.com:~/zenglOX/zenglOX_v3.0.0$ make
zengl@zengl.com:~/zenglOX/zenglOX_v3.0.0$ make iso
zengl@zengl.com:~/zenglOX/zenglOX_v3.0.0$ chmod +x startQemu 
zengl@zengl.com:~/zenglOX/zenglOX_v3.0.0$ ./startQemu 

上面的chmod +x startQemu命令是让startQemu脚本文件具有可执行权限, ./startQemu命令在启动qemu时, qemu会像之前版本的bochs一样等待gdb的连接(处于暂停状态), 

然后另起一个终端, 使用gdb来连接:
zengl@zengl.com:~/zenglOX/zenglOX_v3.0.0$ gdb -q zenglOX.bin
Reading symbols from /home/zengl/zenglOX/zenglOX_v3.0.0/zenglOX.bin...done.
(gdb) target remote localhost:1234
Remote debugging using localhost:1234
0x0000fff0 in ?? ()
(gdb) b zlox_kernel_main 
Breakpoint 1 at 0x10003c: file zlox_kernel.c, line 36.
(gdb) c
Continuing.

Breakpoint 1, zlox_kernel_main (mboot_ptr=0x10000, initial_stack=524032)
    at zlox_kernel.c:36
36	{
(gdb) c
Continuing.

上面gdb连接时也是用的1234端口, 因为startQemu脚本里设置的就是1234端口, 上面在一开始时, 通过b命令来设置了断点, 当中断下来后, 直接c命令继续执行, 你也可以不下断点, 直接c命令, 让qemu一路执行下去, 稍后需要下断点时, 可以在gdb里通过 ctrl + c 组合键将qemu中断下来.

如果要用bochs来运行的话, 只需要chmod +x startBochs , 然后 sudo ./startBochs 运行即可, bochs运行时的相关注意事项, 请参考v3.0.0之前的版本的官方文章说明.

如果要将zenglOX写入U盘, 然后通过U盘启动的话, 需要先将linux中的grub2安装到U盘中(只有grub2才能启动到VBE图形模式, 传统的grub在没打相关的patch补丁之前,
无法启动到VBE图形模式)

将grub2写入U盘的方法, 可以参考 http://www.kissthink.com/archive/ru-he-zai--i-n-u-x-zhi-zuo-qi-dong--pan--shi-yong-g-r-u-b-huo-g-r-u-b-2-.html 该链接对应的文章(这个链接地址好长啊!).

例如作者的U盘被ubuntu自动挂载到了/media/KINGSTON目录, U盘在系统中的设备节点为/dev/sdb , 输入如下命令:

sudo grub-install --force --no-floppy --root-directory=/media/KINGSTON/ /dev/sdb

grub2安装好了，此时，在你的U盘根目录下应该存在一个"boot/grub/"的目录，里面存放有一些文件。

将源代码根目录中的grub.cfg文件放置到boot/grub目录内, 然后将编译生成的initrd.img与zenglOX.bin文件放置到boot目录中. (grub.cfg, initrd.img及
zenglOX.bin文件在sourceforge网盘的v3.0.0的文件夹内也有, 可以直接下载到)

这样, 就可以用U盘来启动zenglOX了, 重启机子, 在BIOS中将U盘设置为启动盘, 再重启就可以先看到grub2的菜单界面, 接着就会进入zenglOX了.

不过真机上, 只能使用ramdisk里的程式.

此外, qemu在v3.0.0版本中, 使用的是user用户模式的网络, 只能ping到网关, 要ping外网的话, 需要用到tun/tap, 由于配置比较麻烦, 放在v3.0.0之后的版本再做, 
bochs, virtualbox及vmware下, 应该可以ping到外网的ip. (zenglOX中和网络相关的内容请参考 zenglOX v2.0.0版本相关的官方文章)

v3.0.0版本中, 窗口管理器的核心代码位于源码根目录下的zlox_my_windows.c文件里, 桌面程式的源代码位于build_initrd_img/desktop.c文件中, term程式的
源代码位于build_initrd_img/term.c文件中, 鼠标相关的驱动程式位于源码根目录下的zlox_mouse.c文件中.

作者: zenglong
官网: www.zengl.com
```
