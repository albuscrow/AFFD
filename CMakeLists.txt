include_directories(${PROJECT_SOURCE_DIR})

cmake_minimum_required(VERSION 2.8)
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
file(GLOB SRC_FILES *.cpp *.h *.cu *.cc)

cuda_add_executable(affd ${SRC_FILES})
CUDA_ADD_CUBLAS_TO_TARGET(affd)

target_link_libraries(affd Qt5::Widgets Qt5::OpenGL ${OPENGL_LIBRARIES} ${GLUT_LIBRARY} lapackpp)


