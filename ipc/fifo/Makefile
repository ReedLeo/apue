CFLAGS+=-g -ggdb -gdwarf-4 -Og -Wall

SRC = $(wildcard *.c)
OBJ = $(patsubst %.c, %, $(SRC))

all:$(OBJ)

%:%.c
	gcc $< -o $@ $(CFLAGS)

clean:
	rm -rf $(OBJ) *.o