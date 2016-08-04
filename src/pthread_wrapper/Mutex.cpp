//
// Created by maxxie on 16-7-13.
//

#include "Mutex.h"

using namespace pthread;

#ifdef TARGET_LINUX

Mutex::Mutex() {
    pthread_mutex_init(&mutex, 0);
}

void Mutex::lock() {
    pthread_mutex_lock(&mutex);
}

void Mutex::unlock() {
    pthread_mutex_unlock(&mutex);
}

#endif // TARGET_LINUX

#ifdef TARGET_WIN32
Mutex::Mutex() {
	InitializeCriticalSection(&mutex);
}

void Mutex::lock() {
	EnterCriticalSection(&mutex);
}

void Mutex::unlock() {
	LeaveCriticalSection(&mutex);
}
#endif // TARGET_WIN32
