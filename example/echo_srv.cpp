
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "dk_core.h"
#include "ucper.pb.h"

#define RESP_DATA_HEADER "HTTP/1.1 200 OK\r\nServer: nginx/0.8.44\r\nContent-length: %d\r\nConnection: keep-alive\r\n\r\n"
#define dlog1 printf

using namespace std;

#pragma pack(1)
typedef struct SrvProtoHeader {
  char      prefix[5];    /* vdc */
  int32_t   data_len;     /* length of gpb data */
  char      reserved[7];  /*  reserved */
  char      data[0];      /*  gpb data */
} SrvProtoHeader;
#pragma pack()

class MyServer;

static MyServer *server;

class SrvConnection: public DKBaseConnection {

  virtual void OnConnect() {
    set_keep_alive(true);
    dlog1("new SrvConnection fd:%d %s:%d\n", fd_, host_.c_str(), port_);
  }
  
  virtual void OnClose() {
    dlog1("SrvConnection close fd:%d %s:%d\n",
        fd_, host_.c_str(), port_); 
  }

  virtual void OnError() {
    dlog1("SrvConnection Error %s\n", this->get_error_string());
  }

  virtual void OnWrite() {
    //dlog1("SrvConnection %s\n", __func__);

  }

  virtual enum READ_STATUS OnRead() {
    SrvProtoHeader *header;
    int        buf_size;
    int        data_len;
    int        total_size;
    struct evbuffer *buf = get_input_buffer();

    buf_size = evbuffer_get_length(buf);
  
    if (buf_size < (int)sizeof(SrvProtoHeader)) {
      dlog1("[error] ParseFromEvbuffer need at least %d bytes but %d bytes\n",
            sizeof(SrvProtoHeader), buf_size);
      return READ_NEED_MORE_DATA;
    }

    total_size = sizeof(SrvProtoHeader) + data_len;
    if (buf_size < total_size) {
      return READ_NEED_MORE_DATA;
    }
  
    header = (SrvProtoHeader *)evbuffer_pullup(buf, sizeof(SrvProtoHeader));
    data_len = ntohl(header->data_len);
    
    ADD add;
    add.ParseFromArray(header->data, data_len);
    add.set_result(add.a() + add.b());

    string reply_buff;
    add.SerializeToString(&reply_buff);

    SrvProtoHeader reply_header;
    memset(&reply_header, 0, sizeof(reply_header));
    reply_header.data_len = htonl(reply_buff.size());

    AddOutputBuffer(&reply_header, sizeof(reply_header));
    AddOutputBuffer(reply_buff);
    StartWrite();

    evbuffer_drain(buf, total_size);
    return READ_ALL_DATA;
  }
};

class MyServer: public DKBaseServer {
  virtual void ClockCallback() {
    //dlog1("conns: %d\n", this->get_conns_map_size()); 
  }

  virtual DKBaseConnection *NewConnection() {
    return new SrvConnection(); 
  }

  virtual void ConnectionMade(DKBaseConnection *conn) {
  }
};

static void thread_func(DKBaseThread *thread, void *arg) {
  printf("threadid:%d %d\n", thread->get_thread_id(), (long)arg);
}

#define PORT 60006

int main(int argc, char **argv) {
  DKThreadPool th_pool;
  server = new MyServer();

  if (!server || !server->Init()) {
    dlog1("new DKBaseServer Init error\n");
    return 1;
  }
  
  dlog1("server start ......\n");

  /*
  th_pool.Init(10);
  for (int i=0; i<10000; i++) {
    th_pool.CallInThread(thread_func, (void *)i);
  }
  */

  signal(SIGPIPE, SIG_IGN);
  signal(SIGHUP, SIG_IGN);

  int port = PORT;
  if (argc > 1) {
    port = atoi(argv[1]);
  }

  int timeout = 600;
  if (argc > 1) {
    timeout = atoi(argv[1]);
  }

  if (!server->StartListenTCP(NULL, port, 1024)) {
    dlog1("StartListenTCP fail (:%d)\n", port);
    return 1;
  }

  server->set_timeout(timeout);
  server->EventLoop();

  delete server;
}
