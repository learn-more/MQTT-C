#ifndef __POSIX_THREAD_TEMPLATE_H__
#define __POSIX_THREAD_TEMPLATE_H__

typedef pthread_t thread_handle_type;
typedef void* thread_return_type;

int create_thread(thread_handle_type* thread, thread_return_type(*start_routine) (void *), void *arg)
{
    return pthread_create(thread, NULL, start_routine, arg);
}

void cancel_thread(thread_handle_type handle)
{
    pthread_cancel(handle);
}

#endif
