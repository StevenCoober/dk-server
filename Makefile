OBJECTS=donkey_server.o donkey_base_connection.o donkey_http.o donkey_util.o \
				donkey_base_thread.o donkey_thread.o donkey_worker.o

CXXFLAGS=-g -Wall #-D_DONKEY_DEBUG

LIB_NAME=libdonkey_server.a

$(LIB_NAME):$(OBJECTS)
		ar cru  $(LIB_NAME) $(OBJECTS)
		ranlib $(LIB_NAME)

.PHONY:clean
clean:
	rm $(LIB_NAME) $(OBJECTS)

SOURCES=donkey_server.cpp donkey_base_connection.cpp donkey_http.cpp donkey_util.cpp \
				donkey_base_thread.cpp donkey_thread.cpp donkey_worker.cpp

include $(SOURCES:.cpp=.d)

%.d: %.cpp
	set -e; rm -f $@; \
		$(CC) -MM $(CXXFLAGS) $< > $@.$$$$; \
			sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
				rm -f $@.$$$$
