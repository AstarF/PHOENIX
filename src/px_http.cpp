#include <cstdlib>
#include "px_http.h"
#include "px_error.h"
#include "px_connections.h"
#include <iostream>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
using std::cout;
using std::endl;
using std::string;
using std::to_string;
unordered_map<const char*, int px_http_request::*, px_charhash, px_charcmp> px_http_map = {
	{"Method",&px_http_request::method},//用户代理
	{"User-Agent",&px_http_request::user_agent},//用户代理
	{"Accept",&px_http_request::accept},//说明用户代理可处理的媒体类型。
	{"Referer",&px_http_request::referer},//网页是从哪个页面链接过来的
	{"Accept-Encoding",&px_http_request::accept_encoding},//说明用户代理支持的内容编码。
	{"Accept-Language",&px_http_request::accept_language},//说明用户代理能够处理的自然语言集。
	{"Host",&px_http_request::host},//给出请求资源所在服务器的域名。
	{"X-Forwarded-For",&px_http_request::x_forward_for},
	{"Cache-Control",&px_http_request::cache_control},//缓存控制
	{"Connection",&px_http_request::connection},//连接管理，可以是Keep-Alive或close。
	{"Content-Type",&px_http_request::content_type},//说明实现主体的媒体类型。
	{"Cookie",&px_http_request::cookie},//Cookie。

	{"method",& px_http_request::method},//用户代理
	{"user-agent",&px_http_request::user_agent},//用户代理
	{"accept",&px_http_request::accept},//说明用户代理可处理的媒体类型。
	{"referer",&px_http_request::referer},//网页是从哪个页面链接过来的
	{"accept-encoding",&px_http_request::accept_encoding},//说明用户代理支持的内容编码。
	{"accept-language",&px_http_request::accept_language},//说明用户代理能够处理的自然语言集。
	{"host",&px_http_request::host},//给出请求资源所在服务器的域名。
	{"x-forwarded-for",&px_http_request::x_forward_for},
	{"cache-control",&px_http_request::cache_control},//缓存控制
	{"connection",&px_http_request::connection},//连接管理，可以是Keep-Alive或close。
	{"content-type",&px_http_request::content_type},//说明实现主体的媒体类型。
	{"cookie",&px_http_request::cookie},//Cookie。
};

unordered_map<string, int px_http_request::*> px_http_map_integer = {
	{"Content-Length",&px_http_request::content_length},//说明内容主体的大小。
	{"content-length",&px_http_request::content_length},//说明内容主体的大小。
	{"Upgrade-Insecure-Requests",&px_http_request::upgrade_insecure_requests}//在http请求中加入 ‘Upgrade-Insecure-Requests: 1’ ，服务器收到请求后会返回 “Content-Security-Policy: upgrade-insecure-requests” 头，告诉浏览器，可以把所属本站的所有 http 连接升级为 https 连接
};

unordered_map<int, string> px_http_response_map = {
	{200,"HTTP/1.1 200 OK\n"},//客户端请求被正常处理。
	{301,"HTTP/1.1 301 Moved Permanently\n"},//永久重定向，该资源已被永久移动到新位置，将来任何对该资源的访问都要使用本响应返回的若干个URI之一
	{302,"HTTP/1.1 302 Found\n"},//临时重定向，请求的资源现在临时从不同的URI中获得。
	{400,"HTTP/1.1 400 Bad Request\n"},//请求报文存在语法错误。
	{403,"HTTP/1.1 403 Forbidden\n"},//请求被服务器拒绝
	{404,"HTTP/1.1 404 Not Found\n"},//请求不存在，服务器上找不到请求的资源
	{500,"HTTP/1.1 500 Internal Server Error\n"},//服务器端错误--服务器处理请求出错。
};

unordered_set<string> px_http_resources_type = {
	".html",".js",".css",".json",".txt",".jpeg",".jpg",".png",".ico",".tif",".tiff",".gif",".map",".woff"
};

unordered_map<string, string> px_http_resources_mapping = {
	{".html","text/html;charset=utf-8"},
	{".js","text/javascript;charset=utf-8"},
	{".map","text/javascript"},
	{".css","text/css"},
	{".json","application/json;charset=utf-8"},
	{".txt","text/plain"},
	{".jpeg","image/jpeg"},
	{".jpg","image/jpeg"},
	{".png","image/png"},
	{".gif","image/gif"},
	{".ico","image/x-icon"},
	{".tif","image/tiff"},
	{".tiff","image/tiff"},
	{".woff","font/tiff"}
};

/*解析url
*/
void px_url::parse(px_connection* conn,int ptr) {
	
	this->conn = conn;
	url_paramstr = ptr;
	url = string(this->conn->buffer+ptr);
	if (url == "/")url = "/index.html";
	current_ind = 0;
	string value = "";
	size_t sz = url.size();
	for (int i = 1; i < sz; ++i) {
		if (url[i] == '/') {
			if (value == "..") {
				type == "error";
				return;
			}
			path.push_back(value);
			value = "";
		}
		else if (url[i] == '?') {		
			path.push_back(value);
			url_paramstr = ptr + i + 1;
			param.parse(this->conn->buffer + ptr + i + 1);
			int pos = path[path.size() - 1].find_last_of(".");
			if (pos == -1) {
				type = "interface";
			}
			else {
				value = path[path.size() - 1].substr(pos, path.size() - pos);
				if (px_http_resources_type.count(value))type = value;
				else type = "error";
			}
			return;
		}
		else {
			value += url[i];
		}
	}
	path.push_back(value);
	int pos = value.find_last_of(".");
	if (pos == -1) {
		if(value=="")type = "none";
		else type = "interface";
	}
	else {
		value = value.substr(pos, value.size() - pos);
		if (px_http_resources_type.count(value))type = value;
		else type = "error";
	}
}

const char* px_url::get_xwwwform_data(const char* attri) {
	if (param.xwwwform.count(attri)) {
		return this->conn->buffer + url_paramstr + param.xwwwform[attri];
	}
	else return nullptr;
	
}

//==px_http_content==
void px_http_content::parse(char* buffer, size_t bufsz,const char* content_type) {
	this->content_buf = buffer;
	//判断content类型
	if (strlen(content_type) > 20) {
		char type[64];
		memset(type, 0, sizeof(type));
		strncpy(type, content_type, 20);
		if (strcmp(type, "multipart/form-data;") ==0) {
			int i = 20;
			for (; i < strlen(content_type); ++i)if (content_type[i] == '=')break;
			strcpy(type, content_type + i + 1);
			this->xform.parse(buffer, bufsz, type);
		}
	}
}


const char* px_http_content::get_xform_type(const char* attri) {
	return this->content_buf + this->xform.xform[attri].type;
}
const char* px_http_content::get_xform_name(const char* attri) {
	return this->content_buf + this->xform.xform[attri].name;
}
const char* px_http_content::get_xform_val(const char* attri) {
	return this->content_buf + this->xform.xform[attri].val;
}
const char* px_http_content::get_xform_data(const char* attri) {
	return this->content_buf + this->xform.xform[attri].data;
}
int px_http_content::get_xform_size(const char* attri) {
	return this->xform.xform[attri].size;
}


//==px_http_request==
/*Constructor
*/
px_http_request::px_http_request(px_connection* connect) :conn(connect),read_start_idx(0),
parse_status(px_httpparse_sataus::PAESE_REQUEST) {}

/*根据http请求为request对象的成员赋值
*/
bool px_http_request::assignment(const char* key, int offset,char* buf) {
	if (px_http_map.count(key) == 1) {
		this->*px_http_map[key] = offset;
		return true;
	}
	else if (px_http_map_integer.count(key) == 1) {
		this->*px_http_map_integer[key] = atoi(buf+offset);
		return true;
	}
	return false;
}

/*解析http请求
*/
int px_http_request::parse(){
	int res = PX_FINE;
	while (this->parse_status != px_httpparse_sataus::PARSE_FINISH) {
		if (this->parse_status == px_httpparse_sataus::PAESE_REQUEST) {
			res = parse_request();	
			if (res != PX_FINE)return res;
		}
		else if (this->parse_status == px_httpparse_sataus::PARSE_HEADER) {
			res = parse_header();
			if (res != PX_FINE)return res;
		}
		else if (this->parse_status == px_httpparse_sataus::PARSE_CONTENT) {
			res = parse_content();
			if (res != PX_FINE)return res;
		}
		else if (this->parse_status == px_httpparse_sataus::PARSE_ERROR) {
			return PX_HTTP_PARSE_ERROR;
		}
		//print_all();
	}
	return PX_FINE;
}

/*解析http请求行
*/
int px_http_request::parse_request() {
	int index = this->read_start_idx, num = 0;
	int ptr = index;
	for (; index < conn->buffer_used; ++index) {
		if (conn->buffer[index] == ' ') {
			if (num == 0) {
				this->method = ptr;
				ptr = index+1;
				conn->buffer[index] = '\0';
			}
			else if (num == 1) {
				conn->buffer[index] = '\0';
				this->url.parse(conn,ptr);
				ptr = index + 1;
			}
			else return PX_HTTPREQUEST_ERROR;
			num++;
		}
		else if (conn->buffer[index] == '\n') {
			conn->buffer[index] = '\0';
			this->http_version = ptr;
			this->read_start_idx = index + 1;
			this->parse_status = px_httpparse_sataus::PARSE_HEADER;
			return PX_FINE;
		}
		else if (conn->buffer[index] == '\r') {
			conn->buffer[index] = '\0';
		}
	}
	return PX_HTTP_CONTINUE;
}

/*解析http请求首部行
*/
int px_http_request::parse_header() {
	int index = this->read_start_idx, num = 0;
	char* key = nullptr;
	int ptr = index;
	for (; index < conn->buffer_used; ++index) {
		if (key == nullptr)conn->buffer[index] = tolower(conn->buffer[index]);
		if (conn->buffer[index] == ':') {
			if (key == nullptr) {
				key = conn->buffer+ptr;
				conn->buffer[index] = '\0';
				ptr = index + 2;
			}
		}
		else if (conn->buffer[index] == '\n') {
			if (key == nullptr) {
				this->parse_status = px_httpparse_sataus::PARSE_CONTENT;
				return PX_FINE;
			}
			conn->buffer[index] = '\0';
			this->assignment(key, ptr,conn->buffer);
			//cout << key << "|" << conn->buffer + ptr << endl;
			ptr = index + 1;
			key = nullptr;
			this->read_start_idx = index+1;
		}
		else if (conn->buffer[index] == '\r') {
			conn->buffer[index] = '\0';
		}
	}
	return PX_HTTP_CONTINUE;
}

/*获取内部数据
*/
const char* px_http_request::get_attr(const char* key) {
	if (px_http_map.count(key) == 1 && this->*px_http_map[key]!=-1) {
		return conn->buffer+this->*px_http_map[key];
	}
	return nullptr;
}

/*解析http请求实体部分
*/
int px_http_request::parse_content() {
	//printf("buffer_used: %d  read_start_idx: %d  content_length: %d\n", conn->buffer_used, this->read_start_idx,this->content_length);
	if (conn->buffer_used >= (this->read_start_idx + this->content_length)) {
		this->content.parse(conn->buffer + this->read_start_idx, this->content_length+3, conn->buffer + this->content_type);
		this->parse_status = px_httpparse_sataus::PARSE_FINISH;
	}else return PX_HTTP_CONTINUE;
	//this->read_start_idx + this->content_length
	return PX_FINE;
}

/*测试解析是否正确
*/
void px_http_request::print_all() {
	char* buffer = conn->buffer;
	printf("\n=========================================================================\n");
	printf(" request:\n");
	printf(" method: %s\n", buffer+this->method);
	printf(" url: %s\n", this->url.url.c_str());
	printf(" version: %s\n", buffer + this->http_version);
	printf(" header:\n");
	printf(" upgrade insecure requests: %d\n",this->upgrade_insecure_requests);
	printf(" user_agent: %s\n", buffer + this->user_agent);
	printf(" accept: %s\n", buffer + this->accept);
	printf(" accept_encoding: %s\n", buffer + this->accept_encoding);
	printf(" accept_language: %s\n", buffer + this->accept_language);
	printf(" cache_control: %s\n", buffer + this->cache_control);
	printf(" host: %s\n", buffer + this->host);
	printf(" referer: %s\n", buffer + this->referer);
	printf(" connection: %s\n", buffer + this->connection);
	printf(" cookie:%s\n", buffer + this->cookie);
	printf(" content_type: %s\n", buffer + this->content_type);
	printf(" content length:%d\n", this->content_length);
	printf("=========================================================================\n");
}

/*Constructor
*/
px_http_response::px_http_response(px_connection* conn) :conn(conn),
block_used(0), ismmap(false), mmptr(nullptr), mm_size(0), read(0) {}

/*Destructor
*/
px_http_response::~px_http_response() {
	if (ismmap)munmap(mmptr, mm_size);
}

/*添加http响应的response行
*/
void px_http_response::add_responseline(int code) {
	char* cptr = conn->wbuffer;
	int sz = conn->wbuffer_used;
	strcpy(cptr + sz, px_http_response_map[code].c_str());
	conn->wbuffer_used += px_http_response_map[code].size();
}

/*
* 使用一些常用的首部行
*/
void px_http_response::add_header_common() {
	char* cptr = conn->wbuffer;
	//产生日期
	time_t now = time(0);
	//该消息起源的日期和时间
	int sz = conn->wbuffer_used;
	sz += strftime(cptr + sz, conn->wbuffer_size - sz, "Date: %a, %d %m %Y %H:%M:%S GMT\n", localtime(&now));
	const char* ptr = "Access-Control-Allow-Methods: GET, POST\nAccept-Ranges: bytes\nConnection: keep-alive\nServer: phoenix\n";
	strcpy(cptr + sz, ptr);
	sz += strlen(ptr);
	//允许的HTTP请求方法
	//表示可用于定义范围的单位
	//指示所述媒体类型
	//控制网络连接是否保持打开状态
	//对主体执行的编码方式
	//服务器应用软件名称和版本
	//查看其他首部列表，可能会使响应发生变化
	conn->wbuffer_used = sz;
}

/*添加response的首部行
* 不需要添加Content-Length，添加content后会自动加上
* 主要用于：
* Content-Type
* 可添加值:
* jpg=image/jpeg
* tiff=image/tiff
* gif=image/gif
* png=image/png
* tif=image/tiff
* ico=image/x-icon
* jpeg=image/jpeg
* html=text/html;charset=utf-8
* application/x-javascript
* text/plain:纯文本
* text/css
* text/javascript; charset=UTF-8
* application/octet-stream:二进制流
* application/x-www-form-urlencoded:HTTP会将请求参数用key1=val1&key2=val2的方式进行组织，并放到请求实体里面
* multipart/form-data:一般需要上传文件的表单则用该类型。
* application/json;charset=utf-8:以“键-值”对的方式组织的数据
* application/xml 和 text/xml:xml格式的数据,text/xml的话，将忽略xml数据里的编码格式
*/
void px_http_response::add_header(const char* key, const char* val) {
	char* cptr = conn->wbuffer;
	int sz = conn->wbuffer_used;
	sz += sprintf(cptr + sz, "%s: %s\n", key, val);
	conn->wbuffer_used = sz;
}

/*添加文件类型实体
*/
void px_http_response::add_file_content(const char* filename) {
	//打开文件
	int fd = open(filename, O_RDONLY);
	struct stat statbuf;
	//获取文件属性
	stat(filename, &statbuf);
	mm_size = statbuf.st_size;
	mmptr = (char*)mmap(0, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	ismmap = true;
	char* cptr = conn->wbuffer;
	int sz = conn->wbuffer_used;
	sz += sprintf(cptr + sz, "Content-Length: %ld\n\n", mm_size);
	conn->wbuffer_used = sz;
}

/*添加字符串类型实体
*/
void px_http_response::add_str_content(const char* str) {
	//printf("strlen: %d  --- %d\n", strlen(str), conn->wbuffer_used);
	if (conn->wbuffer_size <= strlen(str) + conn->wbuffer_used) {
		conn->expansionwbuffer(strlen(str)+1024);
	}
	char* cptr = conn->wbuffer;
	int sz = conn->wbuffer_used;
	sz += sprintf(cptr + sz, "Content-Length: %ld\n\n%s", strlen(str), str);
	conn->wbuffer_used = sz;
}

/*发送前必须调用
* 组装response
*/
void px_http_response::send_response() {
	block[0].iov_base = conn->wbuffer;
	block[0].iov_len = conn->wbuffer_used;
	block_used = 1;
	if (ismmap) {
		block[1].iov_base = mmptr;
		block[1].iov_len = mm_size;
		block_used += 1;
	}
}