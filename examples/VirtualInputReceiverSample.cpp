#include "VirtualInputReceiver.h"
#include <getopt.h>
#include <iostream>
#include <stdio.h>
#include <string>

using namespace std;
int debug = 0x1;
void
TestButton1(VirtualInputReceiver* vir)
{
    if ((debug & 0x1) > 0)
        printf("\n\n\n\t%s:%d BUTTON_1\n", __func__, __LINE__);
    vir->onJoystickMessage("k 288 1\n"); // BUTTON_1 Down
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);
    vir->onJoystickMessage("k 288 0\n"); // BUTTON_1 Up
    vir->onJoystickMessage("c\n");
}

void
TestButtonMode(VirtualInputReceiver* vir)
{
    if ((debug & 0x1) > 0)
        printf("\n\n\n\t%s:%d BUTTON_MODE\n", __func__, __LINE__);
    vir->onJoystickMessage("k 316 1\n"); // BUTTON_MODE Down
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);
    vir->onJoystickMessage("k 316 0\n"); // BUTTON_MODE Up
    vir->onJoystickMessage("c\n");
}

void
TestJoystick(VirtualInputReceiver* vir)
{
    if ((debug & 0x1) > 0)
        printf("\n\n\n\t%s:%d BTN_TL\n", __func__, __LINE__);
    vir->onJoystickMessage("k 310 1\n"); // BTN_TL Down
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);
    vir->onJoystickMessage("k 310 0\n"); // BTN_TL Up
    vir->onJoystickMessage("c\n");

    usleep(200 * 1000);

    if ((debug & 0x1) > 0)
        printf("\n\n\n\t%s:%d BTN_TR\n", __func__, __LINE__);
    vir->onJoystickMessage("k 311 1\n"); // BTN_TR Down
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);
    vir->onJoystickMessage("k 311 0\n"); // BTN_TR Up
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);

    if ((debug & 0x1) > 0)
        printf("\n\n\n\t%s:%d BTN_TL2\n", __func__, __LINE__);
    vir->onJoystickMessage("k 312 1\n"); // BTN_TL2 Down
    vir->onJoystickMessage("c\n");
    usleep(40000);
    vir->onJoystickMessage("a 62 0\n");
    vir->onJoystickMessage("c\n");
    usleep(40000);
    vir->onJoystickMessage("a 62 50\n");
    vir->onJoystickMessage("c\n");
    usleep(40000);
    vir->onJoystickMessage("a 62 100\n");
    vir->onJoystickMessage("c\n");
    usleep(40000);
    vir->onJoystickMessage("a 62 150\n");
    vir->onJoystickMessage("c\n");
    usleep(40000);
    vir->onJoystickMessage("a 62 200\n");
    vir->onJoystickMessage("c\n");
    usleep(40000);
    vir->onJoystickMessage("a 62 255\n");
    vir->onJoystickMessage("c\n");

    usleep(200 * 1000);

    vir->onJoystickMessage("a 62 200\n");
    vir->onJoystickMessage("c\n");
    usleep(40000);

    vir->onJoystickMessage("a 62 150\n");
    vir->onJoystickMessage("c\n");
    usleep(40000);
    vir->onJoystickMessage("a 62 100\n");
    vir->onJoystickMessage("c\n");
    usleep(40000);
    vir->onJoystickMessage("a 62 50\n");
    vir->onJoystickMessage("c\n");
    usleep(40000);
    vir->onJoystickMessage("a 62 0\n");
    vir->onJoystickMessage("c\n");
    usleep(40000);
    vir->onJoystickMessage("k 312 0\n"); // BTN_TL2 Up
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);

    if ((debug & 0x1) > 0)
        printf("\n\n\n\t%s:%d BTN_TR2\n", __func__, __LINE__);
    vir->onJoystickMessage("k 313 1\n"); // BTN_TR2 Down
    vir->onJoystickMessage("c\n");
    usleep(40000);
    vir->onJoystickMessage("a 63 0\n");
    vir->onJoystickMessage("c\n");
    usleep(40000);
    vir->onJoystickMessage("a 63 50\n");
    vir->onJoystickMessage("c\n");
    usleep(40000);
    vir->onJoystickMessage("a 63 100\n");
    vir->onJoystickMessage("c\n");
    usleep(40000);
    vir->onJoystickMessage("a 63 150\n");
    vir->onJoystickMessage("c\n");
    usleep(40000);
    vir->onJoystickMessage("a 63 200\n");
    vir->onJoystickMessage("c\n");
    usleep(40000);
    vir->onJoystickMessage("a 63 255\n");
    vir->onJoystickMessage("c\n");

    usleep(200 * 1000);

    vir->onJoystickMessage("a 63 200\n");
    vir->onJoystickMessage("c\n");
    usleep(40000);

    vir->onJoystickMessage("a 63 150\n");
    vir->onJoystickMessage("c\n");
    usleep(40000);
    vir->onJoystickMessage("a 63 100\n");
    vir->onJoystickMessage("c\n");
    usleep(40000);
    vir->onJoystickMessage("a 63 50\n");
    vir->onJoystickMessage("c\n");
    usleep(40000);
    vir->onJoystickMessage("a 63 0\n");
    vir->onJoystickMessage("c\n");
    usleep(40000);
    vir->onJoystickMessage("k 313 0\n"); // BTN_TR2 Up
    vir->onJoystickMessage("c\n");

    usleep(200 * 1000);

    if ((debug & 0x1) > 0)
        printf("\n\n\n\t%s:%d BTN_SELECT\n", __func__, __LINE__);
    vir->onJoystickMessage("k 314 1\n"); // BTN_SELECT Down
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);
    vir->onJoystickMessage("k 314 0\n"); // BTN_SELECT Up
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);

    if ((debug & 0x1) > 0)
        printf("\n\n\n\t%s:%d BTN_START\n", __func__, __LINE__);
    vir->onJoystickMessage("k 315 1\n"); // BTN_START Down
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);
    vir->onJoystickMessage("k 315 0\n"); // BTN_START Up
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);

    if ((debug & 0x1) > 0)
        printf("\n\n\n\t%s:%d BTN_THUMBL\n", __func__, __LINE__);
    vir->onJoystickMessage("k 317 1\n"); // BTN_THUMBL Down
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);
    vir->onJoystickMessage("k 317 0\n"); // BTN_THUMBL Up
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);

    if ((debug & 0x1) > 0)
        printf("\n\n\n\t%s:%d BTN_THUMBR\n", __func__, __LINE__);
    vir->onJoystickMessage("k 318 1\n"); // BTN_THUMBR Down
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);
    vir->onJoystickMessage("k 318 0\n"); // BTN_THUMBR Up
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);

    // BTN_Y
    if ((debug & 0x1) > 0)
        printf("\n\n\n\t%s:%d BTN_Y\n", __func__, __LINE__);
    vir->onJoystickMessage("k 308 1\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);
    vir->onJoystickMessage("k 308 0\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);

    // BTN_X
    if ((debug & 0x1) > 0)
        printf("\n\n\n\t%s:%d BTN_X\n", __func__, __LINE__);
    vir->onJoystickMessage("k 307 1\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);
    vir->onJoystickMessage("k 307 0\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);

    // BTN_A
    if ((debug & 0x1) > 0)
        printf("\n\n\n\t%s:%d BTN_A\n", __func__, __LINE__);
    vir->onJoystickMessage("k 304 1\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);
    vir->onJoystickMessage("k 304 0\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);

    // BTN_B
    if ((debug & 0x1) > 0)
        printf("\n\n\n\t%s:%d BTN_B\n", __func__, __LINE__);
    vir->onJoystickMessage("k 305 1\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);
    vir->onJoystickMessage("k 305 0\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);

    // Directional pad ABS_HAT0Y North
    if ((debug & 0x1) > 0)
        printf("\n\n\n\t%s:%d Directional pad ABS_HAT0Y North\n",
               __func__,
               __LINE__);
    vir->onJoystickMessage("a 17 -1\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);
    vir->onJoystickMessage("a 17 0\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);

    // Directional pad ABS_HAT0Y South
    if ((debug & 0x1) > 0)
        printf("\n\n\n\t%s:%d Directional pad ABS_HAT0Y South\n",
               __func__,
               __LINE__);
    vir->onJoystickMessage("a 17 1\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);
    vir->onJoystickMessage("a 17 0\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);

    // Directional pad ABS_HAT0X West
    if ((debug & 0x1) > 0)
        printf("\n\n\n\t%s:%d Directional pad ABS_HAT0X West\n",
               __func__,
               __LINE__);
    vir->onJoystickMessage("a 16 -1\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);
    vir->onJoystickMessage("a 16 0\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);

    // Directional pad ABS_HAT0X East
    if ((debug & 0x1) > 0)
        printf("\n\n\n\t%s:%d Directional pad ABS_HAT0X East\n",
               __func__,
               __LINE__);
    vir->onJoystickMessage("a 16 1\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);
    vir->onJoystickMessage("a 16 0\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);

    // Right Stick West
    if ((debug & 0x1) > 0)
        printf("\n\n\n\t%s:%d Right Stick West\n", __func__, __LINE__);
    vir->onJoystickMessage("a 2 0\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 2 -31\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 2 -63\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 2 -95\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 2 -127\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);
    vir->onJoystickMessage("a 2 -95\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 2 -63\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 2 -31\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 2 0\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);

    // Right Stick East
    if ((debug & 0x1) > 0)
        printf("\n\n\n\t%s:%d Right Stick East\n", __func__, __LINE__);
    vir->onJoystickMessage("a 2 0\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 2 31\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 2 63\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 2 95\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 2 127\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);
    vir->onJoystickMessage("a 2 95\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 2 63\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 2 31\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 2 0\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);

    // Right Stick North
    if ((debug & 0x1) > 0)
        printf("\n\n\n\t%s:%d Right Stick North\n", __func__, __LINE__);
    vir->onJoystickMessage("a 5 0\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 5 -31\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 5 -63\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 5 -95\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 5 -127\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);
    vir->onJoystickMessage("a 5 -95\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 5 -63\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 5 -31\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 5 0\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);

    // Right Stick South
    if ((debug & 0x1) > 0)
        printf("\n\n\n\t%s:%d Right Stick South\n", __func__, __LINE__);
    vir->onJoystickMessage("a 5 0\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 5 31\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 5 63\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 5 95\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 5 127\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);
    vir->onJoystickMessage("a 5 95\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 5 63\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 5 31\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 5 0\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);

    // Left Stick West
    if ((debug & 0x1) > 0)
        printf("\n\n\n\t%s:%d Left Stick West\n", __func__, __LINE__);
    vir->onJoystickMessage("a 0 0\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 0 -31\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 0 -63\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 0 -95\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 0 -127\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);
    vir->onJoystickMessage("a 0 -95\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 0 -63\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 0 -31\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 0 0\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);

    // Left Stick East
    if ((debug & 0x1) > 0)
        printf("\n\n\n\t%s:%d Left Stick East\n", __func__, __LINE__);
    vir->onJoystickMessage("a 0 0\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 0 31\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 0 63\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 0 95\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 0 127\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);
    vir->onJoystickMessage("a 0 95\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 0 63\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 0 31\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 0 0\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);

    // Left Stick North
    if ((debug & 0x1) > 0)
        printf("\n\n\n\t%s:%d Left Stick North\n", __func__, __LINE__);
    vir->onJoystickMessage("a 1 0\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 1 -31\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 1 -63\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 1 -95\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 1 -127\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);
    vir->onJoystickMessage("a 1 -95\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 1 -63\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 1 -31\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 1 0\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);

    // Left Stick South
    if ((debug & 0x1) > 0)
        printf("\n\n\n\t%s:%d Left Stick South\n", __func__, __LINE__);
    vir->onJoystickMessage("a 1 0\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 1 31\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 1 63\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 1 95\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 1 127\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);
    vir->onJoystickMessage("a 1 95\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 1 63\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 1 31\n");
    vir->onJoystickMessage("c\n");
    usleep(10000);
    vir->onJoystickMessage("a 1 0\n");
    vir->onJoystickMessage("c\n");
    usleep(200 * 1000);
}

void
TestTouch(VirtualInputReceiver* vir)
{
    // Touch Event Please map to 32768 x 32768
    if ((debug & 0x1) > 0)
        printf("\n\n\n\t%s:%d Touch Event\n", __func__, __LINE__);

    vir->onInputMessage("d 0 16384 16384 200\n");
    vir->onInputMessage("c\n");

    usleep(3 * 1000);
    vir->onInputMessage("m 0 16384 15203 200\n");
    vir->onInputMessage("c\n");

    usleep(3 * 1000);
    vir->onInputMessage("m 0 16384 14022 200\n");
    vir->onInputMessage("c\n");

    usleep(3 * 1000);
    vir->onInputMessage("m 0 16384 13841 200\n");
    vir->onInputMessage("c\n");

    usleep(3 * 1000);
    vir->onInputMessage("m 0 16384 12660 200\n");
    vir->onInputMessage("c\n");

    usleep(3 * 1000);
    vir->onInputMessage("m 0 16384 11479 200\n");
    vir->onInputMessage("c\n");

    usleep(3 * 1000);
    vir->onInputMessage("m 0 16384 10298 200\n");
    vir->onInputMessage("c\n");

    usleep(3 * 1000);
    vir->onInputMessage("m 0 16384 9117 200\n");
    vir->onInputMessage("c\n");

    usleep(3 * 1000);
    vir->onInputMessage("m 0 16384 8936 200\n");
    vir->onInputMessage("c\n");

    usleep(3 * 1000);
    vir->onInputMessage("m 0 16384 7755 200\n");
    vir->onInputMessage("c\n");

    usleep(3 * 1000);
    vir->onInputMessage("m 0 16384 6574 200\n");
    vir->onInputMessage("c\n");

    usleep(3 * 1000);
    vir->onInputMessage("m 0 16384 5479 200\n");
    vir->onInputMessage("c\n");

    usleep(3 * 1000);
    vir->onInputMessage("u 0\n");
    vir->onInputMessage("c\n");
}

void
TestEnableJoystick(VirtualInputReceiver* vir)
{
    if ((debug & 0x1) > 0)
        printf("\n\n\n\t%s:%d enable joystick down\n", __func__, __LINE__);
    vir->onJoystickMessage("k 631 1\n");
    vir->onJoystickMessage("c\n");
    usleep(2000);
    if ((debug & 0x1) > 0)
        printf("\n\n\n\t%s:%d enable joystick up\n", __func__, __LINE__);
    vir->onJoystickMessage("k 631 0\n");
    vir->onJoystickMessage("c\n");
}

void
TestDisableJoystick(VirtualInputReceiver* vir)
{
    if ((debug & 0x1) > 0)
        printf("\n\n\n\t%s:%d disable joystick down\n", __func__, __LINE__);
    vir->onJoystickMessage("k 632 1\n"); // BTN_TL Down
    vir->onJoystickMessage("c\n");
    usleep(2000);
    if ((debug & 0x1) > 0)
        printf("\n\n\n\t%s:%d disable joystick up\n", __func__, __LINE__);
    vir->onJoystickMessage("k 632 0\n"); // BTN_TL Down
    vir->onJoystickMessage("c\n");
}

string        short_options  = "c:hi:n:";
struct option long_options[] = {
    { "cmd", 0, NULL, 'c' },
    { "help", 1, NULL, 'h' },
    { "instance", 1, NULL, 'i' },
    { "input", 1, NULL, 'n' },
    { 0, 0, 0, 0 },
};

int
main(int argc, char* argv[])
{
    int   c;
    int   index     = 0;
    char* p_opt_arg = NULL;
    int   cmd       = 0;
    int   instance  = 0;
    int   input     = 0;

    while ((c = getopt_long(argc,
                            argv,
                            short_options.c_str(),
                            long_options,
                            &index)) != -1) {
        switch (c) {
            case 'c':
                p_opt_arg = optarg;
                cmd       = atoi(p_opt_arg);
                if ((debug & 0x2) > 0)
                    printf("cmd: %d\n", cmd);
                break;
            case 'i':
                p_opt_arg = optarg;
                instance  = atoi(p_opt_arg);
                if ((debug & 0x2) > 0)
                    printf("instance: %d\n", instance);
                break;
            case 'n':
                p_opt_arg = optarg;
                input     = atoi(p_opt_arg);
                if ((debug & 0x2) > 0)
                    printf("input: %d\n", input);
                break;
            case 'h':
                if ((debug & 0x2) > 0)
                    printf("%s\n"
                           "\t-c, --cmd cmd\n"
                           "\t      1: Test BUTTON_1. \n"
                           "\t      2: Test BUTTON_MODE. \n"
                           "\t      4: Test Joystick. \n"
                           "\t      8: Test touchscreen. \n"
                           "\t      16: Test enable Joystick. \n"
                           "\t      32: Test disable Joystick. \n"
                           "\t-i, --instance instance ID \n"
                           "\t-n, --iNput joystick ID \n"
                           "\t-h, --help help \n",
                           argv[0]);
                break;
            default:
                if ((debug & 0x2) > 0)
                    printf("Nock: c = %c, index =%d \n", c, index);
        }
    }
    VirtualInputReceiver* vir = new VirtualInputReceiver(instance, input);
    if ((debug & 0x1) > 0)
        printf("\t%s:%d Remote input test:\n", __func__, __LINE__);

    if ((cmd & 0x1) > 0)
        TestButton1(vir);

    if ((cmd & 0x2) > 0)
        TestButtonMode(vir);

    if ((cmd & 0x4) > 0)
        TestJoystick(vir);

    if ((cmd & 0x8) > 0)
        TestTouch(vir);

    if ((cmd & 0x10) > 0)
        TestEnableJoystick(vir);

    if ((cmd & 0x20) > 0)
        TestDisableJoystick(vir);

    return 0;
}
