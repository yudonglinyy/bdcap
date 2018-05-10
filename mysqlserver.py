#!/usr/bin/env python
#coding：

from gevent import monkey; monkey.patch_socket()
import gevent
from gevent.server import StreamServer

import os
import socket
import threading
from datetime import datetime
import time
import tempfile

from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker
from modules import Init_sta1, init_db

import pymysql
pymysql.install_as_MySQLdb()

DB_CONNECT_STRING = "mysql+mysqldb://root:123@192.168.3.108:3306/router"
engine = create_engine(DB_CONNECT_STRING, encoding="utf-8", echo=True)
DB_Session = sessionmaker(bind=engine)
session = DB_Session()

init_db(engine)


def get_reqtime(reqtime_str):
    try:
        reqtime = datetime.strptime(reqtime_str, "%Y-%m-%d %H:%M:%S")
    except Exception as e:
        raise ValueError(str(e))
    return reqtime
  

def tcplink(sock, addr):
    print('accept from %s:%s' % addr)
    with tempfile.TemporaryFile() as f:
        while True:
            try:
                sock.settimeout(5)
                buf = sock.recv(65536)
                if not buf:
                    break
                f.write(buf)
            except socket.timeout as e:
                print("time out")
                break
            except Exception as e:
                print(e)
                break

        print('sock close')
        sock.close()
        f.seek(0, 0)
        for line in f.readlines():
            try:
                datalist = line.strip().decode('utf-8').split('<-->')
                if len(datalist) == 5:
                    reqtime = get_reqtime(datalist[3])
                    item = Init_sta1(mac1=datalist[0], apname=datalist[1], manu=datalist[2], reqtime=reqtime, station=datalist[4], newitem=True)
                    session.add(item)
                    session.commit()
            except ValueError as e:
                print("The line data is bad : " + str(e))
            except Exception as e:
                print(e)

    print("recv file end")


if __name__ == '__main__':
    # to make the server use SSL, pass certfile and keyfile arguments to the constructor
    server = StreamServer(('0.0.0.0', 5100), tcplink)
    # to start the server asynchronously, use its start() method;
    # we use blocking serve_forever() here because we have no other jobs
    print('Starting echo server on port 5100')
    server.serve_forever()
