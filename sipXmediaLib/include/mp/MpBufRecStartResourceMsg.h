//  
// Copyright (C) 2007 SIPez LLC. 
// Licensed to SIPfoundry under a Contributor Agreement. 
//
// Copyright (C) 2007 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef _MpBufRecStartResourceMsg_h_
#define _MpBufRecStartResourceMsg_h_

// SYSTEM INCLUDES

// APPLICATION INCLUDES
#include "os/OsMsg.h"
#include "mp/MpResourceMsg.h"
#include "mp/MprBufferRecorder.h"
#include "utl/UtlString.h"

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS

/**
*  @brief Message used for starting to record to a buffer.
*
*  This message is constructed with a user supplied buffer, and is intended
*  to be put on a flowgraph queue, and eventually received by an
*  MprBufferRecorder for use in starting recording.
*/
class MpBufRecStartResourceMsg : public MpResourceMsg
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

   /* ============================ CREATORS ================================== */
   ///@name Creators
   //@{

     /// Constructor
   inline MpBufRecStartResourceMsg(const UtlString& msgDestName, 
                                   UtlString* pAudioBuf);

     /// Copy constructor
   inline MpBufRecStartResourceMsg(const MpBufRecStartResourceMsg& rMpResourceMsg);

     /// Create a copy of this msg object (which may be of a derived type)
   inline OsMsg* createCopy(void) const;

     /// Destructor
   inline ~MpBufRecStartResourceMsg();

   //@}

     /* ============================ MANIPULATORS ============================== */
     ///@name Manipulators
     //@{

     /// Assignment operator
   inline MpBufRecStartResourceMsg& operator=(const MpBufRecStartResourceMsg& rhs);

     /// @brief Set the audio buffer that is associated with this message.
   inline void setAudioBuffer(UtlString* pAudioBuf);
     /**<
     *  Set the record buffer that is associated with this message.
     *  Ownership remains with the caller of this function  We are just a user.
     *  @param pAudioBuf the new audio buffer that is to be set in this resource.
     */

   /* ============================ ACCESSORS ================================= */
   ///@name Accessors
   //@{

     /// @brief Get the record buffer that is associated with this resource.
   inline UtlString* getAudioBuffer(void) const;

   //@}

   /* ============================ INQUIRY =================================== */
   ///@name Inquiry
   //@{

   //@}

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   UtlString* mpAudioBuf; ///< The buffer to record audio to.
};

/* ============================ INLINE METHODS ============================ */

MpBufRecStartResourceMsg::MpBufRecStartResourceMsg(const UtlString& msgDestName, 
                                                   UtlString* pAudioBuf)
: MpResourceMsg(MPRM_BUF_RECORDER_START, msgDestName)
, mpAudioBuf(pAudioBuf)
{
}

MpBufRecStartResourceMsg::MpBufRecStartResourceMsg(const MpBufRecStartResourceMsg& rMpResourceMsg)
: MpResourceMsg(rMpResourceMsg)
, mpAudioBuf(rMpResourceMsg.mpAudioBuf)
{
}

inline OsMsg* MpBufRecStartResourceMsg::createCopy(void) const 
{
   return new MpBufRecStartResourceMsg(*this); 
}

inline MpBufRecStartResourceMsg::~MpBufRecStartResourceMsg() 
{
   mpAudioBuf = NULL;
}

MpBufRecStartResourceMsg& 
MpBufRecStartResourceMsg::operator=(const MpBufRecStartResourceMsg& rhs)
{
   if (this == &rhs) 
      return *this;  // handle the assignment to self case

   MpResourceMsg::operator=(rhs);  // assign fields for parent class
   mpAudioBuf = rhs.mpAudioBuf;
   return *this;
}

void MpBufRecStartResourceMsg::setAudioBuffer(UtlString* pAudioBuf) 
{ 
   mpAudioBuf = pAudioBuf;
}

UtlString* MpBufRecStartResourceMsg::getAudioBuffer(void) const
{
   return mpAudioBuf;
}

#endif  // _MpBufRecStartResourceMsg_h_
