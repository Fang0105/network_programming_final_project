PROGS =	central_server test client test_roomserver_client_audience test_roomserver_client_host \
		test_roomserver_room
ODIR = output
INCLUDE = RoomServer.hpp
CppFLAGS = -std=c++11
OPENCV_FLAGS = `pkg-config --cflags --libs opencv4`


# all:	${PROGS}

central_server:	central_server.cpp
		g++ ${CppFLAGS} -o ${ODIR}/$@ $< ${LIBS} 

client:	Client.cpp
		g++ ${CppFLAGS} -g -o ${ODIR}/$@ $< ${OPENCV_FLAGS}

test_roomserver_client_audience:	test_roomserver_client_audience.cpp
		g++ ${CppFLAGS} -o ${ODIR}/$@ $< ${LIBS}

test_roomserver_client_host:	test_roomserver_client_host.cpp
		g++ ${CppFLAGS} -o ${ODIR}/$@ $< ${LIBS}

test_roomserver_room:	test_roomserver_room.cpp
		g++ ${CppFLAGS} -o ${ODIR}/$@ $< ${LIBS} 

test_central_server_host:	test_central_server_host.cpp
		g++ ${CppFLAGS} -o ${ODIR}/$@ $< ${LIBS}

test_central_server_audience:	test_central_server_audience.cpp
		g++ ${CppFLAGS} -o ${ODIR}/$@ $< ${LIBS}

my:
	g++ -std=c++11 central_server.cpp -o central_server
	g++ -std=c++11 test_central_server_host.cpp -o test_central_server_host
	g++ -std=c++11 test_central_server_audience.cpp -o test_central_server_audience


clean:
		rm -f ${ODIR}/${PROGS} ${CLEANFILES}