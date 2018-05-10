# 部署流程

## 服务端

下载mongodb安装包，并安装，请参考

[官方文档](https://docs.mongodb.com/manual/tutorial/install-mongodb-on-ubuntu/)
[中文教程](http://www.runoob.com/mongodb/mongodb-linux-install.html)

服务器监听端口在server.py修改，默认监听5100

server.py:接收程序

macname.py:处理程序

运行server.py 和 macname.py

```shell
$ ./server.py
$ ./macname.py
```

## 客户端

bdcap:被动式程序

client:客户端程序

client_crontab.sh: 循环执行client的脚本，默认2秒执行一次

```shell
$ mkdir /bdcapdir
#源码移动到/bdcapdir下，并编译
$ make
#开机运行被动式程序
$ echo "(/bdcapdir/bdcap >& /dev/null &) " >> /etc/rc.local
#开机运行client_crontab.sh程序，ip为服务器地址，port为端口
$ echo "(/bdcapdir/client_crontab.sh ip port >& /dev/null &) " >> /etc/rc.local
```

