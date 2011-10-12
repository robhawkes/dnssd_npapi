/**********************************************************\

 Auto-generated Factory.cpp

 This file contains the auto-generated factory methods
 for the DNSSDPlugin project

\**********************************************************/

#include "FactoryBase.h"
#include "DNSSDPlugin.h"
#include <boost/make_shared.hpp>

class PluginFactory : public FB::FactoryBase
{
public:
    ///////////////////////////////////////////////////////////////////////////////
    /// @fn FB::PluginCorePtr createPlugin(const std::string& mimetype)
    ///
    /// @brief  Creates a plugin object matching the provided mimetype
    ///         If mimetype is empty, returns the default plugin
    ///////////////////////////////////////////////////////////////////////////////
    FB::PluginCorePtr createPlugin(const std::string& mimetype)
    {
        return boost::make_shared<DNSSDPlugin>();
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// @see FB::FactoryBase::globalPluginInitialize
    ///////////////////////////////////////////////////////////////////////////////
    void globalPluginInitialize()
    {
        DNSSDPlugin::StaticInitialize();
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// @see FB::FactoryBase::globalPluginDeinitialize
    ///////////////////////////////////////////////////////////////////////////////
    void globalPluginDeinitialize()
    {
        DNSSDPlugin::StaticDeinitialize();
    }

    // Set FireBreath's log methods
    void getLoggingMethods(FB::Log::LogMethodList& outMethods)
    {
        outMethods.push_back(std::make_pair(FB::Log::LogMethod_Console, std::string()));
    }

    // Set FireBreath's log level
    FB::Log::LogLevel getLogLevel(){
        return FB::Log::LogLevel_Info;
    }

};

///////////////////////////////////////////////////////////////////////////////
/// @fn getFactoryInstance()
///
/// @brief  Returns the factory instance for this plugin module
///////////////////////////////////////////////////////////////////////////////
FB::FactoryBasePtr getFactoryInstance()
{
    static boost::shared_ptr<PluginFactory> factory = boost::make_shared<PluginFactory>();
    return factory;
}
