/**
 * author:lijian2@ucweb.com
 * date:2012-03-08
 */

#include "donkey_core.h"

unsigned int DonkeyServer::current_time_;

DonkeyServer::DonkeyServer()
    : base_(NULL),
      listener_(NULL),
      signal_event_(NULL) {
}

DonkeyServer::~DonkeyServer() {
  if (base_)
    event_base_free(base_);
  if (listener_)
    evconnlistener_free(listener_);
  if (signal_event_)
    event_free(signal_event_);
}

bool DonkeyServer::Init() {
  base_ = event_base_new();
  if (!base_)
    return false;
  ClockHandler(0, 0, this);
  return true;
}

bool DonkeyServer::StartListenTCP(const char *address,
                                  unsigned short port,
                                  int backlog) {
  if (!base_)
    return false;

  if (listener_)
    return false;

  memset(&sin_, 0, sizeof(sin_));
	sin_.sin_family = AF_INET;
	sin_.sin_port = htons(port);
  if (address)
    inet_aton(address, &sin_.sin_addr);

	listener_ = evconnlistener_new_bind(base_, ListenerCallback,
      (void *)this, LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE,
      backlog, (struct sockaddr*)&sin_, sizeof(sin_));

	if (!listener_) {
    DK_DEBUG("[error] %s %s\n", __func__, strerror(errno));
		return false;
	}
  
  return true;
}

int DonkeyServer::EventLoop() {
  return event_base_dispatch(base_);
}

bool DonkeyServer::MakeConnection(int fd,
                                  const char *host,
                                  unsigned short port) {
  DonkeyBaseConnection *conn = NULL;  
  if (free_conns_.empty()) {
    conn = NewConnection();
  } else {
    conn = free_conns_.back();
    free_conns_.pop_back();
    DK_DEBUG("%s get free conn %x\n", __func__, conn);
  }

  
  if (!conn) {
    evutil_closesocket(fd);    
    return false;
  }
  
  if (!conn->Init(base_, fd, host, port)) {
    evutil_closesocket(fd); 
    delete conn;
    return false;
  }

  conn->server_ = this;

  ConnectionMade(conn);

  conn->ConnectMade();

  return conn->StartRead();
}

void DonkeyServer::ListenerCallback(struct evconnlistener *listener,
                                    evutil_socket_t fd,
                                    struct sockaddr *sa,
                                    int salen,
                                    void *arg) {
  DonkeyServer *server = (DonkeyServer *)arg;
  assert(server);

  char ntop[NI_MAXHOST];
	char strport[NI_MAXSERV];  
  
  int ret = getnameinfo(sa, salen,
                        ntop, sizeof(ntop), strport, sizeof(strport),
                        NI_NUMERICHOST|NI_NUMERICSERV);
  if (ret != 0)
    perror("getnameinfo");

  if (!server->MakeConnection(fd, ntop, atoi(strport)))
    DK_DEBUG("[error] %s MakeConnection failed\n", __func__);
}

void DonkeyServer::ClockHandler(int fd, short which, void *arg) {
  static struct event clockevent;
  static bool initialized = false;
   
  struct timeval t;
  DonkeyServer *server = (DonkeyServer *)arg;

  if (initialized) {
    /* only delete the event if it's actually there. */
    evtimer_del(&clockevent);
  } else {
    initialized = true;
  }

  struct timeval timer;

  gettimeofday(&timer, NULL);
  current_time_ = (unsigned int)timer.tv_sec;

  server->ClockCallback();

  t.tv_sec = 1;
  t.tv_usec = 0;
  evtimer_set(&clockevent, ClockHandler, arg);
  event_base_set(server->get_base(), &clockevent);
  evtimer_add(&clockevent, &t);
}

void DonkeyServer::AddFreeConn(DonkeyBaseConnection *conn) {
  if (!conn)
    return;

  free_conns_.push_back(conn); 
}

DonkeyBaseConnection *DonkeyServer::NewConnection() {
  return new DonkeyBaseConnection(); 
}
