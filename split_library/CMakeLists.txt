set(CMAKE_AUTOMOC OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

#add_definitions(-DTEST)
add_definitions(-DCVT_TIMES=0)

file(GLOB SPLIT_LIBRARY_SRC_FILES *.cpp *.h)

add_library(splitTriangle ${SPLIT_LIBRARY_SRC_FILES})
target_link_libraries(splitTriangle lapackpp)

add_executable(testSplit test/source_file_test.cpp)

target_link_libraries(testSplit splitTriangle)

