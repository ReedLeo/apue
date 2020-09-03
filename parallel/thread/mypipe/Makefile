CFLAGS+=-g -ggdb -g3 -gdwarf-4 -pthread
LDFLAGS+=-pthread

all : simpleTest

simpleTest : main.o mypipe.o
	$(CC) $^ $(CFLAGS) $(LDFLAGS) -o $@

#%.o : %.c
#	$(CC) $^ $(CFLAGS) -o $@

clean:
	rm -rf *.o simpleTest
