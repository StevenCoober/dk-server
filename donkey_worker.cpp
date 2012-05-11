
/*
 * Copyright (C) lijian2@ucweb.com
 */

#include "donkey_common.h"
#include "donkey_worker.h"

bool DonkeyWorker::Init() {
  base_ = event_base_new();
  if (!base_)
    return false;
  
  int fds[2];
  if (pipe(fds)) {
      perror("Can't create notify pipe");
      return false;
  }

  notify_receive_fd_ = fds[0];
  notify_send_fd_ = fds[1];
  evutil_make_socket_nonblocking(notify_receive_fd_);
  evutil_make_socket_nonblocking(notify_send_fd_);

  event_assign(&notify_event_, base_, notify_receive_fd_ ,
               EV_READ | EV_PERSIST, EventNotifyCb, this);
  if (event_add(&notify_event_, 0) == -1)
    return false;

  return true;
}

int DonkeyWorker::ThreadRoutine() {
  return event_base_dispatch(base_);
}

void DonkeyWorker::EventNotifyCb(int fd, short which, void *arg) {
  DonkeyWorker *worker = (DonkeyWorker *)arg;
  assert(worker);
  
  worker->NotifyCb(fd, which);
}

void DonkeyWorker::NotifyCb(int fd, short which) {
  assert(base_);
  char buf[1024];

  while (read(fd, buf, sizeof(buf)) > 0)
    ;

  for ( ; !pending_cbs_.empty(); ) {
    try {
      DeferredCb deferred_cb = pending_cbs_.pop();  
      deferred_cb.Call(this);
    } catch (const std::exception e) {
      break;
    }
  }

}
