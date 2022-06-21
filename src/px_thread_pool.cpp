#include "px_thread_pool.h"
#include "px_worker.h"
#include "px_log.h"
#include "px_error.h"
/*Constructor
*/
px_thread_pool::px_thread_pool() :isstop(false), pool_thread_num(0), thread_list(nullptr), func(nullptr), isterminate(false) {}

void px_thread_pool::stop() { this->isstop = true; }

void px_thread_pool::begin() { this->isstop = false; }

void px_thread_pool::terminate() { this->isterminate = true; }

void px_thread_pool::set_threaddata(int i, px_thread_param* data) { thread_list[i].data = data; }

/*
* pthread_num:线程数
* pfunc：线程运行时调用的函数
* params：线程附带的数据结构，与worker（进程）息息相关，具体定义见worker
*/
int px_thread_pool::init(int pthread_num, void* (*pfunc)(px_thread* pxthread), px_thread_param* params)
{
	pool_thread_num = pthread_num;
	func = pfunc;
	if (pool_thread_num <= 0) {
		px_log::get_log_instance().write("thread_num smaller than 0.");
		return PX_ATTRIBUTE_ERROR;
	}
	thread_list = new px_thread[pool_thread_num];
	if (thread_list == nullptr) {
		px_log::get_log_instance().write("malloc thread_list failed.");
		return PX_MALLOC_ERROR;
	}
	for (int i = 0; i < pool_thread_num; ++i) {
		thread_list[i].thread_index = i;
		thread_list[i].thread_pool = this;
		thread_list[i].status = px_thread_status::ACTIVE;
		thread_list[i].data = &(params[i]);
	}
	return PX_FINE;
}

/*创建线程
* 特意将初始化创建线程分开，确保线程运行时，所有数据已准备完毕
*/
int px_thread_pool::create_threads() {
	for (int i = 0; i < pool_thread_num; ++i) {
		if (pthread_create(&(thread_list[i].thread_ptr), NULL, run, thread_list + i) != 0) {
			px_log::get_log_instance().write("pthread_create() failed.");
			delete[] thread_list;
			return PX_SYSAPI_ERROR;
		}
		if (pthread_detach(thread_list[i].thread_ptr)) {//这将该子线程的状态设置为detached,则该线程运行结束后会自动释放所有资源。
			px_log::get_log_instance().write("pthread_detach() failed.");
			delete[] thread_list;
			return PX_SYSAPI_ERROR;
		}
	}
	return PX_FINE;
}

/*Destructor
*/
px_thread_pool::~px_thread_pool() {
	delete[] thread_list;
	isstop = true;
}

/*用于控制线程的运行
*/
void* px_thread_pool::run(void* arg) {
	px_thread* pxthread = (px_thread*)arg;
	px_thread_pool* thread_pool = (px_thread_pool*)pxthread->thread_pool;
	//printf("threadid: %d\n", pxthread->thread_index);
	while (!thread_pool->isterminate) {
		if (!thread_pool->isstop && pxthread->status == px_thread_status::ACTIVE)thread_pool->func(pxthread);
		if (pxthread->status == px_thread_status::TERMINATE)break;
	}
	return nullptr;//pthread_exit(nullptr);
}