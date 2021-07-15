add_executable (camera_socket_client camera_socket_client.cc)
target_link_libraries (camera_socket_client LINK_PUBLIC ${PROJECT_NAME})

add_executable (camera_client camera_client.cc)
target_link_libraries (camera_client LINK_PUBLIC ${PROJECT_NAME})

add_executable (sensor_client sensor_client.cc)
target_link_libraries (sensor_client LINK_PUBLIC ${PROJECT_NAME})

find_package(PkgConfig REQUIRED)

pkg_check_modules(PKG_FFMPEG REQUIRED
    libavdevice
    libavfilter
    libavformat
    libavcodec
    libswresample
    libswscale
    libavutil
    IMPORTED_TARGET
)

add_library(FFmpeg INTERFACE)

target_link_libraries(FFmpeg
    INTERFACE
    PkgConfig::PKG_FFMPEG
)

add_executable (stream host_camera_server.cc)

find_package(Threads REQUIRED)

target_link_libraries(stream
    PRIVATE
    FFmpeg
    Threads::Threads
    ${PROJECT_NAME}
    ${CMAKE_DL_LIBS}
)
add_executable (audio_record_client audio_record_client.cc)
target_link_libraries (audio_record_client LINK_PUBLIC ${PROJECT_NAME})

add_executable (audio_playback_client audio_playback_client.cc)
target_link_libraries (audio_playback_client LINK_PUBLIC ${PROJECT_NAME})

add_executable (VirtualInputReceiverSample VirtualInputReceiverSample.cpp)
target_link_libraries (VirtualInputReceiverSample LINK_PUBLIC ${PROJECT_NAME})

install(TARGETS camera_socket_client
                camera_client
                sensor_client
                stream
                audio_record_client
                audio_playback_client
        DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
