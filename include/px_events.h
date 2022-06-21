
#ifndef PX_EVENTS
#define PX_EVENTS
#include "px_type.h"
#include<time.h>
#include <memory>
using std::shared_ptr;

//==Predefined Constant==
/*�¼�״̬
*/
enum class px_event_status {
	ACTIVE,
	FREE,
	CLOSE
};

/*�¼����ú�����ʲô
* ��ʱû���õ�
*/
enum class px_after_callback {
	DO_NOTHING,
	REMOVE_EVENT,
	ADD_EVENT,
	CLOSE_CONNECT
};

/* �¼�����
* �������¼������õ�;��
*/
enum class px_event_type {
	EV_READ,
	EV_WRITE,
	EV_TIMEOUT,
	EV_SIGNAL
};

//==Event== :this is just function handler
/* �¼�������д��ʱ���¼�
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
	px_connection* connect;//�¼���Ӧ������
	int priority;//�¼������ȼ���Ӱ��ͬһ���¼�ִ��˳��
	px_uint instance;//��δ�����ó��������ڵķ�����˵��д�ͳ�ʱ�¼���һ���ջ����������¼���Ҫִ��ʱ���ӱ��жϲ����µ��������õ����
	//�Ƿ�����ִ�У�Ϊtrueʱ��epoll���غ������ִ�У�Ϊfalseʱ����epoll���غ󱻷��뵱ǰ�̵߳��Ӻ���У�֮�����߳��Լ�ִ��
	bool immediacy;
	bool execute_once;//�¼��Ƿ�ִֻ��һ��
	px_event_status event_status;//ͬ����px_event_status����
	px_event_type event_type;//ͬ����px_event_type����
	px_after_callback after_callback;//ͬ����px_after_callback����
	px_module* evt_module;//֧���¼�ִ�еľ�̬ģ�飬�����ص���������ؾ�̬����
	shared_ptr<void> data;//�洢�¼�ִ��ʱ�õ�����ʱ����
	//timer
	clock_t create_time;//the time of create
	clock_t save_time_slot;//��ʱʱ��Ĵ��ʱ��
	clock_t invilad_time;//the time of invalid
	bool updatable;//��ʱ���Ƿ���Ա�����
	px_event* tlast;//timer��ʱ���¼�����ָ�����ӳ�����
	px_event* tnext;//timer��ʱ���¼�����ָ�����ӳ�����
	px_event* next;//for connection�����ӵ�time�¼�����ָ�����ӳ�����
	px_event* last;//for connection�����ӵ�time�¼�����ָ�����ӳ�����
};

#endif // !PX_EVENTS


