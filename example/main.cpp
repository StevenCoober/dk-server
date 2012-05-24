
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "donkey_core.h"
#include "donkey_common.h"
#include "main.h"

//#undef dlog1
//#define dlog1

using namespace std;

class MyServer;

static MyServer *server;
static DonkeyWorker *worker;
static DonkeyEventThread *ev_thread;

class MyHttpRequest: public DonkeyHttpRequest {
  virtual void HandleResponse(struct evhttp_request *req) {
    dlog1("MyHttpRequest HandleResponse\n"); 
    DonkeyHttpRequest::DebugResponse(req);
    delete this;
  }
};


class PSConnection: public DonkeyBaseConnection {
  static void DoInWorkerThread(DonkeyBaseThread *th, void *arg) {
    dlog1("PSConnection::%s\n", __func__);
  }

  virtual enum READ_STATUS RecvData() {
    char *resp = (char *)evbuffer_pullup(get_input_buffer(), -1);
    dlog1(">>>>>>>>>>>>>>\n");
    dlog1("PSConnection::%s: %s\n", __func__, resp);
    dlog1("<<<<<<<<<<<<<<\n");
    evbuffer_drain(get_input_buffer(), -1); 
    worker->CallInThread(DoInWorkerThread, this);
  }
};

class SrvConnection: public DonkeyBaseConnection {
  static void PrintInfo(DonkeyBaseThread *th, void *arg) {
    //cout << (const char *)arg << endl;
  }

  virtual void ConnectedCallback() {
    dlog1("new SrvConnection fd:%d %s:%d\n", fd_, host_.c_str(), port_);
  }
  
  virtual void CloseCallback() {
    dlog1("SrvConnection close fd:%d %s:%d\n",
        fd_, host_.c_str(), port_); 
  }

  virtual void ErrorCallback() {
    dlog1("SrvConnection Error %s\n", this->get_error_string());
  }

  virtual void WriteCallback() {
    worker->CallInThread(PrintInfo, (void *)"WorkerThread WriteCallback");
    ev_thread->CallInThread(PrintInfo, (void *)"EvThread WriteCallback");
    dlog1("SrvConnection %s\n", __func__);

    //visit back server http server
    //VisitTCPServer();
    //VisitHttpServer();
  }

  virtual enum READ_STATUS RecvData() {
    struct evbuffer *buf = get_input_buffer();
    size_t rbuf_size = evbuffer_get_length(buf);
    
    dlog1("RecvData %d\n", rbuf_size);
    /* 
    size_t total_size = 8;

    if (rbuf_size < total_size) {
      dlog1("READ_NEED_MORE_DATA\n");
      return READ_NEED_MORE_DATA;
    }
    */

    /* reply what you send */
    //AddOutputBuffer(buf); /* buf will be drained hear */
    //StartWrite();
    if (!Send(buf))
      printf("Send error\n");
    
    set_keep_alive(false);
    /*  buf already drained in AddOutputBuffer 
    evbuffer_drain(buf, total_size);
    */
    return READ_ALL_DATA;
  }

  void VisitTCPServer() {
    dlog1("VisitTCPServer\n");

    if (!ps_conn_) {
      ps_conn_ = new PSConnection();
      if (!ps_conn_) {
        dlog1("new PSConnection failed\n");
        return;
      }
      
      bool res = ps_conn_->Init(get_base(), -1, BACK_HOST, BACK_PORT);
      if (!res) {
        delete ps_conn_;
        ps_conn_ = NULL;
        dlog1("ps_conn_->Init failed\n");
        return;
      }
    }
 
    ps_conn_->set_keep_alive(true);
    ps_conn_->Send("GET / HTTP/1.1\r\nConnection: keep-alive\r\n\r\n");
  }

  bool VisitHttpServer() {
    if (!s_http_client_) {
      s_http_client_ = new DonkeyHttpClient();
      if (!s_http_client_)
        return false;
    
      if (!s_http_client_->Init(get_base(), HTTP_HOST, HTTP_PORT))
        return false;
    }

    MyHttpRequest *req = new MyHttpRequest();
    if (!req || !req->Init())
      return false;

    req->AddHeader("Host", HTTP_HOST);
    req->AddHeader("Connection", "keep-alive");

    int post_data_len = strlen(HTTP_POST_DATA);
    char temp[16];
		sprintf(temp, "%d", post_data_len);
    
    req->AddHeader("Content-length", temp);
    req->AddPostData((void *)HTTP_POST_DATA, post_data_len);

    s_http_client_->SendRequest(req, EVHTTP_REQ_POST, "/index.html");
  }

  static PSConnection * ps_conn_;
  static DonkeyHttpClient * s_http_client_;
};

PSConnection * SrvConnection::ps_conn_ = NULL;
DonkeyHttpClient * SrvConnection::s_http_client_ = NULL;

class MyServer: public DonkeyServer {
  virtual void ClockCallback() {
  }

  virtual DonkeyBaseConnection *NewConnection() {
    return new SrvConnection(); 
  }

  virtual void ConnectionMade(DonkeyBaseConnection *conn) {
  }
};

static void hello_in_mainthread(DonkeyBaseThread *th, void *arg) {
  cout << "hello, i'm in main thread id: " << pthread_self() << endl;
}

static void hello_in_workerthread(DonkeyBaseThread *th, void *arg) {
  MyServer *server = (MyServer *)arg; 

  cout << "hello, i'm in worker thread id: " << pthread_self() << endl;
  if (server)
    server->CallInThread(hello_in_mainthread, NULL);
}

static void hello_in_evthread(DonkeyBaseThread *th, void *arg) {
  cout << "hello, i'm in event thread id: " << pthread_self() << endl;
}

int main(int argc, char **argv) {
  server = new MyServer();
  worker = new DonkeyWorker();  
  ev_thread = new DonkeyEventThread();

  if (!worker || !worker->Init() || !server || !server->Init() ||
      !ev_thread || !ev_thread->Init()) {
    dlog1("new DonkeyServer Init error\n");
    return 1;
  }
  
  dlog1("server start ......\n");

  signal(SIGPIPE, SIG_IGN);
  signal(SIGHUP, SIG_IGN);

  if (!server->StartListenTCP(NULL, PORT, 1024))
    return 1;

  //start a worker thread
  worker->Create();
  ev_thread->Create();
  for (int i = 0; i < 10; i++) 
    worker->CallInThread(hello_in_workerthread, server);

  for (int i = 0; i < 10; i++)
    ev_thread->CallInThread(hello_in_evthread, server);

  server->EventLoop();

  delete server;
}
