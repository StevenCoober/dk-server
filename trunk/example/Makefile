LIBS=../libdk_base_server.a\
		 ./libevent.a \
		 ./libevent_pthreads.a \
		 ./libprotobuf.a


PACK_PATH=$(HOME)/pack
CXXFLAGS=-g -I.. -I. -I$(PACK_PATH)/gpb/include
LDFLAGS=-lrt
#-levent -levent_pthreads

PROGS=echo_srv
all:$(PROGS)

.PHONY:all

OBJS=echo_srv.o ucper.pb.o

echo_srv:$(OBJS) $(LIBS)
	g++ $(CXXFLAGS) -o echo_srv $(OBJS) $(LIBS) $(LDFLAGS)

.PHONY:clean
clean:
	rm *.o $(PROGS)
