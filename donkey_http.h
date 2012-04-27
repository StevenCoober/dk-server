/**
 * author:lijian2@ucweb.com
 * date:2012-03
 */

#ifndef __DONKEY_HTTP_INCLUDE__
#define __DONKEY_HTTP_INCLUDE__

#include "donkey_common.h"

class DonkeyHttpClient {
public: 
  DonkeyHttpClient() : http_conn_(NULL) {
  }
  
  virtual ~DonkeyHttpClient() {
    if (http_conn_)
      free(http_conn_);
  }

  bool Init(struct event_base *evbase, const char *host, unsigned short port); 

  struct evhttp_connection *get_http_conn() {
    return http_conn_;
  }

  /* auto free after response by libevent */
  struct evhttp_request *NewRequest() {
    return evhttp_request_new(EventHttpRequestCb, (void *)this);  
  }
  
  bool AddHeader(struct evhttp_request *req,
                 const char *key,
                 const char *value) {
    return 0 == evhttp_add_header(evhttp_request_get_output_headers(req),
      key, value);  
  }
  
  bool AddPostData(struct evhttp_request *req, const void *data, size_t len) {
    return 0 == evbuffer_add(evhttp_request_get_output_buffer(req), data, len); 
  }

  /* defined in <event2/http.h>
   * enum evhttp_cmd_type {
        EVHTTP_REQ_GET     = 1 << 0,
        EVHTTP_REQ_POST    = 1 << 1,
        EVHTTP_REQ_HEAD    = 1 << 2,
        EVHTTP_REQ_PUT     = 1 << 3,
        EVHTTP_REQ_DELETE  = 1 << 4,
        EVHTTP_REQ_OPTIONS = 1 << 5,
        EVHTTP_REQ_TRACE   = 1 << 6,
        EVHTTP_REQ_CONNECT = 1 << 7,
        EVHTTP_REQ_PATCH   = 1 << 8
    };
  */
  bool SendRequest(struct evhttp_request *req,
                  enum evhttp_cmd_type cmd_type,
                  const char *uri) {
    if (!http_conn_ || !req || !uri)
      return false;
    
    return 0 == evhttp_make_request(http_conn_, req, cmd_type, uri);
  }

  void DebugResponse(struct evhttp_request *req);
  void DebugHeaders(struct evkeyvalq *headers);
  void DebugRequest(struct evhttp_request *req);

  virtual void HandleResponse(struct evhttp_request *req);
  virtual void CloseCallback() {}

private:
  static void EventHttpRequestCb(struct evhttp_request *req, void *arg);
  static void EventHttpCloseCb(struct evhttp_connection *conn, void *arg);

protected:
  struct evhttp_connection *http_conn_;
  string                    host_;
  unsigned short            port_;
};

#endif
