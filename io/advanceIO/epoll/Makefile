SRC = $(wildcard *.c)
OBJ = $(patsubst %.c,%,$(SRC))

all:$(OBJ)

%:%.c
	gcc $< -o $@ $(CFLAGS)

clean:
	rm $(OBJ) *.o