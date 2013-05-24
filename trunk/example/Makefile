
LIBS=../libdonkey_server.a /home/lijian2/local/libevent/lib/libevent.a /home/lijian2/local/libevent/lib/libevent_pthreads.a
CXXFLAGS=-g -I.. -D_DONKEY_DEBUG
LDFLAGS=-lrt

PROGS=echo_srv http_client tcp_client
all:$(PROGS)

.PHONY:all

echo_srv:echo_srv.o logbase.o $(LIBS)
	g++ $(CXXFLAGS) -o echo_srv echo_srv.o logbase.o $(LIBS) $(LDFLAGS)

http_client:http_client.o $(LIBS)
	g++ $(CXXFLAGS) -o http_client http_client.o $(LIBS) $(LDFLAGS)

tcp_client:tcp_client.o $(LIBS)
	g++ $(CXXFLAGS) -o tcp_client tcp_client.o $(LIBS) $(LDFLAGS)

.PHONY:clean
clean:
	rm *.o $(PROGS)
