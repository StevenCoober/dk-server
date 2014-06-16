#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <vector>

#include "dk_core.h"

using namespace std;

#define dlog1 printf

class MyServer;
class MyConnection;
static MyServer *server;

static void conn_close_handler(MyConnection *c);

class MyConnection: public DKBaseConnection {
public:
  unsigned int last_send_time;
private:
  virtual enum READ_STATUS OnRead() {
    set_keep_alive(true);
    evbuffer_drain(get_input_buffer(), -1);
    return READ_ALL_DATA;
  }

  virtual void OnConnect() {
    //dlog1("new Connection fd:%d %s:%d\n", fd_, host_.c_str(), port_);
  }
  
  virtual void OnClose() {
    dlog1("Connection close fd:%d %s:%d\n",
        fd_, host_.c_str(), port_); 
  }

  virtual void OnError(DKConnectionError error) {
    dlog1("Connection Error %s\n", StrError(error));
  }
};


class MyServer : public DKBaseServer {
public:
  MyServer(const char *host, int port,
           const char *path, int conns,
           int timeout, int interval, int new_c_per_s) {
    host_ = host;
    port_ = port;
    path_ = path;
    conns_ = conns;
    timeout_ = timeout;
    interval_ = interval;
    new_c_per_s_ = new_c_per_s;
    dlog1("host:%s port:%d path:%s conns:%d timeout:%d interval:%d new_c_per_s:%d\n", host, port, path, conns, timeout, interval, new_c_per_s);
  }

  void collect_closed_conn(MyConnection *c) {
    //closed_conns_.push(c);
  }

  void load() {
    if (new_c_per_s_ > conns_)
      new_c_per_s_ = conns_;
    for (int i = 0; i < new_c_per_s_; i++)
      load_one();
  }

  void load_one() {
    if (my_conns_.size() >= conns_)
      return;

    MyConnection *c = new MyConnection();
    if (!c) {
      dlog1("new PSConnection failed\n");
      return;
    }
   
    bool res = c->Init(server->get_base(), -1, host_.c_str(), port_);
    if (!res) {
      delete c;
      c = NULL;
      dlog1("conn_->Init failed\n");
      return;
    }
    
    c->Connect();
    c->set_timeout(timeout_);
    c->set_keep_alive(true);
    my_conns_.push_back(c);
    send_data(c);
  }

  void send_data(MyConnection *c) {
    char data[1024];
    int len = sprintf(data,
        "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: keep-alive\r\n\r\n",
        path_.c_str(), host_.c_str());
    c->Send(data, len);
    c->last_send_time = get_current_time();
  }

  int check_conns() {
    int active_conns = 0;
    int new_conns = 0;
    for (int i = 0; i < my_conns_.size(); i++) {
      MyConnection *c = my_conns_[i];
      if (!c->IsConnected()) {
        if (new_conns < new_c_per_s_) {
          send_data(c);
          new_conns += 1;
        }
      }
      else if (get_current_time() - c->last_send_time > interval_) {
        send_data(c);
      }

      if (c->IsConnected()) {
        active_conns += 1;
      }
    }

    return active_conns; 
  }

  void ClockCallback() {
    load();
    int active_conns = check_conns();
    
    dlog1("total conns %d, established %d\n", my_conns_.size(), active_conns);
  }

private:
  string host_;
  int port_;
  int conns_;
  int timeout_;
  int interval_;
  string path_;
  int new_c_per_s_;
  int active_conn_;
  vector<MyConnection *> my_conns_;
  queue<MyConnection *> closed_conns_;
};

static void conn_close_handler(MyConnection *c) {
  if (!server || !c)
    return;

  server->collect_closed_conn(c);
}

int main(int argc, char **argv) {
  if (argc < 5) {
    printf("tcp_client host port path conns [conn_timeout(default 300s)] [secs_per_req(default 10s)]\n");
    return 1;
  }

  const char *host = argv[1];
  int port = atoi(argv[2]);
  const char *path = argv[3];
  int conns = atoi(argv[4]);

  int timeout = 300;
  if (argc >= 6)
    timeout = atoi(argv[5]);

  int interval = 10;
  if (argc >= 7)
    interval = atoi(argv[6]);

  int new_c_per_s = 100; 
  if (argc >= 8)
    new_c_per_s = atoi(argv[7]); 

  server = new MyServer(host, port, path, conns,
      timeout, interval, new_c_per_s);

  if (!server || !server->Init()) {
    dlog1("new DKBaseServer Init error\n");
    return 1;
  }
  
  dlog1("server start ......\n");

  signal(SIGPIPE, SIG_IGN);
  signal(SIGHUP, SIG_IGN);

  dlog1("begin http_load run ....\n");
  server->load();
  server->EventLoop();

  delete server;
}
