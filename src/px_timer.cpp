#include "px_timer.h"
#include "px_connections.h"
#include "px_events.h"
#include "px_worker.h"
#include "px_thread_pool.h"
#include "px_list.h"

//比较时间的超时时间
bool px_jump_table::compare(px_event* lptr, px_event* rptr) {
	return lptr->invilad_time < rptr->invilad_time;
}

//Constructor
px_jump_table::px_jump_table() :currnetblocks(0), active(nullptr) {}

//删除指定时间事件
void px_jump_table::del_element(px_event* val) {
	if (val->tlast) {
		val->tlast->tnext = val->tnext;
	}
	else active = active->tnext;
	if (val->tnext) {
		val->tnext->tlast = val->tlast;
	}
	val->tlast = nullptr;
	currnetblocks--;
}

/*插入时间事件
* 时间会按照超时时间排序
*/
void px_jump_table::push(px_event* node) {
	if (node) {
		if (active) {
			if (compare(active, node)) {
				node->tnext = active;
				active->tlast = node;
				active = node;
			}
			else {
				px_event* ptr = active;
				while (ptr->tnext) {
					if (compare(ptr->tnext, node))break;
					ptr = ptr->tnext;
				}
				node->tnext = ptr->tnext;
				node->tlast = ptr;
				ptr->tnext = node;
			}
		}
		else {
			node->tnext = active;
			active = node;
		}
		currnetblocks++;
	}
}

//返回最前面的时间事件
px_event* px_jump_table::top() {
	return active;
}

/*弹出最前面的时间事件
* 如果没有，则返回nullptr
*/
px_event* px_jump_table::pop_front() {
	if (active) {
		px_event* t = active;
		active = active->tnext;
		if (active)active->tlast = nullptr;
		currnetblocks--;
		t->tnext = t->tlast = nullptr;
		return t;
	}
	return nullptr;
}

//返回时间事件的数量
int px_jump_table::getcurrnetblocks() { return currnetblocks; }


/*==Timer==
* 是一个单例
* 返回单例
*/
px_timer* px_timer::get_instance() {
	static px_timer* instancee = new px_timer;
	return instancee;
}


/*处理超时事件
*/
void px_timer::tick(px_thread* pxthread) {
	px_event* res = nullptr;
	clock_t now_time = clock();
	while (timer_list.top() && timer_list.top()->invilad_time <= now_time) {
		timer_list.top()->invilad_time = -1;
		if (timer_list.top()->immediacy) {
			timer_list.top()->callback(pxthread);
		}
		else {
			timer_list.top()->last = nullptr;
			res = timer_list.top();
			pxthread->data->workqueue->push_back(res);
		}
		timer_list.pop_front();
	}
}

void px_timer::push_event(px_event* evt) {
	timer_list.push(evt);
}
void px_timer::del_event(px_event* evt) {
	if (evt->invilad_time == -1)return;
	evt->invilad_time = -1;
	timer_list.del_element(evt);
}
void px_timer::del_event(px_connection* conn) {
	px_event* ptr = conn->time_evs;
	while (ptr)
	{
		del_event(ptr);
		ptr = ptr->next;
	}
}

//返回时间事件的数量
int px_timer::gettotal_num() {
	return timer_list.getcurrnetblocks();
}