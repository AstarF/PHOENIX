#include "px_locker.h"
#include "px_log.h"
#include "px_error.h"

px_locker::px_locker() {
	if (pthread_mutex_init(&m_mutex, NULL) != 0) {
		px_log::get_log_instance().write("pthread_mutex_init() failed.");
	}
}
px_locker::~px_locker() {
	pthread_mutex_destroy(&m_mutex);
}
bool px_locker::lock() {
	return pthread_mutex_lock(&m_mutex) == 0;
}
bool px_locker::unlock() {
	return pthread_mutex_unlock(&m_mutex) == 0;
}