//�������������ο�

//#ifndef PX_SERVER
//#define PX_SERVER
//#include "px_type.h"
//#include <string>
//using std::string;
///*==px_server==
//* ����soeketӦ�õĶ��ڲ�ģ��ķ�װ
//* �������ο���Ҳ��ֱ��ʹ��
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
//    px_service_module* serv_module;//serviceģ��
//    px_module* c_socketfd;//one:���ڴ�������Ľ�����Ϣ���ļ���������ģ��
//    px_module* accept_socketfd;//two:��one���ļ��������Ͻ����µ�����
//    px_module* read_socketfd;//��two�õ������ӵĶ��¼�ģ��
//    px_module* write_socketfd;//��two�õ������ӵ�д�¼�ģ��
//};
//
//#endif // !PX_SERVER
