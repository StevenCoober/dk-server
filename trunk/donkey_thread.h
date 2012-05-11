
/*
 * Copyright (C) lijian2@ucweb.com
 */

#ifndef __DONKEY_THREAD__INCLUDE__
#define __DONKEY_THREAD__INCLUDE__

#include "queue.h"
#include "donkey_base_thread.h"

class DonkeyThread;

typedef void (*deferred_cb_fn)(DonkeyThread *worker, void *arg);

class DeferredCb {
public:
  DeferredCb(deferred_cb_fn cb, void *arg)
      : cb_(cb), arg_(arg) {
  }

  void Call(DonkeyThread *worker) {
    if (cb_)
      cb_(worker, arg_);
  }

private:
  deferred_cb_fn cb_;
  void *arg_;
};

class DonkeyThread : public DonkeyBaseThread {
public:
  DonkeyThread() : base_(NULL) {
  }

  bool Init();

  virtual ~DonkeyThread() {
    if (base_)
      event_base_free(base_);
  }

  virtual bool Stop() {
    if (base_)
      return 0 == event_base_loopbreak(base_);
    return false;
  }

  struct event_base *get_base() {
    return base_; 
  }

  bool CallInThread(deferred_cb_fn cb, void *arg) {
    pending_cbs_.push(DeferredCb(cb, arg));
    return Notify();
  }

  virtual int ThreadRoutine();

  void NotifyCb(int fd, short which); 

  static void EventNotifyCb(int fd, short which, void *arg);


  bool Notify() {
    int rv, cnt = 0;
    do {
      rv = write(notify_send_fd_, "", 1);
    } while (rv < 0 && errno == EAGAIN && ++cnt < 100);
    return rv > 0;
  }

protected:
  struct event_base     *base_;
  struct event           notify_event_;
  int                    notify_receive_fd_;
  int                    notify_send_fd_;
  LockQueue<DeferredCb>  pending_cbs_;
};

#endif
