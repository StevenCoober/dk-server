
/*
 * Copyright (C) lijian2@ucweb.com
 */

#include "donkey_common.h"
#include "donkey_worker.h"

int DonkeyWorker::ThreadRoutine() {
  assert(base_);
  int ret = 0;

  for ( ; ; ) {
    ret = event_base_loop(base_, EVLOOP_ONCE);
    if (ret == -1)
      break;
  
    /* no events and pending cb */
    if (ret == 1 && pending_cbs_.empty()) {
      struct timespec ts;
      if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
        ts.tv_sec += 1;

        if (sem_timedwait(&event_sem_, &ts) == -1) {
          if (errno == EINTR) {
            continue;
          } else if (errno == ETIMEDOUT) {
          } else {
            perror("sem_timedwait");
            break;
          }
        }
      }
    }

    for ( ; !pending_cbs_.empty(); ) {
      try {
        DeferredCb deferred_cb = pending_cbs_.pop();  
        deferred_cb.Call();
      } catch (const std::exception e) {
        break;
      }
    }

  }

  return ret;
}
