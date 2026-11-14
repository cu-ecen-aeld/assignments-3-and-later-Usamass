CC ?= gcc

CFLAGS += -g -Wall -Werror

all: aesdsocket

clean:
	-rm -f aesdsocket *.o

*.o: *.c

aesdsocket: aesdsocket.o
	$(CC) $(CFLAGS) aesdsocket.o -o aesdsocket $(LDFLAGS)
