//废弃，仅用来参考

//#ifndef PX_SERVER
//#define PX_SERVER
//#include "px_type.h"
//#include <string>
//using std::string;
///*==px_server==
//* 面向soeket应用的对内部模块的封装
//* 可用作参考，也可直接使用
//*/
//class px_server {
//public:
//    typedef void*(*call_back_type)(px_event*, px_thread*,px_module*, void*);
//    px_server() {}
//    ~px_server() {}
//    int initsocket(int port = 55398);
//    void create_service();
//    px_module* create_process(const string& name, call_back_type, int priority=0);
//    px_module* create_time_event(const string& name, call_back_type, clock_t life_time, int priority=0, bool immediacy = false, bool execute_once = true);
//    px_module* create_dispath(const string& name, call_back_type, int priority=0);
//    void run_service();
//public:
//    px_service_module* serv_module;//service模块
//    px_module* c_socketfd;//one:用于创建最初的接收信息的文件描述符的模块
//    px_module* accept_socketfd;//two:从one的文件描述符上建立新的连接
//    px_module* read_socketfd;//从two得到的连接的读事件模块
//    px_module* write_socketfd;//从two得到的连接的写事件模块
//};
//
//#endif // !PX_SERVER
