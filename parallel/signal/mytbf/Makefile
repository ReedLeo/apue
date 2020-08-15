CFLAGS += -g -g3 -gdwarf-4 -Og -ggdb

all:mytbf

mytbf:main.o mytbf.o
	$(CC) $^ $(CFLAGS) -o $@

clean:
	rm -rf *.o mytbf