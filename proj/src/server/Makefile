CFLAGS+=-I../include -I../utils -pthread
LDFLAGS+=-pthread

all : server

server : server.o medialib.o thr_channel.o thr_list.o mytbf.o
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)

clean :
	$(RM) -rf *.o server