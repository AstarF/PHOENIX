#ifndef  PX_LOCKER
#define PX_LOCKER
#include <pthread.h>
/*�����Ƕ�linux pthread_mutex_t��һ���򵥵ķ�װ
*/
class px_locker {
public:
	px_locker();
	~px_locker();
	bool lock();
	bool unlock();
private:
	pthread_mutex_t m_mutex;
};
#endif // ! PX_LOCKER
