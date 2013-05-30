all:
	gcc -Wall -Werror -lpthread -lasound server.c capture.c -o server
