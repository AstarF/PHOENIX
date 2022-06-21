#include "px_resources.h"
#include "px_timer.h"
/*获取单例
*/
px_resources* px_resources::get_instance() {
	static px_resources* instance = new px_resources;
	return instance;
}

/*Constructor
*/
px_resources::px_resources() :
	conn_pool(new px_objpool<px_connection>(1000)), event_pool(new px_objpool<px_event>(2000)),
	timer(nullptr){
	timer = px_timer::get_instance();
}

/*打印当前全局资源的使用状态
*/
void dispaly_status() {
	auto conn_pool = px_resources::get_instance()->conn_pool;
	auto evt_pool = px_resources::get_instance()->event_pool;
	printf("conn pool: %d / %d \n", conn_pool->getfreeblocks(), conn_pool->getotalblocks());
	printf("event pool: %d / %d \n", evt_pool->getfreeblocks(), evt_pool->getotalblocks());
	printf("timer: %d \n", px_resources::get_instance()->timer->gettotal_num());
}