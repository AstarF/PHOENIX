#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include "px_connections.h"
#include "px_error.h"
#include "px_list.h"
#include "px_events.h"
#include "px_worker.h"
#include "px_locker.h"
#include "px_epoll.h"
#include "px_resources.h"
#include "px_timer.h"
#include<time.h>
#include<fcntl.h>

const int px_deuault_bufsize = 2048;
const int px_max_bufsize = 1<<27;//128M
/*Constructor
*/
px_connection::px_connection() :
	read_evs(new px_event), write_evs(new px_event),time_evs(nullptr),
	fd(-1), addrlen(0), oneshot(false),
	buffer(new char[px_deuault_bufsize]), buffer_size(px_deuault_bufsize), buffer_used(0),
	wbuffer(new char[px_deuault_bufsize]), wbuffer_size(px_deuault_bufsize), wbuffer_used(0),wbuffer_read(0),
	status(pxconn_status_type::FREE), instance(1){}

/*Destructor
*/
px_connection::~px_connection() {
	delete this->read_evs;
	delete this->write_evs;
	delete[] this->buffer;
	delete[] this->wbuffer;
}

/*递增instance
* 目前没派上用场
*/
void px_connection::add_instance() {
	this->instance = (this->instance << 1) & MAX_INSTANCE;
}

/*重设读buffer大小，删除原有内容
*/
void px_connection::resizebuffer(size_t bufsize) {
	delete[] this->buffer;
	this->buffer = new char[bufsize];
	this->buffer_size = bufsize;
	memset(this->buffer, 0, buffer_size);
	buffer_used = 0;
}

/*重设读buffer大小，保留原有内容
* 读buffer不知道自己要读多少
*/
void px_connection::expansionbuffer() {
	size_t orisz = this->buffer_size;
	if (this->buffer_size < px_max_bufsize)this->buffer_size *= 2;
	else return;
	char* nbuf = new char[buffer_size];
	memcpy(nbuf, this->buffer, orisz);
	delete[] this->buffer;
	this->buffer = nbuf;
}

/*清除读写buffer
*/
void px_connection::clearbuffer() {
	buffer_used = 0;
	memset(buffer, 0, buffer_size);
	wbuffer_used = 0;
	wbuffer_read = 0;
	memset(wbuffer, 0, wbuffer_size);
}

/*重设写buffer大小，删除原有内容
* 写buffer知道自己要写多少
*/
void px_connection::resizewbuffer(size_t bufsize) {
	delete[] this->wbuffer;
	this->wbuffer = new char[bufsize];
	this->wbuffer_size = bufsize;
	memset(this->wbuffer, 0, this->wbuffer_size);
	this->wbuffer_used = 0;
}

/*重设写buffer大小，保留原有内容
*/
void px_connection::expansionwbuffer(size_t bufsize) {
	char* nbuf = new char[bufsize];
	memset(nbuf, 0, sizeof(nbuf));
	memcpy(nbuf, this->wbuffer, this->wbuffer_size);
	delete[] this->wbuffer;
	this->wbuffer = nbuf;
	this->wbuffer_size = bufsize;
}

/*清除写buffer
*/
void px_connection::clearwbuffer() {
	wbuffer_used = 0;
	wbuffer_read = 0;
	memset(wbuffer, 0, wbuffer_size);
}

/*初始化函数
* 每次使用connectoin时必须调用
*/
void px_connection::init_connection() {
	this->add_instance();
	this->read_evs->init_pxevent();
	this->read_evs->connect = this;
	this->read_evs->event_type = px_event_type::EV_READ;
	this->read_evs->instance = this->instance;
	this->read_evs->next = nullptr;
	this->write_evs->init_pxevent();
	this->write_evs->connect = this;
	this->write_evs->event_type = px_event_type::EV_WRITE;
	this->write_evs->instance = this->instance;
	this->write_evs->next = nullptr;
	this->time_evs = nullptr;
	if (this->fd)close(fd);
	this->fd = -1;
	this->oneshot = false;
	memset(&this->address, 0, sizeof(this->address));

	this->addrlen = 0;
	if (buffer_size ^ px_deuault_bufsize) {
		delete[] this->buffer;
		buffer = new char[px_deuault_bufsize];
		buffer_size = px_deuault_bufsize;
	}
	buffer_used = 0;
	memset(buffer, 0, px_deuault_bufsize);

	if (wbuffer_size ^ px_deuault_bufsize) {
		delete[] this->wbuffer;
		wbuffer = new char[px_deuault_bufsize];
		wbuffer_size = px_deuault_bufsize;
	}
	wbuffer_used = 0;
	wbuffer_read = 0;
	memset(wbuffer, 0, px_deuault_bufsize);
	this->status = pxconn_status_type::CONNECT;
}

/*添加事件
* 只能添加timer事件
* 仅仅是将事件添加到time_evs首部
*/
void px_connection::add_events(px_event* evs) {
	switch (evs->event_type)
	{
	case px_event_type::EV_WRITE:
		//禁止添加读事件
		//evs->set_connect(this);
		//evs->set_instance(this->instance);
		//evs->next = write_evs;
		//write_evs = evs;
		break;
	case px_event_type::EV_TIMEOUT:
		evs->connect = this;
		evs->instance = this->instance;
		evs->next = time_evs;
		if(time_evs)time_evs->last = evs;
		time_evs = evs;
		break;
	default:
		//禁止添加写事件
		//evs->set_connect(this);
		//evs->set_instance(this->instance);
		//evs->next = read_evs;
		//read_evs = evs;
		break;
	}
	evs->connect = this;
	evs->instance = this->instance;
}

/*关闭连接
* 仅仅将连接状态设置为CLOSE
*/
void px_connection::close_connect() {
	status = pxconn_status_type::CLOSE;
}

/*释放连接
* 将连接状态设置为FREE
* 并关闭连接的文件描述符
*/
void px_connection::free_connect() {
	status = pxconn_status_type::FREE;
	if (fd)close(fd);
	fd = -1;
}

/*更新timer事件
* 将所有允许更新目标时间的timer事件进行更新
*/
void px_connection::update_timer() {
	px_event* ptr = time_evs;
	while (ptr) {
		ptr->reset_save_time();
		ptr = ptr->next;
	}
}

/*释放连接：
* 为非成员函数：
* 1.连接不应该自己归还自己，因而定义该释放函数
* 2.对象池是非必要的，这种情况下用不到该释放函数
* 多线程情况下需要加锁
* 释放并归还时间事件->删除epoll模块的文件描述符->释放连接->将连接对归还对象池
*/
void conn_free_recovery(px_connection* conn) {
	px_objpool<px_event>* ep = px_resources::get_instance()->event_pool;
	px_timer* timer = px_resources::get_instance()->timer;
	while (conn->time_evs) {
		px_event* t = conn->time_evs;
		conn->time_evs = conn->time_evs->next;
		timer->del_event(t);
		t->free_event();
		ep->push_front(t);
	}
	px_worker::get_instance()->epoll_module->px_del_conn(conn);
	conn->read_evs->data.reset();
	conn->write_evs->data.reset();
	conn->free_connect();
	px_resources::get_instance()->conn_pool->push_front(conn);
}