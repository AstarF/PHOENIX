#include "px_type.h"
#include "px_log.h"
#include "px_worker.h"
#include "px_module.h"
#include "px_connections.h"
#include "px_events.h"
#include "px_locker.h"
#include "px_epoll.h"
#include "px_error.h"
#include "px_resources.h"
#include "px_server.h"
#include "px_timer.h"
#include "px_thread_pool.h"
#include "px_list.h"
#include "px_util.h"
#include "px_http.h"
#include <sys/mman.h>

/*����һЩͨ�õĻص�����
*/

/*
* �ر�һ������
* ��Ϊ������ȫ����Դ�����Թر�ǰ�����
*/
void close_connection(px_connection* conn) {
    if (conn->status == pxconn_status_type::CONNECT) {
        conn->close_connect();
    }
}

/*
* ����һ��soket�ļ�������
* ip�Ͷ˿ںŴ�service������
* �����һ�����ӣ�connection����������ģ��������Ӧ���¼�
*/
void* c_tcpsocket_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* last_return) {
    int fd = -1;
    px_service_module* serv = px_worker::get_instance()->serv_module;
    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        px_log::get_log_instance().write("socket() failed.");
    }
    set_file(fd, O_NONBLOCK);
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, serv->listened_ip, &(addr.sin_addr.s_addr));
    addr.sin_port = htons(serv->listened_port);

    int option = 1;
    socklen_t optlen = sizeof(option);
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void*)&option, optlen)) {
        px_log::get_log_instance().write("setsockopt(SO_REUSEADDR) failed.");
    }

    if (bind(fd, (sockaddr*)&(addr), sizeof(addr)) == -1) {
        px_log::get_log_instance().write("bind() failed.");
    }
    if (listen(fd, 30) == -1) {
        px_log::get_log_instance().write("listen() failed.");
    }
    evt->evt_module->module_link(evt, fd, (sockaddr*)&addr);
}

/*
* �Ӽ�����socket�׽����ļ��������Ͻ����µ�����
* ����ÿһ�������ӣ������һ�����ӣ�connection����������ģ��������Ӧ���¼�
*/
void* accept_tcpsocket_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg) {
    while (true)//ÿ�ξ����ܽ��ܶ������
    {
        int fd = -1;
        sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        socklen_t addr_len = sizeof(addr);
        fd = accept(evt->connect->fd, (sockaddr*)&addr, &addr_len);
        if (fd < 0) {
            //--px_log::get_log_instance().write("no more connect.");
            break;
        }
        set_file(fd, O_NONBLOCK);
        //printf("new connect socketfd |: theadid: %d accept : %d ip_digit: %u ip: %s port: %d\n", pxthread->thread_index, evt->connect->fd, ntohl(addr.sin_addr.s_addr), inet_ntoa((addr.sin_addr)), ntohs(addr.sin_port));
        evt->evt_module->module_link(evt, fd, (sockaddr*)&addr);
    }
}

/*
* �Ӽ�����socket�׽����ļ��������Ͻ����µ�����
* ����ÿһ�������ӣ������һ�����ӣ�connection����������ģ��������Ӧ���¼�
*/
//void* accept_tcpsocket_func_multi_reactors(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg) {
//    while (true)//ÿ�ξ����ܽ��ܶ������
//    {
//        int fd = -1;
//        sockaddr_in addr;
//        memset(&addr, 0, sizeof(addr));
//        addr.sin_family = AF_INET;
//        socklen_t addr_len = sizeof(addr);
//        fd = accept(evt->connect->fd, (sockaddr*)&addr, &addr_len);
//        if (fd < 0) {
//            //--px_log::get_log_instance().write("no more connect.");
//            break;
//        }
//        set_file(fd, O_NONBLOCK);
//        //printf("new connect socketfd |: theadid: %d accept : %d ip_digit: %u ip: %s port: %d\n", pxthread->thread_index, evt->connect->fd, ntohl(addr.sin_addr.s_addr), inet_ntoa((addr.sin_addr)), ntohs(addr.sin_port
//        evt->evt_module->module_link(evt, fd, (sockaddr*)&addr);
//    }
//}

/*
* ��socket�յ�����Ϣ�������û�connection������
* ÿ�ν��ջ��������пɸ��µļ�ʱ��
* ���������������expansionbuffer��������
* ֻҪ���յ��ֽھͷ��ؽ��ܵ����ֽ��������򷵻�PX_EVENT_BREAK
*/
void* read_tcpsocket_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg) {
    int recvtotal = 0;
    if ((evt->connect->status == pxconn_status_type::CONNECT) && (evt->event_type == px_event_type::EV_READ)) {
        char* buffer = evt->connect->buffer;
        size_t& buffer_used = evt->connect->buffer_used;
        size_t buffer_size = evt->connect->buffer_size;
        while (true) {
            int ret = recv(evt->connect->fd, buffer + buffer_used, buffer_size - buffer_used, 0);
            if (ret > 0) {
                buffer_used += ret;
                evt->connect->update_timer();
                //printf("theadid: %d | recv fd: %d size: %d  text:\n%s\n", pxthread->thread_index, evt->connect->fd, buffer_used, buffer);
                recvtotal += ret;
            }
            if (ret == 0) {//���recv�����ڵȴ�Э���������ʱ�����ж��ˣ���ô������0
                //printf("theadid: %d | recv fd: %d size: %d  text:\n%s --------------|\n", pxthread->thread_index, evt->connect->fd, buffer_used, buffer);
                printf("when reading net interruption. \n");
                close_connection(evt->connect);
                return nullptr;
            }
            if (ret < 0) {//�������У�����δ���������
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    //printf("theadid: %d | recv fd: %d size: %d  text:\n%s --------------|\n", pxthread->thread_index, evt->connect->fd, buffer_used, buffer);
                    break;
                }
                printf("when reading net interruption. \n");
                printf("errno: %d\n", errno);
                close_connection(evt->connect);
                return nullptr;
            }
            //buffer�ռ�ľ������
            if (buffer_used >= buffer_size) {
                evt->connect->expansionbuffer();
                buffer = evt->connect->buffer;
                buffer_size = evt->connect->buffer_size;
                if (buffer_used >= buffer_size) {
                    printf("no more memory! %lu\n",buffer_size);
                    close_connection(evt->connect);
                    return &PX_EVENT_BREAK;
                }
            }
        }
    }
    return recvtotal > 0 ? nullptr : &PX_EVENT_BREAK;
}

/*
* ���û�connection���������͸�socket
* ÿ�ν��ջ��������пɸ��µļ�ʱ��
* �������ֽھͷ��ؽ��ܵ����ֽ��������򷵻�PX_EVENT_BREAK
*/
void* write_tcpsocket_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg) {
    int sendtotal = 0;
    if ((evt->connect->status == pxconn_status_type::CONNECT) && (evt->event_type == px_event_type::EV_WRITE)) {
        size_t& buffer_used = evt->connect->wbuffer_used;
        if (buffer_used == 0)return nullptr;
        size_t& buffer_read = evt->connect->wbuffer_read;
        char* buffer = evt->connect->wbuffer;
        int buffer_size = evt->connect->wbuffer_size;
        while (true) {
            int ret = send(evt->connect->fd, buffer + buffer_read, buffer_used - buffer_read, 0);
            if (ret > 0) {
                buffer_read += ret;
                sendtotal += ret;
                evt->connect->update_timer();
                //printf("theadid: %d | send fd: %d size: %d  text:\n%s\n", pxthread->thread_index, evt->connect->fd, ret, buffer + buffer_used);
            }
            if (ret == 0) {
                //--printf("when writing net interruption.\n");
                close_connection(evt->connect);
                break;
            }
            if (ret < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    evt->connect->clearwbuffer();
                    break;
                }
                //--printf("when writing net interruption.\n");
                close_connection(evt->connect);
                return nullptr;
            }
        }
    }
    return sendtotal > 0 ? nullptr : &PX_EVENT_BREAK;
}

/*�����Ƶ���px_http_server
* ���û�connection���������͸�socket
* ÿ�ν��ջ��������пɸ��µļ�ʱ��
* �������ֽھͷ��ؽ��ܵ����ֽ��������򷵻�PX_EVENT_BREAK
*/
//void* http_writev_tcpsocket_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg) {
//    int sendtotal = 0;
//    if ((evt->connect->status == pxconn_status_type::CONNECT) && (evt->event_type == px_event_type::EV_WRITE)) {
//        size_t buffer_used = evt->connect->wbuffer_used;
//        //printf("buffer_used : %ld\n", buffer_used);
//        if (buffer_used == 0)return nullptr;
//        px_http_response* response = (px_http_response*)evt->data.get();
//        px_http_request* req = (px_http_request*)evt->connect->read_evs->data.get();
//        size_t& read = response->read;
//        while (true) {
//            int ret = writev(evt->connect->fd, response->block, response->block_used);
//            if (ret > 0) {
//                sendtotal += ret;
//                read += ret;
//                //printf("-----buffer_used: %d , read: %d\n", buffer_used, read);
//                if (response->ismmap && read >= buffer_used) {
//                    response->block[0].iov_len = 0;
//                    response->block[1].iov_base = response->mmptr + (read - buffer_used);
//                    response->block[1].iov_len = response->mm_size - (read - buffer_used);
//                }
//                else {
//                    response->block[0].iov_base = evt->connect->wbuffer + read;
//                    response->block[0].iov_len = buffer_used - read;
//                }
//                evt->connect->update_timer();
//                //printf("theadid: %d | send fd: %d size: %d  text:\n%s\n", pxthread->thread_index, evt->connect->fd, ret, buffer + buffer_used);
//                if (response->block[1].iov_len == 0) {//������,����request��response
//                    evt->connect->clearwbuffer();
//                }
//            }
//            if (ret == 0) {
//                //--printf("when writing net interruption.\n");
//                //���ӶϿ���,����request��response
//                close_connection(evt->connect);
//                break;
//            }
//            if (ret < 0) {
//                if (errno == EAGAIN || errno == EWOULDBLOCK) {
//                    //printf("ret<9 and buffer used: %d and iov_len: %d\n", buffer_used, response->block[1].iov_len);
//                    break;
//                }
//                //����,����request��response
//                //printf("when writing net interruption.\n");
//                //printf("%d.\n", errno);
//                close_connection(evt->connect);
//                return nullptr;
//            }
//        }
//    }
//    return sendtotal > 0 ? nullptr : &PX_EVENT_BREAK;
//}

/*
* �Ա�׼���봴������
* �����һ�����ӣ�connection����������ģ��������Ӧ���¼�
*/
void* c_input_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg) {
    evt->evt_module->module_link(evt, STDIN_FILENO);
}

/*
* ���ļ��յ�����Ϣ�������û�connection������
* ÿ�ν��ջ��������пɸ��µļ�ʱ��
* ���������������expansionbuffer��������
* ֻҪ���յ��ֽھͷ��ؽ��ܵ����ֽ��������򷵻�PX_EVENT_BREAK
*/
void* read_file_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg) {
    int recvtotal = 0;
    if ((evt->connect->status == pxconn_status_type::CONNECT) && (evt->event_type == px_event_type::EV_READ)) {
        char* buffer = evt->connect->buffer;
        size_t& buffer_used = evt->connect->buffer_used;
        size_t buffer_size = evt->connect->buffer_size;
        while (true) {
            int ret = read(evt->connect->fd, buffer, sizeof(buffer) - 1);
            if (ret > 0) {
                buffer_used += ret;
                evt->connect->update_timer();
                printf("theadid: %d | recv fd: %d size: %d  text:\n%s\n", pxthread->thread_index, evt->connect->fd, ret, buffer);
                recvtotal += ret;
            }
            else return nullptr;
            if (buffer_used == buffer_size)evt->connect->expansionbuffer();
        }
    }
    return recvtotal > 0 ? nullptr : &PX_EVENT_BREAK;
}

/*
* �Խ����źŽ��չܵ���������
* �����һ�����ӣ�connection����������ģ��������Ӧ���¼�
*/
void* c_signal_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg) {
    //����Ҫ������ź�
    //px_worker::get_instance()->addsig(SIGHUP);//�����ն˹���
    //px_worker::get_instance()->addsig(SIGCHLD);//�ӽ���״̬�����仯
    //px_worker::get_instance()->addsig(SIGTERM);//��ֹ����killĬ������
    //px_worker::get_instance()->addsig(SIGINT);//Ctrl+C
    px_worker::get_instance()->addsig(SIGPIPE);
    evt->evt_module->module_link(evt, px_worker::get_instance()->signal_pipe[0]);
}

/*
* ��ʱ�ر������¼�
*/
void* timeout_connclose_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg) {
    printf("timeout,connection close. \n");
    close_connection(evt->connect);
    return nullptr;
}

/*
* ���������������¼�
*/
void* deal_input_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg) {
    char key;
    if (evt->event_type == px_event_type::EV_READ) {
        char buffer[1024];
        int ret = read(evt->connect->fd, buffer, sizeof(buffer) - 1);
        printf("recv input: %d  text: %s \n", ret, buffer);
        if (buffer[0] == 'q') {
            px_worker::get_instance()->terminate();
        }
        if (buffer[0] == 's') {
            px_worker::get_instance()->stop();
        }
        if (buffer[0] == 't') {
            dispaly_status();
        }
    }
    return nullptr;
}

/*
* �����ź��¼�
*/
void* read_signal_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg) {
    if (evt->event_type == px_event_type::EV_SIGNAL) {
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        int ret = recv(evt->connect->fd, buffer, sizeof(buffer) - 1, 0);
        printf("recv signal: %s\n", buffer);
    }
    return nullptr;
}
