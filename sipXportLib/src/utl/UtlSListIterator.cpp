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
// APPLICATION INCLUDES
#include "utl/UtlSListIterator.h"
#include "utl/UtlSList.h"
#include "utl/UtlContainable.h"
#include "os/OsLock.h"

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
UtlSListIterator::UtlSListIterator(const UtlSList& list)
   : UtlListIterator(list)
{
}


/* ============================ MANIPULATORS ============================== */



// Find the next like-instance of the designated object .
UtlContainable* UtlSListIterator::findNext(const UtlContainable* containableToMatch) 
{
   UtlContainable* match = NULL;

   UtlContainer::acquireIteratorConnectionLock();
   OsLock takeContainer(mContainerRefLock);
   UtlSList* myList = static_cast<UtlSList*>(mpMyContainer);
   
   OsLock take(myList->mContainerLock);
   UtlContainer::releaseIteratorConnectionLock();

   // advance the iterator
   UtlLink* nextLink = (mpCurrentNode == NULL
                        ? myList->head()
                        : mpCurrentNode->next()
                        );

   // search for the next match forward
   while (nextLink && !match)
   {
      UtlContainable *candidate = (UtlContainable*)nextLink->data;
      if (candidate && candidate->compareTo(containableToMatch) == 0)
      {
         mpCurrentNode = nextLink;
         match = candidate;
      }
      else
      {
         nextLink = nextLink->next();
      }
   }
   
   return match;
}


UtlContainable* UtlSListIterator::insertAfterPoint(UtlContainable* insertedObject) 
{
   UtlContainable*    result = NULL;

   UtlContainer::acquireIteratorConnectionLock();
   OsLock takeContainer(mContainerRefLock);
   UtlSList* myList = static_cast<UtlSList*>(mpMyContainer);

   OsLock take(myList->mContainerLock);
   UtlContainer::releaseIteratorConnectionLock();

   if (mpCurrentNode == UtlListIterator::OFF_LIST_END)
   {
      mpCurrentNode = UtlLink::listBefore(myList, NULL, insertedObject); /* append to tail */
   }
   else
   {
      UtlLink::listAfter(myList, mpCurrentNode, insertedObject);
   }
   
   result = insertedObject; 
   
   return result;
}




/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */
