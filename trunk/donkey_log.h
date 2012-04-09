/**
 * author:lijian2@ucweb.com
 * date:2012-03-22
 */

#ifndef __DONKEY_LOG_INCLUDE__
#define __DONKEY_LOG_INCLUDE__ 

class DonkeyLog {
public: 
  static void Debug(const char *format, ...) {
    va_list va;
    va_start(va, format);
    vfprintf(stderr, format, va);
    va_end(va);
  }
};

#ifdef _DONKEY_DEBUG
#define DK_DEBUG(args...) DonkeyLog::Debug(args)
#else
#define DK_DEBUG(args...)
#endif

#endif
