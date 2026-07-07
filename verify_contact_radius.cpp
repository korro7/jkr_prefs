#include <windows.h>

#include <cmath>
#include <cstring>
#include <fstream>
#include <string>

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
        return false;
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

class DummyCustomPropertyData : public NApiCore::ICustomPropertyDataApi_1_0
{
public:
    DummyCustomPropertyData()
    {
        adhesionDelta[0] = 0.0;
        frictionDelta[0] = 0.0;
    }

    void getApiVersion(NApi::tApiMajorVersion& major,
                       NApi::tApiMinorVersion& minor) override
    {
        major = 1;
        minor = 0;
    }

    NApiCore::EApiId getApiId() const override
    {
        return NApiCore::eCustomPropertyData;
    }

    bool readOnly() const override
    {
        return false;
    }

    NApiCore::IApi* getManager(NApi::tApiMajorVersion,
                               NApi::tApiMinorVersion) override
    {
        return nullptr;
    }

    const double* getValue(unsigned int) override
    {
        return nullptr;
    }

    const double* getValue(const char*) override
    {
        return nullptr;
    }

    double* getDelta(unsigned int) override
    {
        return nullptr;
    }

    double* getDelta(const char* name) override
    {
        if (std::strcmp(name, "Adhesion") == 0)
        {
            return adhesionDelta;
        }

        if (std::strcmp(name, "Friction") == 0)
        {
            return frictionDelta;
        }

        return nullptr;
    }

    bool hasData(unsigned int) override
    {
        return false;
    }

    bool hasData(const char* name) override
    {
        return std::strcmp(name, "Adhesion") == 0 || std::strcmp(name, "Friction") == 0;
    }

    double adhesionDelta[1];
    double frictionDelta[1];
};

struct ForceResult
{
    bool ok;
    double normalX;
    double normalY;
    double normalZ;
};

static ForceResult runCase(NApiCm::IPluginContactModelV2_3_0* plugin,
                           NApiCore::ICustomPropertyDataApi_1_0* elem2PropData,
                           const char* elem2Type,
                           double elem1ContactRadius,
                           double elem1PhysicalRadius,
                           double elem2ContactRadius,
                           double elem2PhysicalRadius,
                           double normalContactOverlap,
                           double normalPhysicalOverlap)
{
    double tangentialPhysicalOverlapX = 0.0;
    double tangentialPhysicalOverlapY = 0.0;
    double tangentialPhysicalOverlapZ = 0.0;
    double calculatedNormalForceX = 0.0;
    double calculatedNormalForceY = 0.0;
    double calculatedNormalForceZ = 0.0;
    double calculatedUnsymNormalForceX = 0.0;
    double calculatedUnsymNormalForceY = 0.0;
    double calculatedUnsymNormalForceZ = 0.0;
    double calculatedTangentialForceX = 0.0;
    double calculatedTangentialForceY = 0.0;
    double calculatedTangentialForceZ = 0.0;
    double calculatedUnsymTangentialForceX = 0.0;
    double calculatedUnsymTangentialForceY = 0.0;
    double calculatedUnsymTangentialForceZ = 0.0;
    double calculatedElem1AdditionalTorqueX = 0.0;
    double calculatedElem1AdditionalTorqueY = 0.0;
    double calculatedElem1AdditionalTorqueZ = 0.0;
    double calculatedElem1UnsymAdditionalTorqueX = 0.0;
    double calculatedElem1UnsymAdditionalTorqueY = 0.0;
    double calculatedElem1UnsymAdditionalTorqueZ = 0.0;
    double calculatedElem2AdditionalTorqueX = 0.0;
    double calculatedElem2AdditionalTorqueY = 0.0;
    double calculatedElem2AdditionalTorqueZ = 0.0;
    double calculatedElem2UnsymAdditionalTorqueX = 0.0;
    double calculatedElem2UnsymAdditionalTorqueY = 0.0;
    double calculatedElem2UnsymAdditionalTorqueZ = 0.0;
    double calculatedChargeMovedToElem1 = 0.0;
    double orientation[9] = {0};

    NApi::ECalculateResult result = plugin->calculateForce(
        0,
        0.0,
        1.0e-5,
        1,
        "particle",
        0.01,
        2500.0,
        1.0,
        0,
        0.0,
        0.0,
        0.0,
        1.0e6,
        0.25,
        elem1ContactRadius,
        elem1PhysicalRadius,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        orientation,
        nullptr,
        false,
        2,
        elem2Type,
        1.0,
        2500.0,
        1.0,
        0,
        0.0,
        0.0,
        0.0,
        1.0,
        1.0e6,
        0.25,
        elem2ContactRadius,
        elem2PhysicalRadius,
        elem1ContactRadius - normalContactOverlap,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        orientation,
        elem2PropData,
        nullptr,
        nullptr,
        0.5,
        0.5,
        0.1,
        elem1ContactRadius,
        0.0,
        0.0,
        normalContactOverlap,
        normalPhysicalOverlap,
        tangentialPhysicalOverlapX,
        tangentialPhysicalOverlapY,
        tangentialPhysicalOverlapZ,
        calculatedNormalForceX,
        calculatedNormalForceY,
        calculatedNormalForceZ,
        calculatedUnsymNormalForceX,
        calculatedUnsymNormalForceY,
        calculatedUnsymNormalForceZ,
        calculatedTangentialForceX,
        calculatedTangentialForceY,
        calculatedTangentialForceZ,
        calculatedUnsymTangentialForceX,
        calculatedUnsymTangentialForceY,
        calculatedUnsymTangentialForceZ,
        calculatedElem1AdditionalTorqueX,
        calculatedElem1AdditionalTorqueY,
        calculatedElem1AdditionalTorqueZ,
        calculatedElem1UnsymAdditionalTorqueX,
        calculatedElem1UnsymAdditionalTorqueY,
        calculatedElem1UnsymAdditionalTorqueZ,
        calculatedElem2AdditionalTorqueX,
        calculatedElem2AdditionalTorqueY,
        calculatedElem2AdditionalTorqueZ,
        calculatedElem2UnsymAdditionalTorqueX,
        calculatedElem2UnsymAdditionalTorqueY,
        calculatedElem2UnsymAdditionalTorqueZ,
        calculatedChargeMovedToElem1);

    ForceResult forceResult;
    forceResult.ok = (result == NApi::eSuccess);
    forceResult.normalX = calculatedNormalForceX;
    forceResult.normalY = calculatedNormalForceY;
    forceResult.normalZ = calculatedNormalForceZ;
    return forceResult;
}

int main()
{
    std::ofstream log("D:\\wzh_pro\\jkr_src_2010_10\\verify_contact_radius_result.txt");
    const char* dllPath = "D:\\wzh_pro\\EDEM\\EDEM\\lib\\ContactModels\\JKR.dll";
    const char* prefPath = "D:\\wzh_pro\\EDEM\\EDEM\\lib\\ContactModels\\jkr_prefs.txt";

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
        log << "Plugin cast failed.\n";
        releaseInstance(base);
        FreeLibrary(module);
        return 1;
    }

    DummyApiManager apiManager;
    char customMsg[NApi::ERROR_MSG_MAX_LENGTH];
    std::memset(customMsg, 0, sizeof(customMsg));
    bool setupOk = plugin->setup(apiManager, prefPath, customMsg);
    log << "setupOk=" << (setupOk ? "true" : "false") << "\n";

    DummyCustomPropertyData propData;

    ForceResult physicalOnly = runCase(plugin, &propData, "blade", 0.005, 0.005, 0.05, 0.05, 1.0e-5, 1.0e-5);
    ForceResult shellOnly = runCase(plugin, &propData, "blade", 0.006, 0.005, 0.051, 0.05, 1.0e-5, -5.0e-6);
    ForceResult shellAndPhysical = runCase(plugin, &propData, "blade", 0.006, 0.005, 0.051, 0.05, 1.5e-5, 5.0e-6);
    ForceResult radius3ShellOnly = runCase(plugin, &propData, "blade", 3.01, 3.0, 3.01, 3.0, 1.0e-3, -9.0e-3);
    ForceResult radius3NearPhysical = runCase(plugin, &propData, "blade", 3.01, 3.0, 3.01, 3.0, 1.01e-2, 1.0e-4);

    log << "physicalOnly ok=" << physicalOnly.ok << " normal=("
        << physicalOnly.normalX << "," << physicalOnly.normalY << "," << physicalOnly.normalZ << ")\n";
    log << "shellOnly ok=" << shellOnly.ok << " normal=("
        << shellOnly.normalX << "," << shellOnly.normalY << "," << shellOnly.normalZ << ")\n";
    log << "shellAndPhysical ok=" << shellAndPhysical.ok << " normal=("
        << shellAndPhysical.normalX << "," << shellAndPhysical.normalY << "," << shellAndPhysical.normalZ << ")\n";
    log << "radius3ShellOnly ok=" << radius3ShellOnly.ok << " normal=("
        << radius3ShellOnly.normalX << "," << radius3ShellOnly.normalY << "," << radius3ShellOnly.normalZ << ")\n";
    log << "radius3NearPhysical ok=" << radius3NearPhysical.ok << " normal=("
        << radius3NearPhysical.normalX << "," << radius3NearPhysical.normalY << "," << radius3NearPhysical.normalZ << ")\n";

    releaseInstance(base);
    FreeLibrary(module);
    return 0;
}
