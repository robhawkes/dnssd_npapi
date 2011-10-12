#include "JSAPIAuto.h"
#include "DNSSDPlugin.h"
#include "dns_sd.h"

#ifndef H_DNSSDPluginAPI
#define H_DNSSDPluginAPI

FB_FORWARD_PTR(DNSSDPlugin);
FB_FORWARD_PTR(DNSSDPluginOp);

typedef boost::function<DNSServiceErrorType(DNSServiceRef*, void*)> OpFun;

class DNSSDPluginAPI : public FB::JSAPIAuto
{
 public:
  DNSSDPluginAPI(const DNSSDPluginPtr& plugin,
		 const FB::BrowserHostPtr& host);
  virtual ~DNSSDPluginAPI();
  DNSSDPluginPtr getPlugin();
  std::string plugin_version();
  int64_t daemon_version();

  DNSSDPluginOpPtr enum_browse(uint32_t ifnum,
			       const FB::JSObjectPtr& callback);
  static void DNSSD_API enumerate_callback(DNSServiceRef sdref,
					   DNSServiceFlags flags,
					   uint32_t ifnum,
					   DNSServiceErrorType err,
					   const char *domain,
					   void *context);

  DNSSDPluginOpPtr browse(uint32_t ifnum,
			  const std::string& regtype,
			  const std::string& domain,
			  const FB::JSObjectPtr& callback);
  static DNSServiceErrorType browse_init(DNSServiceRef* sdref,
					 uint32_t ifnum,
					 const std::string& regtype,
					 const std::string& domain,
					 void* context);
  static void DNSSD_API browse_callback(DNSServiceRef sdref,
					DNSServiceFlags flags,
					uint32_t ifnum,
					DNSServiceErrorType err,
					const char* name,
					const char* type,
					const char* domain,
					void* context);

    DNSSDPluginOpPtr resolve(uint32_t ifnum,
			     const std::string& name,
			     const std::string& type,
			     const std::string& domain,
			     const FB::JSObjectPtr &callback);
    static DNSServiceErrorType resolve_init(DNSServiceRef* sdref,
					    uint32_t ifnum,
					    const std::string& name,
					    const std::string& regtype,
					    const std::string& domain,
					    void* context);
    static void DNSSD_API resolve_callback(DNSServiceRef sdref,
					   DNSServiceFlags flags,
					   uint32_t ifnum,
					   DNSServiceErrorType err,
					   const char* _fullname,
					   const char* host,
					   uint16_t port,
					   uint16_t txtlen,
					   const unsigned char* txtrecord,
					   void* context);

private:
    DNSSDPluginWeakPtr m_plugin;
    FB::BrowserHostPtr m_host;
};

#endif // H_DNSSDPluginAPI
