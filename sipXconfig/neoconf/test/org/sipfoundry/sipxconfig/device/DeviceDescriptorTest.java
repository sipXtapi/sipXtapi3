/*
 * 
 * 
 * Copyright (C) 2007 SIPfoundry Inc.
 * Licensed by SIPfoundry under the LGPL license.
 * 
 * Copyright (C) 2007 Pingtel Corp.
 * Licensed to SIPfoundry under a Contributor Agreement.
 * 
 * $
 */
package org.sipfoundry.sipxconfig.device;

import junit.framework.TestCase;

public class DeviceDescriptorTest extends TestCase {
    public void testCleanSerialNumber() {
        DeviceDescriptor dd = new DeviceDescriptor() {
        };

        assertEquals("123456789012", dd.cleanSerialNumber("123456789012"));
        assertEquals("123456789012", dd.cleanSerialNumber("1234 5678 9012"));
        assertEquals("123456789012", dd.cleanSerialNumber("12:34:56:78:90:12"));
        assertEquals("aabbccddeeff", dd.cleanSerialNumber("AABBCCDDEEFF"));
        assertEquals("totallybogus", dd.cleanSerialNumber("totallybogus"));
        assertNull(dd.cleanSerialNumber(null));
    }
}