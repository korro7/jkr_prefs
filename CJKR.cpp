#include "CJKR.h"
#include "quarticroot.h"

#include "Helpers.h"

#include "ICustomPropertyManagerApi_1_0.h"
#include <string>
#include <sstream>



using namespace std;
using namespace NApi;
using namespace NApiCore;
using namespace NApiCm;

const string CHertzMindlin::PREFS_FILE = "jkr_prefs.txt";
const string CHertzMindlin::Adhesion_PROPERTY = "Adhesion";
const string CHertzMindlin::Friction_PROPERTY = "Friction";

CHertzMindlin::CHertzMindlin() :
    m_cohesiveItems()

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

    ifstream prefsFile(prefFile);

    if(!prefsFile)
        {
            return false;
        }
    else
        {
            double surfaceEnergy;
            string str;
            while (prefsFile)
                {
                    prefsFile >> str
                              >> surfaceEnergy;
                    
                    string::size_type i (str.find (':'));
                    string surfA = str.substr (0, i);
                    str.erase (0, i + 1);
                    string surfB = str;
                    
                    m_cohesiveItems.addCohesion(surfA, surfB,surfaceEnergy);
                }

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
        return 1;
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

    // 几何属性 第0个：粘附能
    if (propertyIndex == 0 && eGeometry == category)
    {
        strcpy(name, Adhesion_PROPERTY.c_str());
        dataType = eDouble;
        numberOfElements = 1;
        unitType = eForce;
        strcpy(initValBuff, defVal);
        return true;
    }
    // 几何属性 第1个：摩擦属性
    else if (propertyIndex == 1 && eGeometry == category)
    {
        strcpy(name, Friction_PROPERTY.c_str());
        dataType = eDouble;
        numberOfElements = 1;
        unitType = eForce;
        strcpy(initValBuff, defVal);
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

    m_geomMgr->resetCustomProperty("Blade", Adhesion_PROPERTY.c_str(), 0.0);
    m_geomMgr->resetCustomProperty("Blade", Friction_PROPERTY.c_str(), 0.0);

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
    // This contact model assumes that the Physical and the Contact radius are the same

    // Calculate the relative velocities of the elements
    CSimple3DVector relVel = CSimple3DVector(elem1VelX, elem1VelY, elem1VelZ) -
                             CSimple3DVector(elem2VelX, elem2VelY, elem2VelZ);

    // Put the values into a more useful form
    CSimple3DVector angVel1(elem1AngVelX, elem1AngVelY, elem1AngVelZ);
    CSimple3DVector angVel2(elem2AngVelX, elem2AngVelY, elem2AngVelZ);
    CSimple3DPoint  contactPoint(contactPointX, contactPointY, contactPointZ);

    // The unit vector from element 1 to the contact point
    CSimple3DVector unitCPVect = contactPoint - CSimple3DPoint(elem1PosX, elem1PosY, elem1PosZ) ;
    unitCPVect.normalise();

    // Clear return values
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

    // normal and tangential components of the relative velocity at the contact point
    CSimple3DVector relVel_n = unitCPVect * unitCPVect.dot(relVel);
    CSimple3DVector relVel_t = relVel - relVel_n;

    // Equivalent radii & mass
    double nEquivRadius = elem1PhysicalRadius * elem2PhysicalRadius /
        (elem1PhysicalRadius + elem2PhysicalRadius);

    double nEquivMass = elem1Mass * elem2Mass /
                        (elem1Mass + elem2Mass);

    // Effective Young's Modulus
    double nYoungsMod1 = 2 * (1.0 + elem1Poisson) * elem1ShearMod;
    double nYoungsMod2 = 2 * (1.0 + elem2Poisson) * elem2ShearMod;

    // Equivalent Young's & shear modulus
    double nEquivYoungsMod =  ( 1 - pow(elem1Poisson, 2) ) / nYoungsMod1
                            + ( 1 - pow(elem2Poisson, 2) ) / nYoungsMod2;

    nEquivYoungsMod = 1.0 / nEquivYoungsMod;

    double nEquivShearMod =   ( 2.0 - elem1Poisson ) /  elem1ShearMod
                            + ( 2.0 - elem2Poisson ) /  elem2ShearMod;

    nEquivShearMod = 1.0 / nEquivShearMod;


    /*****************************/
    /* Normal Calculation        */
    /*****************************/

    /* Cohesion JKR */
    double nSurfaceEnergy         = m_cohesiveItems.getCohesion(elem1Type, elem2Type);


    //if (nSurfaceEnergy <= 1e-12)
        //nSurfaceEnergy = 5; // 自定义默认值

    double cohesionForce = 0.0;
    double hertzForce = 0.0;

    if ( nSurfaceEnergy > 0.0 )
        {
            double a[5], rr[4], ri[4];
            a[0]= 1.0 / nEquivRadius / nEquivRadius;
            a[1]= 0.0;
            a[2]= - 2.0 * normalPhysicalOverlap / nEquivRadius;
            a[3]= -4.0*PI*nSurfaceEnergy/ nEquivYoungsMod;
            a[4]= normalPhysicalOverlap * normalPhysicalOverlap;
            quart(a, rr, ri);
            double nContactRadius = rr[3]; // right root of quartic equation selected
        
            cohesionForce = 4.0*sqrt(PI * nSurfaceEnergy * nEquivYoungsMod) * pow(nContactRadius, 1.5);
            hertzForce = 4.0 / 3.0 * nEquivYoungsMod / nEquivRadius * pow(nContactRadius, 3.0);
        } else
        {
            hertzForce = ( 4.0 / 3.0 * nEquivYoungsMod * sqrt(nEquivRadius) ) * pow(normalPhysicalOverlap, 1.5);
        }
        
    CSimple3DVector F_hertz  = - unitCPVect * hertzForce;
    CSimple3DVector F_n  = - unitCPVect * (hertzForce - cohesionForce);

    /* end of Cohesion JKR paragraph */




    // Damping calculation
    double B = 0.0;
    if(coeffRest > 0.0)
    {
        double myLog = log(coeffRest);
        B = - myLog/ sqrt(myLog * myLog + PI * PI);
    }

    double S_n = 2.0 * nEquivYoungsMod * sqrt(nEquivRadius * normalPhysicalOverlap);
    CSimple3DVector F_nd = unitCPVect * 2 * sqrt(5.0 / 6.0) * B * sqrt(S_n * nEquivMass) *  relVel_n.length();

    // Are we in a loading situation?
    if(relVel_n.dot(unitCPVect) > 0.0)
    {
        F_nd = -F_nd;
    }

    // Fill in parameters we were passed in
    CSimple3DVector newF_n = F_n + F_nd;
    calculatedUnsymNormalForceX = F_nd.dx();
    calculatedUnsymNormalForceY = F_nd.dy();
    calculatedUnsymNormalForceZ = F_nd.dz();

    calculatedNormalForceX = newF_n.dx();
    calculatedNormalForceY = newF_n.dy();
    calculatedNormalForceZ = newF_n.dz();

    /*****************************/
    /* Tangential Calculation    */
    /*****************************/

    double S_t =  8.0 * nEquivShearMod * sqrt(nEquivRadius * normalPhysicalOverlap);
    CSimple3DVector nOverlap_t(tangentialPhysicalOverlapX, tangentialPhysicalOverlapY, tangentialPhysicalOverlapZ);
    CSimple3DVector F_t = -nOverlap_t * S_t;

    // Damping
    CSimple3DVector F_td, newF_t;

    if(F_t.length() > F_hertz.length() * (staticFriction) )
    {
        newF_t = F_t * F_hertz.length() * (staticFriction) / F_t.length();
        nOverlap_t = -newF_t / S_t; //slippage has occurred so the tangential overlap is reduced a bit

        //at this point we get energy loss from the sliding!
        F_td = newF_t;
    }
    else
    {
        //at this point we get energy loss from the damping!
        F_td = - relVel_t * 2 * sqrt(5.0 / 6.0) * B * sqrt(S_t * nEquivMass);
        newF_t = F_t + F_td;
    }

    // Fill in parameters we were passed in
    calculatedTangentialForceX = newF_t.dx();
    calculatedTangentialForceY = newF_t.dy();
    calculatedTangentialForceZ = newF_t.dz();
    calculatedUnsymTangentialForceX = F_td.dx();
    calculatedUnsymTangentialForceY = F_td.dy();
    calculatedUnsymTangentialForceZ = F_td.dz();

    tangentialPhysicalOverlapX = nOverlap_t.dx();
    tangentialPhysicalOverlapY = nOverlap_t.dy();
    tangentialPhysicalOverlapZ = nOverlap_t.dz();


    /*****************************/
    /* Rolling Friction          */
    /*****************************/

    // Only relevant if actually rolling
    if(!isZero(angVel1.lengthSquared()))
    {
        CSimple3DVector torque1 = angVel1;
        torque1.normalise();
        torque1 *= -newF_n.length() * elem1PhysicalRadius * rollingFriction;
        calculatedElem1AdditionalTorqueX = torque1.dx();
        calculatedElem1AdditionalTorqueY = torque1.dy();
        calculatedElem1AdditionalTorqueZ = torque1.dz();
        calculatedElem1UnsymAdditionalTorqueX = torque1.dx();
        calculatedElem1UnsymAdditionalTorqueY = torque1.dy();
        calculatedElem1UnsymAdditionalTorqueZ = torque1.dz();
    }

    if(!isZero(angVel2.lengthSquared()))
    {
        CSimple3DVector torque2 = angVel2;
        torque2.normalise();
        torque2 *= -newF_n.length() * elem2PhysicalRadius * rollingFriction;
        calculatedElem2AdditionalTorqueX = torque2.dx();
        calculatedElem2AdditionalTorqueY = torque2.dy();
        calculatedElem2AdditionalTorqueZ = torque2.dz();
        calculatedElem2UnsymAdditionalTorqueX = torque2.dx();
        calculatedElem2UnsymAdditionalTorqueY = torque2.dy();
        calculatedElem2UnsymAdditionalTorqueZ = torque2.dz();
    }

    if (elem2IsSurf == false)
    {
        //*Adhesion /
      
        double* AdhesionDelta = elem2PropData->getDelta(Adhesion_PROPERTY.c_str());

        AdhesionDelta[0] += cohesionForce;
    }

    if (elem2IsSurf == false)
    {
        //*Frictiom/

        double* FrictionDelta = elem2PropData->getDelta(Friction_PROPERTY.c_str());

        FrictionDelta[0] += hertzForce * 0.58;
    }
   

    return eSuccess;
}
