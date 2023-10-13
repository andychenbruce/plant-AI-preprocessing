cmake_minimum_required (VERSION 3.8.0)
project(testing_idk_poop CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wshadow -Weffc++")


add_executable(foo 
	chunk.cpp
	chunk.hpp
	common/do_args.hpp
	common/hsv.cpp
	common/hsv.hpp
	common/jpeg.cpp
	common/jpeg.hpp
	common/png.cpp
	common/png.hpp
	common/pnm.cpp
	common/pnm.hpp
	#common/readPng.cpp
	common/utils.cpp
	common/utils.hpp
	#common/writeJpeg.cpp
	common/do_args.cpp
	imageProcessor.cpp
	imageProcessor.hpp
	main.cpp
	)


find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
target_link_libraries(foo ${OpenCV_LIBS})

find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIR})
target_link_libraries(foo ${PNG_LIBRARY})

find_package(JPEG REQUIRED)
include_directories(${JPEG_INCLUDE_DIR})
target_link_libraries(foo ${JPEG_LIBRARY})

