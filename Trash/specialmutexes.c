
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

pthread_mutexattr_t a;
pthread_mutex_t s;
pthread_mutex_t n;

void* work(void* args) {

    int retvalue;
    retvalue = pthread_mutex_trylock(&s);
    printf("RET TRYLOCK NOT LOCKED WORK SPECIAL: %d.\n", retvalue);
    retvalue = pthread_mutex_trylock(&n);
    printf("RET TRYLOCK NOT LOCKED WORK NORMAL: %d.\n\n\n", retvalue);
    
    sleep(4);

    retvalue = pthread_mutex_unlock(&s);
    printf("RET UNLOCK LOCKED BY US WORK SPECIAL: %d.\n", retvalue);
    retvalue = pthread_mutex_unlock(&n);
    printf("RET UNLOCK LOCKED BY US WORK NORMAL: %d.\n\n\n", retvalue);
    return NULL;

}

int main(void) {

    /*

    https://linux.die.net/man/3/pthread_mutexattr_settype
    
The pthread_mutexattr_gettype() and pthread_mutexattr_settype() functions, respectively, shall get and set the mutex type attribute. This attribute is set in the type parameter to these functions. The default value of the type attribute is PTHREAD_MUTEX_DEFAULT.

The type of mutex is contained in the type attribute of the mutex attributes. Valid mutex types include:

PTHREAD_MUTEX_NORMAL
This type of mutex does not detect deadlock. A thread attempting to relock this mutex without first unlocking it shall deadlock. Attempting to unlock a mutex locked by a different thread results in undefined behavior. Attempting to unlock an unlocked mutex results in undefined behavior.

PTHREAD_MUTEX_ERRORCHECK
This type of mutex provides error checking. A thread attempting to relock this mutex without first unlocking it shall return with an error. A thread attempting to unlock a mutex which another thread has locked shall return with an error. A thread attempting to unlock an unlocked mutex shall return with an error.

PTHREAD_MUTEX_RECURSIVE
A thread attempting to relock this mutex without first unlocking it shall succeed in locking the mutex. The relocking deadlock which can occur with mutexes of type PTHREAD_MUTEX_NORMAL cannot occur with this type of mutex. Multiple locks of this mutex shall require the same number of unlocks to release the mutex before another thread can acquire the mutex. A thread attempting to unlock a mutex which another thread has locked shall return with an error. A thread attempting to unlock an unlocked mutex shall return with an error.

PTHREAD_MUTEX_DEFAULT
Attempting to recursively lock a mutex of this type results in undefined behavior. Attempting to unlock a mutex of this type which was not locked by the calling thread results in undefined behavior. Attempting to unlock a mutex of this type which is not locked results in undefined behavior. An implementation may map this mutex to one of the other mutex types.
    
    */


    // Special mutex.
    int ret;

    ret = pthread_mutexattr_init(&a);
    printf("ATTR INIT: %d.\n", ret);

    ret = pthread_mutexattr_settype(&a, PTHREAD_MUTEX_ERRORCHECK);
    printf("ATTR SET: %d.\n", ret);

    ret = pthread_mutex_init(&s, &a);
    printf("MUTEX INIT: %d.\n\n\n", ret);

    // Normal mutex.
    pthread_mutex_init(&n, NULL);  

    ret = pthread_mutex_unlock(&n);
    printf("RET UNLOCK NOT LOCKED NORMAL: %d.\n", ret);
    ret = pthread_mutex_unlock(&s);
    printf("RET UNLOCK NOT LOCKED SPECIAL: %d.\n\n\n", ret);

    pthread_t t;
    pthread_create(&t, NULL, work, NULL);
    sleep(2);

    ret = pthread_mutex_unlock(&n);
    printf("RET UNLOCK LOCKED BUT BY NOT US NORMAL: %d.\n", ret);
    ret = pthread_mutex_unlock(&s);
    printf("RET UNLOCK LOCKED BUT BY NOT US SPECIAL: %d.\n\n\n", ret);

    ret = pthread_mutex_trylock(&s);
    printf("RET TRYLOCK LOCKED SPECIAL: %d.\n", ret);
    ret = pthread_mutex_trylock(&n);
    printf("RET TRYLOCK LOCKED NORMAL: %d.\n\n\n", ret);

    sleep(8);

    ret = pthread_mutex_trylock(&s);
    printf("RET TRYLOCK NOT LOCKED SPECIAL: %d.\n", ret);
    ret = pthread_mutex_trylock(&n);
    printf("RET TRYLOCK NOT LOCKED NORMAL: %d.\n\n\n", ret);  

    ret = pthread_mutex_lock(&s);
    printf("RET LOCK ALREADY LOCKED BY US SPECIAL: %d.\n", ret);
    ret = pthread_mutex_lock(&n);
    printf("RET LOCK ALREADY LOCKED BY US NORMAL: %d.\n\n\n", ret);  

    return 0;

}

