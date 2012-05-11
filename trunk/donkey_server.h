/**
 * author:lijian2@ucweb.com
 * date:2012-03-08
 */

#ifndef __DONKEY_SERVER_INCLUDE__
#define __DONKEY_SERVER_INCLUDE__

#include "donkey_common.h"
#include "donkey_thread.h"

using namespace std;

struct event_base;
struct evconnlistener;
struct event;
struct sockaddr_in;

class DonkeyBaseConnection;

class DonkeyServer : public DonkeyThread {
public:
  DonkeyServer(); 
  virtual ~DonkeyServer();

  bool Init();

  bool StartListenTCP(const char *address,
                      unsigned short port,
                      int backlog);
 
  int EventLoop();

  bool MakeConnection(int fd, const char *host, unsigned short port);  

  void FreeConn(DonkeyBaseConnection *conn);

  /*
  struct event_base *get_base() {
    return base_; 
  }
  */

  unsigned int get_current_time() {
    return current_time_;
  }

protected:
  virtual void ClockCallback() {
  }

  virtual DonkeyBaseConnection *NewConnection();

  virtual void ConnectionMade(DonkeyBaseConnection *conn) {
  }

private:
  static void ListenerCallback(struct evconnlistener *listener,
                               int fd,
                               struct sockaddr *sa,
                               int socklen,
                               void *arg);

  static void ClockHandler(int fd, short which, void *arg);

private:
  /*struct event_base             *base_;*/
	struct evconnlistener         *listener_;
	struct event                  *signal_event_;

	struct sockaddr_in             sin_;
  vector<DonkeyBaseConnection *> free_conns_;
  static unsigned int            current_time_;
};

#endif
