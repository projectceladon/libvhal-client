#include "hwc_vhal.h"
#include <getopt.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include "receiver_log.h"

using namespace std;
using namespace vhal::client;

//static dpipe_t *videoPipe = nullptr;

void cmd_handler(CommandType cmd, cros_gralloc_handle_t handle)
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

    if (getenv("K8S_ENV") == NULL || strcmp(getenv("K8S_ENV"), "true") != 0) {
        // docker env
        cfg.unix_conn_info.socket_dir = std::string(getenv("AIC_WORKDIR")) + "/ipc";
        cfg.unix_conn_info.android_instance_id = 0; //id is required for docker env
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
        }
        this_thread::sleep_for(5ms);
    }
    return 0;

}
