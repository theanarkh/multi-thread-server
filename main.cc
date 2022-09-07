#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread> 
#include <deque>
#include <iostream>
#include <mutex>
#include <condition_variable>

#include "http-parser.h"

const char * res_400 = "HTTP/1.1 400 Bad Request\r\n\r\n\r\n\r\n\r\n";
const int MAX_THREAD = 10;

void worker(std::mutex *mutex,
            std::condition_variable *condition_variable,
            std::deque<int> *requests) {
    int connfd;
    while (true) {
        {
            std::unique_lock<std::mutex> lock(*mutex);
            while ((*requests).size() == 0)
            {
                (*condition_variable).wait(lock);
            }
            connfd = (*requests).front();
            (*requests).pop_front();
        }
        // std::cout<<std::this_thread::get_id()<<std::endl;
        parser_callback callback = {
            &connfd,
            [](on_headers_complete_info info, parser_callback callback) {
                // printf("on_headers_complete\n");
            },

            [](on_body_info info, parser_callback callback) {
                // printf("on_body\n");
            },

            [](on_body_complete_info info, parser_callback callback) {
                int* connfd = (int *)callback.data;
                // printf("on_body_complete\n");
                const char * data = "HTTP/1.1 200 OK\r\nServer: multi-thread-server\r\ncontent-length: 11\r\n\r\nhello:world\r\n\r\n";
                write(*connfd, data, strlen(data));
                close(*connfd);
            },
        };
        
        HTTP_Parser parser(HTTP_REQUEST, callback);
        char buf[4096];
        int ret;
        while (1) {
            memset(buf, 0, sizeof(buf));
            int error = 0;
            ret = read(connfd, buf, sizeof(buf)); 
            if (ret < 0) {
                error = ret;
            } else if (ret == 0) { 
                int finish_code = parser.finish();
                if (finish_code) {
                   error = finish_code;
                }
            } else {
                int parse_code = parser.parse(buf, ret);
                if (parse_code) {
                    error = parse_code;
                }
            }
            if (error) {
                write(connfd, res_400, strlen(res_400));
                close(connfd);
                break;
            }
            // parser.print();
        } 
    }
}

int main() 
{ 
    int server_fd;
    struct sockaddr_in server_addr;

    std::thread threads[MAX_THREAD];
    std::condition_variable condition_variable;
    std::deque<int> requests;
    std::mutex mutex;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int on = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    if (server_fd < 0) { 
        perror("create socket error"); 
        goto EXIT;
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family       = AF_INET;
    server_addr.sin_port         = htons(8888);
    server_addr.sin_addr.s_addr  = htonl(INADDR_ANY);
    
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) { 
        perror("bind address error"); 
        goto EXIT;
    }

    if (listen(server_fd, 511) < 0) { 
        perror("listen port error"); 
        goto EXIT;
    }
    
    for (int i = 0; i < MAX_THREAD; i++) {
        threads[i] = std::thread(worker, &mutex, &condition_variable, &requests);
    }

    while(1) {
        int connfd = accept(server_fd, nullptr, nullptr);
        if (connfd < 0) 
        { 
            perror("accept error"); 
            continue;
        }
        {
            std::lock_guard<std::mutex> lock(mutex);
            requests.push_back(connfd);
            condition_variable.notify_one();
        }
    }

    close(server_fd);

    for (int i = 0; i < MAX_THREAD; i++) {
        threads[i].join();
    }

    return 0;
EXIT:
    exit(1); 
} 