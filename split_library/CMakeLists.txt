set(CMAKE_AUTOMOC OFF)

add_definitions(-std=c++14)

file(GLOB SPLIT_LIBRARY_SRC_FILES *.cpp *.h)

add_executable(splitTest ${SPLIT_LIBRARY_SRC_FILES})
