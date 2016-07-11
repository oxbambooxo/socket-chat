from gevent.server import StreamServer
from gevent import queue
import struct
import gevent
import socket
import time
try:
    import ujson as json
except ImportError:
    import json

clients = {}
names = {}
mq = queue.Queue()

def handle(sock, address):
    clients[address] = sock
    try:
        extra = ''
        while 1:
            while len(extra) < 4:
                tmp = sock.recv(4096)
                if not tmp:
                    return 0
                extra += tmp
            length = struct.unpack('>I', extra[:4])[0]
            total = length + 4
            while len(extra) < total:
                tmp = sock.recv(4096)
                if not tmp:
                    return 0
                extra += tmp
            data = json.loads(extra[4:total])
            extra = extra[total:]
            type = data.get('type', 0)
            if type == 1:
                pack = json.dumps({
                    'type': 1,
                    'sender': repr(address),
                    'message': data['message'],
                    'timestamp': time.time(),
                })
                package = struct.pack('>I', len(pack)) + pack
                mq.put(package)
            elif type == 2:
                # print(repr(address), data['name'], 'online')
                names[repr(address)] = data['name']
            elif type == 3:
                name = names.get(data['sender'], data['sender'])
                # print('find the', data['sender'], 'name is:',name)
                pack = json.dumps({
                    'type': 2,
                    'sender': data['sender'],
                    'name': name,
                })
                package = struct.pack('>I', len(pack)) + pack
                sock.sendall(package)
            
            continue
    finally:
        del clients[address]

def handel_mq(mq):
    while 1:
        data = mq.get()
        for s in clients.values():
            try:
                s.sendall(data)
            except Exception as e:
                print(s, e)

if __name__ == '__main__':
    gevent.spawn(handel_mq, mq)
    import sys
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 56789
    server = StreamServer(('0.0.0.0', port), handle, backlog=1024)
    server.serve_forever()
