#ifndef PX_MODULE
#define PX_MODULE
#include "px_type.h"
#include <string.h>
#include <time.h>
#include <map>
#include <string>
#include <vector>
#include <sys/socket.h>
using std::map;
using std::string;
using std::vector;
//==Statement==
class px_module;
extern char PX_EVENT_BREAK;//用于事件执行流程控制 //Used for event execution process control

//==Predefined Constant==
//模块类型//Module type
enum class pxmodule_type {
	EVENT,//由epoll被动调用
	PROCESS,//主动调用
	DISPATH//主动调用-条件判断
};

//== Module ==
/*
* 用于定义程序运行的框架
* Framework for defining program operation
* 注意，程序运行时，模块的所有信息都是只读的！
* Note that when the program is running, all information of the module is read-only!
* 使用到的动态内存，必须自己释放！
* The dynamic memory used must be released by itself!
*/
class px_module {
public:
	px_module();
	void set_name(const string& str);
	void add_module(px_module* pxmodule);
	void add_dispath(const string& key, px_module* pxmodule);
	void* module_process(px_event* evt, px_thread* thd,void* last_val = nullptr);
	void module_link(px_event* evt,int fd = -1, sockaddr* addr = nullptr);
	void module_dispath(const string& str, px_event* evt, px_thread* thd, void* last_val = nullptr);
public:
	//COMMON
	string name;
	bool top_level;//是否程序启动时直接执行//Whether to execute directly when the program starts
	int priority;
	void* config;//保存一些配置项,需要自己进行删除//Save some configuration items, you need to delete them yourself
	bool oneshot;
	pxmodule_type m_type;
	px_module* process_list;//process时调用//called when process
	px_module* next;//用于连接同等级的模块，模块会按照自己的优先级排在相应的位置//Used to connect modules of the same level, the modules will be arranged in the corresponding position according to their own priority
	px_module* event_list;//link时调用//called when link
	//EVENT for px_event
	px_event_type e_type;
	bool immediacy;
	bool execute_once;
	bool updatable;
	clock_t life_time;//for time event
	//DISPATH
	map<string, px_module*> next_chans;
	//PROCESS
	void* (*callback_func)(px_event* evt, px_thread* thd,px_module* pxmodule, void* arg);
private:
	void event_create(px_module* ptr, px_connection* conn);
	void event_process(void* arg);
};

//==Service Module==
class px_service_module {
public:
	px_service_module();
	void set_name(const string& str);
	void add_init_module(px_module* pxmodule);
	void init_modules();
public:
	string name;
	px_uint process_num;//now only one,because nultiprocessing is not supported
	px_uint thread_num;
	px_module* modules;
	char listened_ip[17];
	int listened_port;
	string wwwpath;
};

#endif // !PX_MODULE
