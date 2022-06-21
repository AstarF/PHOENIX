#include "px_events.h"
#include "px_module.h"
#include "px_thread_pool.h"

/*Constructor
*/
px_event::px_event() :
	connect(nullptr), priority(0), instance(0), immediacy(false),
	event_type(px_event_type::EV_READ), event_status(px_event_status::FREE), after_callback(px_after_callback::DO_NOTHING),
	data(nullptr), updatable(true),
	tlast(nullptr), tnext(nullptr), invilad_time(-1),create_time(0),save_time_slot(0), execute_once(true),
	last(nullptr),next(nullptr) {}

/*ִ���¼��Ļص�����
*/
void px_event::callback(px_thread* pxthread) {
	if(this->evt_module)this->evt_module->module_process(this, pxthread);
}

/*�ر��¼�
* ���¼�״̬����ΪCLOSE
*/
void px_event::close_event() {
	this->event_status = px_event_status::CLOSE;
}

/*�ͷ��¼�
* ���¼�״̬����ΪFREE
*/
void px_event::free_event() {
	this->event_status = px_event_status::FREE;
}

/*����ʱ��ĳ�ʱʱ��
*/
void px_event::reset_save_time() {
	if(updatable)this->invilad_time = clock() + save_time_slot;
}

/*
==init_pxevent==
ʹ���¼�����ǰ�������
-----------------------------
used to init or reinit event
must be called before using
-----------------------------
default value is:
connect:nullptr
priority:0
instance:0
immediacy:false
event_type:EV_READ
after_callback:DO_NOTHING
next:nullptr
*/
void px_event::init_pxevent() {
	this->connect = nullptr;
	this->priority = 0;
	this->instance = 0;
	this->immediacy = false;
	this->event_status = px_event_status::ACTIVE;
	this->event_type = px_event_type::EV_READ;
	this->after_callback = px_after_callback::DO_NOTHING;
	this->next = nullptr;
	this->last = nullptr;
	this->evt_module = nullptr;
	this->create_time = clock();
	this->save_time_slot = 5000;
	this->invilad_time = -1;
	this->tlast = nullptr;
	this->tnext = nullptr;
	this->updatable = true;
	this->execute_once = true;
	this->data.reset();
}