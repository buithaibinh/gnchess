# Gnchess是什么 #
Gnchess是一个中国象棋软件，主要目的是为了满足自己和朋友对战。
# 主要功能 #
  * 人机对战
  * 联网对战(PC上没做，只做了在Android下用蓝牙对战)
# 目标平台 #
Linux / Windows / Android(>=2.2)
# 为什么开发Gnchess #
主要是各种学习。Gnchess的目标很简单：简单易用就行了。
# 开发简介 #
  * Gnchess的人机对战算法基于开源软件象棋小巫师，超高水平不是我的目的，而这个水平已经足够平时玩玩了，其实一开始我是自己写的算法，不过写来写去棋力都不太高，后来还是决定参考参考别人是怎么写的了。关象棋巫师，请参考：[象棋百科全书](http://www.elephantbase.net/)。
  * 软件的目标平台：目标平台是 Linux/Windows/Android(这就不是一个工程，源代码我单放在下载里了)
  * 代码采用C++/Gtkmm开发(Android上当然是Java+JNI了)，出于美观考虑，棋子都是使用SVG图片绘制的，所以依赖 librsvg2 库，编译的时候需要下载，声音直接使用 play 来播放，所以运行时需要安装 sox 软件包（Windows上都不用）。
## 现有功能 ##
  * 人机对战
  * 棋盘大小可调且自适应屏幕(指的是pc上)
  * 难度3级可调
  * 支持声音
# Android上 #
有个编译好的包，装上就能玩。（Android 系统版本 >= 2.2）
# Windows上 #
有个编译好的包，解压就能玩。
# 在Linux上安装 #
> Linux上目前只有Ubuntu 9.10 的源(那会儿9.10还是最新的呢，变化真快...)
  1. 加入PPA源：
> > `sudo echo 'deb http://ppa.launchpad.net/thor-qin/ppa/ubuntu karmic main' >> /etc/apt/sources.list`
  1. 导入密钥
> > `sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 15E7E7FD`
  1. 安装
> > `sudo apt-get update && sudo apt-get install cnchess`

**其实不用什么源，自己make & make install 就很好了。**