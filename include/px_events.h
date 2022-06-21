
#ifndef PX_EVENTS
#define PX_EVENTS
#include "px_type.h"
#include<time.h>
#include <memory>
using std::shared_ptr;

//==Predefined Constant==
/*事件状态
*/
enum class px_event_status {
	ACTIVE,
	FREE,
	CLOSE
};

/*事件调用后发生了什么
* 暂时没有用到
*/
enum class px_after_callback {
	DO_NOTHING,
	REMOVE_EVENT,
	ADD_EVENT,
	CLOSE_CONNECT
};

/* 事件类型
* 决定了事件被调用的途径
*/
enum class px_event_type {
	EV_READ,
	EV_WRITE,
	EV_TIMEOUT,
	EV_SIGNAL
};

//==Event== :this is just function handler
/* 事件：读、写、时间事件
*/
class px_event {
public:
	px_event();
	~px_event() { init_pxevent(); }
	void init_pxevent();
	void callback(px_thread* pxthread = nullptr);
	void close_event();
	void free_event();
	void reset_save_time();
public:
	//base
	px_connection* connect;//事件对应的连接
	int priority;//事件的优先级，影响同一类事件执行顺序
	px_uint instance;//暂未排上用场，就现在的服务来说读写和超时事件是一个闭环，不存在事件刚要执行时连接被中断并被新的连接重用的情况
	//是否立刻执行，为true时在epoll返回后就立刻执行，为false时，在epoll返回后被放入当前线程的延后队列，之后由线程自己执行
	bool immediacy;
	bool execute_once;//事件是否只执行一次
	px_event_status event_status;//同上面px_event_status介绍
	px_event_type event_type;//同上面px_event_type介绍
	px_after_callback after_callback;//同上面px_after_callback介绍
	px_module* evt_module;//支持事件执行的静态模块，包含回调函数和相关静态参数
	shared_ptr<void> data;//存储事件执行时用到的临时数据
	//timer
	clock_t create_time;//the time of create
	clock_t save_time_slot;//超时时间的存活时间
	clock_t invilad_time;//the time of invalid
	bool updatable;//计时器是否可以被更新
	px_event* tlast;//timer中时间事件靠此指针连接成链表
	px_event* tnext;//timer中时间事件靠此指针连接成链表
	px_event* next;//for connection：连接的time事件靠此指针连接成链表
	px_event* last;//for connection：连接的time事件靠此指针连接成链表
};

#endif // !PX_EVENTS


