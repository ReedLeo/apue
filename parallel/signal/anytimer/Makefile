CFLAGS += -g -ggdb -g3 -gdwarf-4 -Og -Wall

all:anytimer

anytimer: main.o anytimer.o
	gcc $^ -o $@

clean:
	rm -rf *.o anytimer