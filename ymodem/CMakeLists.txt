cmake_minimum_required(VERSION 2.8)

project(ymodem)

ADD_DEFINITIONS(
	-g -W -Wall
)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

include_directories(${PROJECT_SOURCE_DIR}/../common)
include_directories(${PROJECT_SOURCE_DIR}/../stream-c/src)
include_directories(${PROJECT_SOURCE_DIR}/../stream-c/serial/include)

set(YMODEM_SRC
	../common/crc.c
	ymodem_sender.c
	ymodem_receiver.c
)

add_library(${PROJECT_NAME} ${YMODEM_SRC})
target_link_libraries(${PROJECT_NAME} stream)

set(YSENDER_SRC ysender.c)
add_executable(ysender ${YSENDER_SRC})
target_link_libraries(ysender ${PROJECT_NAME})

set(YRECEIVER_SRC yreceiver.c)
add_executable(yreceiver ${YRECEIVER_SRC})
target_link_libraries(yreceiver ${PROJECT_NAME})

set(YSENDER_SERIAL_SRC ysender_serial.c)
add_executable(ysender_serial ${YSENDER_SERIAL_SRC})
target_link_libraries(ysender_serial ${PROJECT_NAME})
