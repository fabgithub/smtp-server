#include "util_pthread_pool.h" 

static void pthread_pool_worker_run_cleanup(void *data);
static void *pthread_pool_worker_run(void *data);

struct pthread_pool *
pthread_pool_init(int pthread_num, int max_job_num)
{
    struct pthread_pool *pool = NULL;
    pool = (struct pthread_pool *) malloc(sizeof (struct pthread_pool));
    if (NULL == pool)
	return NULL;

    pool->pthread_num = pthread_num;
    pool->max_job_num = max_job_num;
    // init jobs
    pool->queue.left = 0;
    pool->queue.index = 0;
    pool->queue.jobs =
	(struct pthread_pool_job *) malloc(pool->max_job_num *
					   sizeof (struct pthread_pool_job));
    if (NULL == pool->queue.jobs)
    {
	free(pool);
	return NULL;
    }

    // init worker
    pool->workers =
	(struct pthread_pool_worker *) malloc(pool->pthread_num *
					      sizeof (struct
						      pthread_pool_worker));
    if (NULL == pool->workers)
    {
	free(pool->queue.jobs);
	free(pool);
	return NULL;
    }
    for (int i = 0; i < pool->pthread_num; i++)
    {
	pthread_t pthread;
	pthread_attr_t threadAttr;
	pthread_attr_init(&threadAttr);
	pthread_create(&pthread, &threadAttr, pthread_pool_worker_run,
		       &pool->workers[i]);
	pool->workers[i].pthread = pthread;
	pool->workers[i].pool = pool;
	pool->workers[i].pthread_index = i;
    }

    // init lock and signal, recursive attr
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&pool->queue.pthread_pool_worker_queue_lock,
		       &mutex_attr);
    pthread_mutex_init(&pool->lock_job_signal, &mutex_attr);
    pthread_cond_init(&pool->job_signal, NULL);

    return pool;

}

int
pthread_pool_add_job(struct pthread_pool *pool,
		     void *(*job_run) (void *data), void *data)
{

    int success = 1;
    pthread_mutex_lock(&pool->queue.pthread_pool_worker_queue_lock);
    if (pool->queue.left >= pool->max_job_num)	// too many jobs to process?
    {
	success = 0;
    }
    else
    {
	int ni = (pool->queue.index + pool->queue.left) % pool->max_job_num;
	pool->queue.left++;
	pool->queue.jobs[ni].data = data;
	pool->queue.jobs[ni].job_run = job_run;
    }
    pthread_mutex_unlock(&pool->queue.pthread_pool_worker_queue_lock);

    if (!success)
	return -1;

    // notify jobs with signals
    // [***locka
    // [ to keep the signal reach wait success fully;
    // [***unlocka
    pthread_mutex_lock(&pool->lock_job_signal);
    pthread_cond_signal(&pool->job_signal);
    pthread_mutex_unlock(&pool->lock_job_signal);

    return 0;
}

int
pthread_pool_destroy(struct pthread_pool *pool)
{

    // free jobs queue
    free(pool->queue.jobs);

    // free jobs signal
    pthread_mutex_destroy(&pool->lock_job_signal);
    pthread_cond_destroy(&pool->job_signal);

    // free worker 
    free(pool->workers);

    // free pool
    free(pool);

    return 0;

}

int
pthread_pool_join_pthread(struct pthread_pool *pool)
{
    int ret = 0;
    for (int i = 0; i < pool->pthread_num; i++)
    {
	if (0 == pthread_join(pool->workers[i].pthread, NULL))
	{
	    ret++;
	}
    }
    return ret;
}

int
pthread_pool_detach_pthread(struct pthread_pool *pool)
{
    int ret = 0;
    for (int i = 0; i < pool->pthread_num; i++)
    {
	if (0 == pthread_detach(pool->workers[i].pthread))
	{
	    ret++;
	}
    }
    return ret;
}

int
pthread_pool_cancel_pthread(struct pthread_pool *pool)
{
    for (int i = 0; i < pool->pthread_num; i++)
    {
	pthread_cancel(pool->workers[i].pthread);
    }
    return 0;
}

/***
 *
 * 与线程取消相关的pthread函数
 * int pthread_cancel(pthread_t thread)
 *
 * int pthread_setcancelstate(int state,   int *oldstate)  
 * 设置本线程对Cancel信号的反应，state有两种值：PTHREAD_CANCEL_ENABLE（缺省）和PTHREAD_CANCEL_DISABLE，
 * 分别表示收到信号后设为CANCLED状态和忽略CANCEL信号继续运行；old_state如果不为NULL则存入原来的Cancel状态以便恢复。  
 *
 * int pthread_setcanceltype(int type, int *oldtype)  
 * 设置本线程取消动作的执行时机，type由两种取值：PTHREAD_CANCEL_DEFFERED和PTHREAD_CANCEL_ASYCHRONOUS，
 * 仅当Cancel状态为Enable时有效，分别表示收到信号后继续运行至下一个取消点再退出和立即执行取消动作（退出）；
 * oldtype如果不为NULL则存入运来的取消动作类型值。  
 *
 * void pthread_testcancel(void)
 * 是说pthread_testcancel在不包含取消点，但是又需要取消点的地方创建一个取消点，
 * 以便在一个没有包含取消点的执行代码线程中响应取消请求.
 * 线程取消功能处于启用状态且取消状态设置为延迟状态时，pthread_testcancel()函数有效。
 * 如果在取消功能处处于禁用状态下调用pthread_testcancel()，则该函数不起作用。
 * 请务必仅在线程取消线程操作安全的序列中插入pthread_testcancel()。
 * 除通过pthread_testcancel()调用以编程方式建立的取消点意外，pthread标准还指定了几个取消点。
 * 测试退出点,就是测试cancel信号.
 *
 * */

static void
pthread_pool_worker_run_cleanup(void *data)
{
    struct pthread_pool_worker *worker = (struct pthread_pool_worker *) data;
    struct pthread_pool *pthread_pool_instance = worker->pool;

    printf("pthread %d clean up\n", worker->pthread_index);
    // unlock it 
    pthread_mutex_unlock(&pthread_pool_instance->
			 queue.pthread_pool_worker_queue_lock);
    pthread_mutex_unlock(&pthread_pool_instance->lock_job_signal);

}

static void *
pthread_pool_worker_run(void *data)
{

    pthread_cleanup_push(pthread_pool_worker_run_cleanup, data);

    int ret;
    struct pthread_pool_worker *worker = (struct pthread_pool_worker *) data;
    struct pthread_pool *pthread_pool_instance = worker->pool;
    int oldtype;
    ret=pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldtype);
	if(ret)
	  ;
    for (;;)
    {
	// exit? 
	pthread_testcancel();
	// lock queue 
	//      find one 
	//  else exit
	// unlock 
	// if find process it 
	// if not found
	//              wait 

		/**
                * signal will loss during the time segment
                * so we alter lock like this 
                *			[-----locka
                *			[
                *			[
                *			[-----unlocka
                *			[ here may  happend signal,and loss signal 
                *			[*****lockb
                *			[ wait 
                *			[*****unlockb
                *
                *			[*****lockb
                *			[ notify like this 
                *			[*****unlockb
                *
                *			[-----locka
                *			[
                *			[*******lockb
                *			[
                *			[-----unlocka
                *			[
                *			[ wait and unlockb
                *			[
                *			[*******unlockb
                *			
                *			rule:
                *			    
                *			    students --->                     [wait] lost?
                *			                ---->[operate queue]-------------->signal..
                *			    teachers --->                     [alarm]
                *
                *			    vs.
                *
                *			    students --->                       
                *			                ---->[operate queue]--[wait/alarm]->signal..
                *			    teachers --->                       
                *
                *			    every thing(students/teachers) should call change.
                *			    only two roles in pools.
                *
                * */

	struct pthread_pool_job *job = NULL;
	pthread_mutex_lock(&pthread_pool_instance->
			   queue.pthread_pool_worker_queue_lock);

	if (pthread_pool_instance->queue.left > 0)
	{
	    int ji =
		(pthread_pool_instance->queue.index +
		 pthread_pool_instance->queue.left - 1)
		% pthread_pool_instance->max_job_num;

	    job = &(pthread_pool_instance->queue.jobs[ji]);

	    pthread_pool_instance->queue.index =
		(pthread_pool_instance->queue.index + 1)
		% pthread_pool_instance->max_job_num;

	    pthread_pool_instance->queue.left--;
	}

	if (NULL == job)
	{
	    pthread_mutex_lock(&pthread_pool_instance->lock_job_signal);
	}
	pthread_mutex_unlock(&pthread_pool_instance->
			     queue.pthread_pool_worker_queue_lock);

	if (NULL != job)
	{
	    // while we running job ,you cannot cancel it .
	    int oldstate;
	    ret = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
	    job->job_run(job->data);
	    ret = pthread_setcancelstate(oldstate, NULL);
	}
	else
	{
	    pthread_cond_wait(&pthread_pool_instance->job_signal,
			      &pthread_pool_instance->lock_job_signal);
	    pthread_mutex_unlock(&pthread_pool_instance->lock_job_signal);
	}
    }

    pthread_cleanup_pop(0);

    pthread_exit(0);
}

#ifdef TEST_PTHREAD_POOL 

#define TEST_SIZE 20

void *
test_job(void *data)
{
    int *int_data = (int *) data;

    struct timeval nowtv;
    gettimeofday(&nowtv, NULL);

    srand(nowtv.tv_usec);
    int sleep_seconds = rand() % 3;

    printf("%d  job,cost %d\n", *int_data, sleep_seconds);
    sleep(sleep_seconds);

    return data;
}


void
test_pthread_pool()
{

    int ret;
    struct pthread_pool *pool;
    pool = pthread_pool_init(3, 200);
    _error(NULL == pool, "pthread_pool_init fail\n");

    int *data = (int *) malloc(sizeof (int) * TEST_SIZE);
    for (int i = 0; i < TEST_SIZE; i++)
    {
	data[i] = i;
	if (i % 10 == 0)
	    sleep(1);
	ret = pthread_pool_add_job(pool, test_job, &data[i]);
    }
    printf("jobs add end\n");

    sleep(10);

    pthread_pool_cancel_pthread(pool);

    ret = pthread_pool_join_pthread(pool);
    printf("pthread_pool_join_pthread ret:%d\n", ret);

    ret = pthread_pool_detach_pthread(pool);
    printf("pthread_pool_destroy ret:%d\n", ret);
    ret = pthread_pool_destroy(pool);

    free(data);

    printf("exit main pthread\n");
    return;

}

int
main(int argc, char **argv)
{

    test_pthread_pool();
    return 0;
}
#endif 
