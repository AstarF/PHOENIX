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
extern char PX_EVENT_BREAK;//�����¼�ִ�����̿��� //Used for event execution process control

//==Predefined Constant==
//ģ������//Module type
enum class pxmodule_type {
	EVENT,//��epoll��������
	PROCESS,//��������
	DISPATH//��������-�����ж�
};

//== Module ==
/*
* ���ڶ���������еĿ��
* Framework for defining program operation
* ע�⣬��������ʱ��ģ���������Ϣ����ֻ���ģ�
* Note that when the program is running, all information of the module is read-only!
* ʹ�õ��Ķ�̬�ڴ棬�����Լ��ͷţ�
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
	bool top_level;//�Ƿ��������ʱֱ��ִ��//Whether to execute directly when the program starts
	int priority;
	void* config;//����һЩ������,��Ҫ�Լ�����ɾ��//Save some configuration items, you need to delete them yourself
	bool oneshot;
	pxmodule_type m_type;
	px_module* process_list;//processʱ����//called when process
	px_module* next;//��������ͬ�ȼ���ģ�飬ģ��ᰴ���Լ������ȼ�������Ӧ��λ��//Used to connect modules of the same level, the modules will be arranged in the corresponding position according to their own priority
	px_module* event_list;//linkʱ����//called when link
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
