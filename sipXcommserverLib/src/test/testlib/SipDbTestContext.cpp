// 
// Copyright (C) 2006 SIPfoundry Inc.
// License by SIPfoundry under the LGPL license.
// 
// Copyright (C) 2006 Pingtel Corp.
// Licensed to SIPfoundry under a Contributor Agreement.
// 
// $$
//////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
#include <memory>
#include <stdlib.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

// APPLICATION INCLUDES
#include "os/OsFS.h"
#include "sipdb/SIPDBManager.h"
#include "sipxunit/TestUtilities.h"
#include "testlib/FileTestContext.h"
#include "testlib/SipDbTestContext.h"

// DEFINES
// CONSTANTS
// TYPEDEFS
// FORWARD DECLARATIONS

/// constructor
SipDbTestContext::SipDbTestContext( const char* testInputDir
                                   ,const char* testWorkingDir
                                   )
   : FileTestContext(testInputDir, testWorkingDir)
{
   setFastDbEnvironment();
};

void SipDbTestContext::setFastDbEnvironment()
{
   // Locate the registration DB in a test directory so that
   // we don't collide with the production DB.
   UtlString msg("failed to set environment to '");
   msg.append(mTestWorkingDir);
   msg.append("'");
   int status = setenv("SIPX_DB_CFG_PATH", mTestWorkingDir, 1);
   status += setenv("SIPX_DB_VAR_PATH", mTestWorkingDir, 1);

   CPPUNIT_ASSERT_EQUAL_MESSAGE(msg.data(), 0, status);
}

/// destructor
SipDbTestContext::~SipDbTestContext()
{
   delete SIPDBManager::getInstance();
};