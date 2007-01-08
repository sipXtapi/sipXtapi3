//
// Copyright (C) 2006 Plantronics
// 
//
// $$
////////////////////////////////////////////////////////////////////////
//////


#ifndef _SdpHelper_h_
#define _SdpHelper_h_

#ifndef EXCLUDE_SIPX_SDP_HELPER

// SYSTEM INCLUDES
//#include <...>

// APPLICATION INCLUDES
#include <net/SdpBody.h>
#include <sdp/Sdp.h>
#include <sdp/SdpMediaLine.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS

//: Container for SdpHelper specification
// This class holds the information related to an SdpHelper.
// Included in this information is:  TODO
//

class SdpHelper
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

/* ============================ CREATORS ================================== */

/* ============================ MANIPULATORS ============================== */
   static SdpMediaLine::SdpCryptoSuiteType convertCryptoSuiteType(int sdpBodyType);
   static Sdp* createSdpFromSdpBody(SdpBody& sdpBody);

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */


/* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
};

/* ============================ INLINE METHODS ============================ */

#endif // #ifndef EXCLUDE_SIPX_SDP_HELPER

#endif  // _SdpHelper_h_

