CFLAGS+=-g -g3 -ggdb -gdwarf-4 -w

SRC=$(wildcard *.c)
OBJ=$(patsubst %.c, %, $(SRC))

all:$(OBJ)

%:%.c
	gcc $< -o $@ $(CFLAGS)

clean:
	rm -rf $(OBJ) *.o
