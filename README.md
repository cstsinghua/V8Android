# V8Android
A demo APP for embedding V8 engine in Android APP

1.git clone https://github.com/cstsinghua/V8Android.git;

2.enter app/src/main/cpp/static_lib directory,unzip v8_arm_arm64.zip,then copy all subdirs(arm64-v8a/armeabi-v7a/include/v8_src_include) to app/src/main/cpp directory(overwrite the old files);

3.open Android studio(version 3.1 is recommended),open and load this project;

4.run it and view the result.（you can edit Java and native code if you will）

# 背景
最近公司的移动引擎(自研，用于公司的游戏APP开发，引擎核心采用C++开发，而游戏的UI和业务逻辑采用Lua语言开发)需要支持Javascript和Lua互相调用(支持Android和IOS两大平台)。刚开始的时候，没有什么头绪。由于之前实现过Lua和Android/IOS原生语言(API)即Java/Object-C的互调，其中Android平台交互原理大致如下图(本文主要基于Android平台讲解。IOS下，OC调用C++更简单，这里暂不赘述)：
![](https://i.imgur.com/q5Qk8li.jpg)

由此，大概思考了下，如果要实现JavaScript和业务层的Lua互调，那么应该与上面的原理类似。这基于以下几点分析：
1. Javascript和java一样，都是解释性语言，需要类似VM的虚拟机(一般采用C或C++语言实现)来执行。java世界里，JVM是我们所熟知的。而JS领域，在移动平台方面，除了大名鼎鼎的google的v8引擎，还有其他的一些JS引擎(相当于VM)，比如JavaScriptCore、SpiderMonkey和Rhino(java实现，从这个角度看，Rhino似乎直接在Android上可以无缝对接)。关于JS引擎介绍，可以参见维基百科[JavaScript_engine介绍](https://en.wikipedia.org/wiki/JavaScript_engine "JavaScript_engine")
2. 既然JS引擎是C/C++实现，在Android中，嵌入进引擎的so库中，便可实现JS与C++互调，而Lua语言天生就是C/C++的寄生语言，从而就可以建立JS和Lua互调的纽带。

基于性能和平台适配考虑，最终选择v8作为Android平台的嵌入JS引擎，而JavaScriptCore作为IOS平台的嵌入引擎。本文主要讲解，如果从v8的源码构建出Android平台的嵌入库，然后通过Android NDK开发，进而实现在Android APP(非Hybrid应用，即不会通过Webview来运行JS代码)中运行JS代码，JS与C/C++、Lua互调。

# 平台和环境的选择
如果有过大型开源C++项目编译的经验，就知道，选择开发系统和环境是非常重要的，一般而言，在Windows下编译真的是非常艰难。所以文本先以Linux系统(这里选择Ubuntu)为例，讲解如何构建出适用于Android ARM架构的JS引擎嵌入静态库。再讲解Windows平台下的编译构建。

**注：v8的构建真的是非常复杂和繁琐的过程，各种坑，各种层出不穷的问题。另外一篇文章[编译及嵌入v8遇到的错误汇总](http://cstsinghua.github.io/2018/04/13/%E7%BC%96%E8%AF%91v8%E9%81%87%E5%88%B0%E7%9A%84%E9%94%99%E8%AF%AF%E6%B1%87%E6%80%BB/)详细记录了学习v8构建过程中遇到的一些问题，以及分析和解决途径、链接等，以供探讨。**

<a name="anchor1"></a>
## Linux(Ubuntu 16.04)下构建用于Linux/Mac x64版本的v8(可执行的二进制文件)
v8项目在Github的官方镜像地址为[https://github.com/v8/v8](https://github.com/v8/v8)。而其官方的[wiki文档](https://github.com/v8/v8/wiki)也在这个仓库中。但是如果直接按照这个wiki去构建，失败几乎是不可避免。因为wiki的步骤非常简化(当然先总体阅览一下wiki还是大有必要，而且当回过头来对照的时候将会发现"原来是这样，Soga^_^")，有些关键的环境配置和要点被省略掉，所以如果不跳过这些坑，将浪费大量的时间，甚至有很严重的挫败感。本文的目的就是为了对v8的构建做一个记录，总结遇到的问题，便于以后查阅，也为他人提供一个便捷的指引。

这里，构建主机(Host)是Ubuntu 16.04系统，而目标平台(Target)是Linux/Mac x64。

如果没有单独的Ubuntu主机，那么可以在Windows主机下安装[virtual box](https://www.virtualbox.org/)或者[VMware](https://www.vmware.com/)，建议安装virtualbox，相对来说更轻量级，这里足够满足需求。然后利用virtualbox加载Ubuntu 16.04的ISO镜像(可网上下载，或找CoulsonChen索取)，创建虚拟机环境，即可在虚拟机环境下实现编译。

### 具体步骤
1. 安装Git，在终端里面输入下面命令：
    > apt-get install git
2. 安装depot_tools(详细可参见[安装depot-tools](http://dev.chromium.org/developers/how-tos/install-depot-tools "安装depot-tools"))，在终端输入下面命令：
    > git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git

	将depot_tools的目录添加到系统的PATH路径，可以将depot_tools所在的目录加入到`~/.bashrc`或 `~/.zshrc`(~代表当前用户目录，一般为/home/yourName/，即打开终端时候默认进入的目录)，这样对当前用户的环境变量都生效了)。如果只是在当前终端环境下添加(假定depot_tools所在的目录为/path/to/depot_tools），则执行:
    > export PATH=/path/to/depot_tools:$PATH

	depot_tools已经自带了GN工具(GYP从V8 6.5版本开始就不再使用。请使用GN代替)。V8的构建需要借助于GN。GN是一个元构建系统(meta build system of sorts)，因为它为其他构建系统(比如ninja)生成构建文件(it generates build files for a number of other build systems)。也就是说，GYP和GN 并不是构建系统，V8使用的构建系统使用的是Ninja，而GN是用来生产Ninja构建文件的工具。
3. 更新depot_tool工具，在终端输入如下命令(注意，该命令不带任何参数)：
    > gclient

	该步骤更新depot_tool工具，特别如果是Windows系统下面构建，这一步骤将会在depot_tool目录下下载Git和python(2.7版本)
4. 选择一个目录(该目录用于存放下载v8的源码），比如`/usr/local/v8/`，**在该目录下**打开终端，执行如下命令：
    >fetch v8

    然后进入v8的源文件目录
    >cd v8
    
5. 步骤1-4确保工具和v8的源码下载完成，在执行第5步前，再次确认当前工作目录已经在v8的源目录下(步骤4中的示例来看的话，就是`/usr/local/v8/v8/`)。然后在当前工作目录(比如`/usr/local/v8/v8/`)下执行如下命令，该命令将会下载所有的构建依赖项(Download all the build dependencies）
    > gclient sync

6. (这一步只在linux系统构建时才需要执行，且只需要执行一次--(**only on linux, only needed once**) 安装构建依赖项：
    > ./build/install-build-deps.sh

7. 生成目标平台必需的构建文件(Generate the necessary build files by executing the following in your terminal/shell）:
    > tools/dev/v8gen.py x64.release
	
	**注意：目标平台有很多，可以通过`tools/dev/v8gen.py list`命令查看**。这里以本文写作时的master分支版本为例，执行`tools/dev/v8gen.py --list`命令后，可以看到v8支持如下目标平台构建：
    
		coulsonchen@coulsonchen:/usr/local/v8/v8$ tools/dev/v8gen.py list
		android.arm.debug
		android.arm.optdebug
		android.arm.release
		arm.debug
		arm.optdebug
		arm.release
		arm64.debug
		arm64.optdebug
		arm64.release
		ia32.debug
		ia32.optdebug
		ia32.release
		mips64el.debug
		mips64el.optdebug
		mips64el.release
		mipsel.debug
		mipsel.optdebug
		mipsel.release
		ppc.debug
		ppc.optdebug
		ppc.release
		ppc64.debug
		ppc64.optdebug
		ppc64.release
		s390.debug
		s390.optdebug
		s390.release
		s390x.debug
		s390x.optdebug
		s390x.release
		x64.debug
		x64.optdebug
		x64.release

8. 编译v8源码(Compile the source by executing the following in your terminal/shell）：
    > ninja -C out.gn/x64.release

	该命令是**编译构建v8运行的所有文件**(building all of V8 run (assuming gn generated to the x64.release folder))。编译完成之后，可以在`/path/to/v8 Source dir/out.gn/x64.release`目录(这里示例即`/usr/local/v8/v8/out.gn/x64.release`)及其子目录下查看构建产生的库和可执行文件。可进入该目录下，执行生成的d8可执行程序，进入javascript的交互命令行模式，示例如下图：
	![](https://i.imgur.com/Y6KhNNw.jpg)
	如果只需要**编译构建指定的文件**，比如d8(build specific targets like d8, add them to the command line)。则可执行如下命令(将要指定的构建文件添加到ninja命令的参数中)：
    > ninja -C out.gn/x64.release d8
9. (**可选,用于测试构建是否OK**)执行测试(Run the tests by executing the following in your terminal/shell)：
    > tools/run-tests.py --gn

<a name="anchor2"></a>
## Linux(Ubuntu 16.04)下构建用于Android arm版本的v8(可执行的二进制文件)
这里，构建主机(Host)是Ubuntu 16.04系统，而目标平台(Target)是Android arm。由于构建平台和目标平台是不同的平台，因此这里，需要使用[交叉编译(Cross compiling)](https://en.wikipedia.org/wiki/Cross_compiler)。

前置条件，和`Linux(Ubuntu 16.04)下构建用于Linux/Mac x64版本的v8(可执行的二进制文件)`章节的1-6步完全一致。接下来，可参考[Cross compiling for ARM](https://github.com/v8/v8/wiki/Cross-compiling-for-ARM "Cross compiling for ARM")。

这里单独对Cross compiling for ARM的步骤做下说明：
1. 在配置好上述提到的环境之后，编辑`.gclient配置文件`(.gclient configuration file)，位于用于存放下载v8的源码的目录，对应于`Linux(Ubuntu 16.04)下构建用于Linux/Mac x64版本的v8(可执行的二进制文件)`章节的步骤4中的说明，这里的目录路径示例为`/usr/local/v8`，如下图：
![](https://i.imgur.com/5RXz9UG.jpg)
打开该文件，在末尾添加如下一行：
`target_os = ['android']`  # Add this to get Android stuff checked out.
示例如下：
![](https://i.imgur.com/lLMsqrq.jpg)
2. 然后在v8的源码目录下(`/usr/local/v8/v8`)，执行如下命令：
    > `gclient sync`

	一旦配置了`target_os = ['android']`，再执行`gclient sync`，将会在v8的源码目录下下载Android相关的工具和文件，主要包括android_tools和android_ndk等(示例，对应目录`/usr/local/v8/v8/third_party/android/android_tools`和`/usr/local/v8/v8/third_party/android_ndk`)。**注意：这些文件非常大，有10G左右，所以需要下载很长时间，网络如果不好的话，会很痛苦**
3. 利用8gen.py生成ARM架构编译时必要的构建文件(和`Linux(Ubuntu 16.04)下构建用于Linux/Mac x64版本的v8(可执行的二进制文件)`章节的第7步类似)。在v8的源码目录下(`/usr/local/v8/v8`)，执行如下命令：
    > tools/dev/v8gen.py arm.release
4. 然后，进入生成的`arm.release`子目录(示例，`/usr/local/v8/v8/out.gn/arm.release`)，编辑其中的`args.gn`文件，在其中添加如下几行(如果要查看所有可配置的参数，可以先执行命令`gn args out.gn/arm.release --list`查看)：

        target_os = "android" 
        target_cpu = "arm"  
        v8_target_cpu = "arm"
        is_component_build = false
        
	![](https://i.imgur.com/Da1SRZK.jpg)
    
	![](https://i.imgur.com/JpaKJRR.jpg)
    
	如果是arm64设备，则上面几行应该替换为：
    
        target_os = "android"     
        target_cpu = "arm64"
        v8_target_cpu = "arm64"
        is_component_build = false
5. 编译构建。这一步与`Linux(Ubuntu 16.04)下构建用于Linux/Mac x64版本的v8(可执行的二进制文件)`章节的第8步类似。执行如下命令(构建全部目标文件)：
    > ninja -C out.gn/arm.release

	或(只构建d8)
    > ninja -C out.gn/arm.release d8
6. 构建完成后，可以将生成的d8及其相关文件push到Android手机中，体验一把，看看效果。可以通过adb工具来将文件push到手机(adb工具已经在步骤2完成后下载了，具体目录`/usr/local/v8/v8/third_party/android_tools/sdk/platform-tools`。当然你也可以直接通过`sudo apt install android-tools-adb`命令来额外安装adb。然后adb所在路径添加到PATH路径中)。在v8源文件目录下(`/usr/local/v8/v8`)执行如下命令：
    > adb push out.gn/arm.release/d8 /data/local/tmp
    
    > adb push out.gn/arm.release/natives_blob.bin /data/local/tmp
    
    > adb push out.gn/arm.release/snapshot_blob.bin /data/local/tmp
	
	然后，通过adb shell进入到Android手机的交互shell中。

        > $ adb shell
        > $ cd /data/local/tmp
        > $ ls
            v8 natives_blob.bin snapshot_blob.bin

        > $ ./d8
        V8 version 5.8.0 (candidate)
        d8> 'w00t!'
        "w00t!"
        d8> 

<a name="anchor3"></a>
## Linux(Ubuntu 16.04)下构建用于Android arm版本的v8(静态库，用于Android NDK链接和封装)

注：已经有人在Github上提供编译好的v8静态库供Android NDK嵌入开发，这样可以节省很多时间(只是并非对应v8的最新版，不过也是次新版)。请参见[https://github.com/social-games/CompiledV8](https://github.com/social-games/CompiledV8)

首先理一下思路：

1. 先利用交叉编译，构建出用于Android平台的v8静态库文件；
2. 利用Android studio进行NDK开发，将步骤1中生成的静态库文件进行链接封装，向Java层暴露方法(对应java层的native方法)，最终生成动态库文件；
3. Android APP内部，原生的Java层可以通过JNI调用步骤2中生成的动态库中的方法(去加载JS代码并执行相关逻辑，然后返回相关数据，反之亦然)

实施前的题外话：
	如果是为了练练手，那么可以采用v8的最新master分支进行构建即可；但如果是为了用于生产环境，那么请一定采用最新的稳定版本来构建。怎么查询到最新的稳定版本是多少呢？请参见[官方原文说明](https://github.com/v8/v8/wiki/Version-numbers)。本文写作时最新的适用于Android平台的分支是`6.5.254.43`。因此，在执行前，需要先逐一执行以下命令：
	
	git pull
	
	git checkout branch-heads/6.5
	
	gclient sync

具体实施步骤：
1. 首先与章节`Linux(Ubuntu 16.04)下构建用于Android arm版本的v8(可执行的二进制文件)`(#anchor2)的步骤基本一致，但需要在其第3步修改目标构建平台(即将arm.release改为android.arm.release)，执行命令替换为：
    > tools/dev/v8gen.py gen -m client.v8.ports -b "V8 Android Arm - builder" android.arm.release
2. 首先与章节`Linux(Ubuntu 16.04)下构建用于Android arm版本的v8(可执行的二进制文件)`(#anchor2)的步骤基本一致，只是需要在其第4步修改下`args.gn`配置文件，如下：

        is_component_build = false
        is_debug = false
        symbol_level = 1
        target_cpu = "arm"
        target_os = "android"
        use_goma = false
        v8_android_log_stdout = true
        v8_enable_i18n_support = false
        v8_static_library = true

	注意：use_goma = false、v8_static_library = true、v8_enable_i18n_support = false都需要添加
3. 然后重新执行构建，构建完成之后，在v8源码目录下的子目录`out.gn/android.arm.release/obj`下可以找到对应的静态库。然后按照[这篇文章](https://medium.com/@hyperandroid/compile-v8-for-arm-7-df45372f9d4e)的步骤，将相关静态库和头文件聚合整理到一个单独的目录(比如libs，最终将libs目录copy到Android Studio的工程的cpp源文件目录下)。这里列出关键示例代码：
```cmd
	// Create fat lib files. 
	// You also could add all .* files into one single library.
	// 
	cd out.gn/android_arm.release/obj
	mkdir libs
	cd libs
	// one lib to rule them all.
	ar -rcsD libv8_base.a ../v8_base/*.o
	ar -rcsD libv8_base.a ../v8_libbase/*.o
	ar -rcsD libv8_base.a ../v8_libsampler/*.o
	ar -rcsD libv8_base.a ../v8_libplatform/*.o 
	ar -rcsD libv8_base.a ../src/inspector/inspector/*.o
	// preferred snapshot type: linked into binary.
	ar -rcsD libv8_snapshot.a ../v8_snapshot/*.o 
	// depending on snapshot options, you should left out 
	// v8_snapshot files and include any of these other libs.
	ar -rcsD libv8_nosnapshot.a ../v8_nosnapshot/*.o
	ar -rcsD libv8_external_snapshot.a ../v8_external_snapshot/*.o
	// source headers, for inspector compilation.
	mkdir -p src/base/platform
	cp -R ../../../../src/*.h ./src
	cp -R ../../../../src/base/*.h ./src/base
	cp -R ../../../../src/base/platform/*.h ./src/base/platform
	// copy v8 compilation header files:
	cp -R ../../../../include ./
	// For compilation on Android, always **use the same ndk** as 
	// `gclient sync` downloaded. 
	// As of the time of this writing it was `ndk r12b`
	// Enjoy v8 embedded in an Android app
```
	关于libv8_snapshot.a/libv8_nosnapshot.a/libv8_external_snapshot.a在使用的时候任选其一，这三者的区别如下：
	Currently, snapshots are compiled by default. These snapshots will contain base objects, like for example, Math. There’s no runtime difference among them, just different initialization times. In my nexus 5x, no snapshot takes around 400ms to initialize an Isolate and a Context, and around 30 with snapshot. The external snapshot and snapshot differ in that the external snapshot must be explicitly loaded (.bin files in the compilation output), and snapshot library is a static lib file of roughly 1Mb in size, that will be linked with the final .so file binary instead of externally loaded. Bear in mind that snapshot libs, internal or external, would require you to supply some extra native code for reading the Natives (.bin) files.
4. 新创建一个Android Studio工程(支持C++，利用默认的向导创建完成即可。注：这里AS的版本是3.1，默认采用cmake构建C++代码)，然后将步骤2得到的libs拷贝到工程的cpp源文件对应的目录下。然后就是编辑工程的CMakeLists.txt文件，将libs里面的静态库链接进来。具体代码和配置可以参考我的[Github demo](https://github.com/cstsinghua/V8Android)。另外也可参考[这篇文章](https://medium.com/@hyperandroid/android-v8-embedding-guide-f64173f7958b)。

# win10构建v8 engine的心路历程

这里可以完全参照[https://medium.com/dailyjs/how-to-build-v8-on-windows-and-not-go-mad-6347c69aacd4](https://medium.com/dailyjs/how-to-build-v8-on-windows-and-not-go-mad-6347c69aacd4)这篇文章的步骤来完成。

但需要强调的是，这篇文章编写的时候，当时的v8版本可以使用VS 2015编译，而v8的最新版本(截止到2018.4.4)要求VS 2017(随着时间推移，可能后续会要求更新版本的VS，这里就必须注意，可以在下载下来的v8源目录下面的build子目录下的vs_toolchain.py文件中查看默认的VS版本，如下图)，所以这篇文章里面关于VS 2015的部分，请替换成VS 2017。另外，关于Windows SDK的部分，也请下载最新版本的。
![](https://i.imgur.com/KjQ0CNp.jpg)
