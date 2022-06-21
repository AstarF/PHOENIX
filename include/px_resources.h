#ifndef  PX_RESOURCES
#define PX_RESOURCES
#include "px_type.h"
#include "px_events.h"
#include "px_connections.h"
#include "px_list.h"

/*ȫ����Դ����
* ����һ�����Ӷ���غ�һ���¼������
* �¼�����ؽ�Ϊtimer�¼�����Ϊÿ������ֻ��һ�����¼���д�¼���Ȼ��ÿ���¼�������ж���ص�������
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

