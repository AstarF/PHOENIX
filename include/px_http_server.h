#ifndef PX_HTTP_SERVER
#define PX_HTTP_SERVER
#include "px_type.h"
#include <string>
#include <vector>
#include <random>
#include "px_list.h"
#include "px_signal.h"
#include "px_mysql.h"
#include "px_connections.h"
using std::string;
using std::vector;
using std::default_random_engine;
using std::uniform_int_distribution;

/*空链接，什么都不包含的链接
* Empty links, links that contain nothing
*/
extern px_connection nullconn;

/*http 返回内容包装类 返回类型
*/
enum class px_http_response_type {
    TEXT,
    JSON,
    HTML,
    ERROR
};

/*http返回内容包装类
*/
class px_http_response_data {
public:
    px_http_response_data(const string& data = "", const px_http_response_type& type = px_http_response_type::TEXT);
    void operator()(const string& data, const px_http_response_type& type);
public:
    string data;
    string headval;
    bool creat_session;
    px_http_response_type type;
};
typedef void (*interface_type_json)(px_mysql_connect*, px_http_request*, px_http_response_data*);

/*http返回模块参数包装类
*/
struct m_response_config {
    interface_type_json func;
    string method;
};

/*http模块使用的全局资源
*/
class px_http_resources {
public:
    static px_http_resources* get_instance();
    ~px_http_resources() {
        delete mysqlconn_pool;
        delete mysql_signal;
    }
public:
    px_http_resources();
    px_objpool<px_mysql_connect>* mysqlconn_pool;
    px_signal* mysql_signal;
};

/*http模块使用的模块封装类
*/
class px_http_module {
public:
    ~px_http_module();
    void add_interface(const string& name, interface_type_json func, const string& method);
    void add_module(px_http_module* mod);
    string name;
    px_module* mod;
    px_http_server* serv;
private:
    vector<px_module*> interfaces;
};

/*利用内部模块实现http功能的模块
*/
class px_http_server {
public:
    typedef void* (*call_back_type)(px_event*, px_thread*, px_module*, void*);

    px_http_server();
    ~px_http_server();
    int initsocket(int port = 55398,const char* path="../www/");
    void create_service();
    px_module* create_process(const string& name, call_back_type, int priority = 0);
    px_module* create_time_event(const string& name, call_back_type, clock_t life_time, int priority = 0, bool immediacy = false, bool execute_once = true);
    px_module* create_dispath(const string& name, call_back_type, int priority = 0);
    px_http_module* create_module(const string& name);
    px_module* get_readmodule();
    px_module* get_writemodule();
    px_module* get_acceptmodule();
    void run_service();
public:
    px_service_module* serv_module;//service模块
    px_module* interface_error_module;//error模块
    px_module* time_module;//时间事件模块
    px_http_module* interface_module;
private:
    px_module* create_socketfd_module;//one:用于创建最初的接收信息的文件描述符的模块
    px_module* accept_module;//two:从one的文件描述符上建立新的连接
    px_module* read_module;//从two得到的连接的读事件模块
    px_module* write_module;//从two得到的连接的写事件模块
    px_module* http_timeout_module;//
    px_module* http_request_module;//
    px_module* http_dispath_module;//
    px_module* http_file_response_module;//
    px_module* interface_dispath_module;//
    vector<px_http_module*> http_modules;
};


/*随机数单例-用于生成session id
*/
class px_random_object {
public:
    static px_random_object& getinstance();
    int getrandom();
private:
    px_random_object();
    default_random_engine e;//
    uniform_int_distribution<int> u;
};

unsigned long long generate_sessionid(unsigned long ip);//生成cookie

/*回调函数列表
*/
//void* http_read_tcpsocket_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);//读取新发送来的数据报
void* http_writev_tcpsocket_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);//将用户connection缓冲区发送给socket
void* http_request_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);//根据请求生成http_request
void* http_dispathtest_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);//根据请求的种类送入不同的模块
void* http_interface_dispathtest_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);//根据url转发request请求
void* http_file_response_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);//生成文件类型response
void* http_response_json_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);//json类response函数
void* http_error_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);//错误处理
void* c_timemodule_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* last_return);
#endif // !PX_HTTP_SERVER
