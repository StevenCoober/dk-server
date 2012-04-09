
LIBS=../libdonkey_server.a
CXXFLAGS=-g -I.. -D_DONKEY_DEBUG
LDFLAGS=-levent

OBJS=main.o

all:$(OBJS) $(LIBS)
	g++ $(CXXFLAGS) -o echo_server $(OBJS) $(LIBS) $(LDFLAGS)

.PHONY:clean
clean:
	rm echo_server $(OBJS)
