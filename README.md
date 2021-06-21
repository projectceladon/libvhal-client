# VHAL Client library 

libvhal-client is a library written in C++17 for Camera, Audio and Sensor modules. Currently only Camera is
supported. Using this library, a client can interact with Camera Vhal without worrying about socket connection,
data structure, command details. Instead, the library exposes simple and powerful API that simplifies the life of
VHAL client modules such as CG-Proxy, Streamer, etc. Any internal changes in the Camera VHAL and the library
is abstracted and hence it won’t affect client code. New features or improvements can be provided via APIs
rather than new socket commands, and other details.

## Camera

Camera VHal runs socket server (UNIX, VSock are supported). VHAL Client library shall connect to socket server path or address/port endpoint.

### Architecture
![image](https://github.com/intel-sandbox/libvhal-client/assets/26615772/66a89e80-d316-11eb-814f-c4e75902441c)


### Camera API to interact with VHal
Camera VHal client might do following steps.

Create a socket client (say, UNIX):
```cpp
auto unix_sock_client = make_unique<vhal::client::UnixStreamSocketClient>(move(socket_path));
```

Handover socket client to `vhal::client::VideoSink` that talks to Camera VHal.
```cpp
vhal::client::VideoSink video_sink(move(unix_sock_client));
```

Register with `vhal::client::VideoSink` for Camera open/close callbacks as below:

![image](https://github.com/intel-sandbox/libvhal-client/assets/26615772/2269ce00-d317-11eb-89fe-94e28ae73eb1)

## Audio
TODO

## Sensor
TODO
