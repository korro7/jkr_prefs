#include "CJKR.h"

#include "Helpers.h"

#include "ICustomPropertyManagerApi_1_0.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <string>
#include <sstream>



using namespace std;
using namespace NApi;
using namespace NApiCore;
using namespace NApiCm;

namespace
{
    const double OVERLAP_EPS = 1.0e-12;
    const double VECTOR_EPS_SQUARED = 1.0e-30;
    const double TYPE_C_ROLLING_STIFFNESS_FACTOR = 2.0;
    const int JKR_ROOT_ITERATIONS = 100;

    double clampNonNegative(double value)
    {
        return value > 0.0 ? value : 0.0;
    }

    double safeEquivalentRadius(double radiusA, double radiusB)
    {
        if (radiusA <= OVERLAP_EPS || radiusB <= OVERLAP_EPS)
        {
            return 0.0;
        }

        return radiusA * radiusB / (radiusA + radiusB);
    }

    bool isPositiveFinite(double value)
    {
        return std::isfinite(value) && value > 0.0;
    }

    double effectivePropertyValue(NApiCore::ICustomPropertyDataApi_1_0* data,
                                  const char* name,
                                  unsigned int element)
    {
        if (data == 0 || !data->hasData(name))
        {
            return 0.0;
        }

        const double* value = data->getValue(name);
        double* delta = data->getDelta(name);
        return value != 0 && delta != 0 ? value[element] + delta[element] : 0.0;
    }

    void setPropertyValue(NApiCore::ICustomPropertyDataApi_1_0* data,
                          const char* name,
                          unsigned int element,
                          double target)
    {
        if (data == 0 || !data->hasData(name) || !std::isfinite(target))
        {
            return;
        }

        const double* value = data->getValue(name);
        double* delta = data->getDelta(name);
        if (value != 0 && delta != 0)
        {
            delta[element] += target - (value[element] + delta[element]);
        }
    }

    void setVectorProperty(NApiCore::ICustomPropertyDataApi_1_0* data,
                           const char* name,
                           const CSimple3DVector& value)
    {
        setPropertyValue(data, name, 0, value.dx());
        setPropertyValue(data, name, 1, value.dy());
        setPropertyValue(data, name, 2, value.dz());
    }

    CSimple3DVector getVectorProperty(NApiCore::ICustomPropertyDataApi_1_0* data,
                                      const char* name)
    {
        return CSimple3DVector(effectivePropertyValue(data, name, 0),
                               effectivePropertyValue(data, name, 1),
                               effectivePropertyValue(data, name, 2));
    }

    double jkrSeparation(double patchRadius,
                         double equivalentRadius,
                         double equivalentYoungsModulus,
                         double surfaceEnergy)
    {
        return patchRadius * patchRadius / equivalentRadius
             - std::sqrt(4.0 * PI * surfaceEnergy * patchRadius /
                         equivalentYoungsModulus);
    }

    bool solveStableJkrPatchRadius(double physicalOverlap,
                                   double equivalentRadius,
                                   double equivalentYoungsModulus,
                                   double surfaceEnergy,
                                   double& patchRadius,
                                   double& criticalSeparation)
    {
        patchRadius = 0.0;
        criticalSeparation = 0.0;

        if (!std::isfinite(physicalOverlap) ||
            !isPositiveFinite(equivalentRadius) ||
            !isPositiveFinite(equivalentYoungsModulus) ||
            !isPositiveFinite(surfaceEnergy))
        {
            return false;
        }

        const double cutFactor = 0.75 - 1.0 / std::sqrt(2.0);
        const double cutoffPatch = std::cbrt(
            9.0 * PI * surfaceEnergy * equivalentRadius * equivalentRadius *
            cutFactor / (2.0 * equivalentYoungsModulus));
        criticalSeparation = jkrSeparation(cutoffPatch,
                                           equivalentRadius,
                                           equivalentYoungsModulus,
                                           surfaceEnergy);

        const double separationTolerance = std::max(
            1.0e-15,
            1.0e-12 * std::max(equivalentRadius, std::fabs(criticalSeparation)));
        if (!std::isfinite(criticalSeparation) ||
            physicalOverlap < criticalSeparation - separationTolerance)
        {
            return false;
        }

        // On the stable JKR branch separation(a) is monotonic above this point.
        double lower = std::cbrt(PI * surfaceEnergy * equivalentRadius *
                                 equivalentRadius /
                                 (4.0 * equivalentYoungsModulus));
        if (!isPositiveFinite(lower))
        {
            return false;
        }

        double upper = std::max(2.0 * lower,
            std::sqrt(equivalentRadius * clampNonNegative(physicalOverlap)) +
            2.0 * lower);
        int expansion = 0;
        while (expansion < 64 &&
               jkrSeparation(upper, equivalentRadius,
                             equivalentYoungsModulus, surfaceEnergy) < physicalOverlap)
        {
            upper *= 2.0;
            ++expansion;
        }

        if (!isPositiveFinite(upper) || expansion == 64)
        {
            return false;
        }

        for (int iteration = 0; iteration < JKR_ROOT_ITERATIONS; ++iteration)
        {
            const double middle = 0.5 * (lower + upper);
            const double calculatedSeparation = jkrSeparation(
                middle, equivalentRadius, equivalentYoungsModulus, surfaceEnergy);
            if (!std::isfinite(calculatedSeparation))
            {
                return false;
            }

            if (calculatedSeparation < physicalOverlap)
            {
                lower = middle;
            }
            else
            {
                upper = middle;
            }
        }

        patchRadius = 0.5 * (lower + upper);
        const double residual = std::fabs(jkrSeparation(
            patchRadius, equivalentRadius, equivalentYoungsModulus, surfaceEnergy) -
            physicalOverlap);
        const double residualTolerance = std::max(
            1.0e-14,
            1.0e-10 * std::max(equivalentRadius, std::fabs(physicalOverlap)));
        return isPositiveFinite(patchRadius) && residual <= residualTolerance;
    }
}

const string CHertzMindlin::PREFS_FILE = "jkr_prefs.txt";
const string CHertzMindlin::Adhesion_PROPERTY = "Adhesion";
const string CHertzMindlin::Friction_PROPERTY = "Friction";
const string CHertzMindlin::CONTACT_STATE_PROPERTY = "JKR Contact State";
const string CHertzMindlin::ROLLING_DISPLACEMENT_PROPERTY = "JKR Rolling Displacement";
const string CHertzMindlin::ACTUAL_TANGENTIAL_FORCE_PROPERTY = "JKR Actual Tangential Force";
const string CHertzMindlin::TANGENTIAL_FORCE_LIMIT_PROPERTY = "JKR Tangential Force Limit";
const string CHertzMindlin::REPULSIVE_NORMAL_FORCE_PROPERTY = "JKR Repulsive Normal Force";
const string CHertzMindlin::ADHESIVE_NORMAL_FORCE_PROPERTY = "JKR Adhesive Normal Force";
const string CHertzMindlin::NET_NORMAL_FORCE_PROPERTY = "JKR Net Normal Force";
const string CHertzMindlin::CONTACT_PATCH_RADIUS_PROPERTY = "JKR Contact Patch Radius";
const string CHertzMindlin::CONTACT_SLIP_SPEED_PROPERTY = "JKR Contact Slip Speed";
const string CHertzMindlin::ROLLING_TORQUE_PROPERTY = "JKR Rolling Torque";
const string CHertzMindlin::ROLLING_TORQUE_LIMIT_PROPERTY = "JKR Rolling Torque Limit";

CHertzMindlin::CHertzMindlin() :
    m_cohesiveItems(),
    m_geomMgr(0)

{
    ;
}

CHertzMindlin::~CHertzMindlin()
{
    ;
}

void CHertzMindlin::getPreferenceFileName(char prefFileName[FILE_PATH_MAX_LENGTH])
{
    // Copy in pref file name
    strcpy(prefFileName, PREFS_FILE.c_str());
}

bool CHertzMindlin::isThreadSafe()
{
    // thread safe
    return true;
}

bool CHertzMindlin::setup(NApiCore::IApiManager_1_0& apiManager,
                       const char                 prefFile[],
                       char                       customMsg[NApi::ERROR_MSG_MAX_LENGTH])
{
    customMsg[0] = '\0';

    if (prefFile == 0 || prefFile[0] == '\0')
    {
        std::strncpy(customMsg, "JKR preference file path is empty.",
                     NApi::ERROR_MSG_MAX_LENGTH - 1);
        customMsg[NApi::ERROR_MSG_MAX_LENGTH - 1] = '\0';
        return false;
    }

    ifstream prefsFile(prefFile);
    if (!prefsFile)
    {
        std::strncpy(customMsg, "Unable to open the JKR preference file.",
                     NApi::ERROR_MSG_MAX_LENGTH - 1);
        customMsg[NApi::ERROR_MSG_MAX_LENGTH - 1] = '\0';
        return false;
    }

    m_cohesiveItems.clear();
    string line;
    unsigned int lineNumber = 0;
    while (std::getline(prefsFile, line))
    {
        ++lineNumber;
        const string::size_type comment = line.find('#');
        if (comment != string::npos)
        {
            line.erase(comment);
        }

        istringstream parser(line);
        string pairName;
        double surfaceEnergy = 0.0;
        if (!(parser >> pairName))
        {
            continue;
        }

        const string::size_type separator = pairName.find(':');
        if (!(parser >> surfaceEnergy) || separator == string::npos ||
            separator == 0 || separator + 1 >= pairName.size() ||
            !std::isfinite(surfaceEnergy) || surfaceEnergy < 0.0)
        {
            ostringstream error;
            error << "Invalid JKR preference at line " << lineNumber
                  << ". Expected materialA:materialB nonNegativeSurfaceEnergy.";
            const string message = error.str();
            std::strncpy(customMsg, message.c_str(),
                         NApi::ERROR_MSG_MAX_LENGTH - 1);
            customMsg[NApi::ERROR_MSG_MAX_LENGTH - 1] = '\0';
            return false;
        }

        m_cohesiveItems.addCohesion(pairName.substr(0, separator),
                                    pairName.substr(separator + 1),
                                    surfaceEnergy);
    }

    return true;
}

bool CHertzMindlin::usesCustomProperties()
{
    // Use custom properties
    return true;
}

unsigned int CHertzMindlin::getNumberOfRequiredProperties(
    const NApi::EPluginPropertyCategory category)
{
    if (category == eGeometry)
    {
        return 2;
    }
    else if (category == eContact)
    {
        return 11;
    }
    else
    {
        return 0;
    }
}

bool CHertzMindlin::getDetailsForProperty(
    unsigned int				propertyIndex,
    EPluginPropertyCategory     category,
    char                        name[CUSTOM_PROP_MAX_NAME_LENGTH],
    EPluginPropertyDataTypes& dataType,
    unsigned int& numberOfElements,
    EPluginPropertyUnitTypes& unitType,
    char                            initValBuff[NApi::BUFF_SIZE])

{
    const char* defVal = "0";

    // Geometry totals are retained for existing EDEM decks and Analyst plots.
    if (propertyIndex == 0 && eGeometry == category)
    {
        strcpy(name, Adhesion_PROPERTY.c_str());
        dataType = eDouble;
        numberOfElements = 1;
        unitType = eForce;
        strcpy(initValBuff, defVal);
        return true;
    }
    else if (propertyIndex == 1 && eGeometry == category)
    {
        strcpy(name, Friction_PROPERTY.c_str());
        dataType = eDouble;
        numberOfElements = 1;
        unitType = eForce;
        strcpy(initValBuff, defVal);
        return true;
    }

    if (eContact == category)
    {
        const string* propertyName = 0;
        switch (propertyIndex)
        {
            case 0:
                propertyName = &CONTACT_STATE_PROPERTY;
                unitType = eNone;
                break;
            case 1:
                propertyName = &ROLLING_DISPLACEMENT_PROPERTY;
                unitType = eLength;
                numberOfElements = 3;
                break;
            case 2:
                propertyName = &ACTUAL_TANGENTIAL_FORCE_PROPERTY;
                unitType = eForce;
                break;
            case 3:
                propertyName = &TANGENTIAL_FORCE_LIMIT_PROPERTY;
                unitType = eForce;
                break;
            case 4:
                propertyName = &REPULSIVE_NORMAL_FORCE_PROPERTY;
                unitType = eForce;
                break;
            case 5:
                propertyName = &ADHESIVE_NORMAL_FORCE_PROPERTY;
                unitType = eForce;
                break;
            case 6:
                propertyName = &NET_NORMAL_FORCE_PROPERTY;
                unitType = eForce;
                break;
            case 7:
                propertyName = &CONTACT_PATCH_RADIUS_PROPERTY;
                unitType = eLength;
                break;
            case 8:
                propertyName = &CONTACT_SLIP_SPEED_PROPERTY;
                unitType = eVelocity;
                break;
            case 9:
                propertyName = &ROLLING_TORQUE_PROPERTY;
                unitType = eTorque;
                break;
            case 10:
                propertyName = &ROLLING_TORQUE_LIMIT_PROPERTY;
                unitType = eTorque;
                break;
            default:
                return false;
        }

        strcpy(name, propertyName->c_str());
        dataType = eDouble;
        if (propertyIndex != 1)
        {
            numberOfElements = 1;
        }
        strcpy(initValBuff, propertyIndex == 1 ? "0,0,0" : defVal);
        return true;
    }


    return false;
}


bool CHertzMindlin::starting(NApiCore::IApiManager_1_0& apiManager, int numThreads)
{
    m_geomMgr = static_cast<NApiCore::IGeometryManagerApi_1_0*>(apiManager.getApi(eGeometryManager, 1, 0));

    if (0 == m_geomMgr)
    {
        return false;
    }

    return true;
}

void CHertzMindlin::stopping(NApiCore::IApiManager_1_0& apiManager)
{
    ;
}

void CHertzMindlin::configForTimeStep(NApiCore::ICustomPropertyDataApi_1_0* simPropData,
    NApiCore::ICustomPropertyDataApi_1_0* particlePropData)

{
    /**************************************************************************************/
    /*              Reset the Geometry custom property PRESSURE_PROPERTY                  */
    /*                of the geometry equipment called "Bucket"                           */
    /**************************************************************************************/

    if (m_geomMgr != 0)
    {
        m_geomMgr->resetCustomProperty("blade", Adhesion_PROPERTY.c_str(), 0.0);
        m_geomMgr->resetCustomProperty("blade", Friction_PROPERTY.c_str(), 0.0);
    }

}



NApi::ECalculateResult CHertzMindlin::calculateForce(
    int          threadId,
    double       time,
    double       timestep,
    int          elem1Id,
    const char   elem1Type[],
    double       elem1Mass,
    double       elem1Density,
    double       elem1Volume,
    unsigned int elem1Surfaces,
    double       elem1MoIX,
    double       elem1MoIY,
    double       elem1MoIZ,
    double       elem1ShearMod,
    double       elem1Poisson,
    double       elem1ContactRadius,
    double       elem1PhysicalRadius,
    double       elem1PosX,
    double       elem1PosY,
    double       elem1PosZ,
    double       elem1ComX,
    double       elem1ComY,
    double       elem1ComZ,
    double       elem1ContactPointVelX,
    double       elem1ContactPointVelY,
    double       elem1ContactPointVelZ,
    double       elem1VelX,
    double       elem1VelY,
    double       elem1VelZ,
    double       elem1AngVelX,
    double       elem1AngVelY,
    double       elem1AngVelZ,
    double       elem1Charge,
    double       elem1WorkFunction,
    const double elem1Orientation[9],
    NApiCore::ICustomPropertyDataApi_1_0* elem1PropData,
    bool         elem2IsSurf,
    int          elem2Id,
    const char   elem2Type[],
    double       elem2Mass,
    double       elem2Density,
    double       elem2Volume,
    unsigned int elem2Surfaces,
    double       elem2MoIX,
    double       elem2MoIY,
    double       elem2MoIZ,
    double       elem2Area,
    double       elem2ShearMod,
    double       elem2Poisson,
    double       elem2ContactRadius,
    double       elem2PhysicalRadius,
    double       elem2PosX,
    double       elem2PosY,
    double       elem2PosZ,
    double       elem2ComX,
    double       elem2ComY,
    double       elem2ComZ,
    double       elem2ContactPointVelX,
    double       elem2ContactPointVelY,
    double       elem2ContactPointVelZ,
    double       elem2VelX,
    double       elem2VelY,
    double       elem2VelZ,
    double       elem2AngVelX,
    double       elem2AngVelY,
    double       elem2AngVelZ,
    double       elem2Charge,
    double       elem2WorkFunction,
    const double elem2Orientation[9],
    NApiCore::ICustomPropertyDataApi_1_0* elem2PropData,
    NApiCore::ICustomPropertyDataApi_1_0* contactPropData,
    NApiCore::ICustomPropertyDataApi_1_0* simulationPropData,
    double       coeffRest,
    double       staticFriction,
    double       rollingFriction,
    double       contactPointX,
    double       contactPointY,
    double       contactPointZ,
    double       normalContactOverlap,
    double       normalPhysicalOverlap,
    double& tangentialPhysicalOverlapX,
    double& tangentialPhysicalOverlapY,
    double& tangentialPhysicalOverlapZ,
    double& calculatedNormalForceX,
    double& calculatedNormalForceY,
    double& calculatedNormalForceZ,
    double& calculatedUnsymNormalForceX,
    double& calculatedUnsymNormalForceY,
    double& calculatedUnsymNormalForceZ,
    double& calculatedTangentialForceX,
    double& calculatedTangentialForceY,
    double& calculatedTangentialForceZ,
    double& calculatedUnsymTangentialForceX,
    double& calculatedUnsymTangentialForceY,
    double& calculatedUnsymTangentialForceZ,
    double& calculatedElem1AdditionalTorqueX,
    double& calculatedElem1AdditionalTorqueY,
    double& calculatedElem1AdditionalTorqueZ,
    double& calculatedElem1UnsymAdditionalTorqueX,
    double& calculatedElem1UnsymAdditionalTorqueY,
    double& calculatedElem1UnsymAdditionalTorqueZ,
    double& calculatedElem2AdditionalTorqueX,
    double& calculatedElem2AdditionalTorqueY,
    double& calculatedElem2AdditionalTorqueZ,
    double& calculatedElem2UnsymAdditionalTorqueX,
    double& calculatedElem2UnsymAdditionalTorqueY,
    double& calculatedElem2UnsymAdditionalTorqueZ,
    double& calculatedChargeMovedToElem1)
{
    // This is a complete base model. Contact radii are intentionally not used
    // in any constitutive equation; EDEM uses them only to retain the contact.
    (void)normalContactOverlap;
    (void)elem1ContactRadius;
    (void)elem2ContactRadius;

    calculatedNormalForceX = 0.0;
    calculatedNormalForceY = 0.0;
    calculatedNormalForceZ = 0.0;
    calculatedUnsymNormalForceX = 0.0;
    calculatedUnsymNormalForceY = 0.0;
    calculatedUnsymNormalForceZ = 0.0;
    calculatedTangentialForceX = 0.0;
    calculatedTangentialForceY = 0.0;
    calculatedTangentialForceZ = 0.0;
    calculatedUnsymTangentialForceX = 0.0;
    calculatedUnsymTangentialForceY = 0.0;
    calculatedUnsymTangentialForceZ = 0.0;
    calculatedElem1AdditionalTorqueX = 0.0;
    calculatedElem1AdditionalTorqueY = 0.0;
    calculatedElem1AdditionalTorqueZ = 0.0;
    calculatedElem1UnsymAdditionalTorqueX = 0.0;
    calculatedElem1UnsymAdditionalTorqueY = 0.0;
    calculatedElem1UnsymAdditionalTorqueZ = 0.0;
    calculatedElem2AdditionalTorqueX = 0.0;
    calculatedElem2AdditionalTorqueY = 0.0;
    calculatedElem2AdditionalTorqueZ = 0.0;
    calculatedElem2UnsymAdditionalTorqueX = 0.0;
    calculatedElem2UnsymAdditionalTorqueY = 0.0;
    calculatedElem2UnsymAdditionalTorqueZ = 0.0;
    calculatedChargeMovedToElem1 = 0.0;
    const auto resetContactHistory = [&]()
    {
        tangentialPhysicalOverlapX = 0.0;
        tangentialPhysicalOverlapY = 0.0;
        tangentialPhysicalOverlapZ = 0.0;
        setPropertyValue(contactPropData, CONTACT_STATE_PROPERTY.c_str(), 0, 0.0);
        setVectorProperty(contactPropData, ROLLING_DISPLACEMENT_PROPERTY.c_str(),
                          CSimple3DVector());
        setPropertyValue(contactPropData, ACTUAL_TANGENTIAL_FORCE_PROPERTY.c_str(), 0, 0.0);
        setPropertyValue(contactPropData, TANGENTIAL_FORCE_LIMIT_PROPERTY.c_str(), 0, 0.0);
        setPropertyValue(contactPropData, REPULSIVE_NORMAL_FORCE_PROPERTY.c_str(), 0, 0.0);
        setPropertyValue(contactPropData, ADHESIVE_NORMAL_FORCE_PROPERTY.c_str(), 0, 0.0);
        setPropertyValue(contactPropData, NET_NORMAL_FORCE_PROPERTY.c_str(), 0, 0.0);
        setPropertyValue(contactPropData, CONTACT_PATCH_RADIUS_PROPERTY.c_str(), 0, 0.0);
        setPropertyValue(contactPropData, CONTACT_SLIP_SPEED_PROPERTY.c_str(), 0, 0.0);
        setPropertyValue(contactPropData, ROLLING_TORQUE_PROPERTY.c_str(), 0, 0.0);
        setPropertyValue(contactPropData, ROLLING_TORQUE_LIMIT_PROPERTY.c_str(), 0, 0.0);
    };

    if (!std::isfinite(normalPhysicalOverlap) ||
        !isPositiveFinite(elem1PhysicalRadius) ||
        (elem2IsSurf && !isPositiveFinite(elem2PhysicalRadius)) ||
        !isPositiveFinite(elem1ShearMod) ||
        !isPositiveFinite(elem2ShearMod) ||
        !std::isfinite(elem1Poisson) || !std::isfinite(elem2Poisson))
    {
        resetContactHistory();
        return eSuccess;
    }

    CSimple3DVector unitCPVect =
        CSimple3DPoint(contactPointX, contactPointY, contactPointZ) -
        CSimple3DPoint(elem1PosX, elem1PosY, elem1PosZ);
    if (!std::isfinite(unitCPVect.lengthSquared()) ||
        unitCPVect.lengthSquared() <= VECTOR_EPS_SQUARED)
    {
        resetContactHistory();
        return eSuccess;
    }
    unitCPVect.normalise();

    const CSimple3DVector relVel =
        CSimple3DVector(elem1ContactPointVelX, elem1ContactPointVelY,
                        elem1ContactPointVelZ) -
        CSimple3DVector(elem2ContactPointVelX, elem2ContactPointVelY,
                        elem2ContactPointVelZ);
    const CSimple3DVector relVel_n = unitCPVect * unitCPVect.dot(relVel);
    const CSimple3DVector relVel_t = relVel - relVel_n;
    const CSimple3DVector angVel1(elem1AngVelX, elem1AngVelY, elem1AngVelZ);
    const CSimple3DVector angVel2(elem2AngVelX, elem2AngVelY, elem2AngVelZ);

    // elem2IsSurf is true for a second particle surface and false for geometry.
    const double nEquivRadius = elem2IsSurf
        ? safeEquivalentRadius(elem1PhysicalRadius, elem2PhysicalRadius)
        : elem1PhysicalRadius;
    double nEquivMass = elem1Mass;
    if (elem2IsSurf && isPositiveFinite(elem1Mass) && isPositiveFinite(elem2Mass) &&
        isPositiveFinite(elem1Mass + elem2Mass))
    {
        nEquivMass = elem1Mass * elem2Mass / (elem1Mass + elem2Mass);
    }

    const double nYoungsMod1 = 2.0 * (1.0 + elem1Poisson) * elem1ShearMod;
    const double nYoungsMod2 = 2.0 * (1.0 + elem2Poisson) * elem2ShearMod;
    if (!isPositiveFinite(nEquivRadius) || !isPositiveFinite(nEquivMass) ||
        !isPositiveFinite(nYoungsMod1) || !isPositiveFinite(nYoungsMod2))
    {
        resetContactHistory();
        return eSuccess;
    }

    const double inverseYoungs =
        (1.0 - elem1Poisson * elem1Poisson) / nYoungsMod1 +
        (1.0 - elem2Poisson * elem2Poisson) / nYoungsMod2;
    const double inverseShear =
        (2.0 - elem1Poisson) / elem1ShearMod +
        (2.0 - elem2Poisson) / elem2ShearMod;
    if (!isPositiveFinite(inverseYoungs) || !isPositiveFinite(inverseShear))
    {
        resetContactHistory();
        return eSuccess;
    }

    const double nEquivYoungsMod = 1.0 / inverseYoungs;
    const double nEquivShearMod = 1.0 / inverseShear;
    double nSurfaceEnergy = m_cohesiveItems.getCohesion(elem1Type, elem2Type);
    if (!std::isfinite(nSurfaceEnergy) || nSurfaceEnergy < 0.0)
    {
        nSurfaceEnergy = 0.0;
    }

    const double storedState = effectivePropertyValue(
        contactPropData, CONTACT_STATE_PROPERTY.c_str(), 0);
    bool contactEstablished = storedState > 0.5;
    double contactPatchRadius = 0.0;
    double criticalSeparation = 0.0;

    if (nSurfaceEnergy > 0.0)
    {
        if (!contactEstablished)
        {
            if (normalPhysicalOverlap < -OVERLAP_EPS)
            {
                resetContactHistory();
                return eSuccess;
            }
            contactEstablished = true;
        }

        if (!solveStableJkrPatchRadius(normalPhysicalOverlap, nEquivRadius,
                                       nEquivYoungsMod, nSurfaceEnergy,
                                       contactPatchRadius, criticalSeparation))
        {
            resetContactHistory();
            return eSuccess;
        }
    }
    else
    {
        if (normalPhysicalOverlap <= OVERLAP_EPS)
        {
            resetContactHistory();
            return eSuccess;
        }
        contactEstablished = true;
        contactPatchRadius = std::sqrt(nEquivRadius * normalPhysicalOverlap);
    }

    const double maximumPhysicalPatch = elem2IsSurf
        ? std::min(elem1PhysicalRadius, elem2PhysicalRadius)
        : elem1PhysicalRadius;
    contactPatchRadius = std::min(contactPatchRadius, maximumPhysicalPatch);
    if (!isPositiveFinite(contactPatchRadius))
    {
        resetContactHistory();
        return eSuccess;
    }

    const double patchCubed = contactPatchRadius * contactPatchRadius *
                               contactPatchRadius;
    double hertzForce = 4.0 * nEquivYoungsMod * patchCubed /
                        (3.0 * nEquivRadius);
    double cohesionForce = nSurfaceEnergy > 0.0
        ? 4.0 * std::sqrt(PI * nSurfaceEnergy * nEquivYoungsMod * patchCubed)
        : 0.0;
    if (!std::isfinite(hertzForce) || !std::isfinite(cohesionForce) ||
        hertzForce < 0.0 || cohesionForce < 0.0)
    {
        resetContactHistory();
        return eSuccess;
    }

    double dampingRatio = 0.0;
    if (std::isfinite(coeffRest) && coeffRest < 1.0)
    {
        const double safeRestitution = std::max(coeffRest, 1.0e-12);
        const double restitutionLog = std::log(safeRestitution);
        dampingRatio = -restitutionLog /
            std::sqrt(restitutionLog * restitutionLog + PI * PI);
    }

    // The JKR patch remains finite for negative physical overlap, so it also
    // controls normal and tangential stiffness during adhesive unloading.
    const double normalStiffness = 2.0 * nEquivYoungsMod * contactPatchRadius;
    const double normalDamping = 2.0 * std::sqrt(5.0 / 6.0) * dampingRatio *
        std::sqrt(normalStiffness * nEquivMass);
    const CSimple3DVector dissipativeNormalForce = -relVel_n * normalDamping;
    const CSimple3DVector elasticNormalForce =
        -unitCPVect * (hertzForce - cohesionForce);
    const CSimple3DVector actualNormalForce =
        elasticNormalForce + dissipativeNormalForce;

    calculatedNormalForceX = actualNormalForce.dx();
    calculatedNormalForceY = actualNormalForce.dy();
    calculatedNormalForceZ = actualNormalForce.dz();
    calculatedUnsymNormalForceX = dissipativeNormalForce.dx();
    calculatedUnsymNormalForceY = dissipativeNormalForce.dy();
    calculatedUnsymNormalForceZ = dissipativeNormalForce.dz();

    CSimple3DVector tangentialOverlap(tangentialPhysicalOverlapX,
                                      tangentialPhysicalOverlapY,
                                      tangentialPhysicalOverlapZ);
    tangentialOverlap -= unitCPVect * tangentialOverlap.dot(unitCPVect);
    const double tangentialStiffness =
        8.0 * nEquivShearMod * contactPatchRadius;
    const double tangentialDamping =
        2.0 * std::sqrt(5.0 / 6.0) * dampingRatio *
        std::sqrt(tangentialStiffness * nEquivMass);
    const CSimple3DVector elasticTangentialForce =
        -tangentialOverlap * tangentialStiffness;
    const CSimple3DVector dampingTangentialForce =
        -relVel_t * tangentialDamping;
    const CSimple3DVector trialTangentialForce =
        elasticTangentialForce + dampingTangentialForce;
    const double tangentialForceLimit =
        std::max(0.0, staticFriction) * hertzForce;

    CSimple3DVector actualTangentialForce = trialTangentialForce;
    CSimple3DVector dissipativeTangentialForce = dampingTangentialForce;
    bool tangentialSliding = false;
    const double trialTangentialMagnitude = trialTangentialForce.length();
    if (tangentialForceLimit <= 0.0)
    {
        tangentialSliding = trialTangentialMagnitude * trialTangentialMagnitude >
                            VECTOR_EPS_SQUARED;
        actualTangentialForce = CSimple3DVector();
        dissipativeTangentialForce = CSimple3DVector();
        tangentialOverlap = CSimple3DVector();
    }
    else if (trialTangentialMagnitude > tangentialForceLimit &&
        trialTangentialMagnitude * trialTangentialMagnitude > VECTOR_EPS_SQUARED)
    {
        tangentialSliding = true;
        actualTangentialForce = trialTangentialForce *
            (tangentialForceLimit / trialTangentialMagnitude);
        tangentialOverlap = -actualTangentialForce / tangentialStiffness;
        dissipativeTangentialForce = actualTangentialForce;
    }

    calculatedTangentialForceX = actualTangentialForce.dx();
    calculatedTangentialForceY = actualTangentialForce.dy();
    calculatedTangentialForceZ = actualTangentialForce.dz();
    calculatedUnsymTangentialForceX = dissipativeTangentialForce.dx();
    calculatedUnsymTangentialForceY = dissipativeTangentialForce.dy();
    calculatedUnsymTangentialForceZ = dissipativeTangentialForce.dz();
    tangentialPhysicalOverlapX = tangentialOverlap.dx();
    tangentialPhysicalOverlapY = tangentialOverlap.dy();
    tangentialPhysicalOverlapZ = tangentialOverlap.dz();

    CSimple3DVector rollingDisplacement = getVectorProperty(
        contactPropData, ROLLING_DISPLACEMENT_PROPERTY.c_str());
    rollingDisplacement -= unitCPVect * rollingDisplacement.dot(unitCPVect);
    const CSimple3DVector relativeAngularVelocity = angVel1 - angVel2;
    const CSimple3DVector rollingAngularVelocity = relativeAngularVelocity -
        unitCPVect * relativeAngularVelocity.dot(unitCPVect);
    const double rollingRadius = elem2IsSurf
        ? safeEquivalentRadius(elem1PhysicalRadius, elem2PhysicalRadius)
        : elem1PhysicalRadius;
    const double positiveRollingFriction = std::max(0.0, rollingFriction);

    double rollingStiffnessPerLength = 0.0;
    double rollingTorqueLimit = 0.0;
    if (positiveRollingFriction > 0.0 && isPositiveFinite(rollingRadius))
    {
        if (std::isfinite(timestep) && timestep > 0.0)
        {
            rollingDisplacement += rollingAngularVelocity *
                (rollingRadius * timestep);
        }

        const double coulombRollingLimit =
            positiveRollingFriction * rollingRadius * hertzForce;
        if (nSurfaceEnergy > 0.0)
        {
            const double zeroOverlapPatch = std::cbrt(
                4.0 * PI * nSurfaceEnergy * nEquivRadius * nEquivRadius /
                nEquivYoungsMod);
            const double pullOffForce =
                1.5 * PI * nSurfaceEnergy * nEquivRadius;
            if (isPositiveFinite(zeroOverlapPatch) && isPositiveFinite(pullOffForce))
            {
                rollingStiffnessPerLength = 4.0 * pullOffForce *
                    std::pow(contactPatchRadius / zeroOverlapPatch, 1.5);
                const double adhesiveRollingLimit = rollingStiffnessPerLength *
                    positiveRollingFriction * rollingRadius;
                rollingTorqueLimit = std::min(coulombRollingLimit,
                                               adhesiveRollingLimit);
            }
        }
        else
        {
            rollingStiffnessPerLength =
                TYPE_C_ROLLING_STIFFNESS_FACTOR * normalStiffness *
                positiveRollingFriction * positiveRollingFriction * rollingRadius;
            rollingTorqueLimit = coulombRollingLimit;
        }
    }

    CSimple3DVector actualRollingTorque;
    CSimple3DVector dissipativeRollingTorque;
    bool rollingSlip = false;
    if (isPositiveFinite(rollingStiffnessPerLength) &&
        isPositiveFinite(rollingTorqueLimit))
    {
        const double rollingAngularStiffness =
            rollingStiffnessPerLength * rollingRadius;
        double moment1 = (elem1MoIX + elem1MoIY + elem1MoIZ) / 3.0;
        double moment2 = (elem2MoIX + elem2MoIY + elem2MoIZ) / 3.0;
        double equivalentMoment = moment1;
        if (elem2IsSurf && isPositiveFinite(moment1) && isPositiveFinite(moment2))
        {
            equivalentMoment = moment1 * moment2 / (moment1 + moment2);
        }
        if (!isPositiveFinite(equivalentMoment))
        {
            equivalentMoment = 0.0;
        }

        const CSimple3DVector elasticRollingTorque =
            -rollingDisplacement * rollingStiffnessPerLength;
        const double rollingDamping = equivalentMoment > 0.0
            ? 2.0 * dampingRatio *
              std::sqrt(equivalentMoment * rollingAngularStiffness)
            : 0.0;
        const CSimple3DVector dampingRollingTorque =
            -rollingAngularVelocity * rollingDamping;
        const CSimple3DVector trialRollingTorque =
            elasticRollingTorque + dampingRollingTorque;
        const double trialRollingMagnitude = trialRollingTorque.length();

        actualRollingTorque = trialRollingTorque;
        dissipativeRollingTorque = dampingRollingTorque;
        if (trialRollingMagnitude > rollingTorqueLimit &&
            trialRollingMagnitude * trialRollingMagnitude > VECTOR_EPS_SQUARED)
        {
            rollingSlip = true;
            actualRollingTorque = trialRollingTorque *
                (rollingTorqueLimit / trialRollingMagnitude);
            rollingDisplacement = -actualRollingTorque /
                rollingStiffnessPerLength;
            dissipativeRollingTorque = actualRollingTorque;
        }
    }
    else
    {
        rollingDisplacement = CSimple3DVector();
        rollingTorqueLimit = 0.0;
    }

    setVectorProperty(contactPropData, ROLLING_DISPLACEMENT_PROPERTY.c_str(),
                      rollingDisplacement);
    calculatedElem1AdditionalTorqueX = actualRollingTorque.dx();
    calculatedElem1AdditionalTorqueY = actualRollingTorque.dy();
    calculatedElem1AdditionalTorqueZ = actualRollingTorque.dz();
    calculatedElem1UnsymAdditionalTorqueX = dissipativeRollingTorque.dx();
    calculatedElem1UnsymAdditionalTorqueY = dissipativeRollingTorque.dy();
    calculatedElem1UnsymAdditionalTorqueZ = dissipativeRollingTorque.dz();
    calculatedElem2AdditionalTorqueX = -actualRollingTorque.dx();
    calculatedElem2AdditionalTorqueY = -actualRollingTorque.dy();
    calculatedElem2AdditionalTorqueZ = -actualRollingTorque.dz();
    calculatedElem2UnsymAdditionalTorqueX = -dissipativeRollingTorque.dx();
    calculatedElem2UnsymAdditionalTorqueY = -dissipativeRollingTorque.dy();
    calculatedElem2UnsymAdditionalTorqueZ = -dissipativeRollingTorque.dz();

    // State is a bit mask: 1=established, 2=tangential sliding, 4=rolling slip.
    double contactState = contactEstablished ? 1.0 : 0.0;
    if (tangentialSliding)
    {
        contactState += 2.0;
    }
    if (rollingSlip)
    {
        contactState += 4.0;
    }
    setPropertyValue(contactPropData, CONTACT_STATE_PROPERTY.c_str(), 0, contactState);
    setPropertyValue(contactPropData, ACTUAL_TANGENTIAL_FORCE_PROPERTY.c_str(), 0,
                     actualTangentialForce.length());
    setPropertyValue(contactPropData, TANGENTIAL_FORCE_LIMIT_PROPERTY.c_str(), 0,
                     tangentialForceLimit);
    setPropertyValue(contactPropData, REPULSIVE_NORMAL_FORCE_PROPERTY.c_str(), 0,
                     hertzForce);
    setPropertyValue(contactPropData, ADHESIVE_NORMAL_FORCE_PROPERTY.c_str(), 0,
                     cohesionForce);
    setPropertyValue(contactPropData, NET_NORMAL_FORCE_PROPERTY.c_str(), 0,
                     actualNormalForce.length());
    setPropertyValue(contactPropData, CONTACT_PATCH_RADIUS_PROPERTY.c_str(), 0,
                     contactPatchRadius);
    setPropertyValue(contactPropData, CONTACT_SLIP_SPEED_PROPERTY.c_str(), 0,
                     relVel_t.length());
    setPropertyValue(contactPropData, ROLLING_TORQUE_PROPERTY.c_str(), 0,
                     actualRollingTorque.length());
    setPropertyValue(contactPropData, ROLLING_TORQUE_LIMIT_PROPERTY.c_str(), 0,
                     rollingTorqueLimit);

    if (!elem2IsSurf && elem2PropData != 0)
    {
        if (elem2PropData->hasData(Adhesion_PROPERTY.c_str()))
        {
            double* adhesionDelta = elem2PropData->getDelta(Adhesion_PROPERTY.c_str());
            if (adhesionDelta != 0)
            {
                adhesionDelta[0] += cohesionForce;
            }
        }
        if (elem2PropData->hasData(Friction_PROPERTY.c_str()))
        {
            double* frictionDelta = elem2PropData->getDelta(Friction_PROPERTY.c_str());
            if (frictionDelta != 0)
            {
                frictionDelta[0] += actualTangentialForce.length();
            }
        }
    }

    return eSuccess;
}
