#include "PluginContactModelCore.h"
#include "CJKR.h"

using namespace NApiCm;

EXPORT_MACRO IPluginContactModel* GETCMINSTANCE()
{
    return new CHertzMindlin();
}

EXPORT_MACRO void RELEASECMINSTANCE(IPluginContactModel* plugin)
{
    if (0 != plugin)
    {
        delete plugin;
    }
}

EXPORT_MACRO int GETCMINTERFACEVERSION()
{
    static const int INTERFACE_VERSION_MAJOR = 0x02;
    static const int INTERFACE_VERSION_MINOR = 0x03;
    static const int INTERFACE_VERSION_PATCH = 0x00;

    return (INTERFACE_VERSION_MAJOR << 16 |
            INTERFACE_VERSION_MINOR << 8 |
            INTERFACE_VERSION_PATCH);
}

