//废弃，仅用来参考

//#include "px_worker.h"
//#include "px_module.h"
//#include "px_connections.h"
//#include "px_events.h"
//#include "px_locker.h"
//#include "px_epoll.h"
//#include "px_error.h"
//#include "px_resources.h"
//#include "px_server.h"
//#include "px_timer.h"
//#include "px_thread_pool.h"
//#include "px_list.h"
//#include "px_util.h"
//#include "px_http.h"
//#include "px_callback_hub.h"
//using std::make_shared;
//using std::to_string;
//
///*创建并返回一个process模块
//*/
//px_module* px_server::create_process(const string& name, call_back_type callback, int priority) {
//    px_module* http_request_process = new px_module;
//    http_request_process->set_name(name);
//    http_request_process->top_level = false;
//    http_request_process->m_type = pxmodule_type::PROCESS;
//    http_request_process->callback_func = callback;
//    http_request_process->priority = priority;
//    return http_request_process;
//}
//
///*创建并返回一个time事件模块
//*/
//px_module* px_server::create_time_event(const string& name, call_back_type callback,clock_t life_time,int priority,bool immediacy,bool execute_once) {
//    px_module* deal_timeout = new px_module;
//    deal_timeout->top_level = false;
//    deal_timeout->set_name(name+" :tcp timeout");
//    deal_timeout->m_type = pxmodule_type::EVENT;
//    deal_timeout->e_type = px_event_type::EV_TIMEOUT;
//    deal_timeout->immediacy = immediacy;
//    deal_timeout->execute_once = execute_once;
//    deal_timeout->priority = priority;
//    deal_timeout->life_time = life_time;
//
//    px_module* deal_timeout_process = new px_module;
//    deal_timeout_process->set_name(name+" :tcp timeout process");
//    deal_timeout_process->top_level = false;
//    deal_timeout_process->m_type = pxmodule_type::PROCESS;
//    deal_timeout_process->callback_func = callback;
//    deal_timeout_process->priority = 0;
//
//    deal_timeout->add_module(deal_timeout_process);
//    return deal_timeout;
//}
//
///*创建并返回一个dispatch process模块
//*/
//px_module* px_server::create_dispath(const string& name, call_back_type callback, int priority) {
//    px_module* http_dispath = new px_module;
//    http_dispath->set_name(name);
//    http_dispath->top_level = false;
//    http_dispath->m_type = pxmodule_type::DISPATH;
//    http_dispath->priority = priority;
//    http_dispath->callback_func = callback;
//    return http_dispath;
//}
//
///*初始化
//*/
//int px_server::initsocket(int port) {
//    serv_module = new px_service_module;
//    serv_module->set_name("http serv");
//    serv_module->process_num = 1;
//    serv_module->thread_num = 8;
//    strcpy(serv_module->listened_ip, "0.0.0.0");
//    serv_module->listened_port = port;
//
//    //create socket file descriptor module
//    c_socketfd = new px_module;
//    c_socketfd->top_level = true;
//    c_socketfd->set_name("tcp create listen socket");
//    c_socketfd->m_type = pxmodule_type::PROCESS;
//
//    px_module* c_socket_process = new px_module;
//    c_socket_process->set_name("tcp create listen socket process");
//    c_socket_process->top_level = false;
//    c_socket_process->m_type = pxmodule_type::PROCESS;
//    c_socket_process->callback_func = c_tcpsocket_func;
//    c_socket_process->priority = 0;
//
//    c_socketfd->add_module(c_socket_process);
//    serv_module->add_init_module(c_socketfd);
//
//    //socket accept module
//    accept_socketfd = new px_module;
//    accept_socketfd->set_name("tcp accept socket");
//    accept_socketfd->m_type = pxmodule_type::EVENT;
//    accept_socketfd->e_type = px_event_type::EV_READ;
//    accept_socketfd->immediacy = true;
//    accept_socketfd->execute_once = false;
//    accept_socketfd->oneshot = true;
//    accept_socketfd->priority = 0;
//
//    px_module* accept_socketfd_process = new px_module;
//    accept_socketfd_process->set_name("tcp accept socket process");
//    accept_socketfd_process->top_level = false;
//    accept_socketfd_process->m_type = pxmodule_type::PROCESS;
//    accept_socketfd_process->callback_func = accept_tcpsocket_func;
//    accept_socketfd_process->priority = 0;
//
//    accept_socketfd->add_module(accept_socketfd_process);
//    c_socketfd->add_module(accept_socketfd);
//
//    //socket read module
//    read_socketfd = new px_module;
//    read_socketfd->top_level = false;
//    read_socketfd->set_name("tcp delay read socket");
//    read_socketfd->m_type = pxmodule_type::EVENT;
//    read_socketfd->e_type = px_event_type::EV_READ;
//    read_socketfd->immediacy = false;
//    read_socketfd->execute_once = false;
//    read_socketfd->priority = 0;
//
//    px_module* delay_socketfd_process = new px_module;
//    delay_socketfd_process->set_name("tcp delay read socket process");
//    delay_socketfd_process->top_level = false;
//    delay_socketfd_process->m_type = pxmodule_type::PROCESS;
//    delay_socketfd_process->callback_func = read_tcpsocket_func;
//    delay_socketfd_process->priority = 0;
//
//    read_socketfd->add_module(delay_socketfd_process);
//    accept_socketfd->add_module(read_socketfd);
//
//    //socket write module
//    write_socketfd = new px_module;
//    write_socketfd->top_level = false;
//    write_socketfd->set_name("tcp delay write socket");
//    write_socketfd->m_type = pxmodule_type::EVENT;
//    write_socketfd->e_type = px_event_type::EV_WRITE;
//    write_socketfd->immediacy = false;
//    write_socketfd->execute_once = false;
//    write_socketfd->priority = 0;
//
//    px_module* write_socketfd_process = new px_module;
//    write_socketfd_process->set_name("tcp delay write socket process");
//    write_socketfd_process->top_level = false;
//    write_socketfd_process->m_type = pxmodule_type::PROCESS;
//    write_socketfd_process->callback_func = http_writev_tcpsocket_func;
//    write_socketfd_process->priority = 0;
//
//    write_socketfd->add_module(write_socketfd_process);
//    accept_socketfd->add_module(write_socketfd);
//
//    //create input file descriptor module
//    px_module* c_inputfd = new px_module;
//    c_inputfd->top_level = true;
//    c_inputfd->set_name("input listen socket");
//    c_inputfd->m_type = pxmodule_type::PROCESS;
//    c_inputfd->e_type = px_event_type::EV_READ;
//    c_inputfd->immediacy = true;
//    c_inputfd->execute_once = false;
//    c_inputfd->priority = 0;
//    int* filedescripter = new int(STDIN_FILENO);
//    c_inputfd->config = filedescripter;
//
//    px_module* c_inputfd_process = new px_module;
//    c_inputfd_process->set_name("input listen socket process");
//    c_inputfd_process->top_level = false;
//    c_inputfd_process->m_type = pxmodule_type::PROCESS;
//    c_inputfd_process->callback_func = c_input_func;
//    c_inputfd_process->priority = 0;
//
//    c_inputfd->add_module(c_inputfd_process);
//    serv_module->add_init_module(c_inputfd);
//
//    ////input read module
//    px_module* read_fd = new px_module;
//    read_fd->top_level = false;
//    read_fd->set_name("read file");
//    read_fd->m_type = pxmodule_type::EVENT;
//    read_fd->e_type = px_event_type::EV_READ;
//    read_fd->immediacy = true;
//    read_fd->execute_once = false;
//    read_fd->priority = 0;
//
//    px_module* read_fd_process = new px_module;
//    read_fd_process->set_name("read file process");
//    read_fd_process->top_level = false;
//    read_fd_process->m_type = pxmodule_type::PROCESS;
//    read_fd_process->callback_func = deal_input_func;
//    read_fd_process->priority = 0;
//
//    read_fd->add_module(read_fd_process);
//    c_inputfd->add_module(read_fd);
//
//    //create signal descriptor module
//    px_module* c_signalfd = new px_module;
//    c_signalfd->top_level = true;
//    c_signalfd->set_name("signal listen socket");
//    c_signalfd->m_type = pxmodule_type::PROCESS;
//    c_signalfd->e_type = px_event_type::EV_READ;
//    c_signalfd->immediacy = true;
//    c_signalfd->execute_once = false;
//    c_signalfd->priority = 0;
//
//    px_module* c_signalfd_process = new px_module;
//    c_signalfd_process->set_name("signal listen process");
//    c_signalfd_process->top_level = false;
//    c_signalfd_process->m_type = pxmodule_type::PROCESS;
//    c_signalfd_process->callback_func = c_signal_func;
//    c_signalfd_process->priority = 0;
//
//    c_signalfd->add_module(c_signalfd_process);
//    serv_module->add_init_module(c_signalfd);
//
//    //signal read module
//    px_module* read_sig = new px_module;
//    read_sig->top_level = false;
//    read_sig->set_name("read signal");
//    read_sig->m_type = pxmodule_type::EVENT;
//    read_sig->e_type = px_event_type::EV_READ;
//    read_sig->immediacy = true;
//    read_sig->execute_once = false;
//    read_sig->priority = 0;
//
//    px_module* read_sig_process = new px_module;
//    read_sig_process->set_name("read signal process");
//    read_sig_process->top_level = false;
//    read_sig_process->m_type = pxmodule_type::PROCESS;
//    read_sig_process->callback_func = read_signal_func;
//    read_sig_process->priority = 0;
//
//    read_sig->add_module(read_sig_process);
//    c_signalfd->add_module(read_sig);
//}
//
///*运行服务
//*/
//void px_server::run_service() {
//    //run init
//    px_worker::get_instance()->init(serv_module);
//    printf("\n=======================Phoenix Service is Running=======================\n\n");
//    px_worker::get_instance()->dispatch_loop();
//    printf("\n=======================Phoenix Service is Ending========================\n\n");
//}
