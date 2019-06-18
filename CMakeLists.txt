cmake_minimum_required(VERSION 3.10.2)
project(bbapi CXX)
find_package(Threads)
add_executable(display_example display_example.cpp)
add_executable(sensors_example sensors_example.cpp)
add_executable(unittest unittest.cpp)
target_link_libraries(display_example -static ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(sensors_example -static)
target_link_libraries(unittest -static ${CMAKE_THREAD_LIBS_INIT})
set_target_properties(display_example sensors_example unittest PROPERTIES SUFFIX ".bin")


add_compile_options(
	-lpthread
	-Wall
	-pedantic
	-std=c++11
)

include_directories(
	${CMAKE_SOURCE_DIR}/../
	${CMAKE_SOURCE_DIR}/tools/
)
