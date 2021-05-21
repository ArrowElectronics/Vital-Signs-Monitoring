'''
    Simple socket server using threads
'''
try:
    import socket, sys
    from m2m2_common import *
    from thread import *

    import plot_parsers
    import inspect

    import pyqtgraph as pg
    from pyqtgraph.Qt import QtCore, QtGui
    import numpy as np
except ImportError as e:
    print "Oops, looks like you're missing a Python module!"
    print "Try installing it with pip: `pip install MODULE_NAME`"
    print "Error message is: {}".format(e)
    print "Exiting..."
    raise SystemExit

enable_csv_logs=0
clsmembers = inspect.getmembers(plot_parsers, inspect.isclass)
if len(sys.argv) != 4:
    print "Incorrect arguments: {}".format(sys.argv)
    print "Available processors are:"
    for name in clsmembers:
        if "_plot" in name[0]:
            print name[0]
    sys.exit()

PORT = None
plot = None
for name in clsmembers:
    class_name = name[0]
    class_obj = name[1]
    if "_plot" in class_name:
        for arg in sys.argv:
            if arg == class_obj.selection_name:
                print "Found a plotter for argument '{}': '{}'".format(arg, class_name)
                PORT = int(sys.argv[2])
                enable_csv_logs = int(sys.argv[3])
                print "Save as CSV file option set to '{}' for: '{}'".format(enable_csv_logs, class_name)
                plot = class_obj(pg.GraphicsWindow())
                plot.save_csv_option(enable_csv_logs)
                break            
if PORT == None or plot == None :
    print "Processor not found!"
    sys.exit()
HOST = 'localhost'

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
print 'Socket created'


#Bind socket to local host and port
try:
    s.bind((HOST, PORT))
except socket.error as msg:
    print 'Bind failed. Error Code : ' + str(msg[0]) + ' Message ' + msg[1]
    sys.exit()

print 'Socket bind complete'

#Start listening on socket
s.listen(10)
print 'Socket now listening on {}'.format(PORT)
new_data = []
#Function for handling connections. This will be used to create threads
def clientthread(conn):
    global new_data

    #infinite loop so that function do not terminate and thread do not end.
    while True:

        #Receiving from client
        try:
            data = conn.recv(1024)
        except socket.error:
            print "Socket error!"
            break

        try:
            plot.submit(data)
        except Exception as e:
            print "Error in the received data.: {}".format(e)

        if not data:
            print "The client disconnected!"
            break
    conn.close()

# The main server thread
def listen_thread():
    global new_data
    while 1:
        # Wait to accept a connection - blocking call
        conn, addr = s.accept()
        print 'Connected with ' + addr[0] + ':' + str(addr[1])

        # Start a thread to handle this client
        start_new_thread(clientthread ,(conn,))
    s.close()

def plot_update():
    plot._update()


## Start Qt event loop unless running in interactive mode or using pyside.
if __name__ == '__main__':
    import sys
    timer = pg.QtCore.QTimer()
    # Setup a timer to update the plot every 50ms
    timer.timeout.connect(plot_update)
    timer.start(10)
    # Start the server listening thread to listen for incoming connections
    start_new_thread(listen_thread, ())
    if (sys.flags.interactive != 1) or not hasattr(QtCore, 'PYQT_VERSION'):
        QtGui.QApplication.instance().exec_()
