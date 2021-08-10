/**
 * @file host camera server.cc
 * @author Shiva Kumara R (shiva.kumara.rudrappa@intel.com)
 * @brief
 * @version 1.0
 * @date 2021-04-30
 *
 * Copyright (c) 2021 Intel Corporation
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

#include "vsock_stream_socket_client.h"
#include "video_sink.h"
#include <array>
#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <fcntl.h>

#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <unistd.h>

extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libavdevice/avdevice.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
}
using namespace std::chrono_literals;
using namespace vhal::client;
using namespace std;

typedef struct stream_ctx_t
{
    char *output_path, *output_format;
    AVInputFormat *ifmt;
    AVFormatContext *ifmt_ctx, *ofmt_ctx;
    AVCodec *in_codec, *out_codec;
    AVStream *in_stream, *out_stream;
    AVCodecContext *in_codec_ctx, *out_codec_ctx;
} stream_ctx_t;

stream_ctx_t *stream_ctx;

char device_index[] = "/dev/video0";
int width = 640;
int height = 480;
int fps = 30;
AVPacket *pkt;

#define BUF_COUNT 4
unsigned int buf_count = 0;
unsigned char *buf_list[BUF_COUNT];

void get_all_dev_nodes()
{
    int fd;
    struct v4l2_capability video_cap;
    char dev_name[50];
    for(int devId = 0; devId < 64; devId++) {
        sprintf(dev_name, "/dev/video%d", devId);

        if((fd = open(dev_name, O_RDONLY)) == -1) {
            continue;
	}

        if(ioctl(fd, VIDIOC_QUERYCAP, &video_cap) == -1)
            cout <<"cam_info: Can't get capabilities\n";
        else {
	    if(!strcmp((const char*)video_cap.driver, "v4l2 loopback"))
	        strcpy(device_index, dev_name);
        }
        close(fd);
    }

}

void dumpFrame(unsigned char *bufdest) {
    FILE* pFile;
    char file_name[100] = "output";
    unsigned int img_id = buf_count % 8;
    sprintf(file_name, "%d.yuv", img_id);
    pFile = fopen(file_name,"wb");

    if (pFile ){
        fwrite(bufdest,1,width * height * 1.5,pFile);
    }
    else
        cout << "Can't open file\n";
    if(pFile)
        fclose(pFile);

}

struct pixel_yvuv {
    uint8_t Y0;
    uint8_t V;
    uint8_t Y1;
    uint8_t U;
};

struct pixel_NV21_y_plane{
    uint8_t Y0;
    uint8_t Y1;
    uint8_t Y2;
    uint8_t Y3;
};


struct pixel_NV21_uv_plane{
    uint8_t data;
};

void
yuyv422_to_yuv420sp(unsigned char *bufsrc, unsigned char *dst_buf, int width, int height, bool flipuv)
{
    int i,j;

    volatile struct pixel_yvuv* src;
    volatile struct pixel_NV21_y_plane* dst_p1;
    volatile struct pixel_NV21_uv_plane* dst_p2;
    volatile struct pixel_NV21_uv_plane* dst_p3;
    src = (struct pixel_yvuv*)bufsrc;
    /* plane 1 */
    dst_p1 = (struct pixel_NV21_y_plane*) dst_buf;
    /* plane 2 */
    dst_p2 = (struct pixel_NV21_uv_plane*) (dst_buf + (height*width)); /* offset to UV plane */
    unsigned int v_plane = (height*width) + (height * width * 0.25);
    dst_p3 = (struct pixel_NV21_uv_plane*) (dst_buf + v_plane); /* offset to UV plane */

    for(i=1; i<=height; i++) {
        for(j=1; j<=width/2; j++) {
            if(j%2) {
                dst_p1->Y0 = src->Y0;
                dst_p1->Y1 = src->Y1;
            } else {
                dst_p1->Y2 = src->Y0;
                dst_p1->Y3 = src->Y1;
                 dst_p1++;
            }

            /* vertical subsampling for U and V plane */
            if(i%2) {
             /* U and V  Plane */
                if(flipuv) {
                    dst_p2->data  = src->U;
                    dst_p3->data  = src->V;
                } else {
                    dst_p2->data  = src->V;
                    dst_p3->data  = src->U;
                }
                dst_p2++;
                dst_p3++;
            }
            src++;
        }
    }
}

const char *get_device_family()
{
#ifdef _WIN32
  const char *device_family = "dshow";
#elif __APPLE__
  const char *device_family = "avfoundation";
#elif __linux__
  const char *device_family;
	  device_family = "v4l2";
#endif

  return device_family;
}

int init_device_and_input_context(stream_ctx_t *stream_ctx, const char *device_family, const char *device_index, int width, int height, int fps)
{

    int ret_code = 0;

    std::string fps_str = std::to_string(fps);
    std::string size = std::to_string(width) + std::string("x") + std::to_string(height);

    stream_ctx->ifmt = (AVInputFormat *)av_find_input_format(device_family);
    AVDictionary *options = NULL;
    av_dict_set(&options, "video_size", size.c_str(), 0);
    av_dict_set(&options, "framerate", fps_str.c_str(), 0);
    av_dict_set(&options, "pixel_format", "yuv420p", 0);
    av_dict_set(&options, "probesize", "7000000", 0);

    if (avformat_open_input(&stream_ctx->ifmt_ctx, device_index, stream_ctx->ifmt, &options) != 0)
    {
        fprintf(stderr, "cannot initialize input device!\n");
        ret_code = 1;
    }

    avformat_find_stream_info(stream_ctx->ifmt_ctx, 0);

    stream_ctx->in_codec = (AVCodec *)avcodec_find_decoder(stream_ctx->ifmt_ctx->streams[0]->codecpar->codec_id);
    stream_ctx->in_stream = avformat_new_stream(stream_ctx->ifmt_ctx, stream_ctx->in_codec);
    stream_ctx->in_codec_ctx = avcodec_alloc_context3(stream_ctx->in_codec);

    AVDictionary *codec_options = NULL;
    av_dict_set(&codec_options, "framerate", fps_str.c_str(), 0);
    av_dict_set(&codec_options, "preset", "superfast", 0);
    stream_ctx->in_codec_ctx->pix_fmt = AV_PIX_FMT_YUV422P;
    avcodec_parameters_to_context(stream_ctx->in_codec_ctx, stream_ctx->ifmt_ctx->streams[0]->codecpar);

    if (avcodec_open2(stream_ctx->in_codec_ctx, stream_ctx->in_codec, &codec_options) != 0)
    {
        cout << "cannot initialize video decoder!\n";
        ret_code = 1;
    }

    return 0;
}


int init_output_avformat_context(stream_ctx_t *stream_ctx)
{
    if (avformat_alloc_output_context2(&stream_ctx->ofmt_ctx, NULL, "flv", NULL) != 0)
    {
        fprintf(stderr, "cannot initialize output format context!\n");
        return 1;
    }
    return 0;
}

void set_codec_params(stream_ctx_t *stream_ctx, int width, int height, int fps)
{
    const AVRational dst_fps = {fps, 1};

    stream_ctx->out_codec_ctx->codec_tag = 0;
    stream_ctx->out_codec_ctx->codec_id = AV_CODEC_ID_H264;
    stream_ctx->out_codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    stream_ctx->out_codec_ctx->width = width;
    stream_ctx->out_codec_ctx->height = height;
    stream_ctx->out_codec_ctx->gop_size = 12;
    stream_ctx->out_codec_ctx->pix_fmt = AV_PIX_FMT_YUV422P;
    stream_ctx->in_codec_ctx->pix_fmt = AV_PIX_FMT_YUV422P;
    stream_ctx->out_codec_ctx->framerate = dst_fps;
    stream_ctx->out_codec_ctx->time_base = av_inv_q(dst_fps);
    if (stream_ctx->ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
    {
        stream_ctx->out_codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
}

int open_camera()
{
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 9, 100)
    av_register_all();
#endif
    avdevice_register_all();

    const char *device_family = get_device_family();

    stream_ctx = (stream_ctx_t *)malloc(sizeof(stream_ctx_t));
    if(!stream_ctx)
        return -1;
    stream_ctx->ifmt = NULL;
    stream_ctx->ifmt_ctx = NULL;
    stream_ctx->ofmt_ctx = NULL;
    stream_ctx->out_codec = NULL;
    stream_ctx->out_stream = NULL;
    stream_ctx->out_codec_ctx = NULL;

    if (init_device_and_input_context(stream_ctx, device_family, device_index, width, height, fps) != 0)
    {
        return -1;
    }
    init_output_avformat_context(stream_ctx);

    stream_ctx->out_codec = (AVCodec *)avcodec_find_encoder(AV_CODEC_ID_MPEG1VIDEO);
    stream_ctx->out_stream = avformat_new_stream(stream_ctx->ofmt_ctx, stream_ctx->out_codec);
    stream_ctx->out_codec_ctx = avcodec_alloc_context3(stream_ctx->out_codec);

    set_codec_params(stream_ctx, width, height, fps);

    stream_ctx->out_stream->codecpar->extradata = stream_ctx->out_codec_ctx->extradata;
    stream_ctx->out_stream->codecpar->extradata_size = stream_ctx->out_codec_ctx->extradata_size;

    AVFrame *frame = av_frame_alloc();
    AVFrame *outframe = av_frame_alloc();
    pkt = av_packet_alloc();

    int nbytes = av_image_get_buffer_size(stream_ctx->out_codec_ctx->pix_fmt, stream_ctx->out_codec_ctx->width, stream_ctx->out_codec_ctx->height, 32);
    uint8_t *video_outbuf = (uint8_t *)av_malloc(nbytes);
    av_image_fill_arrays(outframe->data, outframe->linesize, video_outbuf, AV_PIX_FMT_YUV420P, stream_ctx->out_codec_ctx->width, stream_ctx->out_codec_ctx->height, 1);
    outframe->width = width;
    outframe->height = height;
    stream_ctx->out_codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    stream_ctx->in_codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    outframe->format = stream_ctx->out_codec_ctx->pix_fmt;

    struct SwsContext *swsctx = sws_getContext(stream_ctx->in_codec_ctx->width, stream_ctx->in_codec_ctx->height,
           stream_ctx->in_codec_ctx->pix_fmt, stream_ctx->out_codec_ctx->width, stream_ctx->out_codec_ctx->height,
           stream_ctx->out_codec_ctx->pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);
    av_init_packet(pkt);
    buf_count = 0;
    //create buffer
    const size_t inbuf_size = width * height * 1.5;
    for(int count = 0; count < BUF_COUNT; count++)
	    buf_list[count] = (unsigned char*)calloc(1, inbuf_size);

    return 0;
}
int main(int argc, char** argv)
{
    atomic<bool> stop = true;
    int          instance_id = 3;
    thread       file_src_thread;
    atomic<bool> request_negotiation = false;

	//search for virtual device nodes
    get_all_dev_nodes();

    cout <<"open camera " << device_index;

    shared_ptr<VideoSink>   video_sink;

    VsockConnectionInfo conn_info = { instance_id };
    try {
        video_sink = make_shared<VideoSink>(conn_info);
    } catch (const std::exception& ex) {
        cout << "VideoSink creation error :"
             << ex.what() << endl;
        exit(1);
    }

    cout << "Waiting Camera Open callback..\n" << device_index;
    open_camera();

    video_sink->RegisterCallback(
      [&](const VideoSink::camera_config_cmd_t& ctrl_msg) {
          cout << "received new cmd to process ";

          switch (ctrl_msg.cmd) {
              case VideoSink::camera_cmd_t::CMD_OPEN:
	          cout << "Received Open command from Camera VHal\n";
                  stop = false;
                  file_src_thread = thread([&stop,
                                            &video_sink,
                                            &device_index]() {

                     const size_t inbuf_size = width * height * 1.5;
                      while (!stop) {
                          if(av_read_frame(stream_ctx->ifmt_ctx, pkt) < 0)
                              cout << "Fail to read frame";
                          yuyv422_to_yuv420sp(pkt->data, buf_list[buf_count % BUF_COUNT], width, height, false);
                          // Write payload
                          if (auto [sent, error_msg] =
                                video_sink->SendRawPacket(buf_list[buf_count % BUF_COUNT],
                                                            inbuf_size);
                              sent < 0) {
                              cout << "Error in writing payload to Camera VHal: "
                                << error_msg << "\n";
                              exit(1);
                          }
  //                        cout << "[rate=30fps] Sent "
//                               << " bytes to Camera VHal.\n";
                          buf_count++;
                          // sleep for 33ms to maintain 30 fps
                          this_thread::sleep_for(33ms);
                      }

                  });
                  break;

              case VideoSink::camera_cmd_t::CMD_CLOSE:
                  cout << "Received Close command from Camera VHal\n";
                  stop = true;
                  file_src_thread.join();
                  break;

             case VideoSink::camera_cmd_t::CMD_NONE:
                  cout << "Received None\n";
                  break;

              default:
                  cout << "Unknown Command received, exiting with failure : "  << (int)ctrl_msg.cmd << "\n";
                  break;
          }
      });

        if(!request_negotiation) {
            video_sink->GetCameraCapabilty();

            VideoSink::camera_capability_t camera_config;
            camera_config.codec_type = VideoSink::VideoCodecType::kI420;
            camera_config.resolution = VideoSink::FrameResolution::kVGA;
            video_sink->SetCameraCapabilty(&camera_config);
            request_negotiation = true;
        }
    // we need to be alive :)
    while (true) {
        this_thread::sleep_for(5ms);
    }

    return 0;
}
