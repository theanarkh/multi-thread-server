#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <ctime>
#include "http/llhttp.h"

#define MAX_LEN 2048

struct on_headers_complete_info {
    unsigned char major_version;
    unsigned char minor_version;
    unsigned char upgrade;
    unsigned char keepalive;
    time_t parse_start_time;
    time_t header_end_time;
    time_t message_end_time;
    std::string url;
    std::string status;
    std::vector<std::string> keys;
    std::vector<std::string> values;
};

struct on_body_complete_info {
    std::string body;
};

struct on_body_info {
    std::string body;
};

struct parser_callback;

typedef void (*p_on_headers_complete)(on_headers_complete_info, parser_callback);
typedef void (*p_on_body_complete)(on_body_complete_info, parser_callback);
typedef void (*p_on_body)(on_body_info, parser_callback);

struct parser_callback {
    void * data;
    p_on_headers_complete on_headers_complete;
    p_on_body on_body;
    p_on_body_complete on_body_complete;
};

class HTTP_Parser {
    public:
        HTTP_Parser(llhttp_type type, parser_callback callbacks = {});
    
        int on_message_begin(llhttp_t* parser);
        int on_status(llhttp_t* parser, const char* at, size_t length);
        int on_url(llhttp_t* parser, const char* at, size_t length);
        int on_header_field(llhttp_t* parser, const char* at, size_t length);
        int on_header_value(llhttp_t* parser, const char* at, size_t length);
        int on_headers_complete(llhttp_t* parser);
        int on_body(llhttp_t* parser, const char* at, size_t length);
        int on_message_complete(llhttp_t* parser);
        int parse(const char* data, int len);
        int finish();
        void print();
    private: 
        unsigned char major_version;
        unsigned char minor_version;
        unsigned char upgrade;
        unsigned char keepalive;
        time_t parse_start_time;
        time_t header_end_time;
        time_t message_end_time;
        std::string url;
        std::string status;
        std::vector<std::string> keys;
        std::vector<std::string> values;
        std::string body;
        llhttp_t parser;
        parser_callback callback;
        static llhttp_settings_t settings;
};
