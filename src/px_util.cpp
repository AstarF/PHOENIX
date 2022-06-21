#include "px_util.h"
#include "px_log.h"
#include "px_error.h"
#include "px_list.h"
#include "px_http.h"
#include "px_resources.h"
#include <string.h>
#include <iostream>
using std::cout;
using std::endl;


int set_file(int fd, int attri) {
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | attri;
	fcntl(fd, F_SETFL, new_option);
	return old_option;
}

//==px_x_www_form==
void px_x_www_form::parse(char* param) {
	string key;
	int ptr = 0;
	size_t sz = strlen(param);
	for (int i = 0; i < sz; ++i) {
		if (param[i] == '=') {
			if (key == "") {
				param[i] = '\0';
				key = param + ptr;
				ptr = i + 1;
			}
		}
		else if (param[i] == '&') {
			param[i] = '\0';
			xwwwform[key] = ptr;
			key = "";
			ptr = i + 1;
		}
		else if (param[i] == '\r')param[i] = '\0';
		else if (param[i] == '\n') {
			param[i] = '\0'; break;
		}
	}
	if (key != "") {
		xwwwform[key] = ptr;
	}
}

//返回尾后地址
int parse_url(const string& url, int begin) {
	string value = "";
	size_t sz = url.size();
	if (begin >= sz)return -1;
	for (int i = begin; i < sz; ++i) {
		if (url[i] == '/') {
			return i;
		}
		else if (url[i] == '?') {
			return i;
		}
		else {
			value += url[i];
		}
	}
	return sz;
}

//返回尾后地址
int parse_url(const char* url, int begin) {
	string value = "";
	size_t sz = strlen(url);
	if (begin >= sz)return -1;
	for (int i = begin; i < sz; ++i) {
		if (url[i] == '/') {
			return i;
		}
		else if (url[i] == '?') {
			return i;
		}
		else {
			value += url[i];
		}
	}
	return sz;
}

#include <iostream>
using std::cout;
using std::endl;
//==px_x_form==
void px_x_form::parse(char* buffer, size_t bufsz, const char* bond) {
	char boundary[64];
	char tailboundary[64];
	memset(boundary, 0, sizeof(boundary));
	memset(tailboundary, 0, sizeof(tailboundary));
	boundary[0] = boundary[1] = '-';
	strcpy(boundary + 2, bond);
	strcpy(tailboundary, boundary);
	int bsz = strlen(boundary);
	tailboundary[bsz] = tailboundary[bsz + 1] = '-';
	int status = 0;//0:boundary,1:name,2:filename,3:filetype,4:data&boundary
	int ptr = 0;
	int strname = -1;
	int strfilename = -1;
	int strtype = -1;
	for (int i = 0; i < bufsz; ++i) {
		//if(buffer[i] == '\r')cout << i << " :\\r" << endl;
		//else if(buffer[i] == '\n')cout << i << " :\\n" << endl;
		//else if(buffer[i] == ' ')cout << i << " :kkk" << endl;
		//else if (buffer[i] == '\0')cout << i << " :00" << endl;
		//else cout << i << " :" << buffer[i] << endl;
		//获取boundary
		if (status == 0) {
			if (buffer[i] == '\r') {
				buffer[i] = '\0';
				continue;
			}
			else if (buffer[i] == '\n') {
				buffer[i] = '\0';
				if (strcmp(buffer + ptr, boundary) == 0) {
					status = 1;
				}
				//cout<<buffer+ptr<<endl;
				ptr = i + 1;
			}
		}
		//获取name
		else if (status == 1) {
			if (buffer[i] == ' ') {
				ptr = i + 1;
			}
			else if (buffer[i] == '=') {
				buffer[i] = '\0';
				if (strcmp(buffer + ptr, "name") == 0) {
					while (buffer[i] != '\"')i++;
					ptr = i + 1;
					i++;
					while (buffer[i] != '\"')i++;
					buffer[i] = '\0';
					strname = ptr;
					if (buffer[i + 1] == ';') status = 2;
					else status = 4;
					while (buffer[i + 1] == '\r' || buffer[i + 1] == '\n')i++;
					ptr = i + 1;
				}
				else buffer[i] = '=';
			}
			else if (buffer[i] == '\r' || buffer[i] == '\n') {
				buffer[i] = '\0';
			}
		}
		//获取filename(如果有)
		else if (status == 2) {
			if (buffer[i] == ' ') {
				ptr = i + 1;
			}
			else if (buffer[i] == '=') {
				buffer[i] = '\0';
				if (strcmp(buffer + ptr, "filename") == 0) {
					while (buffer[i] != '\"')i++;
					ptr = i + 1;
					i++;
					while (buffer[i] != '\"')i++;
					strfilename = ptr;
					buffer[i] = '\0';
					status = 3;
					while (buffer[i + 1] == '\r' || buffer[i + 1] == '\n')i++;
					ptr = i + 1;
				}
				else buffer[i] = '=';
			}
			else if (buffer[i] == '\r' || buffer[i] == '\n') {
				buffer[i] = '\0';
			}
		}
		//获取filetype(如果有)
		else if (status == 3) {
			if (buffer[i] == ' ') {
				buffer[i] = '\0';
				if (strcmp(buffer + ptr, "Content-Type:") == 0) {
					ptr = i + 1;
					i++;
					while (buffer[i] != '\r' && buffer[i] != '\n') {
						i++;
					}
					strtype = ptr;
					buffer[i] = '\0';
					status = 4;
					while (buffer[i + 1] == '\r' || buffer[i + 1] == '\n')i++;
					ptr = i + 1;
				}
				else break;
			}
			else if (buffer[i] == '\r' || buffer[i] == '\n') {
				buffer[i] = '\0';
			}
		}
		//获取数据
		else {
			int beg = i;//数据开始位置
			string strline = "";//这里直接修改buffer会破坏数据
			for (; i < bufsz; ++i) {
				if (buffer[i] == '\r') {
					continue;
				}
				else if (buffer[i] == '\n') {
					if (strcmp(strline.c_str(), boundary) == 0 || strcmp(strline.c_str(), tailboundary) == 0) {
						buffer[ptr] = '\0';
						if (buffer[ptr - 1] == '\n')buffer[ptr - 1] = '\0';
						if (buffer[ptr - 2] == '\r')buffer[ptr - 2] = '\0';
						if (strtype == -1) {//不是文件
							xform[buffer + strname] = { strtype ,strname ,beg,-1,0 };
						}
						else {
							xform[buffer + strname] = { strtype ,strname ,strfilename,beg,ptr - beg };
						}
						status = 1;
						ptr = i + 1;
						break;
					}
					strline = "";
					ptr = i + 1;
				}
				else {
					strline += buffer[i];
				}
			}
		}
	}

	for (auto& t : xform) {
		cout << "item:" << buffer + t.second.name << "-" << buffer + t.second.val << endl;
	}
}