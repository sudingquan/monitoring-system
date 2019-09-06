# monitoring_system

Master端：

使用`make && make clean && sudo make install`来进行安装

其中，配置文件为`/etc/pihealth_master_sdq.conf`

![e6c20e355fa8380a4bde11ef99698c30.png](http://39.105.82.248/images/2019/09/06/e6c20e355fa8380a4bde11ef99698c30.png)

可以对各项参数进行配置

然后便可以使用`systemctl`来对本项目进行状态管理：

![bc3411a35c771d644bcc054ddaba50f4.png](http://39.105.82.248/images/2019/09/06/bc3411a35c771d644bcc054ddaba50f4.png)

运行时日志如下：

![7c493ff5df2eecbae0cb6a7bd5bf3a49.png](http://39.105.82.248/images/2019/09/06/7c493ff5df2eecbae0cb6a7bd5bf3a49.png)

同样使用`systemctl`来停止该守护进程：

![850f5a99e1f45583e191b0d2bec683b2.png](http://39.105.82.248/images/2019/09/06/850f5a99e1f45583e191b0d2bec683b2.png)

```
.
├── Client
│   ├── Makefile
│   ├── client.c
│   ├── common.c
│   ├── common.h
│   ├── pihealth_client_sdq.conf.sample
│   └── send_client.sh
├── Master
│   ├── Makefile
│   ├── common.c
│   ├── common.h
│   ├── master.c
│   └── pihealth_master_sdq.conf.sample
├── README.md
└── script
    ├── CpuLog.sh
    ├── Detect.sh
    ├── Disk.sh
    ├── MemLog.sh
    ├── SysInfo.sh
    └── Users.sh

3 directories, 18 files
```
