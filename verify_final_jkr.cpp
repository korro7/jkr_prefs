#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

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

    NApiCore::EApiId getApiId() const override { return NApiCore::eApiManager; }
    bool readOnly() const override { return false; }
    NApiCore::IApi* getApi(NApiCore::EApiId,
                           NApi::tApiMajorVersion,
                           NApi::tApiMinorVersion) override { return 0; }
    void release(NApiCore::IApi*) override {}
};

class DummyCustomPropertyData : public NApiCore::ICustomPropertyDataApi_1_0
{
public:
    void add(const std::string& name, unsigned int count = 1)
    {
        m_values[name] = std::vector<double>(count, 0.0);
        m_deltas[name] = std::vector<double>(count, 0.0);
    }

    void commit()
    {
        for (std::map<std::string, std::vector<double> >::iterator it = m_values.begin();
             it != m_values.end(); ++it)
        {
            std::vector<double>& delta = m_deltas[it->first];
            for (unsigned int i = 0; i < it->second.size(); ++i)
            {
                it->second[i] += delta[i];
                delta[i] = 0.0;
            }
        }
    }

    double value(const std::string& name, unsigned int element = 0) const
    {
        std::map<std::string, std::vector<double> >::const_iterator it =
            m_values.find(name);
        return it != m_values.end() && element < it->second.size()
            ? it->second[element]
            : 0.0;
    }

    double effectiveValue(const std::string& name, unsigned int element = 0) const
    {
        std::map<std::string, std::vector<double> >::const_iterator valueIt =
            m_values.find(name);
        std::map<std::string, std::vector<double> >::const_iterator deltaIt =
            m_deltas.find(name);
        return valueIt != m_values.end() && deltaIt != m_deltas.end() &&
               element < valueIt->second.size()
            ? valueIt->second[element] + deltaIt->second[element]
            : 0.0;
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

    bool readOnly() const override { return false; }
    NApiCore::IApi* getManager(NApi::tApiMajorVersion,
                               NApi::tApiMinorVersion) override { return 0; }
    const double* getValue(unsigned int) override { return 0; }
    double* getDelta(unsigned int) override { return 0; }
    bool hasData(unsigned int) override { return false; }

    const double* getValue(const char* name) override
    {
        std::map<std::string, std::vector<double> >::iterator it = m_values.find(name);
        return it == m_values.end() ? 0 : &it->second[0];
    }

    double* getDelta(const char* name) override
    {
        std::map<std::string, std::vector<double> >::iterator it = m_deltas.find(name);
        return it == m_deltas.end() ? 0 : &it->second[0];
    }

    bool hasData(const char* name) override
    {
        return m_values.find(name) != m_values.end();
    }

private:
    std::map<std::string, std::vector<double> > m_values;
    std::map<std::string, std::vector<double> > m_deltas;
};

struct CaseInput
{
    CaseInput()
        : physicalRadius(3.0), contactRadius(3.0), physicalOverlap(1.0e-4),
          contactOverlap(1.0e-4), timestep(1.0e-5), staticFriction(0.5),
          rollingFriction(0.1), tangentialOverlapY(0.0), contactVelocityX(0.0),
          contactVelocityY(0.0), angularVelocityY(0.0) {}

    double physicalRadius;
    double contactRadius;
    double physicalOverlap;
    double contactOverlap;
    double timestep;
    double staticFriction;
    double rollingFriction;
    double tangentialOverlapY;
    double contactVelocityX;
    double contactVelocityY;
    double angularVelocityY;
};

struct CaseResult
{
    bool ok;
    double normal[3];
    double tangential[3];
    double torque1[3];
    double torque2[3];
    double tangentialOverlapY;
};

static double magnitude(const double vector[3])
{
    return std::sqrt(vector[0] * vector[0] + vector[1] * vector[1] +
                     vector[2] * vector[2]);
}

static bool finiteResult(const CaseResult& result)
{
    for (int i = 0; i < 3; ++i)
    {
        if (!std::isfinite(result.normal[i]) ||
            !std::isfinite(result.tangential[i]) ||
            !std::isfinite(result.torque1[i]) ||
            !std::isfinite(result.torque2[i]))
        {
            return false;
        }
    }
    return std::isfinite(result.tangentialOverlapY);
}

static bool nearlyEqual(double first, double second, double relativeTolerance)
{
    const double scale = std::max(1.0, std::max(std::fabs(first), std::fabs(second)));
    return std::fabs(first - second) <= relativeTolerance * scale;
}

static DummyCustomPropertyData makeContactProperties()
{
    DummyCustomPropertyData properties;
    properties.add("JKR Contact State");
    properties.add("JKR Rolling Displacement", 3);
    properties.add("JKR Actual Tangential Force");
    properties.add("JKR Tangential Force Limit");
    properties.add("JKR Repulsive Normal Force");
    properties.add("JKR Adhesive Normal Force");
    properties.add("JKR Net Normal Force");
    properties.add("JKR Contact Patch Radius");
    properties.add("JKR Contact Slip Speed");
    properties.add("JKR Rolling Torque");
    properties.add("JKR Rolling Torque Limit");
    return properties;
}

static DummyCustomPropertyData makeGeometryProperties()
{
    DummyCustomPropertyData properties;
    properties.add("Adhesion");
    properties.add("Friction");
    return properties;
}

static CaseResult runCase(NApiCm::IPluginContactModelV2_3_0* plugin,
                          DummyCustomPropertyData& contactProperties,
                          DummyCustomPropertyData& geometryProperties,
                          const CaseInput& input)
{
    double tangentialPhysicalOverlapX = 0.0;
    double tangentialPhysicalOverlapY = input.tangentialOverlapY;
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
    const double orientation[9] = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};

    const NApi::ECalculateResult calculateResult = plugin->calculateForce(
        0, 0.0, input.timestep,
        1, "particle", 1.0, 2500.0, 1.0, 0,
        0.4, 0.4, 0.4, 1.0e6, 0.25,
        input.contactRadius, input.physicalRadius,
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        input.contactVelocityX, input.contactVelocityY, 0.0,
        0.0, 0.0, 0.0,
        0.0, input.angularVelocityY, 0.0,
        0.0, 0.0, orientation, 0,
        false,
        2, "blade", 1.0e8, 2500.0, 1.0e8, 0,
        1.0e8, 1.0e8, 1.0e8, 1.0,
        1.0e6, 0.25, input.contactRadius, input.physicalRadius,
        input.physicalRadius, 0.0, 0.0,
        input.physicalRadius, 0.0, 0.0,
        0.0, 0.0, 0.0,
        0.0, 0.0, 0.0,
        0.0, 0.0, 0.0,
        0.0, 0.0, orientation, &geometryProperties,
        &contactProperties, 0,
        0.5, input.staticFriction, input.rollingFriction,
        input.physicalRadius, 0.0, 0.0,
        input.contactOverlap, input.physicalOverlap,
        tangentialPhysicalOverlapX, tangentialPhysicalOverlapY,
        tangentialPhysicalOverlapZ,
        calculatedNormalForceX, calculatedNormalForceY, calculatedNormalForceZ,
        calculatedUnsymNormalForceX, calculatedUnsymNormalForceY,
        calculatedUnsymNormalForceZ,
        calculatedTangentialForceX, calculatedTangentialForceY,
        calculatedTangentialForceZ,
        calculatedUnsymTangentialForceX, calculatedUnsymTangentialForceY,
        calculatedUnsymTangentialForceZ,
        calculatedElem1AdditionalTorqueX, calculatedElem1AdditionalTorqueY,
        calculatedElem1AdditionalTorqueZ,
        calculatedElem1UnsymAdditionalTorqueX,
        calculatedElem1UnsymAdditionalTorqueY,
        calculatedElem1UnsymAdditionalTorqueZ,
        calculatedElem2AdditionalTorqueX, calculatedElem2AdditionalTorqueY,
        calculatedElem2AdditionalTorqueZ,
        calculatedElem2UnsymAdditionalTorqueX,
        calculatedElem2UnsymAdditionalTorqueY,
        calculatedElem2UnsymAdditionalTorqueZ,
        calculatedChargeMovedToElem1);

    CaseResult result;
    result.ok = calculateResult == NApi::eSuccess;
    result.normal[0] = calculatedNormalForceX;
    result.normal[1] = calculatedNormalForceY;
    result.normal[2] = calculatedNormalForceZ;
    result.tangential[0] = calculatedTangentialForceX;
    result.tangential[1] = calculatedTangentialForceY;
    result.tangential[2] = calculatedTangentialForceZ;
    result.torque1[0] = calculatedElem1AdditionalTorqueX;
    result.torque1[1] = calculatedElem1AdditionalTorqueY;
    result.torque1[2] = calculatedElem1AdditionalTorqueZ;
    result.torque2[0] = calculatedElem2AdditionalTorqueX;
    result.torque2[1] = calculatedElem2AdditionalTorqueY;
    result.torque2[2] = calculatedElem2AdditionalTorqueZ;
    result.tangentialOverlapY = tangentialPhysicalOverlapY;
    return result;
}

static double criticalSeparation(double radius, double surfaceEnergy)
{
    const double youngs = 2.0 * 1.25 * 1.0e6;
    const double equivalentYoungs = 1.0 /
        (2.0 * (1.0 - 0.25 * 0.25) / youngs);
    const double cutoffPatch = std::cbrt(
        9.0 * 3.14159265358979323846 * surfaceEnergy * radius * radius *
        (0.75 - 1.0 / std::sqrt(2.0)) / (2.0 * equivalentYoungs));
    return cutoffPatch * cutoffPatch / radius -
        std::sqrt(4.0 * 3.14159265358979323846 * surfaceEnergy * cutoffPatch /
                  equivalentYoungs);
}

int main(int argc, char* argv[])
{
    const char* dllPath = argc > 1
        ? argv[1]
        : "D:\\wzh_pro\\jkr_src_2010_10\\x64\\Release\\jkr_src_2010_10.dll";
    const char* prefPath = argc > 2
        ? argv[2]
        : "D:\\wzh_pro\\jkr_src_2010_10\\jkr_prefs.txt";
    std::ofstream log("D:\\wzh_pro\\jkr_src_2010_10\\verify_final_jkr_result.txt");
    int failures = 0;

    const auto check = [&](bool condition, const std::string& description)
    {
        log << (condition ? "PASS " : "FAIL ") << description << "\n";
        if (!condition)
        {
            ++failures;
        }
    };

    HMODULE module = LoadLibraryA(dllPath);
    check(module != 0, "Release DLL loads");
    if (module == 0)
    {
        log << "LoadLibrary error=" << GetLastError() << "\n";
        return 1;
    }

    NApiCm::IPluginContactModel* (*getInstance)() =
        reinterpret_cast<NApiCm::IPluginContactModel* (*)()>(
            GetProcAddress(module, "GETCMINSTANCE"));
    void (*releaseInstance)(NApiCm::IPluginContactModel*) =
        reinterpret_cast<void (*)(NApiCm::IPluginContactModel*)>(
            GetProcAddress(module, "RELEASECMINSTANCE"));
    check(getInstance != 0 && releaseInstance != 0, "Plugin exports resolve");
    if (getInstance == 0 || releaseInstance == 0)
    {
        FreeLibrary(module);
        return 1;
    }

    NApiCm::IPluginContactModel* base = getInstance();
    NApiCm::IPluginContactModelV2_3_0* plugin =
        dynamic_cast<NApiCm::IPluginContactModelV2_3_0*>(base);
    check(plugin != 0, "Plugin implements API V2_3_0");
    if (plugin == 0)
    {
        releaseInstance(base);
        FreeLibrary(module);
        return 1;
    }

    DummyApiManager apiManager;
    char customMessage[NApi::ERROR_MSG_MAX_LENGTH] = {0};
    check(plugin->setup(apiManager, prefPath, customMessage),
          "setup reads jkr_prefs.txt");
    check(plugin->getNumberOfRequiredProperties(NApi::eContact) == 11,
          "all contact history and diagnostics are registered");

    CaseInput shellInput;
    shellInput.contactRadius = 3.01;
    shellInput.physicalOverlap = -0.009;
    shellInput.contactOverlap = 0.001;
    DummyCustomPropertyData shellContact = makeContactProperties();
    DummyCustomPropertyData shellGeometry = makeGeometryProperties();
    const CaseResult shellResult = runCase(plugin, shellContact, shellGeometry, shellInput);
    check(shellResult.ok && finiteResult(shellResult),
          "Physical=3 Contact=3.01 shell-only call is finite");
    check(magnitude(shellResult.normal) == 0.0 &&
          shellContact.effectiveValue("JKR Contact State") == 0.0,
          "first approach at physical overlap -0.009 has zero force");

    CaseInput physicalOnly;
    physicalOnly.contactRadius = 3.0;
    DummyCustomPropertyData physicalContact = makeContactProperties();
    DummyCustomPropertyData physicalGeometry = makeGeometryProperties();
    const CaseResult physicalResult = runCase(plugin, physicalContact,
                                               physicalGeometry, physicalOnly);

    CaseInput expanded = physicalOnly;
    expanded.contactRadius = 3.01;
    expanded.contactOverlap = 0.0101;
    DummyCustomPropertyData expandedContact = makeContactProperties();
    DummyCustomPropertyData expandedGeometry = makeGeometryProperties();
    const CaseResult expandedResult = runCase(plugin, expandedContact,
                                               expandedGeometry, expanded);
    check(nearlyEqual(magnitude(physicalResult.normal),
                      magnitude(expandedResult.normal), 1.0e-11) &&
          nearlyEqual(physicalContact.effectiveValue("JKR Contact Patch Radius"),
                      expandedContact.effectiveValue("JKR Contact Patch Radius"),
                      1.0e-11),
          "contact radius does not change force or JKR patch");

    DummyCustomPropertyData historyContact = makeContactProperties();
    DummyCustomPropertyData historyGeometry = makeGeometryProperties();
    CaseInput load = expanded;
    const CaseResult loadResult = runCase(plugin, historyContact, historyGeometry, load);
    historyContact.commit();
    historyGeometry.commit();
    check(magnitude(loadResult.normal) > 0.0 &&
          historyContact.value("JKR Contact State") > 0.5,
          "physical contact establishes JKR history");

    const double separationLimit = criticalSeparation(3.0, 0.09);
    CaseInput unload = expanded;
    unload.physicalOverlap = 0.5 * separationLimit;
    unload.contactOverlap = 0.01 + unload.physicalOverlap;
    const CaseResult unloadResult = runCase(plugin, historyContact, historyGeometry, unload);
    historyContact.commit();
    check(magnitude(unloadResult.normal) > 0.0 &&
          historyContact.value("JKR Contact State") > 0.5,
          "established JKR contact carries force at negative physical overlap");

    CaseInput broken = unload;
    broken.physicalOverlap = 1.01 * separationLimit;
    broken.contactOverlap = 0.01 + broken.physicalOverlap;
    const CaseResult brokenResult = runCase(plugin, historyContact, historyGeometry, broken);
    historyContact.commit();
    check(magnitude(brokenResult.normal) == 0.0 &&
          historyContact.value("JKR Contact State") == 0.0,
          "JKR contact clears cleanly beyond theoretical separation");

    CaseInput sliding = expanded;
    sliding.tangentialOverlapY = 1.0;
    DummyCustomPropertyData slidingContact = makeContactProperties();
    DummyCustomPropertyData slidingGeometry = makeGeometryProperties();
    const CaseResult slidingResult = runCase(plugin, slidingContact,
                                              slidingGeometry, sliding);
    const double actualTangential = magnitude(slidingResult.tangential);
    const double tangentialLimit =
        slidingContact.effectiveValue("JKR Tangential Force Limit");
    check(actualTangential > 0.0 && actualTangential <= tangentialLimit * (1.0 + 1.0e-12),
          "final tangential force is capped by the JKR repulsive limit");
    check(nearlyEqual(slidingGeometry.effectiveValue("Friction"),
                      actualTangential, 1.0e-12),
          "geometry Friction exports actual force, not capacity");

    CaseInput rolling = expanded;
    rolling.timestep = 1.0e-3;
    rolling.angularVelocityY = 1.0;
    DummyCustomPropertyData rollingContact = makeContactProperties();
    DummyCustomPropertyData rollingGeometry = makeGeometryProperties();
    const CaseResult rollingResult = runCase(plugin, rollingContact,
                                              rollingGeometry, rolling);
    const double rollingTorque = magnitude(rollingResult.torque1);
    const double rollingLimit =
        rollingContact.effectiveValue("JKR Rolling Torque Limit");
    check(rollingTorque > 0.0 && rollingTorque <= rollingLimit * (1.0 + 1.0e-12),
          "history rolling torque is nonzero and limited");
    check(rollingResult.torque1[1] < 0.0 &&
          nearlyEqual(rollingResult.torque1[1], -rollingResult.torque2[1], 1.0e-12),
          "rolling torque opposes motion and reaction torque is balanced");

    CaseInput invalid = expanded;
    invalid.physicalRadius = 0.0;
    DummyCustomPropertyData invalidContact = makeContactProperties();
    DummyCustomPropertyData invalidGeometry = makeGeometryProperties();
    const CaseResult invalidResult = runCase(plugin, invalidContact,
                                              invalidGeometry, invalid);
    check(invalidResult.ok && finiteResult(invalidResult) &&
          magnitude(invalidResult.normal) == 0.0,
          "invalid radius fails safe without NaN or Inf");

    log << "criticalSeparation=" << separationLimit << "\n";
    log << "physical3_contact3.01_initialForce=" << magnitude(shellResult.normal) << "\n";
    log << "contactRadiusIndependentForce=" << magnitude(expandedResult.normal) << "\n";
    log << "actualTangentialForce=" << actualTangential
        << " limit=" << tangentialLimit << "\n";
    log << "rollingTorque=" << rollingTorque << " limit=" << rollingLimit << "\n";
    log << "failures=" << failures << "\n";

    releaseInstance(base);
    FreeLibrary(module);
    return failures == 0 ? 0 : 1;
}
