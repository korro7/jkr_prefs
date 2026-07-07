#include <windows.h>

#include <cstring>
#include <fstream>
#include <iostream>

#include "IPluginContactModelV2_3_0.h"
#include "PluginContactModelCore.h"

class DummyApiManager : public NApiCore::IApiManager_1_0
{
public:
    void getApiVersion(NApi::tApiMajorVersion& major,
                       NApi::tApiMinorVersion& minor) override
    {
        major = 1;
        minor = 0;
    }

    NApiCore::EApiId getApiId() const override
    {
        return NApiCore::eApiManager;
    }

    bool readOnly() const override
    {
        return true;
    }

    NApiCore::IApi* getApi(NApiCore::EApiId,
                           NApi::tApiMajorVersion,
                           NApi::tApiMinorVersion) override
    {
        return nullptr;
    }

    void release(NApiCore::IApi*) override
    {
    }
};

int main()
{
    const char* dllPath = "D:\\wzh_pro\\EDEM\\EDEM\\lib\\ContactModels\\JKR.dll";
    const char* prefPath = "D:\\wzh_pro\\EDEM\\EDEM\\lib\\ContactModels\\jkr_prefs.txt";

    std::ofstream log("D:\\wzh_pro\\jkr_src_2010_10\\verify_jkr_setup_result.txt");

    HMODULE module = LoadLibraryA(dllPath);
    if (!module)
    {
        log << "LoadLibrary failed: " << GetLastError() << "\n";
        return 1;
    }

    auto getInstance = reinterpret_cast<NApiCm::IPluginContactModel* (*)()>(
        GetProcAddress(module, "GETCMINSTANCE"));
    auto releaseInstance = reinterpret_cast<void (*)(NApiCm::IPluginContactModel*)>(
        GetProcAddress(module, "RELEASECMINSTANCE"));

    if (!getInstance || !releaseInstance)
    {
        log << "Failed to resolve plugin exports.\n";
        FreeLibrary(module);
        return 1;
    }

    NApiCm::IPluginContactModel* base = getInstance();
    auto* plugin = dynamic_cast<NApiCm::IPluginContactModelV2_3_0*>(base);
    if (!plugin)
    {
        log << "Plugin is not IPluginContactModelV2_3_0.\n";
        releaseInstance(base);
        FreeLibrary(module);
        return 1;
    }

    char prefFileName[NApi::FILE_PATH_MAX_LENGTH];
    std::memset(prefFileName, 0, sizeof(prefFileName));
    plugin->getPreferenceFileName(prefFileName);

    char customMsg[NApi::ERROR_MSG_MAX_LENGTH];
    std::memset(customMsg, 0, sizeof(customMsg));

    DummyApiManager apiManager;
    bool ok = plugin->setup(apiManager, prefPath, customMsg);

    log << "getPreferenceFileName: " << prefFileName << "\n";
    log << "setup returned: " << (ok ? "true" : "false") << "\n";
    log << "customMsg: " << customMsg << "\n";

    releaseInstance(base);
    FreeLibrary(module);
    return ok ? 0 : 2;
}
