#include"_head.h"

struct pthread_pool_job {
    void *(*job_run) (void *);
    void *data;
};
struct pthread_pool_job_queue {
    struct pthread_pool_job *jobs;
    int index;			// point at jobs[index]
    int left;			// how many jobs left 

    pthread_mutex_t pthread_pool_worker_queue_lock;
};

struct pthread_pool_worker {
    struct pthread_pool *pool;	// point at father pool
    pthread_t pthread;
    int pthread_index;
};

struct pthread_pool {
    // jobs
    int max_job_num;
    struct pthread_pool_job_queue queue;

    // pthread jobs signal
    pthread_mutex_t lock_job_signal;
    pthread_cond_t job_signal;

    // pthreads
    int pthread_num;
    struct pthread_pool_worker *workers;

};

/***
* thread pool 
*
*	-init 
*	-add job
*	-destroy
*
*	#worker pthread
*		check job queue->find one to process->repeat
*					   ->wait for signal,process while signaled-> repeat
*  #job 	
*      add itself into queue->signal 
*	*/

struct pthread_pool *pthread_pool_init(int pthread_num,int max_job_num);
int pthread_pool_add_job(struct pthread_pool *pool,
			 void *(*job_run) (void *data), void *data);
int pthread_pool_destroy(struct pthread_pool *pool);
int pthread_pool_join_pthread(struct pthread_pool *pool);
int pthread_pool_detach_pthread(struct pthread_pool *pool);
/* send cancel signal to pthread */
int pthread_pool_cancel_pthread(struct pthread_pool *pool);


