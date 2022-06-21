#ifndef PX_THREAD_POOL
#define PX_THREAD_POOL
#include "px_type.h"
#include <pthread.h>

//==Statement==
class px_thread_pool;

//==Predefined Constant==
//�߳�״̬
enum class px_thread_status {
	ACTIVE,STOP,TERMINATE
};

/*==Thread==
* �����̶߳����ָ�롢�Զ�����̱߳�š�״̬���������̳߳ء��̶߳�������
*/
struct px_thread
{
	pthread_t thread_ptr;
	px_uint thread_index;
	px_thread_status status;
	px_thread_pool* thread_pool;
	px_thread_param* data;
};

//==Thead Pool==
class px_thread_pool {
public:
	px_thread_pool();
	int init(int thread_num, void* (*func)(px_thread* pxthread), px_thread_param* params);
	int create_threads();
	~px_thread_pool();
	void stop();
	void begin();
	void terminate();
	void set_threaddata(int i, px_thread_param* data);
private:
	static void* run(void* arg);
	int pool_thread_num;
	px_thread* thread_list;
	bool isstop;
	bool isterminate;
	void* (*func)(px_thread* pxthread);
};
#endif // !PX_THREAD_POOL
