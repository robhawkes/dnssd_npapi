#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <boost/weak_ptr.hpp>
#include "JSAPIAuto.h"
#include "variant_map.h"
#include "dns_sd.h"
#include "DNSSDPluginAPI.h"

class DNSSDPluginOp : public FB::JSAPIAuto
{
 public:
  DNSSDPluginOp(const FB::BrowserHostPtr& host,
		const FB::JSObjectPtr &callback,
		const std::string op,
		const OpFun fun);
  virtual ~DNSSDPluginOp();
  void error(std::string state, int64_t error);
  void result(FB::VariantMap info);
  void stop();
 protected:
  void callback(bool error, FB::VariantMap info);
  void thread_run();
  DNSServiceErrorType process_result();
 protected:
  bool m_open;
  boost::mutex m_openmutex;
  FB::BrowserHostPtr m_host;
  FB::JSObjectPtr m_callback;
  std::string m_op;
  boost::thread m_thread;
  DNSServiceRef m_sdref;
  int m_fd;
#ifdef _WIN32
  WSAEVENT m_event;
#else
  int m_pipe[2];
#endif
};
