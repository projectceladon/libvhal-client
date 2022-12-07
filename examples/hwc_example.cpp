/**
 * Copyright (C) 2021-2022 Intel Corporation
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

#include "hwc_vhal.h"
#include <getopt.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include "receiver_log.h"

using namespace std;
using namespace vhal::client;

//static dpipe_t *videoPipe = nullptr;

void cmd_handler(CommandType cmd, const frame_info_t* frame)
{
    switch (cmd) {

        case FRAME_CREATE:
            printf("event %s\n", "FRAME_CREATE");
	    break;

	case FRAME_REMOVE:
            printf("event %s\n", "FRAME_REMOVE");
	    break;

        case FRAME_DISPLAY:
            //printf("event %s\n", "FRAME_DISPLAY");
	    // dont print huge log here
	    break;

        default:
            printf("event %s\n", "unknown event");
            break;

    } // end of switch
}

int main(int argc, char* argv[])
{
    ConfigInfo cfg;
    cfg.video_res_width = 1280;
    cfg.video_res_height = 720;
    cfg.video_device = "/dev/dri/renderD128";
    cfg.user_id = 0;

    const char *ret = getenv("K8S_ENV");
    if (ret == NULL || strncmp(ret, "true", 4) != 0) {
        // docker env
        char *env = getenv("AIC_WORKDIR");
        if (env != NULL) {
            std::string str = env;
            cfg.unix_conn_info.socket_dir = std::string(str + "/ipc");
            cfg.unix_conn_info.android_instance_id = 0; //id is required for docker env
        }
    } else {
        // k8s env
        cfg.unix_conn_info.socket_dir = "/conn";
        cfg.unix_conn_info.android_instance_id = -1; //dont need id for k8s env
    }

    //create pipe
    /*
    video_source_setup_ex(&cfginfo, 1) < 0)
    videoPipe = dpipe_lookup("video-0"))
    */

    //create hwc receiver
    VirtualHwcReceiver* receiver = new VirtualHwcReceiver(cfg, cmd_handler);
    IOResult ior = receiver->start();
    int index = 0;
    while (!std::get<0>(ior)) {
        index ++;

        if (index % 300 == 0) {
            printf("tag: index(%d)\n", index);
        }

        if (index == 3600) {
            printf("tag: index, (%s)!!\n", "setmode");
            IOResult ior = receiver->setMode(1920, 1080);
	     int res = std::get<0>(ior);
	     if (res<0) {
	         printf("tag: setMode(%s)\n", "failed");
	     } else {
	         printf("tag: setMode(%s)\n", "successfully");
	     }
        }

        if (index == 5000) {
            IOResult ior = receiver->stop();
             int res = std::get<0>(ior);
             if (res<0) {
                 printf("tag: stop(%s)\n", "failed");
             } else {
                 printf("tag: stop(%s)\n", "successfully");
             }
        }

        if (index == 6000) {
            delete receiver;
            receiver = NULL;
        }
        this_thread::sleep_for(5ms);
    }

    if (receiver != NULL)
        delete receiver;
    return 0;

}
