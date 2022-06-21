#ifndef PX_HTTP
#define PX_HTTP
#include "px_type.h"
#include "px_util.h"
#include <string>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include<sys/uio.h>
#include<string.h>
using std::string;
using std::map;
using std::unordered_map;
using std::unordered_set;
enum class px_http_method { GET, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATH };
enum class px_httpparse_sataus { PAESE_REQUEST, PARSE_HEADER,PARSE_CONTENT,PARSE_FINISH,PARSE_ERROR };

/*==px_url==
* 用于解析和管理http请求的url
*/
class px_url {
public:
	px_url(){}
	~px_url() {}
	void parse(px_connection* conn,int ptr);
	const char* get_xwwwform_data(const char* attri);
	int url_paramstr;//参数开始地址
	px_connection* conn;
	string url;
	string type;
	vector<string> path;
	px_x_www_form param;
	int current_ind;
};
int parse_url(const char* url, int begin);
int parse_url(const string& url, int begin);

/*==px_http_content==
* 用于解析和管理http内容实体
*/
class px_http_content {
public:
	void parse(char* buffer,size_t bufsz,const char* content_type);
	const char* get_xform_type(const char* attri);
	const char* get_xform_name(const char* attri);
	const char* get_xform_val(const char* attri);
	const char* get_xform_data(const char* attri);
	int get_xform_size(const char* attri);
private:
	char* content_buf;//获取完整的报文才设置该指针，所以直接用字符串指针指向就可以了
	px_x_form xform;//目前就支持这一种形式
};


/*==px_http_request==
* 用于解析和管理http请求
*/
class px_http_request {
public:
	px_http_request(px_connection* connect);
	int parse();
	void print_all();
	const char* get_attr(const char* key);
public:
	px_connection* conn;
	//http parse info
	int read_start_idx;
	px_httpparse_sataus parse_status;

	//http param
	//request
	int method = -1;
	px_url url;
	int http_version = -1;
	//header
	int user_agent = -1;
	int accept = -1;//说明用户代理可处理的媒体类型。
	int accept_encoding = -1;//说明用户代理支持的内容编码。
	int accept_language = -1;//说明用户代理能够处理的自然语言集。
	int cache_control = -1;//缓存控制
	int x_forward_for = -1;//
	int host = -1;//给出请求资源所在服务器的域名。
	int referer = -1;//网页是从哪个页面链接过来的
	int connection = -1;//连接管理，可以是Keep-Alive或close。
	int content_type = -1;//说明实现主体的媒体类型。
	int cookie = -1;
	int content_length=-1;//说明内容主体的大小。
	int upgrade_insecure_requests=-1;
	int keep_a_live = -1;
	//content
	px_http_content content;
private:
	int parse_request();
	int parse_header();
	int parse_content();
	bool assignment(const char* key, int offset, char* buf);

};

class px_charhash
{
public:
	int operator()(const char* str)const
	{
		unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
		unsigned int hash = 0;

		while (*str) {
			hash = hash * seed + (*str++);
		}

		return (hash & 0x7FFFFFFF);
	}
};

class px_charcmp
{
public:
	bool operator()(const char* str1, const char* str2)const
	{
		return strcmp(str1, str2) == 0;
	}
};

//全局的字典
extern unordered_map<const char*, int px_http_request::*,px_charhash,px_charcmp> px_http_map;//记录每个字符串型字段对应的request内成员
extern unordered_map<string, int px_http_request::*> px_http_map_integer;//记录每个整数型字段对应的request内成员
extern unordered_map<int, string> px_http_response_map;//记录各类http response报文response字段
extern unordered_set<string> px_http_resources_type;//支持的文件类型
extern unordered_map<string, string> px_http_resources_mapping;//response中返回的文件类型对应的首部行字段

/*==px_http_response==
* 用于解析和管理http响应
*/
class px_http_response {
public:
	px_http_response(px_connection* conn);
	~px_http_response();
	void send_response();
	void add_responseline(int code);
	void add_header_common();
	void add_header(const char* key,const char* val);
	void add_file_content(const char* filename);
	void add_str_content(const char* str);
public:
	px_connection* conn;
	iovec block[2];
	int block_used;
	bool ismmap;
	char* mmptr;
	size_t mm_size;
	size_t read;//已经读取的大小
};

#endif // !PX_HTTP
