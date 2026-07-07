#if !defined(chertzmindlin_h)
#define chertzmindlin_h

#include <cstring>
#include <fstream>

#include "IPluginContactModelV2_3_0.h"
#include "CCohesionList.h"
#include <string>
#include <map>
#include "IGeometryManagerApi_1_0.h"
using namespace std;

/**
 * This class provides an implementation of IPluginContactModelV2_1_0
 * That adds performs hertz mindlin contact force calculation
 *
 * NOTE: This version of the hertz-midnlin code is provided only
 * as an example of how to create a contact model plugin and is not
 * updated on a regular basis.
 */
class CHertzMindlin : public NApiCm::IPluginContactModelV2_3_0
{
    public:

        /**
         * Name of the preferences file to load bond information
         * from
         */
        static const std::string PREFS_FILE;
        static const string Adhesion_PROPERTY;
        static const string Friction_PROPERTY;


         /**
         * Constructor, does nothing
         */
        CHertzMindlin();

        /**
         * Destructor, does nothing
         */
        virtual ~CHertzMindlin();

        /**
         * Sets prefFileName to PREFS_FILE
         *
         * Implementation of method from IPluginContactModelV2_1_0
         */
        virtual void getPreferenceFileName(char prefFileName[NApi::FILE_PATH_MAX_LENGTH]);

    
        /**
         * Returns true to indicate plugin is thread safe
         *
         * Implementation of method from IPluginContactModelV2_1_0
         */
        virtual bool isThreadSafe();


        /**
         * Reads config file
         *
         * Implementation of method from IPluginParticleBodyForceV2_1_0
         */
        virtual bool setup(NApiCore::IApiManager_1_0& apiManager,
                           const char                 prefFile[],
                           char                       customMsg[NApi::ERROR_MSG_MAX_LENGTH]);

        virtual bool usesCustomProperties();

        virtual bool starting(NApiCore::IApiManager_1_0& apiManager, int numThreads);

        virtual void stopping(NApiCore::IApiManager_1_0& apiManager);

        virtual unsigned int getNumberOfRequiredProperties(
            const NApi::EPluginPropertyCategory category);

        virtual bool getDetailsForProperty(
            unsigned int                    propertyIndex,
            NApi::EPluginPropertyCategory   category,
            char                            name[NApi::CUSTOM_PROP_MAX_NAME_LENGTH],
            NApi::EPluginPropertyDataTypes& dataType,
            unsigned int& numberOfElements,
            NApi::EPluginPropertyUnitTypes& unitType,
            char                            initValBuff[NApi::BUFF_SIZE]);

        virtual void configForTimeStep(NApiCore::ICustomPropertyDataApi_1_0* simPropData, NApiCore::ICustomPropertyDataApi_1_0* particlePropData);

        /**
         *
         *
         * Implementation of method from IPluginContactModelV2_1_0
         */
        virtual NApi::ECalculateResult calculateForce(
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
            double& calculatedChargeMovedToElem1);


private:
    CCohesionList m_cohesiveItems;

private:
    NApiCore::IGeometryManagerApi_1_0* m_geomMgr;

};

#endif
