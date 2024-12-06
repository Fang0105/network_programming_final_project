include ../Make.defines

PROGS =	central_server test

all:	${PROGS}

central_server:	central_server.o
		${CC} ${CFLAGS} -o $@ central_server.o ${LIBS}

test:	test.o
		${CC} ${CFLAGS} -o $@ test.o ${LIBS}

clean:
		rm -f ${PROGS} ${CLEANFILES}