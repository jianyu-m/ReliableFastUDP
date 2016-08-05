//
// Created by jyjia on 2016/7/12.
//

#ifndef RELIABLEFASTUDP_RELIABLEFASTUDP_H
#define RELIABLEFASTUDP_RELIABLEFASTUDP_H

#include "../../src/Sharp.h"

#define MAX_MTU 1600
//#define TARGET_WIN32
//#define TARGET_LINUX
#ifdef TARGET_WIN32
#include <WinSock2.h>
#define msleep(s) Sleep(s)
#endif // TARGET_WIN32
#ifdef TARGET_LINUX
#define msleep(s) usleep((s) * 1000)
#endif // TARGET_LINUX


#endif //RELIABLEFASTUDP_RELIABLEFASTUDP_H
