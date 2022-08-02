#ifndef PX_CONNECTION
#define PX_CONNECTION
#include "px_type.h"
#include<arpa/inet.h>
//==Statement==
class px_event;
extern const int px_deuault_bufsize;//Ĭ�϶�дbuffer��С
extern const int px_max_bufsize;//����дbuffer��С
/*==Predefined Constant==
*/
//��ǰ���ӵ�״̬
enum class pxconn_status_type { 
	FREE, //���п���
	CONNECT, //����ʹ��
	CLOSE //���У�������
};

const unsigned int MAX_INSTANCE = ~(1 << 10);

/*==Connection== :manage connection
* ����==�û�
*/
class px_connection {
public:
	px_connection();
	~px_connection();
	void init_connection();
	void add_instance();
	void add_events(px_event* evs);
	void init_events(px_event* evs);
	void close_connect();
	void free_connect();
	int del_events(px_event* evs);
	void update_timer();
	void resizebuffer(size_t bufsize);
	void clearbuffer();
	void resizewbuffer(size_t bufsize);
	void clearwbuffer();
	void expansionbuffer();
	void expansionwbuffer(size_t bufsize);
public:
	//core
	px_event* read_evs;//read events
	px_event* write_evs;//write events
	int fd;//�ļ�������
	bool oneshot;
	sockaddr address;//client address
	socklen_t addrlen;//address length
	pxconn_status_type status;//connect status
	int instance;//��δ�����ó��������ڵķ�����˵��д�ͳ�ʱ�¼���һ���ջ����������¼���Ҫִ��ʱ���ӱ��жϲ����µ��������õ����
	//read buffer
	char* buffer;
	size_t buffer_used;
	//size_t buffer_read;
	size_t buffer_size;
	//write buffer
	char* wbuffer;
	size_t wbuffer_used;
	size_t wbuffer_read;
	size_t wbuffer_size;
	//timer
	px_event* time_evs;//time events
};

void conn_free_recovery(px_connection* conn);
#endif // !PX_CONNECTION


