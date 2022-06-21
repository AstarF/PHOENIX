#ifndef PX_WORKER
#define PX_WORKER
#include <errno.h> 
#include<signal.h>
#include "px_type.h"

/*==px_thread_param==
* 进程的线程参数
*/
struct px_thread_param {
	px_epoll* pxepoll;//进程的epoll模块
	px_worker* worker;//进程
	px_objqueue<px_event>* workqueue;//线程的工作队列
};

/*==Worker==
* 工作进程
* 是一个单例
*/
class px_worker {
public:
	static px_worker* get_instance();
	~px_worker() { destory(); }
	int init(px_service_module* serv_module);
	void dispatch_loop();
	void work();
	void stop();
	void terminate();
	void addsig(int sig);//for signal
public:
	px_thread_param* param;//线程参数列表
	px_epoll* epoll_module;//epoll模块
	px_objqueue<px_event>* workqueue;//线程工作队列
	px_locker* thread_lock;//thread lock
	px_service_module* serv_module;//工作进程对应的serv模块
	static int signal_pipe[2];//统一事件源管道
private:
	px_worker() {}
	//function
	static void* process(px_thread* pxthread);
	static void signal_handler(int sig);
	void destory();
	//variable
	px_thread_pool* thread_pool;//thread pool
	bool isterminate;
	int thread_num;
};

#endif // !PX_WORKER
