#ifndef __WINAPI_THREAD_TEMPLATE_H__
#define __WINAPI_THREAD_TEMPLATE_H__

#include <process.h>

typedef uintptr_t thread_handle_type;
typedef void thread_return_type;

int create_thread(thread_handle_type* thread, thread_return_type(*start_routine) (void *), void *arg)
{
    uintptr_t handle = _beginthread(start_routine, 0, arg);
    *thread = handle;
    return handle ? 0 : 1;
}

void cancel_thread(thread_handle_type handle)
{
    /* Bad idea: */
    TerminateThread((HANDLE)handle, 0);
}

#endif
