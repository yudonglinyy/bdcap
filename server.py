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
import pymongo
import tempfile


client = pymongo.MongoClient('localhost', 27017)
router = client['router']
init_data = router['init_data']


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
                print("time out\n")
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
                    init_data.insert_one(
                        {'mac': datalist[0],
                         'apname': datalist[1],
                         'manu': datalist[2],
                         'reqtime': datalist[3],
                         'station': datalist[4],
                         'newitem': True
                         })
            except ValueError as e:
                print("The line data is bad : " + str(e))
                for i in datalist:
                    print(i)
            except Exception as e:
                print(e)

    print("recv file end")


if __name__ == '__main__':
    # sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    # sock.bind(('', 5100))
    # sock.listen(5)
    # print("Waiting 5100......")
    # while True:
    #     print("test")
    #     clientsocket, addr = sock.accept()
    #     try:
    #         g = gevent.spawn(tcplink, clientsocket, addr)
    #         g.start()
    #     except Exception as e:
    #         raise

    #     # t = threading.Thread(target=tcplink, args=(clientsocket, addr))
    #     # t.start()

    # sock.close()


    # to make the server use SSL, pass certfile and keyfile arguments to the constructor
    server = StreamServer(('0.0.0.0', 5100), tcplink)
    # to start the server asynchronously, use its start() method;
    # we use blocking serve_forever() here because we have no other jobs
    print('Starting echo server on port 5100')
    server.serve_forever()
