PROGS =	central_server test client test_roomserver_client_audience test_roomserver_client_host \
		test_roomserver_room
ODIR = output
INCLUDE = RoomServer.hpp
CppFLAGS = -std=c++11
OPENCV_FLAGS = -I/usr/include/opencv4 -L /usr/lib -lopencv_stitching -lopencv_alphamat -lopencv_aruco -lopencv_barcode -lopencv_bgsegm -lopencv_bioinspired -lopencv_ccalib -lopencv_dnn_objdetect -lopencv_dnn_superres -lopencv_dpm -lopencv_face -lopencv_freetype -lopencv_fuzzy -lopencv_hdf -lopencv_hfs -lopencv_img_hash -lopencv_intensity_transform -lopencv_line_descriptor -lopencv_mcc -lopencv_quality -lopencv_rapid -lopencv_reg -lopencv_rgbd -lopencv_saliency -lopencv_shape -lopencv_stereo -lopencv_structured_light -lopencv_phase_unwrapping -lopencv_superres -lopencv_optflow -lopencv_surface_matching -lopencv_tracking -lopencv_highgui -lopencv_datasets -lopencv_text -lopencv_plot -lopencv_ml -lopencv_videostab -lopencv_videoio -lopencv_viz -lopencv_wechat_qrcode -lopencv_ximgproc -lopencv_video -lopencv_xobjdetect -lopencv_objdetect -lopencv_calib3d -lopencv_imgcodecs -lopencv_features2d -lopencv_dnn -lopencv_flann -lopencv_xphoto -lopencv_photo -lopencv_imgproc -lopencv_core


# all:	${PROGS}

central_server:	central_server.cpp
		g++ ${CppFLAGS} -o ${ODIR}/$@ $< ${LIBS} 

client:	Client.cpp
		g++ ${CppFLAGS} ${OPENCV_FLAGS} -o ${ODIR}/$@ $<

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