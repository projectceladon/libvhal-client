#ifndef STATUS_PROBER_H
#define STATUS_PROBER_H

/**
 * @file status_prober.h
 * @author Deng Bing(bing.deng@intel.com)
 * @brief Status prober
 * @version 0.1
 * @date 2022-08-23
 *
 * Copyright (c) 2021 Intel Corporation
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
 */

#include <fstream>
#include <iostream>

namespace vhal {
namespace client {

class VirtualInputReceiver::StatusProber
{
public:
    StatusProber(std::string status_file) : mStatusFile(status_file) {}

    void UpdateStatus(const std::string& status)
    {
        std::ofstream statusFile;
        statusFile.open(mStatusFile);
        statusFile << status << "\n";
        statusFile.close();
    }

private:
    std::string mStatusFile;
};

} // namespace client
} // namespace vhal

#endif /* STATUS_PROBER_H */
