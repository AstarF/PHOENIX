#ifndef PX_CALLBACK_HUB
#define PX_CALLBACK_HUB
#include "px_type.h"
#include "px_server.h"
#include "px_http.h"

/*
* �ر�һ������
* ��Ϊ������ȫ����Դ�����Թر�ǰ�����
*/
void close_connection(px_connection* conn);
/*
* ����һ��soket�ļ�������
* ip�Ͷ˿ںŴ�service������
* �����һ�����ӣ�connection����������ģ��������Ӧ���¼�
*/
void* c_tcpsocket_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* last_return);

/*
* �Ӽ�����socket�׽����ļ��������Ͻ����µ�����
* ����ÿһ�������ӣ������һ�����ӣ�connection����������ģ��������Ӧ���¼�
*/
void* accept_tcpsocket_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);
/*
* ��socket�յ�����Ϣ�������û�connection������
* ÿ�ν��ջ��������пɸ��µļ�ʱ��
* ���������������expansionbuffer��������
* ֻҪ���յ��ֽھͷ��ؽ��ܵ����ֽ��������򷵻�PX_EVENT_BREAK
*/
void* read_tcpsocket_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);

/*
* ���û�connection���������͸�socket
* ÿ�ν��ջ��������пɸ��µļ�ʱ��
* �������ֽھͷ��ؽ��ܵ����ֽ��������򷵻�PX_EVENT_BREAK
*/
void* write_tcpsocket_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);

/*�����Ƶ���px_http_server
* ���û�connection���������͸�socket
* ÿ�ν��ջ��������пɸ��µļ�ʱ��
* �������ֽھͷ��ؽ��ܵ����ֽ��������򷵻�PX_EVENT_BREAK
*/
//void* http_writev_tcpsocket_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);

/*
* �Ա�׼���봴������
* �����һ�����ӣ�connection����������ģ��������Ӧ���¼�
*/
void* c_input_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);

/*
* ���ļ��յ�����Ϣ�������û�connection������
* ÿ�ν��ջ��������пɸ��µļ�ʱ��
* ���������������expansionbuffer��������
* ֻҪ���յ��ֽھͷ��ؽ��ܵ����ֽ��������򷵻�PX_EVENT_BREAK
*/
void* read_file_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);

/*
* �Խ����źŽ��չܵ���������
* �����һ�����ӣ�connection����������ģ��������Ӧ���¼�
*/
void* c_signal_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);

/*
* ��ʱ�ر������¼�
*/
void* timeout_connclose_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);

/*
* ���������������¼�
*/
void* deal_input_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);

/*
* �����ź��¼�
*/
void* read_signal_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg);
#endif // !PX_CALLBACK_HUB
