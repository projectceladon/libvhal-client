/**
 * Copyright (C) 2022 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <memory>
#include "CmdHandler.h"

void Usage()
{
    std::cout << "Usage: aic-emu <mandatory options> [non-mandatory options] " << std::endl;
    std::cout << "--help, -h                 : Print this help and exit" << std::endl;
    std::cout << "\nMandatory options:" << std::endl;
    std::cout << "  --cmd <yml file path>    : Path of YML file containing AiC cmd sequence" << std::endl;
    std::cout << "  --content <clip path>    : Path of clip to serve as content" << std::endl;
    std::cout << "  --hwc-sock <socket path> : Path of Unix Socket file. Note Instance ID (default 0) is appended internally." << std::endl;
    std::cout << "\nNon-Mandatory options:" << std::endl;
    std::cout << "  --device <device path>   : Device path. Default /dev/dri/renderD128" << std::endl;
    std::cout << "  --instance <id num>      : Number indicating instance ID. Default 0" << std::endl;
    return;
}

int ParseArgs(AicConfigData_t& config, int argc, char** argv)
{
    int idx;
    for (idx = 1; idx < argc; ++idx)
    {
        if (std::string("-h") == argv[idx] || std::string("--help") == argv[idx])
        {
            Usage();
            exit(0);
        }
        else if (std::string("--cmd") == argv[idx])
        {
            if (++idx >= argc)
                break;
            config.ymlFileName = std::string(argv[idx]);
        }
        else if (std::string("--content") == argv[idx])
        {
            if (++idx >= argc)
                break;
            config.contentFileName = std::string(argv[idx]);
        }
        else if (std::string("--device") == argv[idx])
        {
            if (++idx >= argc)
                break;
            config.deviceString = std::string(argv[idx]);
        }
        else if (std::string("--hwc-sock") == argv[idx])
        {
            if (++idx >= argc)
                break;
            config.socketInfo.hwc_sock = std::string(argv[idx]);
        }
        else if (std::string("--instance") == argv[idx])
        {
            if (++idx >= argc)
                break;
            config.socketInfo.session_id = atoi(argv[idx]);
        }
        else
        {
            break;
        }
    }

    if (config.ymlFileName.empty())
    {
        std::cout << "Specification of YMLfile with AiC commands is mandatory" <<std::endl;
        Usage();
        return AICS_ERR_INVALID_ARGS;
    }

    if (config.contentFileName.empty())
    {
        std::cout << "Specification of Content file is mandatory" <<std::endl;
        Usage();
        return AICS_ERR_INVALID_ARGS;
    }

    if (config.socketInfo.hwc_sock.empty())
    {
        std::cout << "Specification of Unix Socket file path is mandatory" <<std::endl;
        Usage();
        return AICS_ERR_INVALID_ARGS;
    }

    return AICS_ERR_NONE;
}

int main(int argc, char** argv)
{
    int status = AICS_ERR_NONE;

    AicConfigData_t config = {  .socketInfo = {0} };
    status = ParseArgs(config, argc, argv);
    CHECK_STATUS(status);

    auto handler = std::make_unique<CmdHandler>(config);
    status = handler->Init();
    CHECK_STATUS(status);

    int count = 0;
    do{
       status = handler->ProcessNextEntry();
       if (status != AICS_ERR_EOF)
           CHECK_STATUS(status);
       count++;
    }
    while (status == AICS_ERR_NONE);

    std::cout << "End of Run: " << count << " events processed" << std::endl;

    return 0;
}
