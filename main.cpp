#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ev.h>
#include <ev++.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <exception>
#include <vector>
#include <queue>

#include <string>

#include <thread>
#include <mutex>

#include "http.h"

#define BUFFER_SIZE 1024

class Worker {
private:
    ev::dynamic_loop *loop_;
    std::queue<int> sd_queue;
    std::mutex *sd_queue_mutex;
    ev::async *trigger;
    std::thread *thread;
public:
    Worker() {
        sd_queue_mutex = new std::mutex;
        thread = new std::thread(&Worker::run, this);
    }
    ~Worker() { delete sd_queue_mutex; }

    void run() {
        loop_ = new ev::dynamic_loop(EVFLAG_AUTO);
        trigger = new ev::async(*loop_);
        trigger->set<Worker,&Worker::queue_cb>(this);
        trigger->start();
        loop_->run(0);
    }
    ev::dynamic_loop* loop() { return loop_; }

    void push(int sd) {
        sd_queue_mutex->lock();
        sd_queue.push(sd);
        sd_queue_mutex->unlock();

        trigger->send();
    }

    void queue_cb (ev::async &w, int revents) {
        sd_queue_mutex->lock();

        while(!sd_queue.empty()) {
            auto *w_client = new ev::io(*loop_);
            int &client_sd = sd_queue.front();
            fcntl(client_sd, F_SETFL, fcntl(client_sd, F_GETFL, 0) | O_NONBLOCK);
            // Initialize and start watcher to read client requests
            w_client->set<Worker, &Worker::read_cb> (this);
            w_client->start(client_sd, EV_READ);
            sd_queue.pop();
        }

        sd_queue_mutex->unlock();
    }

    void read_cb (ev::io &w, int revents) {
        char buffer[BUFFER_SIZE];
        ssize_t read;

        if(EV_ERROR & revents)
            throw std::runtime_error("got invalid event");

        // Receive message from client socket
        read = recv(w.fd, buffer, BUFFER_SIZE, 0);

        if(read < 0)
            throw std::runtime_error("read error");

        if(read == 0)
        {
            // Stop and free watchet if client socket is closing
            w.stop();
            delete &w;
            return;
        }
        else
        {
            auto parser = HTTPParser(std::string(buffer, read));
            auto data = parser.reply();
            send(w.fd, data.c_str(), data.length(), 0);

//            printf("message:%s\n",buffer);
        }

        // Send message bach to the client
        //send(w.fd, buffer, read, 0);
    }
};

class ThreadsPool {
private:
    size_t nthreads;
    size_t next_worker;
    std::vector<Worker> workers;
public:
    ThreadsPool(size_t num_thread) : nthreads(num_thread), next_worker(0) {
        workers.resize(nthreads);
    }

    void add_sd(int client_sd) {
        workers[next_worker].push(client_sd);
        next_worker = (next_worker + 1) % nthreads;
    }
};

class Listener {
private:
    ev::io w_accept;
    ev::sig sio;
    ThreadsPool *pool;
public:
    Listener(std::string ip, in_port_t port) {
        pool = new ThreadsPool(4);

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        if (port) {
            addr.sin_port = htons(port);
        } else {
            addr.sin_port = htons(8080);
        }
        if (ip.empty()) {
            addr.sin_addr.s_addr = INADDR_ANY;
        } else {
            inet_aton(ip.c_str(), &addr.sin_addr);
        }

        int lsocket;
        if( (lsocket = socket(PF_INET, SOCK_STREAM, 0)) < 0 )
            throw std::runtime_error("socket error");

        int enable = 1;
        if (setsockopt(lsocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
            throw std::runtime_error("setsockopt(SO_REUSEADDR) failed");

        if (bind(lsocket, (struct sockaddr*) &addr, sizeof(addr)) != 0)
            throw std::runtime_error("bind error");

        fcntl(lsocket, F_SETFL, fcntl(lsocket, F_GETFL, 0) | O_NONBLOCK);

        if (listen(lsocket, SOMAXCONN) < 0)
            throw std::runtime_error("listen error");

        w_accept.set<Listener, &Listener::accept_cb> (this);
        w_accept.start(lsocket, EV_READ);

        sio.set<Listener, &Listener::signal_cb>(this);
        sio.start(SIGINT);
    }
    ~Listener() { delete pool; }

    void signal_cb(ev::sig &signal, int revents) {
        signal.loop.break_loop();
    }

    void accept_cb (ev::io &w, int revents) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        if(EV_ERROR & revents)
            throw std::runtime_error("got invalid event");

        // Accept client request
        int client_sd = accept(w.fd, (struct sockaddr *)&client_addr, &client_len);

        if (client_sd < 0)
            throw std::runtime_error("accept error");

        pool->add_sd(client_sd);
    }
};

int main(int argc, char *argv[]) {
    daemon(0, 0);

    std::string host("0.0.0.0");
    in_port_t port = 8080;
    int c;
    while ((c = getopt (argc, argv, "h:p:d:")) != -1) {
        switch (c)
        {
        case 'h':
            host = std::string(optarg);
            break;
        case 'p':
            port = atoi(optarg);
            break;
        case 'd':
            chdir(optarg);
            break;
        default:
            abort();
        }
    }

    ev::default_loop       loop;
    Listener listener(host, port);

    loop.run(0);
}
