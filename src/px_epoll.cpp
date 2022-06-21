#include<sys/epoll.h>
#include "px_log.h"
#include "px_error.h"
#include "px_util.h"
#include "px_list.h"
#include "px_epoll.h"
#include "px_events.h"
#include "px_worker.h"
#include "px_connections.h"
#include "px_thread_pool.h"
#include "px_module.h"

/*Destructor
*/
px_epoll::~px_epoll() {
	close(epollfd);
	delete[] event_list;
}

/*初始化
* 创建epoll文件描述符
* 创建event_list
*/
int px_epoll::px_epoll_init(int connection_num) {
	//create epollfd
	epollfd = epoll_create(connection_num);
	if (epollfd == -1) {
		px_log::get_log_instance().write("epoll_create() failed.");
		return PX_SYSAPI_ERROR;
	}
	//create event_list
	if (event_list) {
		delete[] event_list;
	}
	event_list = new epoll_event[MAX_EVENTS];
	if (event_list == nullptr) {
		px_log::get_log_instance().write("malloc event_list failed.");
		return PX_MALLOC_ERROR;
	}
	return 0;
}

/*添加连接
*/
int px_epoll::px_add_conn(px_connection* conn, int attri) {
	epoll_event event;
	event.data.ptr = conn;
	event.events = attri;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn->fd, &event) == -1) {
		px_log::get_log_instance().write("epoll_ctl() add failed.");
		return PX_SYSAPI_ERROR;
	}
	return PX_FINE;
}

/*设置连接
*/
int px_epoll::px_set_conn(px_connection* conn, int attri) {
	if (conn->status == pxconn_status_type::CONNECT) {
		epoll_event event;
		event.data.ptr = conn;
		event.events = attri;
		if (epoll_ctl(epollfd, EPOLL_CTL_MOD, conn->fd, &event) == -1) {
			px_log::get_log_instance().write("epoll_ctl() mod failed.");
			return PX_SYSAPI_ERROR;
		}
	}
	return PX_FINE;

}

/*删除连接
*/
int px_epoll::px_del_conn(px_connection* conn) {
	if (epoll_ctl(epollfd, EPOLL_CTL_DEL, conn->fd, 0) == -1) {
		px_log::get_log_instance().write("epoll_ctl() del failed.");
		return PX_SYSAPI_ERROR;
	}
	return PX_FINE;
}

/*事件循环，由各个线程调用，调用时需要加锁
*/
int px_epoll::px_process_event(px_thread* pxthread) {
	int event_num = epoll_wait(epollfd, event_list, MAX_EVENTS, 0);
	px_thread_param* tparam = nullptr;
	if (pxthread)tparam = pxthread->data;
	for (int i = 0; i < event_num; ++i) {
		unsigned int etype = event_list[i].events;
		px_connection* conn = (px_connection*)event_list[i].data.ptr;
		if (conn->status == pxconn_status_type::CONNECT && conn->instance == conn->read_evs->instance) {
			if ((etype)&EPOLLIN) {
				px_event* ptr = conn->read_evs;
				if (ptr) {
					if (ptr->immediacy || pxthread == nullptr)ptr->callback(pxthread);
					else tparam->workqueue->push_back(ptr);
				}
			}
			else if (((etype)&EPOLLOUT)) {
				px_event* ptr = conn->write_evs;
				if (ptr) {
					if (ptr->immediacy|| pxthread == nullptr)ptr->callback(pxthread);
					else tparam->workqueue->push_back(ptr);
				}
			}
		}
	}
	return 0;
}
