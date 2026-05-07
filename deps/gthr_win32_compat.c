/* Compatibility shim for precompiled libraries built with win32-threads GCC.
 * Provides __gthr_win32_* symbols expected by those libraries when building
 * with a posix-threads GCC. */
#include <windows.h>

/* Mutex operations */
int __gthr_win32_mutex_init_function(CRITICAL_SECTION *mutex) {
    InitializeCriticalSection(mutex);
    return 0;
}

int __gthr_win32_mutex_lock(CRITICAL_SECTION *mutex) {
    EnterCriticalSection(mutex);
    return 0;
}

int __gthr_win32_mutex_unlock(CRITICAL_SECTION *mutex) {
    LeaveCriticalSection(mutex);
    return 0;
}

int __gthr_win32_mutex_destroy(CRITICAL_SECTION *mutex) {
    DeleteCriticalSection(mutex);
    return 0;
}

/* Recursive mutex operations (same as regular for CRITICAL_SECTION) */
int __gthr_win32_recursive_mutex_init(CRITICAL_SECTION *mutex) {
    InitializeCriticalSection(mutex);
    return 0;
}

int __gthr_win32_recursive_mutex_lock(CRITICAL_SECTION *mutex) {
    EnterCriticalSection(mutex);
    return 0;
}

int __gthr_win32_recursive_mutex_unlock(CRITICAL_SECTION *mutex) {
    LeaveCriticalSection(mutex);
    return 0;
}

int __gthr_win32_recursive_mutex_destroy(CRITICAL_SECTION *mutex) {
    DeleteCriticalSection(mutex);
    return 0;
}

/* Condition variable operations */
typedef struct {
    CONDITION_VARIABLE cv;
} gthr_win32_cond_t;

int __gthr_win32_cond_init_function(CONDITION_VARIABLE *cond) {
    InitializeConditionVariable(cond);
    return 0;
}

int __gthr_win32_cond_broadcast(CONDITION_VARIABLE *cond) {
    WakeAllConditionVariable(cond);
    return 0;
}

int __gthr_win32_cond_signal(CONDITION_VARIABLE *cond) {
    WakeConditionVariable(cond);
    return 0;
}

int __gthr_win32_cond_wait(CONDITION_VARIABLE *cond, CRITICAL_SECTION *mutex) {
    SleepConditionVariableCS(cond, mutex, INFINITE);
    return 0;
}

int __gthr_win32_cond_destroy(CONDITION_VARIABLE *cond) {
    /* Windows condition variables don't need destruction */
    (void)cond;
    return 0;
}

/* Once operation */
int __gthr_win32_once(LONG *once_control, void (*init_routine)(void)) {
    /* Simple implementation using InterlockedCompareExchange */
    if (InterlockedCompareExchange(once_control, 1, 0) == 0) {
        init_routine();
        InterlockedExchange(once_control, 2);
    } else {
        while (*once_control != 2) {
            Sleep(0);
        }
    }
    return 0;
}

/* Yield */
void __gthr_win32_yield(void) {
    Sleep(0);
}
