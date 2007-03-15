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

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import junit.framework.TestCase;

import org.easymock.EasyMock;
import org.easymock.IMocksControl;

public class AbstractProfileGeneratorTest extends TestCase {

    private static class ProfileGeneratorStub extends AbstractProfileGenerator {
        protected void generateProfile(ProfileContext context, OutputStream out) throws IOException {
            out.write(context.getProfileTemplate().getBytes("US-ASCII"));
        }
    }

    private static class DoublingProfileFilter implements ProfileFilter {
        public void copy(InputStream in, OutputStream out) throws IOException {
            int i;
            while ((i = in.read()) != -1) {
                out.write(i);
                out.write(i);
            }
        }
    }

    protected void setUp() throws Exception {
        super.setUp();
    }

    public void testGenerateProfileContextStringString() {
        MemoryProfileLocation location = new MemoryProfileLocation();
        AbstractProfileGenerator pg = new ProfileGeneratorStub();
        pg.setProfileLocation(location);
        pg.generate(new ProfileContext(null, "bongo"), "ignored");
        assertEquals("bongo", location.toString());
    }

    public void testGenerateProfileContextStringProfileFilterString() {
        MemoryProfileLocation location = new MemoryProfileLocation();
        AbstractProfileGenerator pg = new ProfileGeneratorStub();
        pg.setProfileLocation(location);
        pg.generate(new ProfileContext(null, "bongo"), new DoublingProfileFilter(), "ignored");
        assertEquals("bboonnggoo", location.toString());
    }

    public void testRemove() {
        IMocksControl locationControl = EasyMock.createControl();
        ProfileLocation location = locationControl.createMock(ProfileLocation.class);

        AbstractProfileGenerator pg = new ProfileGeneratorStub();
        pg.setProfileLocation(location);

        locationControl.replay();

        pg.remove(null);
        locationControl.verify();

        locationControl.reset();
        locationControl.replay();

        pg.remove("");
        locationControl.verify();

        locationControl.reset();
        location.removeProfile("kuku");
        locationControl.replay();

        pg.remove("kuku");
        locationControl.verify();
    }
}
