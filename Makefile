include ../Make.defines

PROGS =	central_server test client test_roomserver_client_audience test_roomserver_client_host \
		test_roomserver_room
ODIR = output
INCLUDE = RoomServer.hpp


CppFLAGS = -std=c++11

all:	${PROGS}

central_server:	central_server.cpp
		${CC} ${CFLAGS} -o ${ODIR}/$@ $< ${INCLUDE} ${LIBS} 

client:	client.cpp
		g++ ${CFLAGS} -o ${ODIR}/$@ $< ${LIBS}

test_roomserver_client_audience:	test_roomserver_client_audience.cpp
		g++ ${CppFLAGS} -o ${ODIR}/$@ $< ${LIBS}

test_roomserver_client_host:	test_roomserver_client_host.cpp
		g++ ${CFLAGS} -o ${ODIR}/$@ $< ${LIBS}

test_roomserver_room:	test_roomserver_room.cpp
		g++ ${CFLAGS} -o ${ODIR}/$@ $< ${LIBS}

test:	test.c
		${CC} ${CFLAGS} -o ${ODIR}/$@ $< ${LIBS}


clean:
		rm -f ${ODIR}/${PROGS} ${CLEANFILES}