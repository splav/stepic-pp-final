project(final)
cmake_minimum_required(VERSION 2.8)

SET(CMAKE_CXX_FLAGS "-std=c++11")
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
SET(CMAKE_C_FLAGS_DEBUG "-O0 -g")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

set(SRCS main.cpp)
set(HDRS common.h http.h )

find_package(LibEV REQUIRED)
include_directories(${LIBEV_INCLUDE_DIRS})
set(LIBS ${LIBS} ${LIBEV_LIBRARIES})

add_executable(${PROJECT_NAME} ${SRCS} ${HDRS})

find_package(Threads REQUIRED)
#if(THREADS_HAVE_PTHREAD_ARG)
#  target_compile_options(PUBLIC ${PROJECT_NAME} "-pthread")
#endif()
if(CMAKE_THREAD_LIBS_INIT)
  target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT} ${LIBS})
endif()
