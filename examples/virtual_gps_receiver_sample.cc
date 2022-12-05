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

#include "receiver_log.h"
#include "virtual_gps_receiver.h"

#include <arpa/inet.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <getopt.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

using namespace vhal::client;
#define USE_ANDROID_VIRTUAL_GPS_HAL
#define S_SIZE 1024

#ifdef USE_ANDROID_VIRTUAL_GPS_HAL
std::unique_ptr<VirtualGpsReceiver> vgr;
#endif

int       sock_client_fd;
int       epoll_fd;
int       loop_quit;           // Control injection_gps_data_thread
pthread_t injection_thread_id; // injection_gps_data_thread
char      server_ip[20];
int       port;
double    gps_lat;
double    gps_long;
double    gps_alt;
int       loop_exit; // Control receive_server_command_thread
int       injection_thread_flag = 0;

char const    short_options[] = "l:o:a:hs:p:";
struct option long_options[]  = {
    { "lat", 0, NULL, 'l' },
    { "long", 1, NULL, 'o' },
    { "alt", 1, NULL, 'a' },
    { "help", 1, NULL, 'h' },
    { "server-ip", 1, NULL, 's' },
    { "port", 1, NULL, 'p' },
    { 0, 0, 0, 0 },
};

static int
do_geo_nmea(char* args)
{
    if (!args) {
        printf("KO: NMEA sentence missing, try 'help geo nmea'\r\n");
        return -1;
    }

    char     s[S_SIZE];
    size_t   s_len = 0;
    IOResult ior;
    int      ret = -1;
    printf("Note: currently, the size of s is fixed(S_SIZE: %d)\n", S_SIZE);
    memset(s, 0, S_SIZE);
    s_len = strlen(args);
    strncpy(s, args, sizeof(s) - 1);
    s[s_len++] = '\n';

#ifdef USE_ANDROID_VIRTUAL_GPS_HAL
    ior = vgr->Write((uint8_t*)s, s_len);
    ret = std::get<0>(ior);
    if (ret > 0) {
        printf("Data: %s\n", args);
        printf("%s set ret(%d) data to GPS socket.\n", __func__, ret);
    }
#endif
    return ret;
}

static int
do_geo_fix(char* args)
{
    // GEO_SAT2 provides bug backwards compatibility.
    enum
    {
        GEO_LONG = 0,
        GEO_LAT,
        GEO_ALT,
        GEO_SAT,
        GEO_SAT2,
        NUM_GEO_PARAMS
    };
    char*    p         = args;
    int      top_param = -1;
    double   params[NUM_GEO_PARAMS];
    int      n_satellites = 1;
    IOResult ior;
    int      ret = -1;

    if (!p) {
        printf("KO: argument  is NULL\n");
        return -1;
    }

    /* tokenize */
    while (*p) {
        char*  end;
        double val = strtod(p, &end);

        if (end == p) {
            printf("KO: argument '%s' is not a number\n", p);
            return -1;
        }

        params[++top_param] = val;
        if (top_param + 1 == NUM_GEO_PARAMS)
            break;

        p = end;
        while (*p && (p[0] == ' ' || p[0] == '\t'))
            p += 1;
    }

    /* sanity check */
    if (top_param < GEO_LAT) {
        printf("KO: not enough arguments: see 'help geo fix' for details\r\n");
        return -1;
    }

    /* check number of satellites, must be integer between 1 and 12 */
    if (top_param >= GEO_SAT) {
        int sat_index = (top_param >= GEO_SAT2) ? GEO_SAT2 : GEO_SAT;
        n_satellites  = (int)params[sat_index];
        if (n_satellites != params[sat_index] || n_satellites < 1 ||
            n_satellites > 12) {
            printf("KO: invalid number of satellites. Must be an integer "
                   "between 1 and 12\r\n");
            return -1;
        }
    }

    /* generate an NMEA sentence for this fix */
    {
        char   s[S_SIZE];
        size_t s_len = 0;
        double val;
        int    deg, min;
        char   hemi;
        int    hh = 0, mm = 0, ss = 0;

        printf("Note: currently, the size of s is fixed(S_SIZE: %d)\n", S_SIZE);
        memset(s, 0, S_SIZE);
        /* format overview:
         *    time of fix      123519     12:35:19 UTC
         *    latitude         4807.038   48 degrees, 07.038 minutes
         *    north/south      N or S
         *    longitude        01131.000  11 degrees, 31. minutes
         *    east/west        E or W
         *    fix quality      1          standard GPS fix
         *    satellites       1 to 12    number of satellites being tracked
         *    HDOP             <dontcare> horizontal dilution
         *    altitude         546.       altitude above sea-level
         *    altitude units   M          to indicate meters
         *    diff             <dontcare> height of sea-level above ellipsoid
         *    diff units       M          to indicate meters (should be
         * <dontcare>) dgps age         <dontcare> time in seconds since last
         * DGPS fix dgps sid         <dontcare> DGPS station id
         */

        // Get the current time as hh:mm:ss
        struct timeval tm;

        if (0 == gettimeofday(&tm, NULL)) {
            // tm.tv_sec is elapsed seconds since epoch (UTC, which is what we
            // want)
            hh = (int)(tm.tv_sec / (60 * 60)) % 24;
            mm = (int)(tm.tv_sec / 60) % 60;
            ss = (int)(tm.tv_sec) % 60;
        }

        s_len += sprintf(s + s_len, "$GPGGA,%02d%02d%02d", hh, mm, ss);

        /* then the latitude */
        hemi = 'N';
        val  = params[GEO_LAT];
        if (val < 0) {
            hemi = 'S';
            val  = -val;
        }
        deg = (int)val;
        val = 60 * (val - deg);
        min = (int)val;
        val = 10000 * (val - min);
        s_len +=
          sprintf(s + s_len, ",%02d%02d.%04d,%c", deg, min, (int)val, hemi);

        /* the longitude */
        hemi = 'E';
        val  = params[GEO_LONG];
        if (val < 0) {
            hemi = 'W';
            val  = -val;
        }
        deg = (int)val;
        val = 60 * (val - deg);
        min = (int)val;
        val = 10000 * (val - min);
        s_len +=
          sprintf(s + s_len, ",%02d%02d.%04d,%c", deg, min, (int)val, hemi);

        /* bogus fix quality, satellite count and dilution */
        s_len += sprintf(s + s_len, ",1,%02d,", n_satellites);

        /* optional altitude + bogus diff */
        if (top_param >= GEO_ALT) {
            s_len += sprintf(s + s_len, ",%.1g,M,0.,M", params[GEO_ALT]);
        } else {
            s_len += sprintf(s + s_len, ",,,,");
        }
        /* bogus rest and checksum */
        s_len += sprintf(s + s_len, ",,,*47");
        s[s_len] = '\n';
        s_len++;

#ifdef USE_ANDROID_VIRTUAL_GPS_HAL
        /* send it, then free */
        ior = vgr->Write((uint8_t*)s, s_len);
        ret = std::get<0>(ior);
        if (ret > 0) {
            s[s_len] = '\0';
            printf("Success to send s_len = %zd, ret = %d, s = %s\n",
                   s_len,
                   ret,
                   s);
        } else {
            s[s_len] = '\0';
            printf("Fail to send s_len = %zd, ret = %d, s = %s\n",
                   s_len,
                   ret,
                   s);
        }
#endif
    }
    return ret;
}

void*
injection_gps_data_thread(void* args)
{
    char   lat_long[64];
    size_t lat_long_len = 0;
    int    ret          = 0;
    int    long_max     = gps_long + 1;
    long_max            = long_max < -180 ? -180 : long_max;
    long_max            = long_max > 180 ? 180 : long_max;

    int lat_max = gps_lat + 1;
    lat_max     = lat_max < -90 ? -90 : lat_max;
    lat_max     = lat_max > 90 ? 90 : lat_max;

    int alt_max = gps_alt + 1000;
    alt_max     = alt_max < -400 ? -400 : alt_max;
    alt_max     = alt_max > 8848 ? 8848 : alt_max;

    while (!loop_quit) {
        int use_geo_fix = 1;
        if (use_geo_fix == 1) {
            printf("%s FIXME: Replace the data by real data.\n", __func__);
            memset(lat_long, 0, sizeof(lat_long));
            gps_long = gps_long + 0.00001;
            if (gps_long > long_max) {
                gps_long = long_max;
            }
            gps_lat = gps_lat + 0.00001;
            if (gps_lat > lat_max) {
                gps_lat = lat_max;
            }
            gps_alt = gps_alt + 1;
            if (gps_alt > alt_max) {
                gps_alt = alt_max;
            }
            lat_long_len           = sprintf(lat_long,
                                   "%.5f %.5f %.1f 5 6",
                                   gps_long,
                                   gps_lat,
                                   gps_alt);
            lat_long[lat_long_len] = '\0';

            printf("%s Execute command: geo fix %s\n", __func__, lat_long);

            // geo fix <longitude value> <latitude value>
            ret = do_geo_fix(lat_long);
        } else { // ToDo
            char nmea[] = "";
            ret         = do_geo_nmea(nmea);
        }

        if (ret < 0) {
            printf("%s Error: %d %s. Quit.\n", __func__, ret, strerror(errno));
            break;
        }
        printf("%s GPS 1HZ. Sleep 1s.\n", __func__);
        sleep(1);
    }
    return NULL;
}

#ifdef USE_ANDROID_VIRTUAL_GPS_HAL
void
cmd_handler(uint32_t cmd)
{
    if (cmd == VirtualGpsReceiver::Command::kGpsQuit) {
        AIC_LOG(LIBVHAL_DEBUG,
                "Send message: %s",
                VirtualGpsReceiver::gpsQuitMsg.c_str());
    } else if (cmd == VirtualGpsReceiver::Command::kGpsStart) {
        AIC_LOG(LIBVHAL_DEBUG,
                "Send message: %s",
                VirtualGpsReceiver::gpsStartMsg.c_str());
        if (!injection_thread_flag) {
            printf("%s gps thread starting on demand\n", __func__);
            injection_thread_flag = 1;
            loop_quit             = 0;
            pthread_create(&injection_thread_id,
                           NULL,
                           injection_gps_data_thread,
                           NULL);
        }
    } else if (cmd == VirtualGpsReceiver::Command::kGpsStop) {
        AIC_LOG(LIBVHAL_DEBUG,
                "Send message: %s",
                VirtualGpsReceiver::gpsStopMsg.c_str());
        if (injection_thread_flag) {
            printf("%s gps thread stopping on demand\n", __func__);
            loop_quit             = 1;
            injection_thread_flag = 0;
            pthread_join(injection_thread_id, NULL);
        }
    } else {
        AIC_LOG(LIBVHAL_DEBUG, "Unknown command.");
    }
};
#endif

int
main(int argc, char* argv[])
{
    int   c;
    int   index     = 0;
    char* p_opt_arg = NULL;

    printf("Set default value:\n");
    strncpy(server_ip, "172.100.0.2", 20);
    port      = 8766;
    gps_long  = 121.38215;
    gps_lat   = 31.07147;
    gps_alt   = 4;
    loop_exit = 0;

    while ((c = getopt_long(argc, argv, short_options, long_options, &index)) !=
           -1) {
        switch (c) {
            case 'l':
                p_opt_arg = optarg;
                gps_lat   = strtod(p_opt_arg, NULL);
                printf("gps_lat = %lf\n", gps_lat);
                break;
            case 'o':
                p_opt_arg = optarg;
                gps_long  = strtod(p_opt_arg, NULL);
                printf("gps_long = %lf\n", gps_long);
                break;
            case 'a':
                p_opt_arg = optarg;
                gps_alt   = strtod(p_opt_arg, NULL);
                printf("gps_alt = %lf\n", gps_alt);
                break;
            case 's':
                p_opt_arg = optarg;
                strncpy(server_ip, p_opt_arg, sizeof(server_ip) - 1);
                server_ip[sizeof(server_ip) -1] = '\0';
                break;
            case 'p':
                p_opt_arg = optarg;
                port      = atoi(p_opt_arg);
                printf("Set port to %d\n", port);
                break;
            case 'h':
                printf("%s\n"
                       "\t-l, --lat lat\n"
                       "\t-o, --long long\n"
                       "\t-a, --alt alt\n"
                       "\t-h, --help help\n"
                       "\t-s, --server-ip server-ip\n"
                       "\t-p, --port\n",
                       argv[0]);
                break;
            default:
                printf("Unkown: c = %c, index =%d \n", c, index);
        }
    }

#ifdef USE_ANDROID_VIRTUAL_GPS_HAL
    struct TcpConnectionInfo tci;
    tci.ip_addr = server_ip;
    vgr         = std::make_unique<VirtualGpsReceiver>(tci, cmd_handler);
#endif

    char str[16];
    int  flag = 1;

    while (flag) {
        memset(str, 0, sizeof(str));
        printf("%s Please input comand('q' for quit):", __func__);
	char *ret = fgets(str, 1, stdin);
        if (ret == NULL) {
            printf("%s Fail to get input. Continue. \n", __func__);
            continue;
        }
        if (strcmp(str, "q") == 0) {
            printf("%s quit\n", __func__);
            flag = 0;
            break;
        }
    }

    printf("%s Quit\n", __func__);
    return 0;
}
