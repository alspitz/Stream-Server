server: server.c capture.c server.h capture.h
	gcc -pthread -o server server.c capture.c -lasound -Wall -Werror
all:
	server
