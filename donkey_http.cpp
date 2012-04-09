/**
 * author:lijian2@ucweb.com
 * date:2012-03
 */

#include "donkey_core.h"
using namespace std;

bool DonkeyHttpClient::Init(
    struct event_base *evbase, const char *host, unsigned short port) {
  if (http_conn_)
    return false;

  if (!evbase || !host || port == 0)
    return false;
  
  struct hostent hent, *ent;
  char buf[2048];
  int err;
  struct in_addr addr;
  const char *ip = host;
    
  if (gethostbyname_r(host, &hent, buf, sizeof(buf), &ent, &err) ||
    ent == NULL) {
    DK_DEBUG("[error] %s: gethostbyname_r %s\n", __func__, host); 
  }

  for (int i = 0; ent->h_addr_list[i]; ++i) {
    memcpy(&addr, ent->h_addr_list[i], ent->h_length);
    ip = inet_ntoa(addr);
    break;
  }

  http_conn_ = evhttp_connection_base_new(evbase, NULL, ip, port);
  if (!http_conn_)
    return false;
  
  return true;
}

/*********** virtual ************/

void DonkeyHttpClient::HandleResponse(struct evhttp_request *req) {
  DebugResponse(req); 
}

/*********** static ************/

void DonkeyHttpClient::EventHttpRequestCb(
    struct evhttp_request *req, void *arg) {
  DonkeyHttpClient *http_client = (DonkeyHttpClient *)arg;
  
  if (http_client)
    http_client->HandleResponse(req);
}


/*********** debug function ***********/

void DonkeyHttpClient::DebugResponse(struct evhttp_request *req) {
  if (!req)
    return;
  
  struct evkeyvalq    *headers;
  struct evbuffer     *input_buffer;
  const char *         value; 
  int                  content_length;
  string               s_headers = "headers:\n";
  string               body;
 
  DK_DEBUG(">>>>>>>>>>>\n");
  DK_DEBUG("Http status %d\n", evhttp_request_get_response_code(req));
  headers = evhttp_request_get_input_headers(req);
  DebugHeaders(headers);

  value = evhttp_find_header(headers, "Content-Length");
  if (value)
    content_length = atoi(value);

  input_buffer = evhttp_request_get_input_buffer(req); 
  
  if (input_buffer) {
    const char *tmp = (const char *)evbuffer_pullup(input_buffer, content_length); 
    if (tmp)
      body.assign(tmp, content_length); 
    DK_DEBUG("Http body: %.*s\n", body.size(), body.data());
  }
  
  DK_DEBUG("<<<<<<<<<<<\n"); 
}

void DonkeyHttpClient::DebugRequest(struct evhttp_request *req) {
  if (!req)
    return; 
  struct evkeyvalq *headers = evhttp_request_get_output_headers(req);
  DK_DEBUG(">>>>>>>>>>>\n"); 
  DebugHeaders(headers);
  DK_DEBUG("<<<<<<<<<<<\n");
}

void DonkeyHttpClient::DebugHeaders(struct evkeyvalq *headers) {
  struct evkeyval *header;
  string s_headers = "Http headers:\n";
  
  if (!headers)
    return;
   
  for (header = headers->tqh_first; header;
    header = header->next.tqe_next) {
    s_headers += header->key;
    s_headers += ":";
    s_headers += header->value;
    s_headers += "\n";
  }
  
  DK_DEBUG("%.*s\n", s_headers.size(), s_headers.data());
}

