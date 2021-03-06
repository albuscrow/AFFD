cmake_minimum_required(VERSION 2.8)

add_definitions(-DTRILIBRARY)
add_definitions(-DLINUX)
add_definitions(-DANSI_DECLARATORS)
#add_definitions(-DDISABLE_CULL)

include_directories(${PROJECT_SOURCE_DIR})
find_package(CUDA QUIET REQUIRED)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(Project_BINARY_DIR "${PROJECT_SOURCE_DIR}/build")
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

set(QT_USE_QTOPENGL TRUE)
# Find the QtWidgets library
find_package(Qt5Widgets)
find_package(Qt5OpenGL)

find_package(OpenGL)
find_package(GLUT)
file(GLOB CUDA_SRC_FILES *.cu)

cuda_add_library(cudaLib ${CUDA_SRC_FILES})
cuda_add_cublas_to_target(cudaLib)

add_definitions(-std=c++14)
file(GLOB SRC_FILES *.cpp *.h *.cc)
cuda_add_executable(affd ${SRC_FILES})

target_link_libraries(affd Qt5::Widgets Qt5::OpenGL ${OPENGL_LIBRARIES} ${GLUT_LIBRARY} lapackpp splitTriangle cudaLib)

add_subdirectory(split_library)
