#ifndef __LIBSOCKETIO_CONFIG_
#define __LIBSOCKETIO_CONFIG_

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__linux__)
#include <pthread.h>
#endif

#define CC_SAFE_DELETE_ARRAY(_array) do { \
			if(_array != NULL) \
			{ \
				delete [] _array; \
				_array = NULL; \
			} \
		} while(0)

#define CC_SAFE_DELETE(_data) do { \
			if(_data != NULL) \
			{ \
				delete _data; \
				_data = NULL; \
			} \
		} while(0)

#define CC_UNUSED_PARAM(v) (void)(v)
#define CCLOG(fmt,...) do { \
							printf(fmt, ##__VA_ARGS__); \
							printf("\n"); \
						}while (0)

#ifdef _WIN32
typedef CRITICAL_SECTION cc_mutex_t;
typedef HANDLE cc_pthread_t;
#define CC_INIT_MUTEX(l) InitializeCriticalSection(&(l))
#define CC_DESTROY_MUTEX(l) DeleteCriticalSection(&(l))
#define CC_MUTEX_LOCK(l) EnterCriticalSection(&(l))
#define CC_MUTEX_UNLOCK(l) LeaveCriticalSection(&(l))
#define CC_SLEEP(ms) Sleep(ms)
#elif defined(__linux__)
typedef pthread_mutex_t cc_mutex_t;
typedef pthread_t cc_pthread_t;
#define CC_INIT_MUTEX(l) pthread_mutex_init(&(l),NULL)
#define CC_DESTROY_MUTEX(l) pthread_mutex_destroy(&(l))
#define CC_MUTEX_LOCK(l) pthread_mutex_lock(&(l))
#define CC_MUTEX_UNLOCK(l) pthread_mutex_unlock(&(l))
#define CC_SLEEP(ms) usleep((ms) * 1000)
#else
#error "Sorry We do not suppport this platform!!"
#endif

#endif
