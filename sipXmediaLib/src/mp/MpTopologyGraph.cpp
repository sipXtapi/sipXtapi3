//  
// Copyright (C) 2006-2008 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2006-2008 SIPez LLC. 
// Licensed to SIPfoundry under a Contributor Agreement. 
//
// $$
///////////////////////////////////////////////////////////////////////////////

// Author: Dan Petrie <dpetrie AT SIPez DOT com>

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <utl/UtlInt.h>
#include <utl/UtlHashMap.h>
#include <utl/UtlHashBag.h>
#include <utl/UtlContainablePair.h>
#include <utl/UtlHashBagIterator.h>
#include <utl/UtlVoidPtr.h>
#include <mp/MpTopologyGraph.h>
#include <mp/MpMediaTask.h>
#include <mp/MpResourceFactory.h>
#include <mp/MpResourceTopology.h>


//  REMOVE THESE WHEN MpResourceFactory and MpResourceTopology are implemented
#include <mp/MprToneGen.h>

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
#define MAX_CONSTRUCTED_RESOURCES 10
//#define TEST_PRINT

// STATIC VARIABLE INITIALIZATIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
MpTopologyGraph::MpTopologyGraph(int samplesPerFrame, 
                                 int samplesPerSec,
                                 MpResourceTopology& initialResourceTopology,
                                 MpResourceFactory& resourceFactory,
                                 OsMsgDispatcher *pNotifDispatcher)
: MpFlowGraphBase(samplesPerFrame, samplesPerSec, pNotifDispatcher)
, mpResourceFactory(&resourceFactory)
{
    OsStatus result;

    // construct the new resources defined in the topology and add to the flowgraph
    UtlHashBag newResourcesAdded;
    addTopologyResources(initialResourceTopology, 
                         resourceFactory,
                         newResourcesAdded);

    // Add virtual ports defined in the topology
    addVirtualInputs(initialResourceTopology, newResourcesAdded);
    addVirtualOutputs(initialResourceTopology, newResourcesAdded);

    // Add the links for the resources in the topology
    linkTopologyResources(initialResourceTopology, newResourcesAdded);

    // ask the media processing task to manage the new flowgraph
    MpMediaTask* mediaTask = MpMediaTask::getMediaTask(0);
    result = mediaTask->manageFlowGraph(*this);
    assert(result == OS_SUCCESS);

    // start the flowgraph
    result = mediaTask->startFlowGraph(*this);
    if(result != OS_SUCCESS)
    {
        printf("MpTopologyGraph mediaTask->startFlowGraph failed: %d\n", result);
    }
    assert(result == OS_SUCCESS);
}

// Destructor
MpTopologyGraph::~MpTopologyGraph()
{
    // unmanage the flowgraph
    MpMediaTask* mediaTask = MpMediaTask::getMediaTask(0);
    OsStatus result = mediaTask->unmanageFlowGraph(*this);
    assert(result == OS_SUCCESS);

    // wait until the flowgraph is unmanaged.
    while (mediaTask->isManagedFlowGraph(this))
    {
        OsTask::delay(20);   // wait 20 msecs before checking again
    }

    mVirtualInputs.destroyAll();
    mVirtualOutputs.destroyAll();
}

/* ============================ MANIPULATORS ============================== */

OsStatus MpTopologyGraph::addResources(MpResourceTopology& incrementalTopology,
                                       MpResourceFactory* resourceFactory,
                                       int resourceInstanceId)
{
    if(resourceFactory == NULL)
    {
        resourceFactory = mpResourceFactory;
    }

    assert(resourceFactory);

    // Add new resources
    UtlHashBag newResourcesAdded;
    addTopologyResources(incrementalTopology, 
                         *resourceFactory, 
                         newResourcesAdded,
                         TRUE, 
                         resourceInstanceId);

    // Add new virtual ports
    addVirtualInputs(incrementalTopology,
                     newResourcesAdded,
                     TRUE,
                     resourceInstanceId);
    addVirtualOutputs(incrementalTopology,
                      newResourcesAdded,
                      TRUE,
                      resourceInstanceId);

    // Add the links for the resources in the topology
    linkTopologyResources(incrementalTopology, 
                          newResourcesAdded,
                          TRUE, 
                          resourceInstanceId);

    return OS_SUCCESS;
}

OsStatus MpTopologyGraph::destroyResources(MpResourceTopology& resourceTopology,
                                          int resourceInstanceId)
{
    // Add the resources
    int resourceIndex = 0;
    MpResource* resourcePtr = NULL;
    UtlString resourceName;
    OsStatus result;
    while(resourceTopology.getResource(resourceIndex, resourceName)
          == OS_SUCCESS)
    {
         resourceTopology.replaceNumInName(resourceName, resourceInstanceId);
         result = destroyResource(resourceName);
         assert(result == OS_SUCCESS);
         resourceIndex++;
    }
    return OS_SUCCESS;
}

OsStatus MpTopologyGraph::postMessage(const MpFlowGraphMsg& message,
                                      const OsTime& waitTime)
{
    // TODO:  handle or dispatch specific resource messages
    return(MpFlowGraphBase::postMessage(message, waitTime));
}

UtlBoolean MpTopologyGraph::handleMessage(OsMsg& message)
{
    // TODO:  handle or dispatch specific resource messages
    return(MpFlowGraphBase::handleMessage(message));
}

OsStatus MpTopologyGraph::processNextFrame()
{
    //printf("making the doughnuts\n");
    return(MpFlowGraphBase::processNextFrame());
}

// Notification that this flow graph has just been granted the focus.
// Enable our microphone and speaker resources
OsStatus MpTopologyGraph::gainFocus(void)
{
   UtlBoolean    boolRes;
   OsStatus      result;

#ifndef DISABLE_LOCAL_AUDIO // ]
   // enable the FromInputDevice and ToOutputDevice -- we have focus
   {
      MpResource* pResource = NULL;
      result = lookupResource("FromMic1",
                              pResource);
      assert(result == OS_SUCCESS);
      assert(pResource);
      boolRes = pResource->enable();
      assert(boolRes);
   }
   {
      MpResource* pResource = NULL;
      result = lookupResource("ToSpeaker1",
                              pResource);
      assert(result == OS_SUCCESS);
      assert(pResource);
      boolRes = pResource->enable();
      assert(boolRes);
   }
#endif // DISABLE_LOCAL_AUDIO

   return OS_SUCCESS;
}

// Notification that this flow graph has just lost the focus.
// Disable our microphone and speaker resources
OsStatus MpTopologyGraph::loseFocus(void)
{
   UtlBoolean    boolRes;
   OsStatus      result;

#ifndef DISABLE_LOCAL_AUDIO // [
   // disable the FromInputDevice and ToOutputDevice -- we no longer have the focus.
   {
      MpResource* pResource = NULL;
      result = lookupResource("FromMic1",
                              pResource);
      assert(result == OS_SUCCESS);
      assert(pResource);
      boolRes = pResource->disable();
      assert(boolRes);
   }
   {
      MpResource* pResource = NULL;
      result = lookupResource("ToSpeaker1",
                              pResource);
      assert(result == OS_SUCCESS);
      assert(pResource);
      boolRes = pResource->disable();
      assert(boolRes);
   }
#endif //DISABLE_LOCAL_AUDIO ]

   return OS_SUCCESS;
}

/* ============================ ACCESSORS ================================= */

MpFlowGraphBase::FlowGraphType MpTopologyGraph::getType()
{
   return MpFlowGraphBase::TOPOLOGY_FLOWGRAPH;
}

OsStatus MpTopologyGraph::lookupVirtualInput(const UtlString& virtualName,
                                             int virtualPortIdx,
                                             MpResource*& rpResource,
                                             int &portIdx)
{
   UtlInt virtualPortNum(virtualPortIdx);
   UtlContainablePair virtPair((UtlContainable*)&virtualName, &virtualPortNum);
   UtlContainablePair *realPair;
   
   // Attempt to search for real port as is
   realPair = (UtlContainablePair*)mVirtualInputs.findValue(&virtPair);
   if (realPair == NULL && virtualPortIdx >= 0)
   {
      // If not found - try to search for virtual port -1 (wildcard value).
      virtualPortNum.setValue(-1);
      realPair = (UtlContainablePair*)mVirtualInputs.findValue(&virtPair);
   }

   // Set to NULL to prevent deletion of stack variables
   virtPair.setFirst(NULL);
   virtPair.setSecond(NULL);

   if (!realPair)
   {
      // Nothing found
      return OS_NOT_FOUND;
   }

   rpResource = (MpResource*)((UtlVoidPtr*)realPair->getFirst())->getValue();
   portIdx = ((UtlInt*)realPair->getSecond())->getValue();

   return OS_SUCCESS;
}

OsStatus MpTopologyGraph::lookupVirtualOutput(const UtlString& virtualName,
                                              int virtualPortIdx,
                                              MpResource*& rpResource,
                                              int &portIdx)
{
   UtlInt virtualPortNum(virtualPortIdx);
   UtlContainablePair virtPair((UtlContainable*)&virtualName, &virtualPortNum);
   UtlContainablePair *realPair;
   
   // Attempt to search for real port as is
   realPair = (UtlContainablePair*)mVirtualOutputs.findValue(&virtPair);
   if (realPair == NULL && virtualPortIdx >= 0)
   {
      // If not found - try to search for virtual port -1 (wildcard value).
      virtualPortNum.setValue(-1);
      realPair = (UtlContainablePair*)mVirtualOutputs.findValue(&virtPair);
   }

   // Set to NULL to prevent deletion of stack variables
   virtPair.setFirst(NULL);
   virtPair.setSecond(NULL);

   if (!realPair)
   {
      // Nothing found
      return OS_NOT_FOUND;
   }

   rpResource = (MpResource*)((UtlVoidPtr*)realPair->getFirst())->getValue();
   portIdx = ((UtlInt*)realPair->getSecond())->getValue();

   return OS_SUCCESS;
}

OsStatus MpTopologyGraph::lookupInput(const UtlString& resourceName,
                                      int portIdx,
                                      MpResource*& pFoundResource,
                                      int &foundPortIdx)
{
   OsStatus result;

   // Set default value of port.
   foundPortIdx = portIdx;

   // Look for real resource first.
   result = lookupResource(resourceName, pFoundResource);
   if(result != OS_SUCCESS)
   {
      // Look for virtual port
      int virtPortIdx = portIdx>=0?portIdx:-1;
      int realPortIdx;
      result = lookupVirtualInput(resourceName, virtPortIdx,
                                  pFoundResource, realPortIdx);
      if (result == OS_SUCCESS)
      {
         if (realPortIdx >= 0)
         {
            // Update found port index only if returned port index points to
            // real input.
            foundPortIdx = realPortIdx;
         }
         assert(!(portIdx < -1 && realPortIdx >= 0));
      }
   }

   return result;
}

OsStatus MpTopologyGraph::lookupOutput(const UtlString& resourceName,
                                       int portIdx,
                                       MpResource*& pFoundResource,
                                       int &foundPortIdx)
{
   OsStatus result;

   // Set default value of port.
   foundPortIdx = portIdx;

   // Look for real resource first.
   result = lookupResource(resourceName, pFoundResource);
   if(result != OS_SUCCESS)
   {
      // Look for virtual port
      int virtPortIdx = portIdx>=0?portIdx:-1;
      int realPortIdx;
      result = lookupVirtualOutput(resourceName, virtPortIdx,
                                   pFoundResource, realPortIdx);
      if (result == OS_SUCCESS)
      {
         if (realPortIdx >= 0)
         {
            // Update found port index only if returned port index points to
            // real output.
            foundPortIdx = realPortIdx;
         }
         assert(!(portIdx < -1 && realPortIdx >= 0));
      }
   }

   return result;
}

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

int MpTopologyGraph::addTopologyResources(MpResourceTopology& resourceTopology,
                                          MpResourceFactory& resourceFactory,
                                          UtlHashBag& newResources,
                                          UtlBoolean replaceNumInName,
                                          int resourceNum)
{
    // Add the resources
    int resourceIndex = 0;
    MpResource* resourcePtr = NULL;
    MpResource* resourceArray[MAX_CONSTRUCTED_RESOURCES];
    UtlString resourceType;
    UtlString resourceName;
    MpConnectionID resourceConnId;
    int resourceStreamId;
    OsStatus result;
    while(resourceTopology.getResource(resourceIndex, resourceType, resourceName,
                                       resourceConnId, resourceStreamId) == OS_SUCCESS)
    {
        if(replaceNumInName)
        {
           MpResourceTopology::replaceNumInName(resourceName, resourceNum);
        }

        int numConstructorResources;
        OsStatus status;
        status = resourceFactory.newResource(resourceType, resourceName, MAX_CONSTRUCTED_RESOURCES,
                                             numConstructorResources, resourceArray);        
        if(status == OS_SUCCESS)
        {
           assert(numConstructorResources > 0);
           int arrayIndex;
           // We now can potentially get more than one resource back from the
           // constructor
           for(arrayIndex = 0; arrayIndex < numConstructorResources; arrayIndex++)
           {
              resourcePtr = resourceArray[arrayIndex];
              assert(resourcePtr);
              if(resourcePtr)
              {
#ifdef TEST_PRINT
                  printf("constructed and adding resource name: %s type: %s\n",
                         resourcePtr->getName().data(),
                         resourceType.data());
#endif
                  if(replaceNumInName && resourceConnId == MP_INVALID_CONNECTION_ID)
                  {
                     resourcePtr->setConnectionId(resourceNum);
                  }
                  else
                  {
                     resourcePtr->setConnectionId(resourceConnId);
                  }
                  resourcePtr->setStreamId(resourceStreamId);
                  newResources.insert(resourcePtr);
                  result = addResource(*resourcePtr, FALSE);
                  assert(result == OS_SUCCESS);
               }
           }
        }
        else
        {
           OsSysLog::add(FAC_MP, PRI_ERR, 
              "Failed to create resource type: %s name: %s status: %d",
              resourceType.data(), resourceName.data(), status);
        }
        resourceIndex++;
    }
    return(resourceIndex);
}

int MpTopologyGraph::addVirtualInputs(MpResourceTopology& resourceTopology,
                                      UtlHashBag& newResources,
                                      UtlBoolean replaceNumInName,
                                      int resourceNum)
{
   int portsAdded = 0;
   MpResourceTopology::VirtualPortIterator portIter;
   UtlString realResourceName;
   int realPortIdx;
   UtlString virtualResourceName;
   int virtualPortIdx;

   resourceTopology.initVirtualInputIterator(portIter);
   while (resourceTopology.getNextVirtualInput(portIter,
                                               realResourceName, realPortIdx,
                                               virtualResourceName, virtualPortIdx)
          == OS_SUCCESS)
   {
      if(replaceNumInName)
      {
         MpResourceTopology::replaceNumInName(realResourceName, resourceNum);
         MpResourceTopology::replaceNumInName(virtualResourceName, resourceNum);
      }

      // Lookup real resource.
      MpResource *pResource = (MpResource*)newResources.find(&realResourceName);
      assert(pResource);
      if (!pResource)
      {
         continue;
      }

      // Check port index correctness. Note, that this check gracefully
      // handles case with realPortIdx equal -1.
      if (realPortIdx >= pResource->maxInputs())
      {
         assert(!"Trying to map virtual port to non existing real port!");
         continue;
      }

      // Add entry to virtual ports map.
      // We need to create UtlVoidPtr wrapper for pResource, or it will be
      // destroyed on pair deletion.
      UtlContainablePair *pRealPort = new UtlContainablePair(new UtlVoidPtr(pResource),
                                                             new UtlInt(realPortIdx));
      UtlContainablePair *pVirtPort = new UtlContainablePair(new UtlString(virtualResourceName),
                                                             new UtlInt(virtualPortIdx));
      mVirtualInputs.insertKeyAndValue(pVirtPort, pRealPort);
      portsAdded++;
   }
   resourceTopology.freeVirtualInputIterator(portIter);

   return portsAdded;
}

int MpTopologyGraph::addVirtualOutputs(MpResourceTopology& resourceTopology,
                                       UtlHashBag& newResources,
                                       UtlBoolean replaceNumInName,
                                       int resourceNum)
{
   int portsAdded = 0;
   MpResourceTopology::VirtualPortIterator portIter;
   UtlString realResourceName;
   int realPortIdx;
   UtlString virtualResourceName;
   int virtualPortIdx;

   resourceTopology.initVirtualOutputIterator(portIter);
   while (resourceTopology.getNextVirtualOutput(portIter,
                                                realResourceName, realPortIdx,
                                                virtualResourceName, virtualPortIdx)
          == OS_SUCCESS)
   {
      if(replaceNumInName)
      {
         MpResourceTopology::replaceNumInName(realResourceName, resourceNum);
         MpResourceTopology::replaceNumInName(virtualResourceName, resourceNum);
      }

      // Lookup real resource.
      MpResource *pResource = (MpResource*)newResources.find(&realResourceName);
      assert(pResource);
      if (!pResource)
      {
         continue;
      }

      // Check port index correctness. Note, that this check gracefully
      // handles case with realPortIdx equal -1.
      if (realPortIdx >= pResource->maxOutputs())
      {
         assert(!"Trying to map virtual port to non existing real port!");
         continue;
      }

      // Add entry to virtual ports map.
      // We need to create UtlVoidPtr wrapper for pResource, or it will be
      // destroyed on pair deletion.
      UtlContainablePair *pRealPort = new UtlContainablePair(new UtlVoidPtr(pResource),
                                                             new UtlInt(realPortIdx));
      UtlContainablePair *pVirtPort = new UtlContainablePair(new UtlString(virtualResourceName),
                                                             new UtlInt(virtualPortIdx));
      mVirtualOutputs.insertKeyAndValue(pVirtPort, pRealPort);
      portsAdded++;
   }
   resourceTopology.freeVirtualOutputIterator(portIter);

   return portsAdded;
}

int MpTopologyGraph::linkTopologyResources(MpResourceTopology& resourceTopology,
                                           UtlHashBag& newResources,
                                           UtlBoolean replaceNumInName,
                                           int resourceNum)
{
    // Link the resources
    int connectionIndex = 0;
    UtlString outputResourceName;
    UtlString inputResourceName;
    int outputResourcePortIndex;
    int inputResourcePortIndex;
    MpResource* outputResource = NULL;
    MpResource* inputResource = NULL;
    OsStatus result;
    UtlHashMap newConnectionIds;

#ifdef TEST_PRINT
    osPrintf("%d new resources in the list\n", newResources.entries());
    UtlHashBagIterator iterator(newResources);
    MpResource* containerResource = NULL;
    while(containerResource = (MpResource*) iterator())
    {
        osPrintf("found list resource: \"%s\" value: \"%s\"\n",
                 containerResource->getName().data(), containerResource->data());
    }
#endif

    while(resourceTopology.getConnection(connectionIndex,
                                         outputResourceName,
                                         outputResourcePortIndex,
                                         inputResourceName,
                                         inputResourcePortIndex) == OS_SUCCESS)
    {
        if(replaceNumInName)
        {
            resourceTopology.replaceNumInName(outputResourceName, resourceNum);
            resourceTopology.replaceNumInName(inputResourceName, resourceNum);
        }

        // Look in the container of new resources first as this is more 
        // efficient and new resources are not added immediately to a running
        // flowgraph
        outputResource = (MpResource*) newResources.find(&outputResourceName);
        if(outputResource == NULL)
        {
            result = lookupResource(outputResourceName, outputResource);
            if(result != OS_SUCCESS)
            {
                int virtPortIdx = outputResourcePortIndex>=0?outputResourcePortIndex:-1;
                int realPortIdx;
                result = lookupVirtualOutput(outputResourceName, virtPortIdx,
                                             outputResource, realPortIdx);
                if (result == OS_SUCCESS && outputResourcePortIndex>=0)
                {
                   outputResourcePortIndex = realPortIdx;
                }
            }
            assert(result == OS_SUCCESS);
        }
        inputResource = (MpResource*) newResources.find(&inputResourceName);
        if(inputResource == NULL)
        {
            result = lookupResource(inputResourceName, inputResource);
            if(result != OS_SUCCESS)
            {
                int virtPortIdx = inputResourcePortIndex>=0?inputResourcePortIndex:-1;
                int realPortIdx;
                result = lookupVirtualInput(inputResourceName, virtPortIdx,
                                            inputResource, realPortIdx);
                if (result == OS_SUCCESS && inputResourcePortIndex>=0)
                {
                   inputResourcePortIndex = realPortIdx;
                }
            }
            assert(result == OS_SUCCESS);
        }
        assert(outputResource);
        assert(inputResource);

        if(outputResource && inputResource)
        {
            if(outputResourcePortIndex == MpResourceTopology::MP_TOPOLOGY_NEXT_AVAILABLE_PORT)
            {
                outputResourcePortIndex = outputResource->reserveFirstUnconnectedOutput();
                assert(outputResourcePortIndex >= 0);
            }
            else if(outputResourcePortIndex < MpResourceTopology::MP_TOPOLOGY_NEXT_AVAILABLE_PORT)
            {
                // First see if a real port is already in the dictionary
                UtlInt searchKey(outputResourcePortIndex);
                UtlInt* foundValue = NULL;
                if((foundValue = (UtlInt*) newConnectionIds.findValue(&searchKey)))
                {
                    // Use the mapped index
                    outputResourcePortIndex = foundValue->getValue();
                }
                else
                {
                    // Find an available port and add it to the map
                    int realPortNum = outputResource->reserveFirstUnconnectedOutput();
                    assert(realPortNum >= 0);
                    UtlInt* portKey = new UtlInt(outputResourcePortIndex);
                    UtlInt* portValue = new UtlInt(realPortNum);
                    newConnectionIds.insertKeyAndValue(portKey, portValue);
                    outputResourcePortIndex = realPortNum;
                }
            }

            if(inputResourcePortIndex == MpResourceTopology::MP_TOPOLOGY_NEXT_AVAILABLE_PORT)
            {
                inputResourcePortIndex = inputResource->reserveFirstUnconnectedInput();
                assert(inputResourcePortIndex >= 0);
            }
            else if(inputResourcePortIndex < MpResourceTopology::MP_TOPOLOGY_NEXT_AVAILABLE_PORT)
            {
                // First see if a real port is already in the dictionary
                UtlInt searchKey(inputResourcePortIndex);
                UtlInt* foundValue = NULL;
                if((foundValue = (UtlInt*) newConnectionIds.findValue(&searchKey)))
                {
                    // Use the mapped index
                    inputResourcePortIndex = foundValue->getValue();
                }
                else
                {
                    // Find an available port and add it to the map
                    int realPortNum = inputResource->reserveFirstUnconnectedInput();
                    assert(realPortNum >= 0);
                    UtlInt* portKey = new UtlInt(inputResourcePortIndex);
                    UtlInt* portValue = new UtlInt(realPortNum);
                    newConnectionIds.insertKeyAndValue(portKey, portValue);
                    inputResourcePortIndex = realPortNum;
                }
            }


            result = addLink(*outputResource, outputResourcePortIndex, *inputResource, inputResourcePortIndex);
            assert(result == OS_SUCCESS);
        }
        connectionIndex++;
    }

    newConnectionIds.destroyAll();
    return(connectionIndex);
}
    
/* ============================ FUNCTIONS ================================= */

