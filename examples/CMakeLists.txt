add_executable (camera_socket_client camera_socket_client.cc)
target_link_libraries (camera_socket_client LINK_PUBLIC ${PROJECT_NAME})

add_executable (camera_client camera_client.cc)
target_link_libraries (camera_client LINK_PUBLIC ${PROJECT_NAME})

add_executable (sensor_client sensor_client.cc)
target_link_libraries (sensor_client LINK_PUBLIC ${PROJECT_NAME})
