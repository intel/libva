#ifndef _I965_MUTEX_H_
#define _I965_MUTEX_H_

#include "intel_compiler.h"

#if defined PTHREADS
#include <pthread.h>

typedef pthread_mutex_t _I965Mutex;

static INLINE void _i965InitMutex(_I965Mutex *m)
{
    pthread_mutex_init(m, NULL);
}

static INLINE void
_i965DestroyMutex(_I965Mutex *m)
{
    pthread_mutex_destroy(m);
}

static INLINE void
_i965LockMutex(_I965Mutex *m)
{
    pthread_mutex_lock(m);
}

static INLINE void
_i965UnlockMutex(_I965Mutex *m)
{
    pthread_mutex_unlock(m);
}

#define _I965_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#define _I965_DECLARE_MUTEX(m)                    \
    _I965Mutex m = _I965_MUTEX_INITIALIZER

#else

typedef int _I965Mutex;
static INLINE void _i965InitMutex(_I965Mutex *m) { (void) m; }
static INLINE void _i965DestroyMutex(_I965Mutex *m) { (void) m; }
static INLINE void _i965LockMutex(_I965Mutex *m) { (void) m; }
static INLINE void _i965UnlockMutex(_I965Mutex *m) { (void) m; }

#define _I965_MUTEX_INITIALIZER 0
#define _I965_DECLARE_MUTEX(m)                    \
    _I965Mutex m = _I965_MUTEX_INITIALIZER

#endif

#endif /* _I965_MUTEX_H_ */
