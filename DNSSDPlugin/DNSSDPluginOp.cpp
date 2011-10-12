#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

#include "DNSSDPluginAPI.h"
#include "DNSSDPluginOp.h"
#include "variant_list.h"

// temp hack
// #define DNSSDPluginEnableDebugLogging
#ifdef DNSSDPluginEnableDebugLogging
# define DEBUG_LOG(...) FBLOG_DEBUG(__FILE__, boost::this_thread::get_id() \
				    << " - " << __VA_ARGS__)
#else
# define DEBUG_LOG(...)
#endif

DNSSDPluginOp::DNSSDPluginOp(const FB::BrowserHostPtr& host,
			     const FB::JSObjectPtr &callback,
			     const std::string op,
			     const OpFun fun)
  : m_open(true), m_host(host), m_callback(callback), m_op(op)
{
  DEBUG_LOG("start");
  assert(host->isMainThread());
#ifdef _WIN32
  m_event = WSACreateEvent();
  DEBUG_LOG("created event");
#else
  if (pipe(m_pipe) == 0) {
    DEBUG_LOG("created pipe");
  } else {
    DEBUG_LOG("failed to create pipe");
    error("pipe", -1);
    m_open = false;
    return;
  }
#endif
  DNSServiceErrorType err = fun(&m_sdref, this);
  if (err != kDNSServiceErr_NoError) {
    DEBUG_LOG("fun returned error " << err);
    error("op_init", (int64_t) err);
    return;
  }
  m_fd = DNSServiceRefSockFD(m_sdref);
  if (m_fd == -1) {
    DEBUG_LOG("DNSServiceRefSockFD returned -1");
    error("op_fd", -1);
    return;
  } else {
    DEBUG_LOG("DNSServiceRefSockFD returned " << m_fd);
  }
  m_thread = boost::thread(boost::bind(&DNSSDPluginOp::thread_run, this));
  registerMethod("stop", make_method(this, &DNSSDPluginOp::stop));
  DEBUG_LOG("op init complete - thread " << m_thread.get_id());
}

DNSSDPluginOp::~DNSSDPluginOp() {
  DEBUG_LOG("start");
  stop();
}

void DNSSDPluginOp::error(std::string state, int64_t error) {
  callback(true, FB::variant_map_of<std::string>("call", state)
	   ("error", error));
}

void DNSSDPluginOp::result(FB::VariantMap info) {
  callback(false, info);
}

void DNSSDPluginOp::callback(bool error, FB::VariantMap info) {
  info.insert(std::pair<std::string,std::string>("op", m_op));
  FB::VariantList args = FB::variant_list_of(error)(info)(shared_from_this());
  std::string payload(error ? " error " : " result ");
  try {
    m_callback->InvokeAsync("", args);
    DEBUG_LOG(m_op << payload << "callback succeeded");
  }
  catch ( std::runtime_error ) {
    DEBUG_LOG(m_op << payload << "callback failed");
  }
}

#ifdef _WIN32
void DNSSDPluginOp::thread_run() {
  DEBUG_LOG("start");
  HANDLE readhandles[2];
  WSAEVENT DNSServiceEvent = WSACreateEvent();
  bool run = true;
  DNSServiceErrorType err = kDNSServiceErr_NoError;
  boost::mutex::scoped_lock olock(m_openmutex, boost::defer_lock);
  WSAEventSelect(m_fd, DNSServiceEvent, FD_READ);
  readhandles[0] = m_event;
  readhandles[1] = DNSServiceEvent;
  while(run) {
    switch(WaitForMultipleObjects(2, readhandles, FALSE, INFINITE))
      {
      case WAIT_OBJECT_0:
	DEBUG_LOG("m_event set");
	WSAResetEvent(m_event);
	run = false;
	break;
      case WAIT_OBJECT_0 + 1:
	DEBUG_LOG("DNSServiceEvent set");
	WSAResetEvent(DNSServiceEvent);
	err = m_host->CallOnMainThread(boost::bind<DNSServiceErrorType>
				       (DNSServiceProcessResult, m_sdref));
	DEBUG_LOG("DNSServiceProcessResult " << err);
	olock.lock();
	run = m_open;
	olock.unlock();
	if (run && err != kDNSServiceErr_NoError) {
	  error("DNSServiceProcessResult", (int64_t) err);
	  err = kDNSServiceErr_NoError;
	}
	break;
      default:
	DEBUG_LOG("WaitForMultipleObjects returned error " << GetLastError());
	olock.lock();
	run = m_open;
	olock.unlock();
	if (m_open) error("polling", -1);
	run = false;
      }
  }
  DEBUG_LOG("closing events");
  WSACloseEvent(DNSServiceEvent);
  WSACloseEvent(m_event);
  DEBUG_LOG("exit");
}
#else
void DNSSDPluginOp::thread_run() {
  DEBUG_LOG("start");
  fd_set readfds;
  bool run = true;
  DNSServiceErrorType err = kDNSServiceErr_NoError;
  int nfds = m_fd > m_pipe[0] ? m_fd + 1 : m_pipe[0] + 1;
  int result;
  boost::mutex::scoped_lock olock(m_openmutex, boost::defer_lock);
  while(run) {
    FD_ZERO(&readfds);
    FD_SET(m_fd, &readfds);
    FD_SET(m_pipe[0], &readfds);
    result = select(nfds, &readfds, (fd_set*)NULL, (fd_set*)NULL, NULL);
    if (result < 0) {
      DEBUG_LOG("select returned < 0 " << std::dec << result);
      olock.lock();
      run = m_open;
      olock.unlock();
      if (run) error("polling", -1);
      run = false;
    } else {
      if (FD_ISSET(m_pipe[0], &readfds)) {
	DEBUG_LOG("m_pipe[0] set");
	run = false;
      } else if (FD_ISSET(m_fd, &readfds)) {
	DEBUG_LOG("fd set");
	err = m_host->CallOnMainThread(boost::bind<DNSServiceErrorType>
				       (&DNSSDPluginOp::process_result, this));
	DEBUG_LOG("DNSServiceProcessResult " << err);
	olock.lock();
	run = m_open;
	olock.unlock();
	if (run && err != kDNSServiceErr_NoError) {
	  error("DNSServiceProcessResult", (int64_t) err);
	  err = kDNSServiceErr_NoError;
	}
      }
    }
  }
  DEBUG_LOG("closing pipes");
  close(m_pipe[0]);
  DEBUG_LOG("exit");
}
#endif

DNSServiceErrorType DNSSDPluginOp::process_result() {
  if (m_open) {
    return DNSServiceProcessResult(m_sdref);
  } else {
    return kDNSServiceErr_NoError;
  }
}

void DNSSDPluginOp::stop() {
  DEBUG_LOG(m_thread.get_id());
  assert(host->isMainThread());
  boost::mutex::scoped_lock olock(m_openmutex);
  if (m_open) {
    DEBUG_LOG("m_open");
    m_open = false;
    olock.unlock();
#ifdef _WIN32
    WSASetEvent(m_event);
    WSACloseEvent(m_event);
#else
    write(m_pipe[1], "x", 1);
    close(m_pipe[1]);
#endif
    DNSServiceRefDeallocate(m_sdref);
  }
}
