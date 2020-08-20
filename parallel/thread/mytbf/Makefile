CFLAGS += -g -g3 -gdwarf-4 -Og -ggdb -pthread
LDFLAGS += -pthread

all:mytbf

mytbf:main.o mytbf.o
	$(CC) $^ $(CFLAGS) $(LDFLAGS) -o $@ 

clean:
	rm -rf *.o mytbf