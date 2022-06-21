#ifndef PX_UTIL
#define PX_UTIL
#include "px_type.h"
#include<fcntl.h>
#include <map>
#include <string>
#include <vector>
using std::map;
using std::string;
using std::vector;

int set_file(int fd, int attri);

class px_x_www_form {
public:
	px_x_www_form() {}
	~px_x_www_form() {}
	void parse(char*);
	map<string, int> xwwwform;
};

struct px_xform_node {
	int type=-1;
	int name = -1;
	int val = -1;//val or filename
	int data = -1;//for file
	int size = -1;//file size
};

class px_x_form {
public:
	px_x_form() {}
	~px_x_form() {}
	void parse(char* buffer, size_t bufsz, const char*);
	map<string, px_xform_node> xform;
};

#endif // !PX_UTIL