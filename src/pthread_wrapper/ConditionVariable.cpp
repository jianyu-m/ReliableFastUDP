//
// Created by maxxie on 16-7-13.
//

#include "ConditionVariable.h"

using namespace pthread;

#ifdef TARGET_LINUX

ConditionVariable::ConditionVariable() {
    pthread_condattr_t init_t;
    pthread_condattr_init(&init_t);
    pthread_condattr_setclock(&init_t, CLOCK_MONOTONIC);
    pthread_cond_init(&condition_variable, &init_t);
}

void ConditionVariable::wait(Mutex *mutex) {
    pthread_cond_wait(&condition_variable, &mutex->mutex);
}

int ConditionVariable::wait_time(Mutex *mutex, unsigned long ts) {
    struct timespec tv;
    clock_gettime(CLOCK_MONOTONIC, &tv);
    tv.tv_nsec += ts * 1000000;
    if (tv.tv_nsec >= 1000000000) {
        tv.tv_sec += 1;
        tv.tv_nsec -= 1000000000;
    }
    return pthread_cond_timedwait(&this->condition_variable, &mutex->mutex, &tv);
}

void ConditionVariable::signal() {
    pthread_cond_signal(&condition_variable);
}

void ConditionVariable::broadcast() {
    pthread_cond_broadcast(&condition_variable);
}

#endif // TARGET_LINUX

#ifdef TARGET_WIN32

ConditionVariable::ConditionVariable() {
	InitializeConditionVariable(&condition_variable);
}

void ConditionVariable::wait(Mutex *mutex) {
	SleepConditionVariableCS(&condition_variable, &mutex->mutex, INFINITE);
}

int ConditionVariable::wait_time(Mutex *mutex, unsigned long ts) {
    return SleepConditionVariableCS(&condition_variable, &mutex->mutex, ts);
}

void ConditionVariable::signal() {
	WakeConditionVariable(&condition_variable);
}

void ConditionVariable::broadcast() {
	WakeAllConditionVariable(&condition_variable);
}
#endif // TARGET_WIN32
