Stream-Server
=============

A TCP based server that streams live audio to clients over the network

Can be used to broadcast audio to a group of willing listeners over a network.
E.g.
Broadcaster's computer starts the server: ./server 
Starts talking etc...

Listener tunes in: python client.py [broadcaster's IP] 

More of an academic pursuit than a practical application...

--------------------------------------------------------------------------------
To recompile using capture-portaudio.c replace -lasound with -lportaudio and capture.c with capture-portaudio.c in the Makefile.

To run the client:
python client.py [IP=localost] [PORT=1234]
A connection refused error likely means the server is not running.

Note that the sample rate should probably be lowered if streaming across anything bigger than a small home network.
