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
* ���ڽ����͹���http�����url
*/
class px_url {
public:
	px_url(){}
	~px_url() {}
	void parse(px_connection* conn,int ptr);
	const char* get_xwwwform_data(const char* attri);
	int url_paramstr;//������ʼ��ַ
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
* ���ڽ����͹���http����ʵ��
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
	char* content_buf;//��ȡ�����ı��Ĳ����ø�ָ�룬����ֱ�����ַ���ָ��ָ��Ϳ�����
	px_x_form xform;//Ŀǰ��֧����һ����ʽ
};


/*==px_http_request==
* ���ڽ����͹���http����
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
	int accept = -1;//˵���û�����ɴ����ý�����͡�
	int accept_encoding = -1;//˵���û�����֧�ֵ����ݱ��롣
	int accept_language = -1;//˵���û������ܹ��������Ȼ���Լ���
	int cache_control = -1;//�������
	int x_forward_for = -1;//
	int host = -1;//����������Դ���ڷ�������������
	int referer = -1;//��ҳ�Ǵ��ĸ�ҳ�����ӹ�����
	int connection = -1;//���ӹ���������Keep-Alive��close��
	int content_type = -1;//˵��ʵ�������ý�����͡�
	int cookie = -1;
	int content_length=-1;//˵����������Ĵ�С��
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

//ȫ�ֵ��ֵ�
extern unordered_map<const char*, int px_http_request::*,px_charhash,px_charcmp> px_http_map;//��¼ÿ���ַ������ֶζ�Ӧ��request�ڳ�Ա
extern unordered_map<string, int px_http_request::*> px_http_map_integer;//��¼ÿ���������ֶζ�Ӧ��request�ڳ�Ա
extern unordered_map<int, string> px_http_response_map;//��¼����http response����response�ֶ�
extern unordered_set<string> px_http_resources_type;//֧�ֵ��ļ�����
extern unordered_map<string, string> px_http_resources_mapping;//response�з��ص��ļ����Ͷ�Ӧ���ײ����ֶ�

/*==px_http_response==
* ���ڽ����͹���http��Ӧ
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
	size_t read;//�Ѿ���ȡ�Ĵ�С
};

#endif // !PX_HTTP
