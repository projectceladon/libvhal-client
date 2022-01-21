find_package( Threads )

include_directories (
	"${CMAKE_CURRENT_SOURCE_DIR}"
)
list (APPEND SOURCES unix_stream_socket_client.cc)
list (APPEND SOURCES tcp_stream_socket_client.cc)
list (APPEND SOURCES video_sink.cc)
list (APPEND SOURCES sensor_interface.cc)
list (APPEND SOURCES vsock_stream_socket_client.cc)
list (APPEND SOURCES audio_sink.cc)
list (APPEND SOURCES audio_source.cc)
list (APPEND SOURCES virtual_input_receiver.cc)
list (APPEND SOURCES hwc_vhal.cc)
list (APPEND SOURCES virtual_gps_receiver.cc)
list (APPEND SOURCES command_channel_interface.cc)

# Build libvhal-client
add_library(${PROJECT_NAME} SHARED ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${vhal_version_major}.${vhal_version_minor}
    SOVERSION ${vhal_version_major})

target_link_libraries( ${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT} )

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR})
# Make sure the compiler can find include files for vhal-client library
# when other libraries or executables link to vhal-client
target_include_directories (${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
