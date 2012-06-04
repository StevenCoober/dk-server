/**
 * author:lijian2@ucweb.com
 * date:2012-03-08
 */

#include "donkey_core.h"
#include "donkey_util.h"

int DonkeyBaseConnection::last_conn_id_;

DonkeyBaseConnection::DonkeyBaseConnection()
    : inited_(false), 
      base_(NULL),
      port_(0),
      fd_(-1),
      keep_alive_(false),
      timeout_(-1), 
      id_(-1),
      bufev_(NULL),
      state_(DKCON_DISCONNECTED),
      kind_(CON_INCOMING),
      error_(DKCON_ERROR_NONE) {
}

DonkeyBaseConnection::~DonkeyBaseConnection() {
  Destruct();
}

void DonkeyBaseConnection::Destruct() {
  if (bufev_)
    bufferevent_free(bufev_);
}

bool DonkeyBaseConnection::Init(struct event_base *base,
                                int fd,
                                const char *host,
                                unsigned short port) {
  if (inited_)
    return true;

  if (!base)
    return false;

  if (fd == -1)
    kind_ = CON_OUTGOING;
  else
    kind_ = CON_INCOMING;

  fd_ = fd;
  base_ = base;
  host_ = host;
  port_ = port;

  if (bufev_) {
    bufferevent_setfd(bufev_, fd_);
    bufferevent_setcb(bufev_,
                      EventReadCb,
                      EventWriteCb,
                      EventErrorCb,
                      this);
  } else {
    bufev_ = bufferevent_new(fd_,
                             EventReadCb,
                             EventWriteCb,
                             EventErrorCb,
                             (void *)this);
  }

  if (!bufev_)
    return false;

  bufferevent_base_set(base_, bufev_);

  id_ = ++last_conn_id_;
  if (id_ == -1)
    id_ = ++last_conn_id_;
 
  inited_ = true;

  return true;
}

bool DonkeyBaseConnection::StartRead() {
  assert(bufev_);

    /* Set up an event to read data */
	DisableWrite();
	EnableRead();
	state_ = DKCON_READING;
	/* Reset the bufferevent callbacks */
	bufferevent_setcb(bufev_,
	    EventReadCb,
      EventWriteCb,
      EventErrorCb,
	    this);  
 
  evbuffer *buf = bufferevent_get_input(bufev_);
  if (evbuffer_get_length(buf) > 0) {
    ReadHandler();
  }

  return true;
}

bool DonkeyBaseConnection::StartWrite() {
  if (!IsConnected()) {
    DK_DEBUG("[warn] %s: not connected\n", __func__);
    return false;
  }

  assert(bufev_);

  state_ = DKCON_WRITING; 
  EnableWrite();

	/* Disable the read callback: we don't actually care about data;
	 * we only care about close detection.  (We don't disable reading,
	 * since we *do* want to learn about any close events.) */
	bufferevent_setcb(bufev_,
	    NULL, /*read*/
	    EventWriteCb,
	    EventErrorCb,
	    this);
  
  return true;
}

void DonkeyBaseConnection::ReadHandler() {
  if (!bufev_)
    return;

  enum READ_STATUS read_status;
  bool stop = false;
  int nreqs = 200;

  switch (state_) {
  case DKCON_READING:
  case DKCON_IDLE:
    {
      if (state_ == DKCON_IDLE)
        state_ = DKCON_READING;

      while (!stop && nreqs-- > 0) {
        read_status = ReadCallback();

        switch (read_status) {
        case READ_ALL_DATA:
          ReadDone();
          if (evbuffer_get_length(get_input_buffer()) == 0)
            stop = true;
          break;

        case READ_INNER_ERROR:
        case READ_MEMORY_ERROR:
        case READ_BAD_CLIENT:
          Fail(DKCON_ERROR_PARSE_ERROR);
          stop = true;
          break;

        case READ_NEED_MORE_DATA:
          stop = true;
          break;

        default:
          stop = true;
          break;
        }
      } /* while */
    }
    break;
 
  default:
    DK_DEBUG("[warn] %s state %d is uncorrect\n", __func__, state_);
    break;
  }
}

void DonkeyBaseConnection::ReadDone() {
  if (kind_ == CON_OUTGOING) {
    set_state(DKCON_IDLE);

    if (!keep_alive()) {
      Reset();
      return;
    }

    if (!IsConnected())
      Connect();
    else {
      if (evbuffer_get_length(get_output_buffer()) > 0)
        StartWrite();
    }
    return;
  }
}

void DonkeyBaseConnection::WriteDone() {
  WriteCallback();

  if (kind_ == CON_INCOMING) {
    if (!keep_alive()) {
      Reset(); 
      AddToFreeConn();
      return;
    }
    
    StartRead();
    return;
  }

  if (kind_ == CON_OUTGOING) {
    StartRead();
  }
}

void DonkeyBaseConnection::EventReadCb(struct bufferevent *bufev,
                                   void *arg) {
  DonkeyBaseConnection *conn = (DonkeyBaseConnection *)arg;
  assert(conn);

  conn->ReadHandler();
}

void DonkeyBaseConnection::EventWriteCb(struct bufferevent *bufev,
                                    void *arg) {
  DonkeyBaseConnection *conn = (DonkeyBaseConnection *)arg; 
  assert(conn);

  conn->WriteDone();
}

void DonkeyBaseConnection::EventErrorCb(struct bufferevent *bufev,
                                    short what, 
                                    void *arg) {
  DonkeyBaseConnection *conn = (DonkeyBaseConnection *)arg; 
  assert(conn); 

  switch (conn->get_state()) {
  case DKCON_CONNECTING:
    if (what & BEV_EVENT_TIMEOUT) {
      DK_DEBUG("[error] %s: conecting timeout for %s:%d on %d\n",
          __func__, conn->get_host().c_str(), conn->get_port(),
          conn->get_fd());
      conn->ConnectFail(DKCON_ERROR_TIMEOUT); 
      return;
    }
    break;

  default:
    break;
  }

  if (what & (BEV_EVENT_TIMEOUT|BEV_EVENT_EOF|BEV_EVENT_ERROR)) {
    DonkeyConnectionError error;

    if (what & BEV_EVENT_TIMEOUT) {
      error = DKCON_ERROR_TIMEOUT;
    }
    if (what & BEV_EVENT_EOF) {
      error = DKCON_ERROR_EOF; 
    }
    if (what & BEV_EVENT_ERROR) {
      error = DKCON_ERROR_ERRNO;
    }
    
    DK_DEBUG("[error] %s: %s for %s:%d on %d\n", __func__, StrError(error),
        conn->get_host().c_str(), conn->get_port(), conn->get_fd());
    conn->Fail(error);
  } else {
    conn->Fail(DKCON_ERROR_ERRNO);
  }
}

void DonkeyBaseConnection::EventConnectCb(struct bufferevent *bufev,
                                      short what,
                                      void *arg) {
  DonkeyBaseConnection *conn = (DonkeyBaseConnection *)arg;
  int error;
	ev_socklen_t errsz = sizeof(error);

  int fd = bufferevent_getfd(bufev);
  conn->set_fd(fd);

  if (!(what & BEV_EVENT_CONNECTED)) {
    EventErrorCb(bufev, what, arg);
    return;
  }

  /* Check if the connection completed */
	if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (void*)&error,
		      &errsz) == -1) {
     
    goto cleanup; 
  }

  if (error) {
    DK_DEBUG("[error] %s: %s %s:%d on %d\n", __func__, strerror(errno),
        conn->get_host().c_str(), conn->get_port(), conn->get_fd());
    goto cleanup;
  }

  conn->ConnectMade();
  return;

cleanup:
  conn->ConnectFail(DKCON_ERROR_ERRNO);
}

void DonkeyBaseConnection::Fail(DonkeyConnectionError error) {
  error_ = error;

  DisableReadWrite();
  ErrorCallback();
  Reset();
  
  if (kind_ == CON_INCOMING) {
    AddToFreeConn();
    return;
  }
}

void DonkeyBaseConnection::ConnectFail(DonkeyConnectionError error) {
  error_ = error;
  ErrorCallback();
  Reset();
}

void DonkeyBaseConnection::Reset() {
  if (!bufev_)
    return;

  DisableReadWrite();

  if (fd_ == -1)
    fd_ = bufferevent_getfd(bufev_);

  if (fd_ != -1) {
    //if (IsConnected())
      CloseCallback();

    shutdown(fd_, SHUT_WR);
    close(fd_);
    fd_ = -1;
  }

  struct evbuffer *tmp = get_input_buffer();
	evbuffer_drain(tmp, evbuffer_get_length(tmp));
	tmp = get_output_buffer();
  evbuffer_unfreeze(tmp, 1);	
  evbuffer_drain(tmp, evbuffer_get_length(tmp));
  evbuffer_freeze(tmp, 1);
  
  inited_ = false;
	state_ = DKCON_DISCONNECTED;
  keep_alive_ = false;
  timeout_ = -1;
  error_ = DKCON_ERROR_NONE;
}

bool DonkeyBaseConnection::Connect() {
  if (state_ == DKCON_CONNECTING)
    return true;

  if (!bufev_)
    return false;
  
  Reset();
  kind_ = CON_OUTGOING;
 
  bufferevent_setfd(bufev_, fd_);
	bufferevent_setcb(bufev_,
	    NULL /* EventReadCb */,
	    NULL /* EventWriteCb */,
	    EventConnectCb,
	    (void *)this);
	bufferevent_settimeout(bufev_, 0,
	    timeout_ != -1 ? timeout_ : DK_CONNECT_TIMEOUT);

  EnableWrite();

  string ip;

  /* Libevent evget_addrinfo has a bug */
  DonkeyGetHostByName(host_, ip);
  if (ip.empty())
    ip = host_;

  int res = bufferevent_socket_connect_hostname(bufev_,
      NULL, AF_INET, ip.c_str(), port_);
  
  if (res < 0) {
    DK_DEBUG("[error] %s: bufferevent_socket_connect_hostname return %d\n",
        __func__, res);
    ConnectFail(DKCON_ERROR_ERRNO);     
    return false;
  }

  state_ = DKCON_CONNECTING;

  return true;
}

void DonkeyBaseConnection::ConnectMade() {
  ConnectedCallback();

  if (kind_ == CON_OUTGOING) {
    set_state(DKCON_IDLE);
    bufferevent_setcb(bufev_,
        EventReadCb,
        EventWriteCb,
        EventErrorCb,
        this);

    if (timeout_ != -1) {
      struct timeval tv;
      tv.tv_sec = timeout_;
      tv.tv_usec = 0;
      bufferevent_set_timeouts(bufev_, &tv, &tv);
    }

    if (evbuffer_get_length(get_output_buffer()) > 0)
      StartWrite();
  }
}

void DonkeyBaseConnection::AddToFreeConn() {
  if (server_)
    server_->FreeConn(this);
  else
    delete this; 
}
