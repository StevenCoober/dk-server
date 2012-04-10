OBJECTS=donkey_server.o donkey_base_connection.o donkey_http.o donkey_util.o

CXXFLAGS=-g -D_DONKEY_DEBUG

LIB_NAME=libdonkey_server.a

$(LIB_NAME):$(OBJECTS)
		ar cru  $(LIB_NAME) $(OBJECTS)
		ranlib $(LIB_NAME)

$(OBJECTS):donkey_common.h donkey_internal.h donkey_log.h donkey_core.h donkey_error.h donkey_util.h
donkey_server.o:donkey_server.h
donkey_base_connection.o:donkey_base_connection.h donkey_server.h
donkey_http.o:donkey_http.h
donkey_util.o:donkey_util.h

.PHONY:clean
clean:
	rm $(LIB_NAME) $(OBJECTS)