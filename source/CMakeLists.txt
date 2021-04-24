list (APPEND SOURCES unix_socket.cc)

# Build libvhal-client
add_library(${PROJECT_NAME} SHARED ${SOURCES})

# Make sure the compiler can find include files for vhal-client library
# when other libraries or executables link to vhal-client
target_include_directories (${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
