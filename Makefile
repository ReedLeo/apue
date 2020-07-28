CFLAGS += -g -ggdb -g3 -gdwarf-4 -Og -w

all:

process_basic: process_basic.c
	$(CC) $^ $(CFLAGS) -o $@

clean:
	$(RM) *.o