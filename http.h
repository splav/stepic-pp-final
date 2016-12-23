#include <string>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

char reply_404[] = "HTTP/1.0 404 Not Found\r\nServer: final/0.1\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";
char reply_200[] = "HTTP/1.0 200 OK\r\nServer: final/0.1\r\nContent-Type: text/html\r\nContent-Length: ";


class HTTPParser {
private:
    std::string path;
public:
    HTTPParser(std::string s) : path("") {

        char *p = const_cast<char*>(s.c_str());
        while (*p++ != ' ');
        char *e = p++;
        while (*e != ' ' && *e != '?') e++;
        *e = '\0';
        path = std::string(p);
    }

    std::string reply() {
        if(!path.empty()) {
            int fd = open(path.c_str(), O_RDONLY);
            if(fd > 0) {
                char buf[1024];
                int len = read(fd, buf, sizeof(buf));
                if(len > 0) {
                    return std::string(reply_200) + std::to_string(len) + "\r\n\r\n" + std::string(buf, len);
                }
            }
        }
        return std::string(reply_404);
    }
};
