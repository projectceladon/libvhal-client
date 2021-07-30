cmake_minimum_required (VERSION 3.0)
project (vhal-client VERSION 1.0 DESCRIPTION "VHAL Client library written in C++17 for Audio, Camera and Sensor" LANGUAGES CXX)

option(BUILD_EXAMPLES "Build examples?" ON)

message(STATUS "Project name: ${PROJECT_NAME}")

set (TARGET_OS "linux")
if(NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE Release)
endif()
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wformat -Wformat-security -Werror=format-security \
                     -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fPIE -fPIC -Wall")
set (CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
set (CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

set (LINK_FLAGS "-pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack")
set (CMAKE_EXE_LINKER_FLAGS "${LINK_FLAGS}")
set (CMAKE_SHARED_LINKER_FLAGS "${LINK_FLAGS}")

include_directories (
	"${CMAKE_CURRENT_SOURCE_DIR}/include/"
)

set (EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/bins/${TARGET_OS}")
set (LIBRARY_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libs/${TARGET_OS}")

link_directories ("${LIBRARY_OUTPUT_PATH}")

add_subdirectory (source)
if (BUILD_EXAMPLES)
  add_subdirectory (examples)
endif()
# TODO: Add tests
