#include "http-parser.h"


llhttp_settings_t HTTP_Parser::settings = {
    [](llhttp_t * parser) {
        return ((HTTP_Parser *)parser->data)->on_message_begin(parser);
    },
    [](llhttp_t * parser, const char * data, size_t len) {
        return ((HTTP_Parser *)parser->data)->on_url(parser, data, len);
    },
    [](llhttp_t * parser, const char * data, size_t len) {
        return ((HTTP_Parser *)parser->data)->on_status(parser, data, len);
    },
    [](llhttp_t * parser, const char * data, size_t len) {
        return ((HTTP_Parser *)parser->data)->on_header_field(parser, data, len);
    },
    [](llhttp_t * parser, const char * data, size_t len) {
        return ((HTTP_Parser *)parser->data)->on_header_value(parser, data, len);
    },
    [](llhttp_t * parser) {
        return ((HTTP_Parser *)parser->data)->on_headers_complete(parser);
    },
    [](llhttp_t * parser, const char * data, size_t len) {
        return ((HTTP_Parser *)parser->data)->on_body(parser, data, len);
    },
    [](llhttp_t * parser) {
        return ((HTTP_Parser *)parser->data)->on_message_complete(parser);
    }
};

HTTP_Parser::HTTP_Parser(llhttp_type type, parser_callback callbacks) {
    llhttp_init(&parser, type, &HTTP_Parser::settings);
    // set data after llhttp_init, because llhttp_init will call memset to fill zero to memory 
    parser.data = this;
    memset((void *)&callback, 0, sizeof(callback));
    callback = callbacks;
}

int HTTP_Parser::on_message_begin(llhttp_t* parser) {
    major_version = 0;
    minor_version = 0;
    upgrade = 0;
    keepalive = 0;
    parse_start_time = 0;
    header_end_time = 0;
    message_end_time = 0;
    url.clear();
    status.clear();
    keys.clear();
    values.clear();
    body.clear();
    
    parse_start_time = time(NULL);
    return 0;
}

int HTTP_Parser::on_status(llhttp_t* parser, const char* at, size_t length) {
    status.append(at, length);
    return 0;
}

int HTTP_Parser::on_url(llhttp_t* parser, const char* at, size_t length) { 
    url.append(at, length);
    return 0;
}

int HTTP_Parser::on_header_field(llhttp_t* parser, const char* at, size_t length) {   
    keys.push_back(std::string(at, length));
    return 0;
}

int HTTP_Parser::on_header_value(llhttp_t* parser, const char* at, size_t length) {
    values.push_back(std::string(at, length));
    return 0;
}

int HTTP_Parser::on_headers_complete(llhttp_t* parser) {
    header_end_time = time(NULL);
    major_version = parser->http_major;
    minor_version = parser->http_minor;
    upgrade = parser->upgrade;
    keepalive = llhttp_should_keep_alive(parser);
    if (callback.on_headers_complete) {
        on_headers_complete_info info = {
            major_version,
            minor_version,
            upgrade,
            keepalive,
            parse_start_time,
            header_end_time,
            message_end_time,
            url,
            status,
            keys,
            values,
        };
        callback.on_headers_complete(info, callback);
    }
    return 0;
}

int HTTP_Parser::on_body(llhttp_t* parser, const char* at, size_t length) {
    body.append(at, length);
     if (callback.on_body) {
        on_body_info info = {
            std::string(at, length)
        };
        callback.on_body(info, callback);
    }
    return 0;
}

int HTTP_Parser::on_message_complete(llhttp_t* parser) {
    message_end_time = time(NULL);
    if (callback.on_body_complete) {
        on_body_complete_info info = {
            body
        };
        callback.on_body_complete(info, callback);
    }
    
    return 0;
}

int HTTP_Parser::parse(const char* data, int len) {
    enum llhttp_errno err = llhttp_execute(&parser, data, len);

    if (err != HPE_OK) {
        fprintf(stderr, "Parse error: %s %s\n", llhttp_errno_name(err), parser.reason);
        return 1;
    }
    return 0;
}

int HTTP_Parser::finish() {
    enum llhttp_errno err = llhttp_finish(&parser);

    if (err != HPE_OK) {
        fprintf(stderr, "Parse error: %s %s\n", llhttp_errno_name(err), parser.reason);
        return 1;
    }
    return 0;
}

void HTTP_Parser::print() {
    std::cout<<"parse start: "<<ctime(&parse_start_time);
    std::cout<<"url: "<<url<<std::endl;
    std::cout<<"headers: "<<std::endl;
    for (int i = 0; i < keys.size(); i++) {
        std::cout <<"    "<<keys[i]<<": "<<values[i]<< std::endl;
    }
    std::cout<<"header on complete: "<<ctime(&header_end_time);
    std::cout<<"body: "<<body<<std::endl;
    std::cout<<"message on complete: "<<ctime(&message_end_time);
    printf("major_version: %d major_version: %d keppalive: %d upgrade: %d", major_version, minor_version, keepalive, upgrade);
}
