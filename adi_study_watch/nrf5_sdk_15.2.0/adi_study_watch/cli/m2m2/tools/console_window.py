import socket, sys, time, colorama, os
from threading import *

colorama.init(autoreset=True)
serversocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
host = "localhost"
port = int(sys.argv[1])

try:
    serversocket.bind((host, port))
except Exception as e:
    import traceback
    print("An error occurred while trying to open socket {}.{}:\n".format(host, port, str(e)))
    traceback.print_exc()
    raw_input("Hit any key to exit")
    sys.exit(-1)

class client(Thread):
    def __init__(self, socket, address):
        Thread.__init__(self)
        self.sock = socket
        self.addr = address
        self.start()

    def run(self):
        while 1:
            try:
                data = self.sock.recv(1024)
            except:
                raw_input("The socket was broken! Hit any key to exit")
                os._exit(0)
            if not data:
                raw_input("The client disconnected! Hit any key to exit")
                os._exit(0)
            print data

serversocket.listen(5)
print "Console monitor started, listening on {}:{}".format(host, port)
while 1:
    clientsocket, address = serversocket.accept()
    client(clientsocket, address)
