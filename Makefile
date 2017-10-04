server: server.c capture.c server.h capture.h
	gcc -Wall -Werror -lpthread -lasound server.c capture.c -o server
all:
	server
