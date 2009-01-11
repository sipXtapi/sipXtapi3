//
// Copyright (C) 2007 SIPez LLC.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// Copyright (C) 2007 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// $$
///////////////////////////////////////////////////////////////////////////////

// Author: Sergey Kostanbaev <Sergey DOT Kostanbaev AT sipez DOT com>

#ifndef _MpodOss_h_
#define _MpodOss_h_

// SYSTEM INCLUDES
#include <pthread.h>
#include <semaphore.h>

#ifdef __linux__
#include <sys/types.h>
#include <sys/soundcard.h>
#endif // __linux__


// APPLICATION INCLUDES
#include "mp/MpOutputDeviceDriver.h"
#include "mp/MpOss.h"


// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS
class MpOutputDeviceManager;


/**
*  @brief Container for device specific input OSS driver.
*/
class MpodOss : public MpOutputDeviceDriver
{
    friend class MpOss;
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

/* ============================ CREATORS ================================== */
///@name Creators
//@{

     /// @brief Default constructor.
   MpodOss(const UtlString& name);
     /**<
     *  @param name - (in) unique device driver name (e.g. "/dev/dsp",
     *         "YAMAHA AC-XG WDM Audio", etc.).
     *  @param nInputBuffers - (in) Maximum number of frames in internal buffer.
     */

     /// @brief Destructor
   ~MpodOss();

//@}

/* ============================ MANIPULATORS ============================== */
///@name Manipulators
//@{

     /// @copydoc MpOutputDeviceDriver::enableDevice()
   OsStatus enableDevice(unsigned samplesPerFrame,
                         unsigned samplesPerSec,
                         MpFrameTime currentFrameTime);

     /// @copydoc MpOutputDeviceDriver::disableDevice()
   OsStatus disableDevice();

     /// @copydoc MpOutputDeviceDriver::pushFrame()
   OsStatus pushFrame(unsigned int numSamples,
                      const MpAudioSample* samples,
                      MpFrameTime frameTime);

     /// @copydoc MpOutputDeviceDriver::setTickerNotification()
   OsStatus setTickerNotification(OsNotification *pFrameTicker);

   OsStatus setNotificationMode(UtlBoolean bThreadNotification);

//@}

/* ============================ ACCESSORS ================================= */
///@name Accessors
//@{

//@}

/* ============================ INQUIRY =================================== */
///@name Inquiry
//@{

     /// @brief Inquire if the OSS device is valid
   inline UtlBoolean isDeviceValid();

//@}

/* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:
   MpAudioSample* mAudioFrame;  ///< Wave buffer.
   OsNotification *pNotificator; ///< Notificator used for signaling next frame
   MpFrameTime mCurrentFrameTime; ///< The current frame time for this device.
   UtlBoolean mNotificationThreadEn;

   MpOssContainer* mpCont; ///< Pointer to Wrapper container

     /// @brief Signaling for next frame if notificator used do nothing otherwise
   OsStatus signalForNextFrame();

     /// @brief Adding frame time to <tt>mCurrentFrameTime</tt>.
   void skipFrame();

/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   MpOss *pDevWrapper;

     /// @brief Copy constructor (not implemented for this class)
   MpodOss(const MpodOss& rMpOutputDeviceDriver);

     /// @brief Assignment operator (not implemented for this class)
   MpodOss& operator=(const MpodOss& rhs);


};

/* ============================ INLINE METHODS ============================ */

UtlBoolean MpodOss::isDeviceValid()
{
   //printf("MpRsOdOss::isDeviceValid()\n"); fflush(stdout);
   return ((pDevWrapper != NULL) && pDevWrapper->isDeviceValid());
}

#endif  // _MpodOss_h_
