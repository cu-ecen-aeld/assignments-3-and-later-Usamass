CC = gcc
SRC = aesdsocket.c
OBJ = aesdsocket.o
EXEC = aesdsocket

ifdef CROSS_COMPILE
	CC = $(CROSS_COMPILE)gcc
endif

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $(EXEC) $(OBJ)

$(OBJ): $(SRC)
	$(CC) -c $(SRC) -o $(OBJ)

clean:
	rm -f $(EXEC) $(OBJ)