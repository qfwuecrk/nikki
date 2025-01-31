OBJS=client.o
CC=gcc
CFLAGS+=-I../include -c -Wall -g

client:$(OBJS)
	$(CC) $^ -o client

%.o:%.c
	$(CC) $^ $(CFLAGS) -o $@

clean:
	$(RM) *.o client -r