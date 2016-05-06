from gevent.server import StreamServer
import struct
import gevent
import socket
import time
try:
	import ujson as json
except ImportError:
	import json

clients = {}

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
			if data.get('type') == 1:
				data['timestamp'] = time.time()
				pack = json.dumps(data)
				package = struct.pack('>I', len(pack)) + pack
				for s in clients.values():
					# dangerous's using socket in other greenlet
					gevent.spawn(s.sendall, package)
			continue
	finally:
		del clients[address]


server = StreamServer(('0.0.0.0', 56789), handle, backlog=1024)
server.serve_forever()
