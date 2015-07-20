set(CMAKE_AUTOMOC OFF)

add_definitions(-std=c++14)

#add_definitions(-DTEST)

file(GLOB SPLIT_LIBRARY_SRC_FILES *.cpp *.h)

add_library(splitTriangle ${SPLIT_LIBRARY_SRC_FILES})
target_link_libraries(splitTriangle lapackpp)

add_executable(testSplit test/source_file_test.cpp)

target_link_libraries(testSplit splitTriangle)

