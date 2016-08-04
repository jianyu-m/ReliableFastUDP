//
// Created by maxxie on 16-7-13.
//

#ifndef RELIABLEFASTUDP_PTHREADMUTEX_H
#define RELIABLEFASTUDP_PTHREADMUTEX_H
#include "../ReliableFastUDP.h"
#ifdef TARGET_LINUX
#include <pthread.h>
#endif // TARGET_LINUX

#ifdef TARGET_WIN32
#include <Windows.h>
#endif // TARGET_WIN32

namespace pthread {
    class Mutex {
	public:
#ifdef TARGET_LINUX
		pthread_mutex_t mutex;
#endif // TARGET_LINUX
#ifdef TARGET_WIN32
		CRITICAL_SECTION mutex;
#endif // TARGET_WIN32
        Mutex();
        void lock();
        void unlock();
    };
}


#endif //RELIABLEFASTUDP_PTHREADMUTEX_H
