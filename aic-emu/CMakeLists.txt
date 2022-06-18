# Copyright (C) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(yaml-cpp REQUIRED)

Message(status "  " Build Aic Emu!)
set (aic_executable "aic-emu")

add_executable( ${aic_executable} AicEmu.cc CmdHandler.cc YmlParser.cc GfxHandler.cc SocketServer.cc)

target_include_directories (${aic_executable} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries (${aic_executable} LINK_PUBLIC  Threads::Threads yaml-cpp)

install(TARGETS ${aic_executable}
        DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
