Name: @PROJECT_NAME@
Description: @CMAKE_PROJECT_DESCRIPTION@
Version: @vhal_version_major@.@vhal_version_minor@

prefix=@CMAKE_INSTALL_PREFIX@
libdir=@CMAKE_INSTALL_FULL_LIBDIR@
includedir=@CMAKE_INSTALL_FULL_INCLUDEDIR@
Libs: -L${libdir} -lvhal-client
Cflags: -I${includedir} -I${includedir}/libvhal
