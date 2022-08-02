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

/*�����ӣ�ʲô��������������
* Empty links, links that contain nothing
*/
extern px_connection nullconn;

/*http �������ݰ�װ�� ��������
*/
enum class px_http_response_type {
    TEXT,
    JSON,
    HTML,
    ERROR
};

/*http�������ݰ�װ��
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

/*http����ģ�������װ��
*/
struct m_response_config {
    interface_type_json func;
    string method;
};

/*httpģ��ʹ�õ�ȫ����Դ
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

/*httpģ��ʹ�õ�ģ���װ��
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

/*�����ڲ�ģ��ʵ��http���ܵ�ģ��
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
    px_service_module* serv_module;//serviceģ��
    px_module* interface_error_module;//errorģ��
    px_module* time_module;//ʱ���¼�ģ��
    px_http_module* interface_module;
private:
    px_module* create_socketfd_module;//one:���ڴ�������Ľ�����Ϣ���ļ���������ģ��
    px_module* accept_module;//two:��one���ļ��������Ͻ����µ�����
    px_module* read_module;//��two�õ������ӵĶ��¼�ģ��
    px_module* write_module;//��two�õ������ӵ�д�¼�ģ��
    px_module* http_timeout_module;//
    px_module* http_request_module;//
    px_module* http_dispath_module;//
    px_module* http_file_response_module;//
    px_module* interface_dispath_module;//
    vector<px_http_module*> http_modules;
};


/*���������-��������session id
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

unsigned long long generate_sessionid(unsigned long ip);//����cookie

/*�ص������б�
*/
//void* http_read_tcpsocket_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);//��ȡ�·����������ݱ�
void* http_writev_tcpsocket_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);//���û�connection���������͸�socket
void* http_request_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);//������������http_request
void* http_dispathtest_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);//����������������벻ͬ��ģ��
void* http_interface_dispathtest_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);//����urlת��request����
void* http_file_response_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);//�����ļ�����response
void* http_response_json_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);//json��response����
void* http_error_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);//������
void* c_timemodule_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* last_return);
#endif // !PX_HTTP_SERVER
