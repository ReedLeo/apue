CFLAGS += -g -ggdb -g3 -gdwarf-4 -Og -Wall -pthread
LDFLAGS += -pthread

all:relayer

relayer: main.o relayer.o
	gcc $^ -o $@ $(CFLAGS) $(LDFLAGS)

clean:
	rm -rf *.o relayer