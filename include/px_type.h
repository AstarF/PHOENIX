#ifndef  PX_TYPE
#define PX_TYPE

//==Statement==
typedef unsigned int px_uint;
const unsigned int BUFFER_SIZE = 2048;
class px_worker;
class px_epoll;
class px_connection;
class px_event;
class px_module;
enum class pxaddress_type;
enum class pxmodule_type;
enum class px_event_type;
class px_locker;
class px_thread_pool;
class px_service_module;
struct px_thread_param;
class px_thread;
enum class px_event_status;
enum class px_after_callback;
template <typename T>
class px_objqueue;
template <typename T>
struct objbox;
template <typename T>
class px_objpool;
class px_server;
class px_resources;
class px_jump_table;
class px_timer;
class px_json;
class px_x_www_form;
class px_xform;
class px_multipartfrom;
class px_signal;
class px_mysql;
class px_mysql_connect;
class px_http_request;
class px_http_server;
class px_http_resources;
#endif // ! PX_TYPE
