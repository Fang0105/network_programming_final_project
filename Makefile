include ../Make.defines

PROGS =	central_server test
ODIR = output

all:	${PROGS}

central_server:	central_server.c
		${CC} ${CFLAGS} -o ${ODIR}/$@ $< ${LIBS}

test:	test.c
		${CC} ${CFLAGS} -o ${ODIR}/$@ $< ${LIBS}

clean:
		rm -f ${ODIR}/${PROGS} ${CLEANFILES}