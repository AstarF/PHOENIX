#include <stdio.h>
#include <sys/socket.h>
#include "px_worker.h"
#include "px_log.h"
#include "px_error.h"
#include "px_epoll.h"
#include "px_thread_pool.h"
#include "px_locker.h"
#include "px_resources.h"
#include "px_events.h"
#include "px_connections.h"
#include "px_module.h"
#include "px_timer.h"
#include "px_list.h"
#include "px_util.h"
int px_worker::signal_pipe[2];//得定义一下

px_worker* px_worker::get_instance() {
	static px_worker* instance = new px_worker;
	return instance;
}

void px_worker::work() { thread_pool->begin(); }
void px_worker::stop() { thread_pool->stop(); }
void px_worker::terminate() { thread_pool->terminate(); isterminate = true; }

/*工作进程初始化
* 初始化epoll模块
* 初始化signal pipe
* 初始化模块
* 初始化线程池
*/
int px_worker::init(px_service_module* serv_module) {
	this->serv_module = serv_module;
	this->isterminate = false;
	this->thread_num = serv_module->thread_num;

	this->thread_lock = new px_locker;
	this->epoll_module = new px_epoll;
	this->epoll_module->px_epoll_init();

	//init signal channel
	if (pipe(signal_pipe) == -1) {
		px_log::get_log_instance().write("pipe() failed.");
		return PX_SYSAPI_ERROR;
	}
	set_file(signal_pipe[0], O_NONBLOCK);
	set_file(signal_pipe[1], O_NONBLOCK);

	serv_module->init_modules();

	//init threadpool
	param = new px_thread_param[thread_num];
	workqueue = new px_objqueue<px_event>[thread_num];
	for (int i = 0; i < this->thread_num; ++i) {
		param[i].pxepoll = epoll_module;
		param[i].worker = this;
		param[i].workqueue = workqueue + i;
	}
	thread_pool = new px_thread_pool;
	thread_pool->init(thread_num, process, param);
	thread_pool->create_threads();
}

//删除相关资源
void px_worker::destory() {
	delete epoll_module;
	delete param;
	delete thread_lock;
	delete[] workqueue;
}

//设置捕获的信号
void px_worker::addsig(int sig) {
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = signal_handler;
	sa.sa_flags |= SA_RESTART;//当系统调用被中断时，重新启动系统调用
	sigfillset(&sa.sa_mask);
	if (sigaction(sig, &sa, NULL) == -1) {
		px_log::get_log_instance().write("sigaction() failed.");
		return;//
	}
}

/*
* woker的循环
* 目前保持一个循环，然后什么事都不做
*/
void px_worker::dispatch_loop() {
	work();
	//fork()
	while (!isterminate) {}
}

/*
* 信号处理函数
* 仅仅是简单的转发信号
*/
void px_worker::signal_handler(int sig) {
	int save_errno = errno;//保证可重入性
	int msg = sig;
	printf("sig: %d\n", sig);
	send(signal_pipe[1], (char*)&msg, 1, 0);//fwrite
	errno = save_errno;
}

/*
* 线程的循环
*/
void* px_worker::process(px_thread* pxthread) {
	px_thread_param* param = pxthread->data;
	if (param) {
		px_worker* worker = param->worker;
		worker->thread_lock->lock();
		worker->epoll_module->px_process_event(pxthread);
		px_resources::get_instance()->timer->tick(pxthread);
		worker->thread_lock->unlock();		
		int size = param->workqueue->getcurrnetblocks();
		while (size--) {
			px_event* events = param->workqueue->pop_front();
			events->callback(pxthread);
		}
	}
	return nullptr;
}
