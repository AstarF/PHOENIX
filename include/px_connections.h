#ifndef PX_CONNECTION
#define PX_CONNECTION
#include "px_type.h"
#include<arpa/inet.h>
//==Statement==
class px_event;
extern const int px_deuault_bufsize;//默认读写buffer大小
extern const int px_max_bufsize;//最大读写buffer大小
/*==Predefined Constant==
*/
//当前连接的状态
enum class pxconn_status_type { 
	FREE, //空闲可用
	CONNECT, //正在使用
	CLOSE //空闲，不可用
};

const unsigned int MAX_INSTANCE = ~(1 << 10);

/*==Connection== :manage connection
* 连接==用户
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
	int fd;//文件描述符
	bool oneshot;
	sockaddr address;//client address
	socklen_t addrlen;//address length
	pxconn_status_type status;//connect status
	int instance;//暂未排上用场，就现在的服务来说读写和超时事件是一个闭环，不存在事件刚要执行时连接被中断并被新的连接重用的情况
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


