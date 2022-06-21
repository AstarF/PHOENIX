#ifndef  PX_LOCKER
#define PX_LOCKER
#include <pthread.h>
/*仅仅是对linux pthread_mutex_t的一个简单的封装
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
