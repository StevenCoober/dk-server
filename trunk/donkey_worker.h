
/*
 * Copyright (C) lijian2@ucweb.com
 */

#ifndef __DONKEY_WORKER__INCLUDE__
#define __DONKEY_WORKER__INCLUDE__

#include "queue.h"
#include "donkey_base_thread.h"
#include "donkey_internal.h"

class DonkeyWorker : public DonkeyBaseThread {
public:
  DonkeyWorker() : stop_(false) {
  }

  bool Init() {
    return 0 == sem_init(&event_sem_, 0, 0); 
  }

  virtual ~DonkeyWorker() {
  }

  virtual bool Stop() {
    stop_ = true;
    return true;
  }

  size_t PendingQueSize() {
    return pending_cbs_.size();
  }

  virtual int ThreadRoutine();

  bool CallInThread(deferred_cb_fn cb, void *arg) {
    pending_cbs_.push(DeferredCb(cb, arg));
    return 0 == sem_post(&event_sem_);
  }

protected:
  bool                   stop_;
  sem_t                  event_sem_;
  LockQueue<DeferredCb>  pending_cbs_;
};

#endif
