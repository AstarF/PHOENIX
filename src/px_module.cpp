#include "px_module.h"
#include "px_log.h"
#include "px_events.h"
#include "px_worker.h"
#include "px_locker.h"
#include "px_epoll.h"
#include "px_timer.h"
#include "px_resources.h"
#include "px_connections.h"
#include "px_thread_pool.h"


char PX_EVENT_BREAK = '0';//�˳�ģ����Ϊ

/*Consturctor
*/
px_module::px_module() :m_type(pxmodule_type::EVENT), top_level(false), priority(0),
process_list(nullptr), next(nullptr), event_list(nullptr), config(nullptr),
oneshot(false), e_type(px_event_type::EV_READ), updatable(true),
immediacy(false), execute_once(true), life_time(0),
next_chans(map<string, px_module*>()),
callback_func(nullptr) {}

/*set name
*/
void px_module::set_name(const string& str) {
    name = str;
}

/*���ģ��
* ����ģ������ȼ���ģ�������������Ӧ��λ��
*/
void px_module::add_module(px_module* pxmodule) {
    px_module* ptr = nullptr;
    px_module** pptr = nullptr;
    if (pxmodule->m_type == pxmodule_type::EVENT) {
        ptr = event_list;
        pptr = &event_list;
    }
    else {
        ptr = process_list;
        pptr = &process_list;
    }
    while (ptr && ptr->priority <= pxmodule->priority) {
        pptr = &(ptr->next);
        ptr = ptr->next;
    }
    pxmodule->next = ptr;
    *pptr = pxmodule;
}

/*DISPATHģ��ר��
* �������key-module�ļ�ֵ��
*/
void px_module::add_dispath(const string& key, px_module* pxmodule) {
    if (this->m_type == pxmodule_type::DISPATH) {
        this->next_chans[key] = pxmodule;
    }
}

/*ִ��ģ���е�һϵ����Ϊ
* ��ͬ��ģ���й�ͬ����ΪҲ�в�ͬ����Ϊ
* �������Ƕ�������Լ���process_list��ִ����ģ�����Ϊ
* Ȼ��ִ���Լ�����Ϊ
*/
void* px_module::module_process(px_event* evt, px_thread* thd, void* last_val) {
    //��������process_module
    px_module* ptr = this->process_list;
    while (ptr) {
        last_val = ptr->module_process(evt, thd, last_val);
        ptr = ptr->next;
    }
    //module����Ϊ
    switch (this->m_type)
    {
    case pxmodule_type::EVENT:
        event_process(evt);
        break;
    case pxmodule_type::PROCESS:
        if(callback_func)last_val = callback_func(evt, thd,this, last_val);
        break;
    case pxmodule_type::DISPATH:
        if (callback_func)last_val = callback_func(evt, thd,this, last_val);
        break;
    default:
        break;
    }
    return last_val;
}

/*װ�غ�����Eventģ��
* Eventģ�鲻���Զ�װ�أ�����Ҫ��ģ��Ļص��������ʹ��module_link���ܽ���ģ����¼�ģ�������е��¼�ģ��װ��
* ���ڷ���ȫ����Դ�����Ի�ʹ����������һ��ʼ�͵��ô˺�����֮���ٵ��ô˺���
* evt����ǰִ�е��¼�
* fd���²������ļ���������module_link��Ϊ�����һ��connection�����ҽ��¼�ģ��������¼���װ�ص���connection��
* addr��socket����fd��Ӧ�ĵ�ַ
*/
void px_module::module_link(px_event* evt, int fd, sockaddr* addr) {
    px_connection* conn = nullptr;
    if(!evt->immediacy)px_worker::get_instance()->thread_lock->lock();
    px_epoll* ep = px_worker::get_instance()->epoll_module;
    if (fd != -1) {
        conn = px_resources::get_instance()->conn_pool->pop_front();//new px_connection();//
        if (conn == nullptr) {
            px_log::get_log_instance().write("conn_pool insufficient cache.");
            close(fd);
            return;//��û����Դ��ʱ�����ڵĲ�����ֱ�Ӷ���
        }
        conn->init_connection();
        conn->fd = fd;
        conn->oneshot = this->oneshot;
        if (addr) {
            conn->address = *addr;
            conn->addrlen = sizeof(*addr);//threre is question
        }
        ep->px_add_conn(conn, get_attr_read_et(conn->oneshot ? EPOLLONESHOT : 0));
    }
    else conn = evt->connect;
    px_module* ptr = evt->evt_module->event_list;

    while (ptr)
    {
        event_create(ptr, conn);
        ptr = ptr->next;
    }
    if (!evt->immediacy)px_worker::get_instance()->thread_lock->unlock();
}

/*��������˵��ʼ��connection�ĸ����¼�������module_link
*/
void px_module::event_create(px_module* ptr, px_connection* conn) {
    if (conn) {
        px_epoll* ep = px_worker::get_instance()->epoll_module;
        switch (ptr->e_type)
        {
        case px_event_type::EV_WRITE:
            {
                px_event* conn_event = conn->write_evs;
                conn_event->priority = ptr->priority;
                conn_event->evt_module = ptr;
                conn_event->immediacy = ptr->immediacy;
                conn_event->updatable = ptr->updatable;
                //conn_event->execute_once = ptr->execute_once;//error  ����Ϊ��
                conn_event->event_type = px_event_type::EV_WRITE;
                break;
            }
        case px_event_type::EV_TIMEOUT:
            {
                px_event* conn_event = px_resources::get_instance()->event_pool->pop_front();
                if (conn_event == nullptr) {
                    px_log::get_log_instance().write("event_pool insufficient cache.");
                    return;
                }
                conn_event->init_pxevent();
                conn_event->priority = ptr->priority;
                conn_event->evt_module = ptr;
                conn_event->immediacy = ptr->immediacy;
                conn_event->updatable = ptr->updatable;
                conn_event->event_type = px_event_type::EV_TIMEOUT;
                conn_event->event_type = ptr->e_type;
                conn_event->save_time_slot = ptr->life_time;
                conn_event->execute_once = ptr->execute_once;
                conn_event->reset_save_time();
                px_resources::get_instance()->timer->push_event(conn_event);
                conn->add_events(conn_event);
                break;
            }
        default:
            {
                px_event* conn_event = conn->read_evs;
                conn_event->priority = ptr->priority;
                conn_event->evt_module = ptr;
                conn_event->immediacy = ptr->immediacy;
                conn_event->updatable = ptr->updatable;
                //conn_event->execute_once = ptr->execute_once;//error  ����Ϊ��
                ep->px_set_conn(conn, get_attr_read_et(conn->oneshot ? EPOLLONESHOT : 0));
                break;
            }
        }
    }
}

/*�¼�ִ��������β����
* ���ִ���ʽ��1.����ִ�� 2.������������Դ
* ���õ���
*/
void px_module::event_process(void* arg) {
    px_event* evt = (px_event*)arg;
    px_connection* conn = evt->connect;
    if (!evt->immediacy)px_worker::get_instance()->thread_lock->lock();
    //��������Ѿ��Ͽ����������Դ
    if (conn->status != pxconn_status_type::CONNECT) {
        conn_free_recovery(conn);
        if (!evt->immediacy)px_worker::get_instance()->thread_lock->unlock();
        return;
    }
    px_epoll* ep = px_worker::get_instance()->epoll_module;
    //�������ִ��
    switch (evt->event_type)
    {
    case px_event_type::EV_WRITE:
        if (evt->execute_once) {
            if(conn->wbuffer_used)ep->px_set_conn(conn, get_attr_write_et(conn->oneshot ? EPOLLONESHOT : 0));
            else ep->px_set_conn(conn, get_attr_read_et(conn->oneshot ? EPOLLONESHOT : 0));
        }
        break;
    case px_event_type::EV_TIMEOUT:
        if (evt->last) {
            evt->last->next = evt->next;
        }
        else conn->time_evs = evt->next;
        if (evt->next) {
            evt->next->last = evt->last;
        }
        px_resources::get_instance()->timer->del_event(evt);
        if (evt->execute_once) { 
            evt->free_event();
            px_resources::get_instance()->event_pool->push_front(evt);
        }
        else {
            evt->reset_save_time();
            px_resources::get_instance()->timer->push_event(evt);
            conn->add_events(evt);
        }
        break;
    default:
        if (evt->execute_once) {
            if (conn->wbuffer_used)ep->px_set_conn(conn, get_attr_write_et(conn->oneshot ? EPOLLONESHOT : 0));
            else ep->px_set_conn(conn, get_attr_read_et(conn->oneshot ? EPOLLONESHOT : 0));
        }
        break;
    }
    if (!evt->immediacy)px_worker::get_instance()->thread_lock->unlock();
}

/*���ݼ�ֵ��ִ����ص�ģ����Ϊ
*/
void px_module::module_dispath(const string& str, px_event* evt, px_thread* thd, void* last_val) {
    if (next_chans.count(str)) {
        next_chans[str]->module_process(evt, thd);
    }
    else if (next_chans.count("px_default"))next_chans["px_default"]->module_process(evt, thd, last_val);
}

//==Service Module==
/*Consturctor
*/
px_service_module::px_service_module() :process_num(1), thread_num(1), modules(nullptr) {}

void px_service_module::set_name(const string& str) {
    name = str;
}

/*��ӳ�ʼģ��
* ��ʼģ�飺��Щģ�����Ϊ���ڳ�ʼ��ʱִ��
* ��˳�ʼģ�������һ���Ǵ�����ص��ļ�������
*/
void px_service_module::add_init_module(px_module* pxmodule) {
    px_module* ptr = modules;
    px_module** pptr = &modules;
    while (ptr && ptr->priority <= pxmodule->priority) {
        pptr = &(ptr->next);
        ptr = ptr->next;
    }
    pxmodule->next = ptr;
    *pptr = pxmodule;
}

/*��ʼ��
*/
void px_service_module::init_modules() {
	px_module* ptr = modules;
	px_event temp;
	while (ptr) {
		temp.evt_module = ptr;
		if (ptr->top_level)ptr->module_process(&temp, nullptr);
		ptr = ptr->next;
	}
}