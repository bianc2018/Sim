# Sim
## 简介 
  Sim是一个Head only的函数库，基本一个头文件实现一个功能模块。此函数库致力于轻量化应用场景，实现一些常用的功能模块，开发过程中仅仅只需包含少量头文件即可使用，而不需要为了使用某一个功能，特定去编译一个功能复杂的程序库。

## 功能
  目前实现的功能有： 
  
  1. Array.hpp 
 
    动态数组容器。

  + Async.hpp
   
    异步网络IO接口，windows系统下使用IOCP实现，linux系统下面使用EPOLL实现。

    支持TCP Client 、Tcp Server和UDP ，以及支持SSL加密通讯。
  + AsyncHttp.hpp
  
    异步Http服务，支持http 、https、ws和wss协议。
  + Base64.hpp

    Base64 加密和解码接口。
  + CmdLineParser.hpp

    命令行解析器，以特定的格式解析命令行参数 
  + CodeConvert.hpp

    编码转换接口，**目前仅仅实现了windows环境下的Utf8与本地编码的相互转换接口**。
  + HttpParser.hpp

    HTTP协议解析接口。
  + Logger.hpp

    日志库。
  + Mutex.hpp

    互斥锁。
  + Queue.hpp

    单向队列。
  + RbTree.hpp

    红黑树。
  + RefObject.hpp

    引用对象。实现了共享指针和共享缓存。
  + Sha1.hpp

    实现了 SHA1摘要 算法。
  + Socket.hpp

    封装了系统底层的套接字接口，使用select实现接口超时等待功能。
  + SSLCtx.hpp
  
    封装OPESSL接口，实现了SSL通讯的握手、和加解密功能接口。
  + TaskPool.hpp

    任务池。
  + Test.hpp
    
    测试框架。
  + Thread.hpp

    系统底层线程函数接口封装。
  + Timer.hpp

    定时器接口。
  + WebSocketParser.hpp

    websocket协议解析器。
 
   
## 安装配置
   无需安装，只需要将```Sim\include```下的对应头文件包含到你的工程中即可。

## 快速教程
   参考 ```demo```

## API 文档
   暂无
