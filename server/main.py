# coding: utf-8
import time
import struct
import socket
import logging

try:
    import ujson as json
except ImportError:
    import json

import gevent
from gevent.server import StreamServer
from gevent import queue

logger = logging.getLogger()
clients = {}
clients_infos = {}
broadcast_mq = queue.Queue()

PACK_HEAD = 8
PACK_TYPE_SENDMESSAGE = 1
PACK_TYPE_MODIFYINFO = 2
PACK_TYPE_QUERYINFO = 3

def handle(sock, address):
    uid = repr(address)
    clients[uid] = sock
    logger.info("%s online" % repr(address))
    try:
        extra = ''
        while 1:
            while len(extra) < PACK_HEAD:
                tmp = sock.recv(4096)
                if not tmp:
                    return 0
                extra += tmp
            type, head_length = struct.unpack('>II', extra[:PACK_HEAD])
            pack_length = head_length + PACK_HEAD
            while len(extra) < pack_length:
                tmp = sock.recv(4096)
                if not tmp:
                    return 0
                extra += tmp
            content = extra[PACK_HEAD:pack_length]
            extra = extra[pack_length:]
            data = json.loads(content)
            logger.info("recv %r type:%d data:%r", address, type, data)
            if type == PACK_TYPE_SENDMESSAGE:
                broadcast_mq.put(packing(type, {
                    'uid': uid,
                    'message': data['message'],
                    'timestamp': time.time(),
                }))
            elif type == PACK_TYPE_MODIFYINFO:
                try:
                    info = clients_infos[uid]
                except KeyError:
                    info = {}
                    clients_infos[uid] = info
                info['name'] = data['name']
            elif type == PACK_TYPE_QUERYINFO:
                try:
                    name = clients_infos[data['uid']]['name']
                except KeyError:
                    name = data['uid']
                sock.sendall(packing(type, {
                    'uid': data['uid'],
                    'name': name,
                }))
            continue
    except Exception as e:
        logger.exception("%s error", repr(address))
    finally:
        del clients[uid]
        logger.info("%s offline" % repr(address))

def packing(type, data):
    pack = json.dumps(data)
    return struct.pack('>II', type, len(pack)) + pack

def broadcast_handle(broadcast_mq):
    while 1:
        data = broadcast_mq.get()
        for s in clients.values():
            try:
                s.sendall(data)
            except Exception as e:
                print(s, e)

if __name__ == '__main__':
    import sys
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 56789

    printlog = logging.StreamHandler()
    printlog.setFormatter(logging.Formatter('%(asctime)s %(levelname)s %(filename)s(%(lineno)s):%(message)s'))
    printlog.setLevel(logging.DEBUG)
    logger.addHandler(printlog)
    logger.setLevel(logging.DEBUG)
    
    gevent.spawn(broadcast_handle, broadcast_mq)

    server = StreamServer(('0.0.0.0', port), handle, backlog=1024)
    server.serve_forever()
