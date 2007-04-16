//  
// Copyright (C) 2006-2007 SIPez LLC. 
// Licensed to SIPfoundry under a Contributor Agreement. 
//
// Copyright (C) 2004-2007 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////


#ifndef _MpResource_h_
#define _MpResource_h_

// SYSTEM INCLUDES

// APPLICATION INCLUDES
#include "os/OsDefs.h"
#include "os/OsRWMutex.h"
#include "os/OsStatus.h"
#include "os/OsMsgQ.h"
#include "utl/UtlContainable.h"
#include "utl/UtlString.h"
#include "mp/MpBuf.h"

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS

// $$$ (rschaaf): keep for now
// typedef int* MpBufPtr;

// FORWARD DECLARATIONS
class MpFlowGraphBase;
class MpFlowGraphMsg;
class MpResourceMsg;

/// Abstract base class for all media processing objects.
/**
*  Each resource has zero or more input ports and zero or more output ports.
*  Each frame processing interval, the <i>processFrame()</i> method is
*  invoked to process one interval's worth of media.
* 
*  Substantive changes to a resource can only be made:
*  1) when the resource is not part of flow graph, or
*  2) at the start of a frame processing interval
*/
class MpResource : public UtlContainable
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

   friend class MpFlowGraphBase;

   /// @brief Graph traversal states that are used when running a topological 
   /// sort to order resources within a flow graph.
   typedef enum
   {
      NOT_VISITED,
      IN_PROGRESS,
      FINISHED
   } VisitState;

static const UtlContainableType TYPE;

/* ============================ CREATORS ================================== */
///@name Creators
//@{

   /// Constructor
   MpResource(const UtlString& rName, int minInputs, int maxInputs,
              int minOutputs, int maxOutputs);

   /// Destructor
   virtual ~MpResource();

//@}

/* ============================ MANIPULATORS ============================== */
///@name Manipulators
//@{

     /// Disable this resource.
   virtual UtlBoolean disable(void);
     /**<
     *  The "enabled" flag is passed to the <i>doProcessFrame()</i> method
     *  and will likely affect the media processing that is performed by this
     *  resource.  Typically, if a resource is not enabled,
     *  <i>doProcessFrame()</i> will perform only minimal processing (for
     *  example, passing the input straight through to the output in the case
     *  of a one input / one output resource).
     *  @returns TRUE if successful, FALSE otherwise.
     */

     /// Post a message to disable the resource named.
   static OsStatus disable(const UtlString& namedResource,
                           OsMsgQ& fgQ);
     /**<
     *  Post a disable message for the named resource to the 
     *  flowgraph queue supplied.
     *  NOTE: This is an asynchronous operation.
     *        The status returned does not indicate that the disable
     *        happened - only that it was properly queued.
     *  @param namedResource - the name of the resource to disable.
     *  @param fgQ - The flowgraph message queue to post the message to.
     *  @returns OS_SUCCESS if the message was successfully queued
     *           to the message queue.
     *  @returns OS_FAILED if the message could not be added to the
     *           message queue.
     */

     /// Enable this resource.
   virtual UtlBoolean enable(void);
     /**<
     *  The "enabled" flag is passed to the <i>doProcessFrame()</i> method
     *  and will likely affect the media processing that is performed by this
     *  resource.  Typically, if a resource is not enabled,
     *  <i>doProcessFrame()</i> will perform only minimal processing (for
     *  example, passing the input straight through to the output in the case
     *  of a one input / one output resource).
     *  @returns TRUE if successful, FALSE otherwise.
     */

     /// Post a message to enable the resource named.
   static OsStatus enable(const UtlString& namedResource,
                          OsMsgQ& fgQ);
     /**<
     *  Post an enable message for the named resource to the 
     *  flowgraph queue supplied.
     *  NOTE: This is an asynchronous operation.
     *        The status returned does not indicate that the enable
     *        happened - only that it was properly queued.
     *  @param namedResource - the name of the resource to enable.
     *  @param fgQ - The flowgraph message queue to post the message to.
     *  @returns OS_SUCCESS if the message was successfully queued
     *           to the message queue.
     *  @returns OS_FAILED if the message could not be added to the
     *           message queue.
     */

     /// @brief Handles a queue-full of incoming messages for this media processing object.
   UtlBoolean handleMessages(OsMsgQ& msgQ);
     /**<
     *  handles a queue-full of incoming messages for this resource.
     *  This is intended to handle messages directly on a resource, 
     *  circumventing a flowgraph's queue, and allowing things like the
     *  application to get resources to process some operations directly.
     *  (usually before a flowgraph is set up, but perhaps else-when too.
     *  NOTE: This makes an assumption that the destination of these 
     *        messages is this resource.
     *
     *  @param msgQ - a message queue full of messages intended for this resource.
     *  @returns TRUE if all the messages were handled, otherwise FALSE. 
     */

     /// This method is called in every MpFlowGraph processing cycle.
   virtual UtlBoolean processFrame(void) = 0;
     /**<
     *  This method prepares the input buffers before calling
     *  <i>doProcessFrame()</i> and distributes the output buffers to the
     *  appropriate downstream resources after <i>doProcessFrame()</i>
     *  returns.
     *  @returns TRUE if successful, FALSE otherwise.
     */

     /// Sets the visit state for this resource.
   void setVisitState(int newState);
     /**< Used in performing a topological sort on the resources contained within a flow graph. */

//@}

/* ============================ ACCESSORS ================================= */
///@name Accessors
//@{

     /// Displays information on the console about the specified resource.
   static void resourceInfo(MpResource* pResource, int index);

///@name Accessors
//@{

     /// Returns parent flow praph.
   MpFlowGraphBase* getFlowGraph(void) const;
     /**<
     *  @returns the flow graph that contains this resource or NULL if the 
     *  resource is not presently part of any flow graph.
     */

     /// Returns information about the upstream end of a connection.
   void getInputInfo(int inPortIdx, MpResource*& rpUpstreamResource,
                     int& rUpstreamPortIdx);
     /**<
     *  Returns information about the upstream end of a connection to the 
     *  <i>inPortIdx</i> input on this resource.  If <i>inPortIdx</i> is 
     *  invalid or there is no connection, then <i>rpUpstreamResource</i> 
     *  will be set to NULL.
     */

     /// Returns the name associated with this resource.
   UtlString getName(void) const;

     /// Returns information about the downstream end of a connection.
   void getOutputInfo(int outPortIdx, MpResource*& rpDownstreamResource,
                      int& rDownstreamPortIdx);
     /**<
     *  Returns information about the downstream end of a connection to the 
     *  <i>outPortIdx</i> output on this resource.  If <i>outPortIdx</i> is 
     *  invalid or there is no connection, then <i>rpDownstreamResource</i> 
     *  will be set to NULL.
     */

     /// Returns the current visit state for this resource
   int getVisitState(void);
     /**< Used in performing a topological sort on the resources contained within a flow graph. */

     /// Returns the maximum number of inputs supported by this resource.
   int maxInputs(void) const;

     /// Returns the maximum number of outputs supported by this resource.
   int maxOutputs(void) const;

     /// Returns the minimum number of inputs required by this resource.
   int minInputs(void) const;

     /// Returns the minimum number of outputs required by this resource.
   int minOutputs(void) const;

     /// Returns the number of resource inputs that are currently connected.
   int numInputs(void) const;

     /// Returns the number of resource outputs that are currently connected.
   int numOutputs(void) const;

     /// Find the first unconnected input port and reserve it
     /** 
       * Reserving a port does not prevent someone from connecting to
       * that port.
       * @returns -1 if no free ports
       */
   int reserveFirstUnconnectedInput();

     /// Find the first unconnected output port and reserve it
     /** 
       * Reserving a port does not prevent someone from connecting to
       * that port.
       * @returns -1 if no free ports
       */
   int reserveFirstUnconnectedOutput();

     /// Calculate a unique hash code for this object.
   virtual unsigned hash() const ;
     /**<
     *  If the equals operator returns true for another object, then both of those
     *  objects must return the same hashcode.
     */

     /// Get the ContainableType for a UtlContainable derived class.
   virtual UtlContainableType getContainableType() const ;

//@}

/* ============================ INQUIRY =================================== */
///@name Inquiry
//@{

     /// Returns TRUE is this resource is currently enabled, FALSE otherwise.
   UtlBoolean isEnabled(void) const;

     /// Returns TRUE if portIdx is valid and the indicated input is connected, FALSE otherwise.
   UtlBoolean isInputConnected(int portIdx);

     /// Returns TRUE if portIdx is valid and the indicated input is not connected, FALSE otherwise.
   UtlBoolean isInputUnconnected(int portIdx);

     /// Returns TRUE if portIdx is valid and the indicated output is connected, FALSE otherwise.
   UtlBoolean isOutputConnected(int portIdx);

     /// Returns TRUE if portIdx is valid and the indicated output is not connected, FALSE otherwise.
   UtlBoolean isOutputUnconnected(int portIdx);

     /// Compare the this object to another like-objects.
   virtual int compareTo(UtlContainable const *) const ;   
     /**<
     *  Results for designating a non-like object are undefined.
     * 
     *  @returns 0 if equal, < 0 if less then and >0 if greater.
     */

//@}

/* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   // Conn is a local class definition

   /// The Conn object maintains information about the "far end" of a connection.
   struct Conn
   {
      MpResource* pResource;    ///< Other end of the connection.
      int         portIndex;    ///< Port number on the other end of the connection.
      UtlBoolean  reserved;     ///< this port is reserved to be used
   };

   UtlString     mName;            ///< name associated with this resource
   MpFlowGraphBase* mpFlowGraph;   ///< flow graph this resource belongs to
   UtlBoolean    mIsEnabled;       ///< TRUE if resource is enabled, FALSE otherwise

   OsRWMutex    mRWMutex;          ///< reader/writer lock for synchronization

   MpBufPtr*    mpInBufs;          ///< input buffers for this resource
   MpBufPtr*    mpOutBufs;         ///< output buffers for this resource
   Conn*        mpInConns;         ///< input connections for this resource
   Conn*        mpOutConns;        ///< output connections for this resource
   int          mMaxInputs;        ///< maximum number of inputs
   int          mMaxOutputs;       ///< maximum number of outputs
   int          mMinInputs;        ///< number of required inputs
   int          mMinOutputs;       ///< number of required outputs
   int          mNumActualInputs;  ///< actual number of connected inputs
   int          mNumActualOutputs; ///< actual number of connected outputs
   int          mVisitState;       ///< (used by flow graph topological sort alg.)
   OsBSem       mLock;             ///< used mainly to make safe changes to ports

   static const OsTime sOperationQueueTimeout;
     ///< The timeout for message operations for all resources when posting to the flowgraph queue.

     /// @brief Handles an incoming flowgraph message for this media processing object.
   virtual UtlBoolean handleMessage(MpFlowGraphMsg& fgMsg);
     /**< @returns TRUE if the message was handled, otherwise FALSE. */

   /// @brief Handles an incoming resource message for this media processing object.
   virtual UtlBoolean handleMessage(MpResourceMsg& rMsg);
   /**< @returns TRUE if the message was handled, otherwise FALSE. */

   /// @brief perform the enable operation on the resource
   virtual UtlBoolean handleEnable();

   /// @brief perform the disable operation on the resource
   virtual UtlBoolean handleDisable();

   /// @brief If there already is a buffer stored for this input port, delete it. 
     /// Then store <i>pBuf</i> for the indicated input port.
   void setInputBuffer(int inPortIdx, const MpBufPtr &pBuf);

     /// Post a message from this resource.
   OsStatus postMessage(MpFlowGraphMsg& rMsg);
     /**<
     *  If this resource is not part of a flow graph, then <i>rMsg</i> is
     *  immediately passed to the <i>handleMessage()</i> method for <i>this</i>
     *  resource.  If this resource is part of a flow graph, then
     *  <i>rMsg</i> will be sent to the message queue for the flow graph
     *  that this resource belongs to.  The <i>handleMessage()</i> method
     *  for <i>destination</i> resource will be invoked at the start of the next
     *  frame processing interval.
     *
     * @warning Feel the difference in method behaviour if resource in the
     *          flow graph and if it is not.
     */

     /// @brief Makes <i>pBuf</i> available to resource connected to the
     /// <i>outPortIdx</i> output port of this resource.
   UtlBoolean pushBufferDownsream(int outPortIdx, const MpBufPtr &pBuf);
     /**<
     *  @returns TRUE if there is a resource connected to the specified output
     *  port, FALSE otherwise.
     */

/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

     /// @brief Connects the <i>toPortIdx</i> input port on this resource to the 
     /// <i>fromPortIdx</i> output port of the <i>rFrom</i> resource.
   UtlBoolean connectInput(MpResource& rFrom, int fromPortIdx, int toPortIdx);
     /**< @returns TRUE if successful, FALSE otherwise. */

     /// @brief Connects the <i>fromPortIdx</i> output port on this resource to the 
     /// <i>toPortIdx</i> input port of the <i>rTo</i> resource.
   UtlBoolean connectOutput(MpResource& rTo, int toPortIdx, int fromPortIdx);
     /**< @returns TRUE if successful, FALSE otherwise. */

     /// @brief Removes the connection to the <i>inPortIdx</i> input port of this resource.
   UtlBoolean disconnectInput(int inPortIdx);
     /**< @returns TRUE if successful, FALSE otherwise. */

     /// @brief Removes the connection to the <i>outPortIdx</i> output port of this resource.
   UtlBoolean disconnectOutput(int outPortIdx);
     /**< @returns TRUE if successful, FALSE otherwise. */

     /// @brief Associates this resource with the indicated flow graph.
   OsStatus setFlowGraph(MpFlowGraphBase* pFlowGraph);
     /**< @returns OS_SUCCESS - for now, this method always returns success */

     /// Sets the name that is associated with this resource.
   void setName(const UtlString& rName);

     /// Copy constructor (not implemented for this class)
   MpResource(const MpResource& rMpResource);

     /// Assignment operator (not implemented for this class)
   MpResource& operator=(const MpResource& rhs);

};

/* ============================ INLINE METHODS ============================ */

#endif  // _MpResource_h_
