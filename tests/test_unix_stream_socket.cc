/**
 *
 * Copyright (c) 2021-2022 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this
                          // in one cpp file
#include "catch.hpp"
#include "unix_stream_socket.h"
#include <memory>

using namespace vhal::client;

TEST_CASE("TestUnixStreamSocket", "[ctor]")
{
    std::string remote_server_path = "/ipc/mycamera-socket0";
    auto        socket =
      std::make_unique<UnixStreamSocket>(std::move(remote_server_path));

    REQUIRE(socket.Valid() == true);
    REQUIRE(socket.Connected() == false);
}

TEST_CASE("TestUnixSocketConnect", "[connect]")
{
    UnixSocket socket;

    REQUIRE(socket.Connect("/ipc/mycamera-socket0") == true);
}

TEST_CASE("TestUnixSocketSend", "[send]")
{
    UnixSocket socket;

    try {
        socket.Connect("/ipc/mycamera-socket0");

    } catch (const std::system_error& error) {
        std::cout << "Error: " << error.code() << " - "
                  << error.code().message() << '\n';
    }
}