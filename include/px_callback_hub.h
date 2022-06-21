#ifndef PX_CALLBACK_HUB
#define PX_CALLBACK_HUB
#include "px_type.h"
#include "px_server.h"
#include "px_http.h"

/*
* 关闭一个连接
* 因为连接是全局资源，所以关闭前会加锁
*/
void close_connection(px_connection* conn);
/*
* 创建一个soket文件描述符
* ip和端口号从service里配置
* 会产生一个连接（connection），并根据模块配置相应的事件
*/
void* c_tcpsocket_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* last_return);

/*
* 从监听的socket套接字文件描述符上接受新的连接
* 对于每一个新连接：会产生一个连接（connection），并根据模块配置相应的事件
*/
void* accept_tcpsocket_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);
/*
* 将socket收到的信息拷贝的用户connection缓冲区
* 每次接收会重置所有可更新的计时器
* 缓冲区不够会调用expansionbuffer进行扩容
* 只要接收到字节就返回接受的总字节数，否则返回PX_EVENT_BREAK
*/
void* read_tcpsocket_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);

/*
* 将用户connection缓冲区发送给socket
* 每次接收会重置所有可更新的计时器
* 发送了字节就返回接受的总字节数，否则返回PX_EVENT_BREAK
*/
void* write_tcpsocket_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);

/*定义移到了px_http_server
* 将用户connection缓冲区发送给socket
* 每次接收会重置所有可更新的计时器
* 发送了字节就返回接受的总字节数，否则返回PX_EVENT_BREAK
*/
//void* http_writev_tcpsocket_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);

/*
* 对标准输入创建连接
* 会产生一个连接（connection），并根据模块配置相应的事件
*/
void* c_input_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);

/*
* 将文件收到的信息拷贝的用户connection缓冲区
* 每次接收会重置所有可更新的计时器
* 缓冲区不够会调用expansionbuffer进行扩容
* 只要接收到字节就返回接受的总字节数，否则返回PX_EVENT_BREAK
*/
void* read_file_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);

/*
* 对进程信号接收管道创建连接
* 会产生一个连接（connection），并根据模块配置相应的事件
*/
void* c_signal_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);

/*
* 超时关闭连接事件
*/
void* timeout_connclose_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);

/*
* 处理命令行输入事件
*/
void* deal_input_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);

/*
* 处理信号事件
*/
void* read_signal_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);
#endif // !PX_CALLBACK_HUB
