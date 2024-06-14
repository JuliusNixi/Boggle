    /*

    From:
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

   /*
   
   I will test PTHREAD_MUTEX_ERRORCHECK because I think I will use it in the project's
   disconnectClient() function.
   
   */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

// Special mutex and its attributes.
pthread_mutexattr_t a;
pthread_mutex_t s;

// Normal mutex.
pthread_mutex_t n;

// I don't feel like using conditions vars... 
int flag = 0;
int flag2 = 0;
int flag3 = 0;

void* f(void* args) {

    int retvalue;
    // Should be OK for both.
    retvalue = pthread_mutex_trylock(&s);
    printf("RET TRYLOCK NOT LOCKED ANOTHER THREAD (NO MAIN) SPECIAL: %d.\n", retvalue);
    retvalue = pthread_mutex_trylock(&n);
    printf("RET TRYLOCK NOT LOCKED ANOTHER THREAD (NO MAIN) NORMAL: %d.\n\n\n", retvalue);
    
    flag2 = 1;
    while(1){
        sleep(1);
        if (flag) break;
    }
    printf("OTHER THREAD CONTINUE...\n\n\n");

    // Should be OK for both.
    retvalue = pthread_mutex_unlock(&s);
    printf("RET UNLOCK LOCKED BY US ANOTHER THREAD (NO MAIN) SPECIAL: %d.\n", retvalue);
    retvalue = pthread_mutex_unlock(&n);
    printf("RET UNLOCK LOCKED BY US ANOTHER THREAD (NO MAIN) NORMAL: %d.\n\n\n", retvalue);

    flag3 = 1;

    return NULL;

}

int main(void) {

    int ret;

    printf("---------INIT START---------\n");


    // Initializing mutex attributes.
    ret = pthread_mutexattr_init(&a);
    printf("ATTR INIT: %d.\n", ret);

    // Setting special mutex attributes (type).
    ret = pthread_mutexattr_settype(&a, PTHREAD_MUTEX_ERRORCHECK);
    printf("ATTR (TYPE) SET: %d.\n", ret);

    // Initializing special mutex.
    // WARNINIG: PTHREAD_MUTEX_INITIALIZER could be used only with static mutex.
    ret = pthread_mutex_init(&s, &a);
    printf("SPECIAL MUTEX INIT: %d.\n", ret);

    // Initializing normal mutex.
    ret = pthread_mutex_init(&n, NULL);  
    printf("NORMAL MUTEX INIT: %d.\n", ret);

    printf("---------INIT COMPLETED---------\n\n\n");






    // Attempting to unlock an unlocked mutex results in undefined behavior.
    ret = pthread_mutex_unlock(&n);
    printf("RET UNLOCK NOT LOCKED NORMAL: %d.\n", ret);
    // A thread attempting to unlock an unlocked mutex shall return with an error.
    ret = pthread_mutex_unlock(&s);
    printf("RET UNLOCK NOT LOCKED SPECIAL: %d.\n\n\n", ret);

    pthread_t t;
    pthread_create(&t, NULL, f, NULL);
    while(1) {
        sleep(1);
        if (flag2) break;
    }
    printf("WARNING: A thread has modified the mutexes.\n\n\n");

    // Attempting to unlock a mutex locked by a different thread results in undefined behavior. 
    ret = pthread_mutex_unlock(&n);
    printf("RET UNLOCK LOCKED BUT BY NOT US NORMAL: %d.\n", ret);
    // A thread attempting to unlock a mutex which another thread has locked shall return with an error.
    ret = pthread_mutex_unlock(&s);
    printf("RET UNLOCK LOCKED BUT BY NOT US SPECIAL: %d.\n\n\n", ret);

    // Should be OK for both, but it's locked by someone else.
    ret = pthread_mutex_trylock(&s);
    printf("RET TRYLOCK LOCKED BUT BY NOT US SPECIAL: %d.\n", ret);
    ret = pthread_mutex_trylock(&n);
    printf("RET TRYLOCK LOCKED BUT BY NOT US NORMAL: %d.\n\n\n", ret);

    // Waking up the other thread.
    flag = 1;

    // Waiting for it.
    while(1){
        sleep(1);
        if (flag3) break;
    }

    // Should be OK.
    ret = pthread_mutex_trylock(&s);
    printf("RET TRYLOCK NOT LOCKED SPECIAL: %d.\n", ret);
    ret = pthread_mutex_trylock(&n);
    printf("RET TRYLOCK NOT LOCKED NORMAL: %d.\n\n\n", ret);  

    // This type of mutex provides error checking.
    // A thread attempting to relock this mutex without first unlocking it shall
    // return with an error.
    ret = pthread_mutex_lock(&s);
    printf("RET LOCK ALREADY LOCKED BY US SPECIAL: %d.\n", ret);
    // This type of mutex does not detect deadlock. 
    // A thread attempting to relock this mutex without first unlocking it shall deadlock.
    printf("DEADLOCK INCOMING...\n");
    fflush(stdout);
    ret = pthread_mutex_lock(&n);
    // This printf will be never executed... :(
    printf("RET LOCK ALREADY LOCKED BY US NORMAL: %d.\n\n\n", ret);  

    return 0;

}

