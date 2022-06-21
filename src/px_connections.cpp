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

/*����instance
* Ŀǰû�����ó�
*/
void px_connection::add_instance() {
	this->instance = (this->instance << 1) & MAX_INSTANCE;
}

/*�����buffer��С��ɾ��ԭ������
*/
void px_connection::resizebuffer(size_t bufsize) {
	delete[] this->buffer;
	this->buffer = new char[bufsize];
	this->buffer_size = bufsize;
	memset(this->buffer, 0, buffer_size);
	buffer_used = 0;
}

/*�����buffer��С������ԭ������
* ��buffer��֪���Լ�Ҫ������
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

/*�����дbuffer
*/
void px_connection::clearbuffer() {
	buffer_used = 0;
	memset(buffer, 0, buffer_size);
	wbuffer_used = 0;
	wbuffer_read = 0;
	memset(wbuffer, 0, wbuffer_size);
}

/*����дbuffer��С��ɾ��ԭ������
* дbuffer֪���Լ�Ҫд����
*/
void px_connection::resizewbuffer(size_t bufsize) {
	delete[] this->wbuffer;
	this->wbuffer = new char[bufsize];
	this->wbuffer_size = bufsize;
	memset(this->wbuffer, 0, this->wbuffer_size);
	this->wbuffer_used = 0;
}

/*����дbuffer��С������ԭ������
*/
void px_connection::expansionwbuffer(size_t bufsize) {
	char* nbuf = new char[bufsize];
	memset(nbuf, 0, sizeof(nbuf));
	memcpy(nbuf, this->wbuffer, this->wbuffer_size);
	delete[] this->wbuffer;
	this->wbuffer = nbuf;
	this->wbuffer_size = bufsize;
}

/*���дbuffer
*/
void px_connection::clearwbuffer() {
	wbuffer_used = 0;
	wbuffer_read = 0;
	memset(wbuffer, 0, wbuffer_size);
}

/*��ʼ������
* ÿ��ʹ��connectoinʱ�������
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

/*����¼�
* ֻ�����timer�¼�
* �����ǽ��¼���ӵ�time_evs�ײ�
*/
void px_connection::add_events(px_event* evs) {
	switch (evs->event_type)
	{
	case px_event_type::EV_WRITE:
		//��ֹ��Ӷ��¼�
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
		//��ֹ���д�¼�
		//evs->set_connect(this);
		//evs->set_instance(this->instance);
		//evs->next = read_evs;
		//read_evs = evs;
		break;
	}
	evs->connect = this;
	evs->instance = this->instance;
}

/*�ر�����
* ����������״̬����ΪCLOSE
*/
void px_connection::close_connect() {
	status = pxconn_status_type::CLOSE;
}

/*�ͷ�����
* ������״̬����ΪFREE
* ���ر����ӵ��ļ�������
*/
void px_connection::free_connect() {
	status = pxconn_status_type::FREE;
	if (fd)close(fd);
	fd = -1;
}

/*����timer�¼�
* �������������Ŀ��ʱ���timer�¼����и���
*/
void px_connection::update_timer() {
	px_event* ptr = time_evs;
	while (ptr) {
		ptr->reset_save_time();
		ptr = ptr->next;
	}
}

/*�ͷ����ӣ�
* Ϊ�ǳ�Ա������
* 1.���Ӳ�Ӧ���Լ��黹�Լ������������ͷź���
* 2.������ǷǱ�Ҫ�ģ�����������ò������ͷź���
* ���߳��������Ҫ����
* �ͷŲ��黹ʱ���¼�->ɾ��epollģ����ļ�������->�ͷ�����->�����ӶԹ黹�����
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