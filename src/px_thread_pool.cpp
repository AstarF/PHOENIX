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
* pthread_num:�߳���
* pfunc���߳�����ʱ���õĺ���
* params���̸߳��������ݽṹ����worker�����̣�ϢϢ��أ����嶨���worker
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

/*�����߳�
* ���⽫��ʼ�������̷ֿ߳���ȷ���߳�����ʱ������������׼�����
*/
int px_thread_pool::create_threads() {
	for (int i = 0; i < pool_thread_num; ++i) {
		if (pthread_create(&(thread_list[i].thread_ptr), NULL, run, thread_list + i) != 0) {
			px_log::get_log_instance().write("pthread_create() failed.");
			delete[] thread_list;
			return PX_SYSAPI_ERROR;
		}
		if (pthread_detach(thread_list[i].thread_ptr)) {//�⽫�����̵߳�״̬����Ϊdetached,����߳����н�������Զ��ͷ�������Դ��
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

/*���ڿ����̵߳�����
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