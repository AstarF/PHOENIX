#ifndef  PX_RESOURCES
#define PX_RESOURCES
#include "px_type.h"
#include "px_events.h"
#include "px_connections.h"
#include "px_list.h"

/*全局资源管理
* 包括一个连接对象池和一个事件对象池
* 事件对象池仅为timer事件，因为每个连接只有一个读事件和写事件（然而每个事件里可以有多个回调方法）
*/
template <typename T>class px_objpool;
class px_resources {
public:
	static px_resources* get_instance();
	~px_resources() {
		delete conn_pool;
		delete event_pool;
	}
public:
	px_objpool<px_connection>* conn_pool;
	px_objpool<px_event>* event_pool;
	px_timer* timer;
private:
	px_resources();
};
void dispaly_status();
#endif // ! PX_RESOURCES

