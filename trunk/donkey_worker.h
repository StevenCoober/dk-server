
/*
 * Copyright (C) lijian2@ucweb.com
 */

#ifndef __DONKEY_WORKER__INCLUDE__
#define __DONKEY_WORKER__INCLUDE__

#include "queue.h"
#include "donkey_base_thread.h"

typedef void (*deferred_cb_fn)(void *arg);

class DeferredCb {
public:
  DeferredCb(deferred_cb_fn cb, void *arg)
      : cb_(cb), arg_(arg) {
  }

  void Call() {
    if (cb_)
      cb_(arg_);
  }

private:
  deferred_cb_fn cb_;
  void *arg_;
};

class DonkeyWorker : public DonkeyBaseThread {
public:
  DonkeyWorker() : base_(NULL) {
  }

  bool Init() {
    base_ = event_base_new();
    if (!base_)
      return false;
    
    return 0 == sem_init(&event_sem_, 0, 0); 
  }

  virtual ~DonkeyWorker() {
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

  virtual int ThreadRoutine();

  bool CallInThread(deferred_cb_fn cb, void *arg) {
    pending_cbs_.push(DeferredCb(cb, arg));
    return 0 == sem_post(&event_sem_);
  }

protected:
  struct event_base     *base_;
  sem_t                  event_sem_;
  LockQueue<DeferredCb>  pending_cbs_;
};

#endif
