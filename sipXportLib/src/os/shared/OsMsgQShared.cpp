//
// Copyright (C) 2007 SIPez LLC. 
// Licensed to SIPfoundry under a Contributor Agreement. 
//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////


// SYSTEM INCLUDES
#include <assert.h>

// APPLICATION INCLUDES
#include "os/OsLock.h"
#include "os/shared/OsMsgQShared.h"
#include "os/OsDateTime.h"
#include "os/OsSysLog.h"

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
// If the name is specified but is already in use, throw an exception
OsMsgQShared::OsMsgQShared(const int maxMsgs, const int maxMsgLen,
                           const int options, const UtlString& name)
: OsMsgQBase(name)
, mGuard(OsMutex::Q_PRIORITY + OsMutex::INVERSION_SAFE +
         OsMutex::DELETE_SAFE)
, mEmpty(OsCSem::Q_PRIORITY, maxMsgs, maxMsgs)
, mFull(OsCSem::Q_PRIORITY, maxMsgs, 0)
, mDlist()
#ifdef MSGQ_IS_VALID_CHECK
, mOptions(options)
, mHighCnt(0)
#endif
{
   mMaxMsgs = maxMsgs;

#ifdef OS_MSGQ_REPORTING
   mIncrementLevel = mMaxMsgs / 20;
   if (mIncrementLevel < 1)
      mIncrementLevel = 1;
   mIncreaseLevel = mIncrementLevel;
   mDecreaseLevel = 0;
#endif

#ifdef MSGQ_IS_VALID_CHECK /* [ */
   OsStatus ret = mGuard.acquire();         // start critical section
   assert(ret == OS_SUCCESS);

   mNumInsertEntry = 0;
   mNumInsertExitOk = 0;
   mNumInsertExitFail = 0;

   mNumRemoveEntry = 0;
   mNumRemoveExitOk = 0;
   mNumRemoveExitFail = 0;

   mLastSuccessTest = 0;

   ret = mGuard.release();         // exit critical section
   assert(ret == OS_SUCCESS);
#endif /* MSGQ_IS_VALID_CHECK ] */
}

// Destructor
OsMsgQShared::~OsMsgQShared()
{
    if (numMsgs())
        flush();    // get rid of any messages in the queue
}

/* ============================ MANIPULATORS ============================== */

OsStatus OsMsgQShared::send(const OsMsg& rMsg,
                            const OsTime& rTimeout)
{
   return doSend(rMsg, rTimeout, FALSE, TRUE);
}

OsStatus OsMsgQShared::sendNoCopy(OsMsg *pMsg,
                                  const OsTime& rTimeout)
{
   return doSend(*pMsg, rTimeout, FALSE, FALSE);
}

OsStatus OsMsgQShared::sendUrgent(const OsMsg& rMsg,
                                  const OsTime& rTimeout)
{
   return doSend(rMsg, rTimeout, TRUE, TRUE);
}

OsStatus OsMsgQShared::sendFromISR(OsMsg& rMsg)
{
   // set a flag in the msg to indicate if the message was sent 
   // from an ISR
   rMsg.setSentFromISR(TRUE);

   return doSend(rMsg, OsTime::NO_WAIT_TIME, FALSE, FALSE);
}

// Remove a message from the head of the queue
// Wait until either a message arrives or the timeout expires.
// The receiver is responsible for freeing the received message.
// Other than for messages sent from an ISR, the receiver is responsible
// for freeing the received message.
OsStatus OsMsgQShared::receive(OsMsg*& rpMsg, const OsTime& rTimeout)
{
   return doReceive(rpMsg, rTimeout);
}


/* ============================ ACCESSORS ================================= */

// Return the number of messages in the queue
int OsMsgQShared::numMsgs(void)
{
   OsLock lock(mGuard);

   return(mDlist.entries());
}

// Print information on the message queue to the console
// Output enabled via a compile-time #ifdef
#ifdef MSGQ_IS_VALID_CHECK
void OsMsgQShared::show(void)
{
   osPrintf("* OsMsgQShared: OsMsgQ=0x%p, options=%d, limitMsgs=%d, maxMsgs=%d, numMsgs=%d\n",
            (void *) this, mOptions, mMaxMsgs, mHighCnt, numMsgs());

   osPrintf("* OsMsgQShared: mEmpty counting semaphore information\n");
   mEmpty.OsCSemShow();

   osPrintf("* OsMsgQShared: mFull counting semaphore information\n");
   mFull.OsCSemShow();
}
#endif

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

OsStatus OsMsgQShared::doSend(const OsMsg& rMsg, const OsTime& rTimeout,
                              const UtlBoolean isUrgent,
                              const UtlBoolean needCopy)
{
   OsStatus ret;
   OsMsg*   pMsg;
   const void*    insResult;

#ifdef MSGQ_IS_VALID_CHECK /* [ */
   int      msgCnt;

   ret = mGuard.acquire();         // start critical section
   assert(ret == OS_SUCCESS);

   testMessageQ();

   mNumInsertEntry++;

   ret = mGuard.release();         // exit critical section
   assert(ret == OS_SUCCESS);
#endif /* MSGQ_IS_VALID_CHECK ] */

   if (mSendHookFunc != NULL)
   {
      if (mSendHookFunc(rMsg))
      {
         // by returning TRUE, the mSendHookFunc indicates that it has handled
         // the message and there is no need to queue the message.
#ifdef MSGQ_IS_VALID_CHECK /* [ */
         OsStatus rc = mGuard.acquire();         // start critical section
         assert(rc == OS_SUCCESS);

         mNumInsertExitOk++;
         testMessageQ();

         rc = mGuard.release();         // exit critical section
         assert(rc == OS_SUCCESS);
#endif /* MSGQ_IS_VALID_CHECK ] */
         return OS_SUCCESS;
      }
   }

   ret = mEmpty.acquire(rTimeout);   // wait for there to be room in the queue
   if (ret != OS_SUCCESS)
   {
      // The caller should be responsible for logging this information
      /*OsSysLog::add(FAC_KERNEL, PRI_CRIT,
                    "OsMsgQShared::doSend message send failed - no room, ret = %d",
                    ret);*/
      if (ret == OS_BUSY || ret == OS_WAIT_TIMEOUT)
         ret =  OS_WAIT_TIMEOUT;     // send timed out
   }
   else
   {
      if (!needCopy || rMsg.isMsgReusable())
      {
         // If the message is sent from an ISR we cannot make a copy 
         // (no allocation allowed), so in that case we just use the message.

         // If the message is marked as reusable, it's safe to use the
         // message without copying it.

         // Just go ahead and use the message without copying it.
         pMsg = (OsMsg*) &rMsg;
      }
      else
      {
         // we place a copy of the message on the queue
         // so that the caller is free to destroy the original
         pMsg = rMsg.createCopy();
      }

      // start critical section
      ret = mGuard.acquire();
      assert(ret == OS_SUCCESS);

      if (isUrgent)
      {
         // If the message is urgent, insert it at the queue head
         insResult = mDlist.insertAt(0, pMsg);
      }
      else
      {
         // If the message is not urgent, insert it at the queue tail
         insResult = mDlist.insert(pMsg);
      }

#ifdef MSGQ_IS_VALID_CHECK
      msgCnt = mDlist.entries();
      if (msgCnt > mHighCnt)
      {
         mHighCnt = msgCnt;
      }
#endif

      if (insResult == NULL)
      {
         // queue insertion failed
         OsSysLog::add(FAC_KERNEL, PRI_CRIT,
                       "OsMsgQShared::doSend message send failed - insert failed");

         if (needCopy && !rMsg.isMsgReusable())
         {
            // destroy the msg copy we made earlier
            delete pMsg;
         }
         assert(FALSE);

         ret = OS_UNSPECIFIED;
      }
      else
      {
         // signal receivers that a msg is available
         ret = mFull.release();
         assert(ret == OS_SUCCESS);
      }

#ifdef OS_MSGQ_REPORTING
      int curCount;
      UtlBoolean increasedLevel = FALSE;
      UtlBoolean decreasedLevel = FALSE;

      curCount = mDlist.entries();
      if (curCount >= mIncreaseLevel)
      {
          increasedLevel = TRUE;
          while (curCount >= mIncreaseLevel)
          {
              mIncreaseLevel += mIncrementLevel;
          }
          mDecreaseLevel = mIncreaseLevel - (2 * mIncrementLevel);
      }

      if (curCount <= mDecreaseLevel)
      {
          decreasedLevel = TRUE;
          while (curCount <= mDecreaseLevel)
          {
              mDecreaseLevel = mDecreaseLevel - mIncrementLevel;
          }
          mIncreaseLevel = mDecreaseLevel + (2 * mIncrementLevel);
      }
#endif

      OsStatus guardRet = mGuard.release();           // exit critical section
      assert(guardRet == OS_SUCCESS);

#ifdef OS_MSGQ_REPORTING
      if (increasedLevel)
      {
          OsSysLogPriority pri = PRI_INFO;
          if (curCount == mMaxMsgs)
                pri = PRI_WARNING;

          OsSysLog::add(FAC_KERNEL, pri,
                    "Message queue 0x%x increased to %d msgs (max=%d)\n",
                    (int) this, curCount, mMaxMsgs);
      }
      else if (decreasedLevel)
      {
          OsSysLog::add(FAC_KERNEL, PRI_INFO,
                        "Message queue 0x%x decreased to %d msgs (max=%d)\n",
                        (int) this, curCount, mMaxMsgs);
      }
#endif
   }

#ifdef MSGQ_IS_VALID_CHECK /* [ */
   OsStatus rc = mGuard.acquire();         // start critical section
   assert(rc == OS_SUCCESS);

   if (ret == OS_SUCCESS)
      mNumInsertExitOk++;
   else
      mNumInsertExitFail++;

   testMessageQ();

   rc = mGuard.release();         // exit critical section
   assert(rc == OS_SUCCESS);
#endif /* MSGQ_IS_VALID_CHECK ] */

   return ret;
}

// Helper function for removing a message from the head of the queue
OsStatus OsMsgQShared::doReceive(OsMsg*& rpMsg, const OsTime& rTimeout)
{
   OsStatus ret;

   rpMsg = NULL;

#ifdef MSGQ_IS_VALID_CHECK /* [ */
   ret = mGuard.acquire();         // start critical section
   assert(ret == OS_SUCCESS);

   testMessageQ();
   mNumRemoveEntry++;

   ret = mGuard.release();         // exit critical section
   assert(ret == OS_SUCCESS);
#endif /* MSGQ_IS_VALID_CHECK ] */

   ret = mFull.acquire(rTimeout);  // wait for a message to be available
   if (ret != OS_SUCCESS)
   {
      if (ret == OS_BUSY || ret == OS_WAIT_TIMEOUT)
         ret = OS_WAIT_TIMEOUT;   // receive timed out
      else
      {
         assert(FALSE);
         ret = OS_UNSPECIFIED;
      }
   }
   else
   {
      ret = mGuard.acquire();         // start critical section
      assert(ret == OS_SUCCESS);

      assert(numMsgs() > 0);
      rpMsg = (OsMsg*) mDlist.get();  // get the first message

      if (rpMsg == NULL)              // was there a message?
      {
         assert(FALSE);
         ret = OS_UNSPECIFIED;
      }
      else
      {
         ret = mEmpty.release();         // the remove operation succeeded, signal
         assert(ret == OS_SUCCESS);      //  senders that there is an available
                                         //  message slot.
      }

      (void)mGuard.release();         // exit critical section
   }

#ifdef MSGQ_IS_VALID_CHECK /* [ */
   OsStatus rc = mGuard.acquire();         // start critical section
   assert(rc == OS_SUCCESS);

   if (ret == OS_SUCCESS)
      mNumRemoveExitOk++;
   else
      mNumRemoveExitFail++;

   testMessageQ();

   rc = mGuard.release();         // exit critical section
   assert(rc == OS_SUCCESS);
#endif /* MSGQ_IS_VALID_CHECK ] */


   return ret;
}

#if defined(MSGQ_IS_VALID_CHECK) && defined(OS_CSEM_DEBUG) /* [ */
// Test for message queue integrity
void OsMsgQShared::testMessageQ()
{

   if ( (mNumInsertEntry - mNumInsertExitOk - mNumInsertExitFail == 0) &&
        (mNumRemoveEntry - mNumRemoveExitOk - mNumRemoveExitFail == 0))
   {
      unsigned int numMsgs = mDlist.entries();
      assert(numMsgs == mNumInsertExitOk - mNumRemoveExitOk);
      assert(mEmpty.getValue() == mMaxMsgs - numMsgs);
      assert(mFull.getValue() == numMsgs);
      mLastSuccessTest = 0;
   }
   else
   {
      mLastSuccessTest++;
   }
}
#endif /* defined(MSGQ_IS_VALID_CHECK) && defined(OS_CSEM_DEBUG) ] */

/* ============================ FUNCTIONS ================================= */
