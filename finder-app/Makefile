CC = gcc
SRC = writer.c
OBJ = writer.o
EXEC = writer

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