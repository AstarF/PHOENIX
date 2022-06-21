#ifndef PX_TIMER
#define PX_TIMER
#include "px_type.h"
#include<time.h>
//尚未实现的跳表
//目前仅仅是一个简单的链表
class px_jump_table {
public:
	px_jump_table();
	void push(px_event* node);
	px_event* pop_front();
	void del_element(px_event* val);
	int getcurrnetblocks();
	bool compare(px_event* lptr, px_event* rptr);
	px_event* top();
private:
	px_event* active;
	int currnetblocks;
};

class px_timer {
public:
	static px_timer* get_instance();
	void tick(px_thread* pxthread);
	void push_event(px_event* evt);
	void del_event(px_event* evt);
	void del_event(px_connection* conn);
	int gettotal_num();
private:
	px_timer() {}
	px_jump_table timer_list;
};
#endif // !PX_TIMER
