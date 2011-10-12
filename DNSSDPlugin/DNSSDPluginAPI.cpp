#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

#include "DOM/Window.h"
#include "global/config.h"
#include "DNSSDPluginAPI.h"
#include "DNSSDPluginOp.h"

///////////////////////////////////////////////////////////////////////////////
/// @fn DNSSDPluginAPI::DNSSDPluginAPI(const DNSSDPluginPtr& plugin, const FB::BrowserHostPtr host)
///
/// @brief  Constructor for your JSAPI object.  You should register your methods, properties, and events
///         that should be accessible to Javascript from here.
///
/// @see FB::JSAPIAuto::registerMethod
/// @see FB::JSAPIAuto::registerProperty
/// @see FB::JSAPIAuto::registerEvent
///////////////////////////////////////////////////////////////////////////////
DNSSDPluginAPI::DNSSDPluginAPI(const DNSSDPluginPtr& plugin,
			       const FB::BrowserHostPtr& host)
  : m_plugin(plugin), m_host(host)
{
  bool safe = true;
  std::string msg = "DNSSDPlugin: ";
  std::string location = m_host->getDOMWindow()->getLocation();
  msg.append("running from ");
  msg.append(location);
#ifndef DEBUG
  safe = (location == MOZILLA_CHROME_URI
	  || location.find("chrome-extension://") == 0);
#endif
  msg.append(safe ? " presumed safe" : " presumed unsafe");
  FBLOG_INFO("DNSSDPluginAPI()", msg);
  if (!safe) return;

  registerMethod("plugin_version",
		 make_method(this, &DNSSDPluginAPI::plugin_version));
  registerMethod("daemon_version",
		 make_method(this, &DNSSDPluginAPI::daemon_version));
  registerMethod("browse_domains",
		 make_method(this, &DNSSDPluginAPI::enum_browse));
  registerMethod("browse", make_method(this, &DNSSDPluginAPI::browse));
  registerMethod("resolve", make_method(this, &DNSSDPluginAPI::resolve));
}

///////////////////////////////////////////////////////////////////////////////
/// @fn DNSSDPluginAPI::~DNSSDPluginAPI()
///
/// @brief  Destructor.  Remember that this object will not be released until
///         the browser is done with it; this will almost definitely be after
///         the plugin is released.
///////////////////////////////////////////////////////////////////////////////
DNSSDPluginAPI::~DNSSDPluginAPI()
{
}

///////////////////////////////////////////////////////////////////////////////
/// @fn DNSSDPluginPtr DNSSDPluginAPI::getPlugin()
///
/// @brief  Gets a reference to the plugin that was passed in when the object
///         was created.  If the plugin has already been released then this
///         will throw a FB::script_error that will be translated into a
///         javascript exception in the page.
///////////////////////////////////////////////////////////////////////////////
DNSSDPluginPtr DNSSDPluginAPI::getPlugin()
{
  DNSSDPluginPtr plugin(m_plugin.lock());
  if (!plugin) {
    throw FB::script_error("The plugin is invalid");
    }
  return plugin;
}

std::string DNSSDPluginAPI::plugin_version() {
  return FBSTRING_PLUGIN_VERSION;
}

int64_t DNSSDPluginAPI::daemon_version() {
#ifdef kDNSServiceProperty_DaemonVersion
  uint32_t value;
  uint32_t size = sizeof(value);
  const char* property = kDNSServiceProperty_DaemonVersion;
  DNSServiceErrorType err = DNSServiceGetProperty(property, &value, &size);
  if (err != kDNSServiceErr_NoError) return -1;
  return (int64_t) value;
#else
  return -2;
#endif
}

DNSSDPluginOpPtr DNSSDPluginAPI::enum_browse(uint32_t ifnum,
					     const FB::JSObjectPtr& callback) {
  OpFun fun = boost::bind<DNSServiceErrorType>(DNSServiceEnumerateDomains,
					       _1,
					       (DNSServiceFlags) kDNSServiceFlagsBrowseDomains,
					       ifnum,
					       DNSSDPluginAPI::enumerate_callback,
					       _2);
  return boost::make_shared<DNSSDPluginOp>(m_host,
					   callback,
					   "enumerate_browse",
					   fun);
}

void DNSSD_API DNSSDPluginAPI::enumerate_callback(DNSServiceRef sdref,
						  DNSServiceFlags flags,
						  uint32_t ifnum,
						  DNSServiceErrorType err,
						  const char* domain,
						  void* context) {
  if (context == NULL) return;
  if (err == kDNSServiceErr_NoError) {
    bool add = flags & kDNSServiceFlagsAdd ? true : false;
    std::string sdomain(domain);
    FB::VariantMap info = FB::variant_map_of<std::string>("add",add)
      ("ifnum",ifnum)("domain",sdomain);
    static_cast<DNSSDPluginOp*>(context)->result(info);
  } else {
    static_cast<DNSSDPluginOp*>(context)->error("enumerate_callback",
						(int64_t) err);
  }
}

DNSSDPluginOpPtr DNSSDPluginAPI::browse(uint32_t ifnum,
					const std::string& regtype,
					const std::string& domain,
					const FB::JSObjectPtr &callback)
{
  OpFun fun = boost::bind<DNSServiceErrorType>(browse_init,
					       _1,
					       ifnum,
					       std::string(regtype),
					       std::string(domain),
					       _2);
  return boost::make_shared<DNSSDPluginOp>(m_host, callback, "browse", fun);
}

DNSServiceErrorType DNSSDPluginAPI::browse_init(DNSServiceRef* sdref,
						uint32_t ifnum,
						const std::string& regtype,
						const std::string& domain,
						void* context) {
  return DNSServiceBrowse(sdref,
			  0,
			  ifnum,
			  regtype.c_str(),
			  domain.c_str(),
			  DNSSDPluginAPI::browse_callback,
			  context);
}

void DNSSDPluginAPI::browse_callback(DNSServiceRef sdref,
				     DNSServiceFlags flags,
				     uint32_t ifnum,
				     DNSServiceErrorType err,
				     const char* name,
				     const char* type,
				     const char* domain,
				     void* context) {
  if (context == NULL) return;
  if (err == kDNSServiceErr_NoError) {
      bool add = flags & kDNSServiceFlagsAdd ? true : false;
      std::string sname(name);
      std::string stype(type);
      std::string sdomain(domain);
      FB::VariantMap info = FB::variant_map_of<std::string>("add",add)
	("ifnum",ifnum)("name",sname)("type",stype)("domain",sdomain);
      static_cast<DNSSDPluginOp*>(context)->result(info);
  } else {
    static_cast<DNSSDPluginOp*>(context)->error("browse_callback",
						(int64_t) err);
  }
}

DNSSDPluginOpPtr DNSSDPluginAPI::resolve(uint32_t ifnum,
					     const std::string& name,
					     const std::string& type,
					     const std::string& domain,
					     const FB::JSObjectPtr &callback) {
  OpFun fun = boost::bind<DNSServiceErrorType>(resolve_init,
					       _1,
					       ifnum,
					       std::string(name),
					       std::string(type),
					       std::string(domain),
					       _2);
  return boost::make_shared<DNSSDPluginOp>(m_host, callback, "resolve", fun);
}

DNSServiceErrorType DNSSDPluginAPI::resolve_init(DNSServiceRef* sdref,
						 uint32_t ifnum,
						 const std::string& name,
						 const std::string& regtype,
						 const std::string& domain,
						 void* context) {
  return DNSServiceResolve(sdref,
			   0,
			   ifnum,
			   name.c_str(),
			   regtype.c_str(),
			   domain.c_str(),
			   DNSSDPluginAPI::resolve_callback,
			   context);
}

void DNSSD_API DNSSDPluginAPI::resolve_callback(DNSServiceRef sdref,
						DNSServiceFlags flags,
						uint32_t ifnum,
						DNSServiceErrorType err,
						const char* _fullname,
						const char* host,
						uint16_t port,
						uint16_t txtlen,
						const unsigned char* txtrecord,
						void* context) {
  if (context == NULL) return;
  if (err == kDNSServiceErr_NoError) {
    std::string shost(host);
    std::string txt = "";
    if (txtlen > 0) txt.assign( (const char*) txtrecord, txtlen);
    FB::VariantMap info = FB::variant_map_of<std::string>("ifnum", ifnum)
      ("host", host)("port", ntohs(port))("txt", txt);
    static_cast<DNSSDPluginOp*>(context)->result(info);
  } else {
    static_cast<DNSSDPluginOp*>(context)->error("resolve_callback",
						(int64_t) err);
  }
}
