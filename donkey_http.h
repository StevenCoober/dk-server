/**
 * author:lijian2@ucweb.com
 * date:2012-03
 */

#ifndef __DONKEY_HTTP_INCLUDE__
#define __DONKEY_HTTP_INCLUDE__

#include "donkey_common.h"

class DonkeyHttpClient;

class DonkeyHttpRequest {
public:
  DonkeyHttpRequest() : http_req_(NULL) {
  }

  virtual ~DonkeyHttpRequest() {
  }

  bool Init() {
    http_req_ = evhttp_request_new(EventHttpRequestCb, (void *)this);
    return http_req_ != NULL; 
  }

  bool AddHeader(const char *key,
                 const char *value) {
    assert(http_req_);
    return 0 == evhttp_add_header(evhttp_request_get_output_headers(http_req_),
      key, value);  
  }
  
  bool AddPostData(const void *data, size_t len) {
    assert(http_req_);
    return 0 == evbuffer_add(evhttp_request_get_output_buffer(http_req_), data, len); 
  }

  struct evhttp_request *get_http_req() {
    return http_req_;
  }

  virtual void HandleResponse(struct evhttp_request *req);
  
  void DebugResponse(struct evhttp_request *req);
  void DebugHeaders(struct evkeyvalq *headers);
  void DebugRequest(struct evhttp_request *req);

private:
  static void EventHttpRequestCb(struct evhttp_request *req, void *arg);

protected:
  struct evhttp_request *http_req_;
};

class DonkeyHttpClient {
public: 
  DonkeyHttpClient() : http_conn_(NULL) {
  }
  
  virtual ~DonkeyHttpClient() {
    if (http_conn_)
      free(http_conn_);
  }

  bool Init(struct event_base *evbase, const char *host, unsigned short port); 

  DonkeyHttpRequest *NewRequest() {
    DonkeyHttpRequest *req = new DonkeyHttpRequest();
    if (req && !req->Init()) {
      delete req;
      req = NULL;
    }

    return req;
  }

  struct evhttp_connection *get_http_conn() {
    return http_conn_;
  }

  void SetTimeout(int timeout_in_secs) {
    if (http_conn_)
      evhttp_connection_set_timeout(http_conn_, timeout_in_secs);
  }

  void SetRetries(int retries) {
    if (http_conn_)
      evhttp_connection_set_retries(http_conn_, retries);
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
    
    dk_http_req will be auto free after response
  */
  bool SendRequest(DonkeyHttpRequest *dk_http_req,
                  enum evhttp_cmd_type cmd_type,
                  const char *uri) {
    if (!http_conn_ || !dk_http_req || !uri)
      return false;
    
    return 0 == evhttp_make_request(
        http_conn_, dk_http_req->get_http_req(), cmd_type, uri);
  }

  virtual void CloseCallback() {}

private:
  static void EventHttpCloseCb(struct evhttp_connection *conn, void *arg);

protected:
  struct evhttp_connection *http_conn_;
  std::string               host_;
  unsigned short            port_;
};

#endif
