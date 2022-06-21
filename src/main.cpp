#include "px_http_server.h"
#include "px_connections.h"
#include<arpa/inet.h>
#include "px_http.h"
#include "px_module.h"
#include <iostream>
#include <string.h>
#include <string>
#include <unistd.h>
using namespace std;

struct justinclude {
    void parse(const MYSQL_ROW& row) {}
};

struct entity_image_count {
    int data = 0;
    void parse(const MYSQL_ROW& row) {
        data = atoi(row[0]);
    }
};

struct entity_image {
    string id;
    string name;
    string path;
    string create_time;
    string update_time;
    void parse(const MYSQL_ROW& row) {
        id = row[0];
        name = row[1];
        path = row[2];
        create_time = row[3];
        update_time = row[4];
    }
};

struct entity_image_tojson {
    char data[512];
    void parse(const MYSQL_ROW& row) {
        memset(data, 0, sizeof(data));
        sprintf(data, "{\"id\":\"%s\",\"name\":\"%s\",\"path\":\"%s\",\"create_time\":\"%s\",\"update_time\":\"%s\"}", row[0], row[1], row[2], row[3], row[4]);
    }
};

template<typename T>
string restojson(const sqlres<T>& sqlstr) {
    string res = "{\"data\":[";
    for (const auto& t : sqlstr.list) {
        res += t.data;
        res += ",";
    }
    res[res.length() - 1] = ']';
    res += ",\"total\":";
    res += to_string(sqlstr.total);
    res += "}";
    return res;
}

//检测是否存在session_id
bool check_session(px_mysql_connect* msconn, const char* cook) {
    if (cook == nullptr)return false;
    char buffer[128];
    string cookie = string(cook);
    string session_id = cookie.substr(cookie.find_first_of("=") + 1);
    sprintf(buffer, "SELECT session_id FROM px_session WHERE session_id=%s;", session_id.c_str());
    sqlres<justinclude> queryres;
    msconn->execute_sql(buffer, queryres);
    if (queryres.total > 0)return true;
    return false;
}

//回调函数
void getimages(px_mysql_connect* msconn, px_http_request* req, px_http_response_data* res) {
    //if (check_session(msconn, req->get_attr("Cookie"))) {
    char buffer[128];
    if (req->url.get_xwwwform_data("page") && req->url.get_xwwwform_data("page_size")) {
        int page = stoi(req->url.get_xwwwform_data("page"));
        int page_sz = stoi(req->url.get_xwwwform_data("page_size"));
        int ret = sprintf(buffer, "SELECT * FROM px_images WHERE name not like 'background_%s' LIMIT %d,%d;", "%", page * page_sz, page_sz);
    }
    else strcpy(buffer, "SELECT * FROM px_images WHERE name not like 'background_%';");
    sqlres<entity_image_tojson> queryres;
    msconn->execute_sql(buffer, queryres);
    (*res)(restojson(queryres), px_http_response_type::JSON);
    //}
}

void getbackimages(px_mysql_connect* msconn, px_http_request* req, px_http_response_data* res) {
    //if (check_session(msconn, req->get_attr("Cookie"))) {
    char buffer[128];
    if (req->url.get_xwwwform_data("page") && req->url.get_xwwwform_data("page_size")) {
        int page = stoi(req->url.get_xwwwform_data("page"));
        int page_sz = stoi(req->url.get_xwwwform_data("page_size"));
        int ret = sprintf(buffer, "SELECT * FROM px_images WHERE name like 'background_%s' LIMIT %d,%d;", "%", page * page_sz, page_sz);
    }
    else strcpy(buffer, "SELECT * FROM px_images WHERE name like 'background_%';");
    sqlres<entity_image_tojson> queryres;
    msconn->execute_sql(buffer, queryres);
    (*res)(restojson(queryres), px_http_response_type::JSON);
    //}
}

void searchimage(px_mysql_connect* msconn, px_http_request* req, px_http_response_data* res) {
    //if (check_session(msconn, req->get_attr("Cookie"))) {
    char buffer[128];
    if (req->url.get_xwwwform_data("name")) {
        string name = req->url.get_xwwwform_data("name");
        int ret = sprintf(buffer, "SELECT * FROM px_images WHERE name = '%s';", name.c_str());
        sqlres<entity_image_tojson> queryres;
        msconn->execute_sql(buffer, queryres);
        (*res)(restojson(queryres), px_http_response_type::JSON);
    }
    //}
}
string wwwpath = "../www/";

void addimage(px_mysql_connect* msconn, px_http_request* req, px_http_response_data* res) {
    bool ret = false;
    if (check_session(msconn, req->get_attr("Cookie"))) {
        if (strcmp(req->content.get_xform_type("image"), "image/jpeg") == 0) {
            char buffer[128];
            memset(buffer, 0, sizeof(buffer));
            time_t tm = time(0);
            unsigned long long num = tm << 10;
            num += px_random_object::getinstance().getrandom();
            string name = "img_" + to_string(num) + ".jpg";
            string path = "images/" + name;
            FILE* fd = fopen((wwwpath + path).c_str(), "wb");
            fwrite(req->content.get_xform_data("image"), req->content.get_xform_size("image"), 1, fd);
            fclose(fd);
            sprintf(buffer, "INSERT INTO px_images (name,path) VALUES ('%s','%s');", name.c_str(), path.c_str());
            ret = msconn->execute_sql(buffer);
        }
    }
    if (ret)(*res)("success! ", px_http_response_type::TEXT);
    else (*res)("fail! ", px_http_response_type::ERROR);
}

void delimage(px_mysql_connect* msconn, px_http_request* req, px_http_response_data* res) {
    bool ret = false;
    if (check_session(msconn, req->get_attr("Cookie"))) {
        if (req->content.get_xform_name("image_id") != nullptr) {
            char buffer[128];
            memset(buffer, 0, sizeof(buffer));
            //查找在不在
            sprintf(buffer, "SELECT * FROM px_images WHERE id = %s;", req->content.get_xform_val("image_id"));
            sqlres<entity_image> queryres;
            msconn->execute_sql(buffer, queryres);
            if (queryres.list.size() > 0) {
                sprintf(buffer, "DELETE FROM px_images WHERE id = %s;", req->content.get_xform_val("image_id"));
                ret = msconn->execute_sql(buffer);
                if (ret && 0 == access((wwwpath + queryres.list[0].path).c_str(), F_OK)) {
                    remove((wwwpath + queryres.list[0].path).c_str());
                }
                else ret = false;
            }
            else ret = false;
        }
    }
    if (ret)(*res)("success! ", px_http_response_type::TEXT);
    else (*res)("fail! ", px_http_response_type::ERROR);
}

void updateimage(px_mysql_connect* msconn, px_http_request* req, px_http_response_data* res) {
    bool ret = false;
    if (check_session(msconn, req->get_attr("Cookie"))) {
        if (req->content.get_xform_name("image_id") != nullptr) {
            char buffer[128];
            memset(buffer, 0, sizeof(buffer));
            //查找在不在
            sprintf(buffer, "SELECT * FROM px_images WHERE id = %s;", req->content.get_xform_val("image_id"));
            sqlres<entity_image> queryres;
            msconn->execute_sql(buffer, queryres);
            if (queryres.list.size() > 0) {
                string path = "images/";
                path += req->content.get_xform_val("image_name");
                sprintf(buffer, "UPDATE px_images SET name = '%s' ,path = '%s' WHERE id = %s;",
                    req->content.get_xform_val("image_name"), path.c_str(), req->content.get_xform_val("image_id"));
                path = wwwpath+ path;
                ret = msconn->execute_sql(buffer);
                if (ret && 0 == access((wwwpath + queryres.list[0].path).c_str(), F_OK)) {
                    rename((wwwpath + queryres.list[0].path).c_str(), path.c_str());
                }
                else ret = false;
            }
            else ret = false;
        }
    }
    if (ret)(*res)("success! ", px_http_response_type::TEXT);
    else (*res)("fail! ", px_http_response_type::ERROR);
}

/*
* 超时清空链接
*/
//int ind = 0;
//
//void testpost(px_mysql_connect* msconn, px_http_request* req, px_http_response_data* res) {
//    if (strcmp(req->content.get_xform_type("file"), "text/plain") == 0) {
//        string path = "../www/temp/" + to_string(px_random_object::getinstance().getrandom()) + "_text.txt";
//        FILE* fd = fopen(path.c_str(), "w");
//        fwrite(req->content.get_xform_data("file"), req->content.get_xform_size("file"), 1, fd);
//        fclose(fd);
//    }
//    else if (strcmp(req->content.get_xform_type("file"), "image/jpeg") == 0) {
//        cout << "size:" << req->content.get_xform_size("file") << endl;
//        string path = "../www/temp/" + to_string(px_random_object::getinstance().getrandom()) + "_img.jpg";
//        FILE* fd = fopen(path.c_str(), "wb");
//        fwrite(req->content.get_xform_data("file"), req->content.get_xform_size("file"), 1, fd);
//        fclose(fd);
//    }
//
//    (*res)(to_string(ind) + "yes there is nothing! " + req->url.url, px_http_response_type::TEXT);
//}

void loadindex(px_mysql_connect* msconn, px_http_request* req, px_http_response_data* res) {
    (*res)(wwwpath+"index.html", px_http_response_type::HTML);
}

void login(px_mysql_connect* msconn, px_http_request* req, px_http_response_data* res) {
    bool ret = false;
    char buffer[128];
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "SELECT user_name FROM px_user WHERE user_name = '%s' AND password = '%s';", req->content.get_xform_val("username"), req->content.get_xform_val("password"));
    sqlres<justinclude> queryres;
    msconn->execute_sql(buffer, queryres);
    string session_id = "fail";
    if (queryres.list.size() > 0) {
        memset(buffer, 0, sizeof(buffer));
        session_id = to_string(generate_sessionid(ntohl(((sockaddr_in*)&(req->conn->address))->sin_addr.s_addr)));
        sprintf(buffer, "INSERT INTO px_session(session_id,ipv4_address)VALUES (%s,'%s');", session_id.c_str(), inet_ntoa(((sockaddr_in*)&(req->conn->address))->sin_addr));
        if (msconn->execute_sql(buffer))ret = true;
    }
    if (ret)(*res)(session_id, px_http_response_type::TEXT);
    else (*res)(session_id, px_http_response_type::ERROR);
}


void* timeout_clear_func(px_event* evt, px_thread* pxthread, px_module* pxmodule, void* arg) {
    px_http_resources::get_instance()->mysql_signal->down();
    px_mysql_connect* msconn = px_http_resources::get_instance()->mysqlconn_pool->pop_front();
    char buffer[128];
    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, "DELETE FROM px_session WHERE create_time < date_add(now(), interval -4 hour);");
    sqlres<justinclude> queryres;
    msconn->execute_sql(buffer);
    px_http_resources::get_instance()->mysqlconn_pool->push_front(msconn);
    px_http_resources::get_instance()->mysql_signal->up();
    return nullptr;
}

int main(int argc, char** argv) {
    clock_t start = clock();
    px_http_server serv;
    if (argc > 2) {
        serv.initsocket(atoi(argv[1]), argv[2]);
        wwwpath = argv[2];
    }
    else if (argc > 1) {
        serv.initsocket(atoi(argv[1]));
    }
    else 	serv.initsocket();
    px_http_module* interface_module = serv.interface_module;
    //在这里定义自己的接口
    px_http_module* image_module = serv.create_module("image");
    image_module->add_interface("imagelist", getimages, "get");
    image_module->add_interface("backgroundimagelist", getbackimages, "get");
    image_module->add_interface("searchimage", searchimage, "get");
    image_module->add_interface("addimage", addimage, "post");
    image_module->add_interface("delimage", delimage, "post");
    image_module->add_interface("updateimage", updateimage, "post");
    //image_module->add_interface("test", testpost, "post");

    interface_module->add_module(image_module);

    interface_module->add_interface("main", loadindex, "get");
    interface_module->add_interface("about", loadindex, "get");
    interface_module->add_interface("login", loadindex, "get");
    interface_module->add_interface("control", loadindex, "get");
    interface_module->add_interface("userlogin", login, "post");

    px_module* timeout_service = serv.create_time_event("http timout events", timeout_clear_func, 200000, 0, true, false);
    serv.time_module->add_module(timeout_service);

    serv.run_service();
    clock_t finish = clock();
    if (argc > 1)printf("running time: %ld  %d\n", (finish - start), atoi(argv[1]));
    else printf("running time: %ld \n", (finish - start));
}


////定义一个方法
//void print_hello(px_mysql_connect* msconn, px_http_request* req, px_http_response_data* res) {
//    (*res)("success! ", px_http_response_type::TEXT);
//}
//
//int main(int argc, char** argv) {
//    px_http_server serv;
//    serv.initsocket();
//    //px_http_module* interface_module = serv.interface_module;
//    ////在这里定义自己的业务代码
//    //px_http_module* test_module = serv.create_module("test");
//    //test_module->add_interface("print", print_hello, "get");
//    //serv.interface_module->add_module(test_module);
//
//    px_http_module* interface_module = serv.interface_module;
//    //创建一个模块
//    m_response_config* config = new m_response_config;
//    config->func = print_hello;
//    config->method = "get";
//    px_module* http_test_process = new px_module;
//    http_test_process->set_name("test_process");
//    http_test_process->top_level = false;
//    http_test_process->m_type = pxmodule_type::PROCESS;
//    http_test_process->callback_func = http_response_json_func;//http_response_json_func是预定义好的方法，它做了许多工作......它的取名有点奇怪，事实上它不止能返回json格式的数据，但暂时就这样吧
//    http_test_process->priority = 0;
//    http_test_process->config = (void*)config;
//    interface_module->mod->add_dispath("print2", http_test_process);
//
//    serv.run_service();
//}