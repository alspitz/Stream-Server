'''
A script that connects to a network machine and attempts to play incoming
audio data. Uses PyAudio (Python bindings for PortAudio) for access to the
audio device, so it should be fairly portable.

Sample rate, number of channels, and sample format must all be edited accordingly.

Alex Spitzer 2013
'''

import socket
import threading, Queue
import sys, signal
import pyaudio

HOST = "localhost" if len(sys.argv) <= 1 else sys.argv[1]
PORT = 1234 if len(sys.argv) <= 2 else int(sys.argv[2])

def acceptData(s, buff):
    while 1:
        data = s.recv(128)
        if not data:
            break
        buff.put(data)
    s.close()


s = socket.socket()
s.connect((HOST, PORT))
buff = Queue.Queue()

t = threading.Thread(target=acceptData, args = (s, buff))
t.daemon = True
t.start() # begin receiving data
    
p = pyaudio.PyAudio()
stream = p.open(format=pyaudio.paInt16, channels=1, rate=48000, output=True)

def interrupt_handler(signal, frame):
    print "Quitting..."

    stream.stop_stream()
    stream.close()
    p.terminate()
    s.close()

    sys.exit(0)

signal.signal(signal.SIGINT, interrupt_handler)  # register ^C handler
while 1:
    try:
        frame = buff.get(True, 1)  # blocking get with 1 second timeout
    except Queue.Empty:
        print "End of stream"
        break

    stream.write(frame)

