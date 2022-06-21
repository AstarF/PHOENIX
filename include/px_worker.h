#ifndef PX_WORKER
#define PX_WORKER
#include <errno.h> 
#include<signal.h>
#include "px_type.h"

/*==px_thread_param==
* ���̵��̲߳���
*/
struct px_thread_param {
	px_epoll* pxepoll;//���̵�epollģ��
	px_worker* worker;//����
	px_objqueue<px_event>* workqueue;//�̵߳Ĺ�������
};

/*==Worker==
* ��������
* ��һ������
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
	px_thread_param* param;//�̲߳����б�
	px_epoll* epoll_module;//epollģ��
	px_objqueue<px_event>* workqueue;//�̹߳�������
	px_locker* thread_lock;//thread lock
	px_service_module* serv_module;//�������̶�Ӧ��servģ��
	static int signal_pipe[2];//ͳһ�¼�Դ�ܵ�
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
