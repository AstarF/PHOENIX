#include "px_http_server.h"
#include "px_worker.h"
#include "px_module.h"
#include "px_connections.h"
#include "px_events.h"
#include "px_locker.h"
#include "px_epoll.h"
#include "px_error.h"
#include "px_resources.h"
#include "px_http_server.h"
#include "px_timer.h"
#include "px_thread_pool.h"
#include "px_list.h"
#include "px_util.h"
#include "px_http.h"
#include "px_mysql.h"
#include "px_log.h"
#include "px_signal.h"
#include "px_callback_hub.h"
#include <iostream>
#include <string.h>
#include <string>
#include <ctime>
#include <memory>
#include <unistd.h>
using std::make_shared;
using std::to_string;

/*�����ӣ�ʲô��������������
*/
px_connection nullconn = px_connection();

//==px_http_resources==

/*��ȡ����
*/
px_http_resources* px_http_resources::get_instance() {
    static px_http_resources* instance = new px_http_resources;
    return instance;
}

/*Constructor
*/
px_http_resources::px_http_resources() :
    mysqlconn_pool(new px_objpool<px_mysql_connect>(8)), mysql_signal(new px_signal(ftok("./phoenix", 128), 8)) {
    for (int i = 0; i < 8; ++i) {
        px_mysql_connect* t = mysqlconn_pool->pop_front();
        t->init("127.0.0.1", "root", "1234a234", "px_database", 3306);
        mysqlconn_pool->push_back(t);
    }
}

//==px_http_module==
/*Destructor
*/
px_http_module::~px_http_module() {
    for (auto delitem : interfaces) {
        delete delitem;
    }
    delete this->mod;
}

//==px_http_response_data==
/*Constructor
*/
px_http_response_data::px_http_response_data(const string& data, const px_http_response_type& type) :data(data), type(type), creat_session(false) {
    switch (type)
    {
    case px_http_response_type::HTML:
        this->headval = "text/html;charset=utf-8";
        break;
    case px_http_response_type::TEXT:
        this->headval = "text/plain";
        break;
    case px_http_response_type::JSON:
        this->headval = "application/json;charset=utf-8";
        break;
    default:
        break;
    }
}

void px_http_response_data::operator()(const string& data, const px_http_response_type& type) {
    this->data = data;
    this->type = type;
    switch (type)
    {
    case px_http_response_type::HTML:
        this->headval = "text/html;charset=utf-8";
        break;
    case px_http_response_type::TEXT:
        this->headval = "text/plain";
        break;
    case px_http_response_type::JSON:
        this->headval = "application/json;charset=utf-8";
        break;
    case  px_http_response_type::ERROR:
        break;
    default:
        break;
    }
}

/*���һ��http�ӿ�
*/
void px_http_module::add_interface(const string& name, interface_type_json func, const string& method) {
    if (this->mod->m_type == pxmodule_type::DISPATH) {
        m_response_config* config = new m_response_config;
        config->func = func;
        config->method = method;
        px_module* imagelist_response_process = serv->create_process(name + "_process", http_response_json_func);
        imagelist_response_process->config = (void*)config;
        this->mod->add_dispath(name, imagelist_response_process);
        interfaces.push_back(imagelist_response_process);
    }
    else {
        printf("add_interface error.it is not a px_http_module.");
        abort();
    }
}

/*���һ��httpģ��
*/
void px_http_module::add_module(px_http_module* mod) {
    if (this->mod->m_type == pxmodule_type::DISPATH) {
        this->mod->add_dispath(mod->name, mod->mod);
    }
    else {
        printf("add_module error.it is not a px_http_module.");
        abort();
    }
}

//==px_http_server==
/*Constructor
*/
px_http_server::px_http_server() :
    serv_module(nullptr), interface_module(nullptr), create_socketfd_module(nullptr),
    accept_module(nullptr), read_module(nullptr), write_module(nullptr),
    http_timeout_module(nullptr), http_request_module(nullptr), http_dispath_module(nullptr),
    http_file_response_module(nullptr), interface_dispath_module(nullptr) {}

/*Destructor
*/
px_http_server::~px_http_server() {
    delete serv_module;
    delete interface_module;
    delete create_socketfd_module;
    delete accept_module;
    delete read_module;
    delete write_module;
    delete http_timeout_module;
    delete http_request_module;
    delete http_dispath_module;
    delete http_file_response_module;
    delete interface_error_module;
    //delete interface_dispath_module;
    for (auto delitem : http_modules) {
        delete delitem;
    }
}

/*����������һ��processģ��
*/
px_module* px_http_server::create_process(const string& name, call_back_type callback, int priority) {
    px_module* http_request_process = new px_module;
    http_request_process->set_name(name);
    http_request_process->top_level = false;
    http_request_process->m_type = pxmodule_type::PROCESS;
    http_request_process->callback_func = callback;
    http_request_process->priority = priority;
    return http_request_process;
}

/*����������һ��time�¼�ģ��
*/
px_module* px_http_server::create_time_event(const string& name, call_back_type callback, clock_t life_time, int priority, bool immediacy, bool execute_once) {
    px_module* deal_timeout = new px_module;
    deal_timeout->top_level = false;
    deal_timeout->set_name(name + " :tcp timeout");
    deal_timeout->m_type = pxmodule_type::EVENT;
    deal_timeout->e_type = px_event_type::EV_TIMEOUT;
    deal_timeout->immediacy = immediacy;
    deal_timeout->execute_once = execute_once;
    deal_timeout->priority = priority;
    deal_timeout->life_time = life_time;

    px_module* deal_timeout_process = new px_module;
    deal_timeout_process->set_name(name + " :tcp timeout process");
    deal_timeout_process->top_level = false;
    deal_timeout_process->m_type = pxmodule_type::PROCESS;
    deal_timeout_process->callback_func = callback;
    deal_timeout_process->priority = 0;

    deal_timeout->add_module(deal_timeout_process);
    return deal_timeout;
}

/*����������һ��dispatch processģ��
*/
px_module* px_http_server::create_dispath(const string& name, call_back_type callback, int priority) {
    px_module* http_dispath = new px_module;
    http_dispath->set_name(name);
    http_dispath->top_level = false;
    http_dispath->m_type = pxmodule_type::DISPATH;
    http_dispath->priority = priority;
    http_dispath->callback_func = callback;
    return http_dispath;
}

px_http_module* px_http_server::create_module(const string& name) {
    px_module* dmod = create_dispath(name + "_process", http_interface_dispathtest_func);
    px_http_module* mod = new px_http_module;
    mod->name = name;
    mod->mod = dmod;
    mod->serv = this;
    dmod->add_dispath("px_default", this->interface_error_module);
    this->http_modules.push_back(mod);
    return mod;
}

px_module* px_http_server::get_readmodule(){
    return this->read_module;
}

px_module* px_http_server::get_writemodule() {
    return this->write_module;
}

px_module* px_http_server::get_acceptmodule() {
    return this->accept_module;
}

/*��ʼ��
*/
int px_http_server::initsocket(int port, const char* path) {
    serv_module = new px_service_module;
    serv_module->set_name("http serv");
    serv_module->process_num = 1;
    serv_module->thread_num = 8;
    strcpy(serv_module->listened_ip, "0.0.0.0");
    serv_module->listened_port = port;
    serv_module->wwwpath= path;

    //create time module
    time_module = create_process("tcp create time module", c_timemodule_func);
    time_module->top_level = true;
    serv_module->add_init_module(time_module);

    //create socket file descriptor module
    create_socketfd_module = create_process("tcp create listen socket", c_tcpsocket_func);
    create_socketfd_module->top_level = true;
    serv_module->add_init_module(create_socketfd_module);

    //socket accept module
    accept_module = new px_module;
    accept_module->set_name("tcp accept socket");
    accept_module->m_type = pxmodule_type::EVENT;
    accept_module->e_type = px_event_type::EV_READ;
    accept_module->immediacy = true;
    accept_module->execute_once = false;
    accept_module->oneshot = true;
    accept_module->priority = 0;

    px_module* accept_module_process = create_process("tcp accept socket process", accept_tcpsocket_func);
    accept_module->add_module(accept_module_process);
    create_socketfd_module->add_module(accept_module);

    //socket read module
    read_module = new px_module;
    read_module->top_level = false;
    read_module->set_name("tcp delay read socket");
    read_module->m_type = pxmodule_type::EVENT;
    read_module->e_type = px_event_type::EV_READ;
    read_module->immediacy = false;
    read_module->execute_once = false;
    read_module->priority = 0;

    px_module* delay_socketfd_process = create_process("tcp delay read socket process", read_tcpsocket_func);
    read_module->add_module(delay_socketfd_process);
    accept_module->add_module(read_module);

    //socket write module
    write_module = new px_module;
    write_module->top_level = false;
    write_module->set_name("tcp delay write socket");
    write_module->m_type = pxmodule_type::EVENT;
    write_module->e_type = px_event_type::EV_WRITE;
    write_module->immediacy = false;
    write_module->execute_once = false;
    write_module->priority = 0;

    px_module* write_module_process = create_process("tcp delay write socket process", http_writev_tcpsocket_func);
    write_module->add_module(write_module_process);
    accept_module->add_module(write_module);

    //create input file descriptor module
    px_module* c_inputfd = create_process("input listen socket", c_input_func);
    c_inputfd->top_level = true;
    serv_module->add_init_module(c_inputfd);

    ////input read module
    px_module* read_fd = new px_module;
    read_fd->top_level = false;
    read_fd->set_name("read file");
    read_fd->m_type = pxmodule_type::EVENT;
    read_fd->e_type = px_event_type::EV_READ;
    read_fd->immediacy = true;
    read_fd->execute_once = false;
    read_fd->priority = 0;

    px_module* read_fd_process = create_process("read file process", deal_input_func);
    read_fd->add_module(read_fd_process);
    c_inputfd->add_module(read_fd);

    //create signal descriptor module
    px_module* c_signalfd = create_process("signal listen socket", c_signal_func);
    c_signalfd->top_level = true;
    serv_module->add_init_module(c_signalfd);

    //signal read module
    px_module* read_sig = new px_module;
    read_sig->top_level = false;
    read_sig->set_name("read signal");
    read_sig->m_type = pxmodule_type::EVENT;
    read_sig->e_type = px_event_type::EV_READ;
    read_sig->immediacy = true;
    read_sig->execute_once = false;
    read_sig->priority = 0;

    px_module* read_sig_process = create_process("read signal process", read_signal_func);
    read_sig->add_module(read_sig_process);
    c_signalfd->add_module(read_sig);

    http_timeout_module = create_time_event("http timout event", timeout_connclose_func, 15000000);
    accept_module->add_module(http_timeout_module);

    http_request_module = create_process("http request", http_request_func);
    read_module->add_module(http_request_module);

    http_dispath_module = create_dispath("http request type dispath", http_dispathtest_func);
    read_module->add_module(http_dispath_module);

    http_file_response_module = create_process("http response", http_file_response_func);
    http_dispath_module->add_dispath("file", http_file_response_module);

    interface_dispath_module = create_dispath("http interface dispath", http_interface_dispathtest_func);
    http_dispath_module->add_dispath("interface", interface_dispath_module);

    interface_error_module = create_process("http error 404", http_error_func);
    http_dispath_module->add_dispath("px_default", interface_error_module);
    interface_dispath_module->add_dispath("px_default", interface_error_module);

    nullconn.init_connection();

    interface_module = new px_http_module();
    interface_module->mod = interface_dispath_module;
}

/*���з���
*/
void px_http_server::run_service() {
    //run init
    px_worker::get_instance()->init(serv_module);
    printf("\n=======================Phoenix Http Service is Running=======================\n\n");
    px_worker::get_instance()->dispatch_loop();
    printf("\n=======================Phoenix Http Service is Ending========================\n\n");
}


//printf("mysql conn pool: %d / %d \n", mysqlconn_pool->getfreeblocks(), mysqlconn_pool->getotalblocks());

/*�ص������б�
*/
/*
* ��socket�յ�����Ϣ�������û�connection������
* ÿ�ν��ջ��������пɸ��µļ�ʱ��
* ���������������expansionbuffer��������
* ֻҪ���յ��ֽھͷ��ؽ��ܵ����ֽ��������򷵻�PX_EVENT_BREAK
*/

//��ȡ�·����������ݱ�
void* http_read_tcpsocket_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg) {
    int recvtotal = 0;
    if ((evt->connect->status == pxconn_status_type::CONNECT) && (evt->event_type == px_event_type::EV_READ)) {
        char* buffer = evt->connect->buffer;
        size_t& buffer_used = evt->connect->buffer_used;
        if (buffer_used != 0) {
            //printf("buffer_used: %d \n", buffer_used);
            return nullptr;
        }
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
                    printf("no more memory! %lu\n", buffer_size);
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
void* http_writev_tcpsocket_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg) {
    int sendtotal = 0;
    if ((evt->connect->status == pxconn_status_type::CONNECT) && (evt->event_type == px_event_type::EV_WRITE)) {
        size_t buffer_used = evt->connect->wbuffer_used;
        //printf("buffer_used : %ld\n", buffer_used);
        if (buffer_used == 0)return nullptr;
        px_http_response* response = (px_http_response*)evt->data.get();
        px_http_request* req = (px_http_request*)evt->connect->read_evs->data.get();
        size_t& read = response->read;
        while (true) {
            int ret = writev(evt->connect->fd, response->block, response->block_used);
            if (ret > 0) {
                sendtotal += ret;
                read += ret;
                //printf("-----buffer_used: %d , write: %d\n", buffer_used, read);
                if (response->ismmap && read >= buffer_used) {
                    response->block[0].iov_len = 0;
                    response->block[1].iov_base = response->mmptr + (read - buffer_used);
                    response->block[1].iov_len = response->mm_size - (read - buffer_used);
                }
                else {
                    response->block[0].iov_base = evt->connect->wbuffer + read;
                    response->block[0].iov_len = buffer_used - read;
                }
                evt->connect->update_timer();
                //printf("theadid: %d | send fd: %d size: %d  text:\n%s\n", pxthread->thread_index, evt->connect->fd, ret, buffer + buffer_used);
                if (response->block[1].iov_len == 0) {//������,����request��response
                    evt->connect->clearwbuffer();
                }
            }
            if (ret == 0) {
                //printf("when writing finish net continue.\n");
                //���ӶϿ���,����request��response
                if (req->get_attr("connection")!=nullptr && (strcmp(req->get_attr("connection"), "keep-alive") == 0|| strcmp(req->get_attr("connection"), "Keep-Alive") == 0)) {
                    //printf("start: %d used: %d \n", req->read_start_idx,evt->connect->buffer_used);
                    //if (req->read_start_idx >= evt->connect->buffer_used) {
                    //    evt->connect->clearbuffer();
                    //}
                    //else {
                    //    evt->connect->buffer_read = req->read_start_idx;
                    //    evt->connect->clearwbuffer();
                    //}

                    evt->connect->clearbuffer();
                    evt->connect->read_evs->data.reset();
                    evt->connect->write_evs->data.reset();
                }
                else {
                    close_connection(evt->connect);
                }
                break;
            }
            if (ret < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    //printf("ret<9 and buffer used: %d and iov_len: %d\n", buffer_used, response->block[1].iov_len);
                    break;
                }
                //����,����request��response
                //printf("when writing net interruption.\n");
                //printf("%d.\n", errno);
                close_connection(evt->connect);
                return nullptr;
            }
        }
    }
    return sendtotal > 0 ? nullptr : &PX_EVENT_BREAK;
}

/*
* ������������http_request
*/
void* http_request_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg) {
    if ((char*)arg != &PX_EVENT_BREAK) {
        if (evt->data == nullptr)evt->data = make_shared<px_http_request>(evt->connect);
        px_http_request* req = (px_http_request*)evt->data.get();
        int res = req->parse();
        if (res == PX_HTTP_CONTINUE) {
            //printf("need receive.  %d\n", res);
            return &PX_EVENT_BREAK;
        }
        if (res != PX_FINE) {
            printf("when parse http interruption.  %d\n", res);
            close_connection(evt->connect);
            return &PX_EVENT_BREAK;
        }
        //������
        if (req->get_attr("Referer") != nullptr) {
            const char* url = "http://43.138.29.143";
            char ref[22];
            strncpy(ref, req->get_attr("Referer"), 21);
            if (strcmp(ref, url) == 0) {
                printf("illegal request page.\n");
                close_connection(evt->connect);
                return &PX_EVENT_BREAK;
            }
        }
        return req;
    }
    else return &PX_EVENT_BREAK;
}

/*����������������벻ͬ��ģ��
*/
void* http_dispathtest_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg) {
    if ((char*)arg != &PX_EVENT_BREAK) {
        px_http_request* req = (px_http_request*)arg;
        if (req->url.type != "error") {
            if (req->url.type == "none")pxmodule->module_dispath("px_default", evt, pxthread, arg);
            else if (req->url.type == "interface")pxmodule->module_dispath("interface", evt, pxthread, arg);
            else pxmodule->module_dispath("file", evt, pxthread, arg);
        }
        else pxmodule->module_dispath("px_default", evt, pxthread, arg);
    }
    else return &PX_EVENT_BREAK;
    return arg;
}

/*����urlת��request����
*/
void* http_interface_dispathtest_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg) {
    if ((char*)arg != &PX_EVENT_BREAK) {
        px_http_request* req = (px_http_request*)evt->data.get();
        if (req->url.type != "error") {
            if (req->url.type == "interface")pxmodule->module_dispath(req->url.path[req->url.current_ind++], evt, pxthread, arg);
            else return &PX_EVENT_BREAK;
        }
        else return &PX_EVENT_BREAK;
    }
    return &PX_EVENT_BREAK;
}

/*�����ļ�����response
*/
void* http_file_response_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg) {
    if ((char*)arg != &PX_EVENT_BREAK) {
        px_http_request* req = (px_http_request*)evt->data.get();
        px_service_module* serv_module = px_worker::get_instance()->serv_module;
        evt->connect->write_evs->data = make_shared<px_http_response>(evt->connect);
        px_http_response* data = (px_http_response*)evt->connect->write_evs->data.get();
        data->add_responseline(200);
        data->add_header_common();
        //if (req->get_attr("Cookie") == nullptr) {
        //    string cookie = to_string(generate_sessionid(ntohl(((sockaddr_in*)&(evt->connect->address))->sin_addr.s_addr)));
        //    data->add_header("Set-Cookie", ("session_id="+cookie).c_str());
        //    //��ȡ���ݿ�����
        //    px_http_resources::get_instance()->mysql_signal->down();
        //    if (!evt->immediacy)px_worker::get_instance()->thread_lock->lock();
        //    px_mysql_connect* msconn = px_http_resources::get_instance()->mysqlconn_pool->pop_front();
        //    if (!evt->immediacy)px_worker::get_instance()->thread_lock->unlock();
        //    if (msconn == nullptr) {
        //        px_log::get_log_instance().write("there is no much mysql connection");
        //        return &PX_EVENT_BREAK;
        //    }
        //    char buffer[128];
        //    sprintf(buffer, "INSERT INTO px_session(session_id,ipv4_address)VALUES (%s,'%s');", cookie.c_str(), inet_ntoa(((sockaddr_in*)&(evt->connect->address))->sin_addr));
        //    msconn->execute_sql(buffer);
        //    //�黹���ݿ�����
        //    if (!evt->immediacy)px_worker::get_instance()->thread_lock->lock();
        //    px_http_resources::get_instance()->mysqlconn_pool->push_front(msconn);
        //    px_http_resources::get_instance()->mysql_signal->up();
        //    if (!evt->immediacy)px_worker::get_instance()->thread_lock->unlock();
        //}
        data->add_header("Content-type", px_http_resources_mapping[req->url.type].c_str());
        string path = serv_module->wwwpath;
        path += req->url.url;
        if (0 == access((path + ".gz").c_str(), F_OK)) {
            path += ".gz";
            data->add_header("Content-Encoding", "gzip");
        }
        if (-1 == access(path.c_str(), F_OK)) {
            http_error_func(evt, pxthread, pxmodule, arg);
            return nullptr;
        }
        data->add_file_content(path.c_str());
        data->send_response();
        return nullptr;
    }
    else return arg;
}

/*json��response����
*/
void* http_response_json_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg) {
    if ((char*)arg != &PX_EVENT_BREAK) {
        //��ⷽ���Ƿ�ƥ��
        m_response_config* configptr = (m_response_config*)pxmodule->config;
        px_http_request* req = (px_http_request*)evt->data.get();
        if (configptr) {
            if (0 != strcasecmp(req->get_attr("Method"), configptr->method.c_str())) {
                http_error_func(evt, pxthread, pxmodule, arg);
                return nullptr;
            }
        }
        else {
            px_log::get_log_instance().write("there is no interface function");
            px_log::get_log_instance().write(pxmodule->name);
            return &PX_EVENT_BREAK;
        }
        //��ȡ���ݿ�����
        px_http_resources::get_instance()->mysql_signal->down();
        if (!evt->immediacy)px_worker::get_instance()->thread_lock->lock();
        px_mysql_connect* msconn = px_http_resources::get_instance()->mysqlconn_pool->pop_front();
        if (!evt->immediacy)px_worker::get_instance()->thread_lock->unlock();
        if (msconn == nullptr) {
            px_log::get_log_instance().write("there is no much mysql connection");
            return &PX_EVENT_BREAK;
        }
        //SQL����Լ���ѯ
        px_http_response_data res;
        configptr->func(msconn, req, &res);
        evt->connect->write_evs->data = make_shared<px_http_response>(evt->connect);
        px_http_response* data = (px_http_response*)evt->connect->write_evs->data.get();
        if (res.type == px_http_response_type::ERROR) {
            //����response�����壨json��ʽ��
            data->add_responseline(500);
            data->add_header_common();
            data->add_header("Content-type", "text/plain");
        }
        //else if (res.type != px_http_response_type::LOGIN) {
        //    //����response�����壨json��ʽ��
        //    string cookie = to_string(generate_sessionid(ntohl(((sockaddr_in*)&(evt->connect->address))->sin_addr.s_addr)));
        //    data->add_header("Set-Cookie", ("session_id=" + cookie).c_str());
        //    data->add_responseline(200);
        //    data->add_header_common();
        //    data->add_header("Content-type", "text/plain");
        //    char buffer[128];
        //    sprintf(buffer, "INSERT INTO px_session(session_id,ipv4_address)VALUES (%s,'%s');", cookie.c_str(), inet_ntoa(((sockaddr_in*)&(evt->connect->address))->sin_addr));
        //    msconn->execute_sql(buffer);
        //}
        else {
            //����response�����壨json��ʽ��
            data->add_responseline(200);
            data->add_header_common();
            data->add_header("Content-type", res.headval.c_str());
        }
        data->add_header("Access-Control-Allow-Origin", "true");
        if (res.type == px_http_response_type::HTML)data->add_file_content(res.data.c_str());
        else data->add_str_content(res.data.c_str());
        data->send_response();
        //�黹���ݿ�����
        if (!evt->immediacy)px_worker::get_instance()->thread_lock->lock();
        px_http_resources::get_instance()->mysqlconn_pool->push_front(msconn);
        px_http_resources::get_instance()->mysql_signal->up();
        if (!evt->immediacy)px_worker::get_instance()->thread_lock->unlock();
        return nullptr;
    }
    else return &PX_EVENT_BREAK;
}

/*������
*/
void* http_error_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg) {
    px_service_module* serv_module = px_worker::get_instance()->serv_module;
    evt->connect->write_evs->data = make_shared<px_http_response>(evt->connect);
    px_http_response* data = (px_http_response*)evt->connect->write_evs->data.get();
    data->add_responseline(404);
    data->add_header_common();
    data->add_header("Content-type", "text/html;charset=utf-8");
    data->add_header("Access-Control-Allow-Origin", "true");

    data->add_file_content((serv_module->wwwpath + "error/404.html").c_str());
    data->send_response();
}

/*���������
*/

/*Constructor
*/
px_random_object::px_random_object() :e(default_random_engine(time(0))), u(uniform_int_distribution<int>(0, 1024)) {}

/*��ȡ����
*/
px_random_object& px_random_object::getinstance() {
    static px_random_object rj;
    return rj;
}

/*��ȡ�����
*/
int px_random_object::getrandom() {
    return u(e);
}

/*��������cookie
*/
unsigned long long generate_sessionid(unsigned long ip) {
    time_t tm = time(0);
    unsigned long long res = (tm << 32) + ip;
    res <<= 10;
    res += px_random_object::getinstance().getrandom();
    return res;
}


void* c_timemodule_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* last_return) {
    int fd = -1;
    evt->connect = &nullconn;
    evt->evt_module->module_link(evt, fd, nullptr);
}