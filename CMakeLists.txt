cmake_minimum_required(VERSION 3.7)
project(ITCT_JPEG)

set(CMAKE_CXX_STANDARD 11)
set(SOURCE_FILES main.cpp Image.cpp Image.h JpegDecoder.cpp JpegDecoder.h FIDCT.cpp FIDCT.h)

find_package( OpenCV REQUIRED )
include_directories( ${OPENCV_INCLUDE_DIR} )
link_directories(${OPENCV_LIB_DIR})
add_executable(ITCT_JPEG ${SOURCE_FILES})
target_link_libraries(ITCT_JPEG ${OpenCV_LIBS})