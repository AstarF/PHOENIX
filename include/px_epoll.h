#ifndef PX_EPOLL
#define PX_EPOLL
#include<sys/socket.h>
#include<sys/types.h>
#include <sys/epoll.h>
#include<fcntl.h>
#include <errno.h> 
#include <string.h>
#include<arpa/inet.h>
#include<unistd.h>
#include "px_type.h"
/*
辅助函数，获取预定义好的配置
*/
constexpr int get_attr_read_et(const int attri = 0) {
	return EPOLLIN | EPOLLET | attri;
}

constexpr int get_attr_write_et(const int attri = 0) {
	return EPOLLOUT | EPOLLET | attri;
}
//==Statement==
#define MAX_EVENTS 100
#define PX_EPOLLET true

//==Epoll Module==
class px_epoll {
public:
	px_epoll():epollfd(-1),event_list(nullptr){}
	~px_epoll();
	int px_epoll_init(int connection_num = 500);
	int px_add_conn(px_connection* conn, int attri=0);
	int px_set_conn(px_connection* conn, int attri = 0);
	int px_del_conn(px_connection* conn);
	int px_process_event(px_thread* pxthread);
private:
	int epollfd;
	epoll_event *event_list;
};

#endif // !PX_EPOLL
