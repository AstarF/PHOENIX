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
int px_worker::signal_pipe[2];//�ö���һ��

px_worker* px_worker::get_instance() {
	static px_worker* instance = new px_worker;
	return instance;
}

void px_worker::work() { thread_pool->begin(); }
void px_worker::stop() { thread_pool->stop(); }
void px_worker::terminate() { thread_pool->terminate(); isterminate = true; }

/*�������̳�ʼ��
* ��ʼ��epollģ��
* ��ʼ��signal pipe
* ��ʼ��ģ��
* ��ʼ���̳߳�
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

//ɾ�������Դ
void px_worker::destory() {
	delete epoll_module;
	delete param;
	delete thread_lock;
	delete[] workqueue;
}

//���ò�����ź�
void px_worker::addsig(int sig) {
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = signal_handler;
	sa.sa_flags |= SA_RESTART;//��ϵͳ���ñ��ж�ʱ����������ϵͳ����
	sigfillset(&sa.sa_mask);
	if (sigaction(sig, &sa, NULL) == -1) {
		px_log::get_log_instance().write("sigaction() failed.");
		return;//
	}
}

/*
* woker��ѭ��
* Ŀǰ����һ��ѭ����Ȼ��ʲô�¶�����
*/
void px_worker::dispatch_loop() {
	work();
	//fork()
	while (!isterminate) {}
}

/*
* �źŴ�����
* �����Ǽ򵥵�ת���ź�
*/
void px_worker::signal_handler(int sig) {
	int save_errno = errno;//��֤��������
	int msg = sig;
	printf("sig: %d\n", sig);
	send(signal_pipe[1], (char*)&msg, 1, 0);//fwrite
	errno = save_errno;
}

/*
* �̵߳�ѭ��
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
