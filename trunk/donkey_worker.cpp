
/*
 * Copyright (C) lijian2@ucweb.com
 */

#include "donkey_common.h"
#include "donkey_worker.h"

int DonkeyWorker::ThreadRoutine() {
  int ret = 0;

  for ( ; !stop_ ; ) {
  
    /* no events and pending cb */
    if (pending_cbs_.empty()) {
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
