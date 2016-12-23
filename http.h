#include <string>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

char reply_404[] = "HTTP/1.0 404 Not Found\r\nConnection: close\r\nServer: final/0.1\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";
char reply_200[] = "HTTP/1.0 200 OK\r\nConnection: close\r\nServer: final/0.1\r\nContent-Type: text/html\r\nContent-Length: ";


class HTTPParser {
private:
    std::string path;
    int log;
public:
    HTTPParser(std::string s) : path("") {

        log = open("/home/box/log", O_CREAT | O_WRONLY | O_APPEND, 0666);
        write(log, s.c_str(), s.length());

        close(log);
        char *p = const_cast<char*>(s.c_str());
        while (*p++ != ' ');
        char *e = p++;
        while (*e != ' ' && *e != '?') e++;
        *e = '\0';
        path = std::string(p);
    }
    ~HTTPParser() {
        close(log);
    }

    std::string reply() {
        if(!path.empty()) {
            int fd = open(path.c_str(), O_RDONLY);
            if(fd > 0) {
                write(log, "opened\n", 7);
                char buf[1024];
                int len = read(fd, buf, sizeof(buf));
                if(len > 0) {
                    write(log, "read\n", 5);
                    return std::string(reply_200) + std::to_string(len) + "\r\n\r\n" + std::string(buf, len);
                }
                close(fd);
            }
        }
        return std::string(reply_404);
    }
};
