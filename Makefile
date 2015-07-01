all: binder librpc.a

XCC = g++
AR	= ar

OBJECTS = clientsocket.o serversocket.o rpc.o constants.o io.o
DEPENDS = ${OBJECTS:.o=.d}

binder: binder.o io.o constants.o
	g++ -Wall -o binder binder.cc io.c
	
%.o: %.c
	$(XCC) -c -Wall -MMD -o $@ $<

-include ${DEPENDS}

librpc.a: ${OBJECTS}
	${AR} -rcs -o $@ $^

clean:
	rm binder librpc.a *.d *.o

