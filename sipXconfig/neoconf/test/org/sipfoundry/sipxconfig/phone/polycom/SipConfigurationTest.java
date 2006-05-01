/*
 * 
 * 
 * Copyright (C) 2004 SIPfoundry Inc.
 * Licensed by SIPfoundry under the LGPL license.
 * 
 * Copyright (C) 2004 Pingtel Corp.
 * Licensed to SIPfoundry under a Contributor Agreement.
 * 
 * $
 */
package org.sipfoundry.sipxconfig.phone.polycom;

import java.io.CharArrayReader;
import java.io.CharArrayWriter;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;

import org.custommonkey.xmlunit.Diff;
import org.custommonkey.xmlunit.XMLTestCase;
import org.custommonkey.xmlunit.XMLUnit;
import org.sipfoundry.sipxconfig.TestHelper;
import org.sipfoundry.sipxconfig.device.VelocityProfileGenerator;
import org.sipfoundry.sipxconfig.phone.PhoneTestDriver;
import org.sipfoundry.sipxconfig.setting.Setting;


public class SipConfigurationTest  extends XMLTestCase {
    
    PolycomPhone phone;
    
    PhoneTestDriver tester;
    
    protected void setUp() {
        XMLUnit.setIgnoreWhitespace(true);
        phone = new PolycomPhone(PolycomModel.MODEL_600);
        tester = new PhoneTestDriver(phone, "US/Eastern");        
    }
    
    public void testGenerateProfile() throws Exception {
        
        // settings selected at random, there are too many
        // to test all.  select a few.
        Setting endpointSettings = phone.getSettings();
        endpointSettings.getSetting("log/sip/level.change.sip").setValue("3");
        endpointSettings.getSetting("call/rejectBusyOnDnd").setValue("0");
        endpointSettings.getSetting("voIpProt.SIP/local/port").setValue("5061");
        endpointSettings.getSetting("call/rejectBusyOnDnd").setValue("0");

        Setting lineSettings = tester.line.getSettings();
        lineSettings.getSetting("call/serverMissedCall/enabled").setValue("1");
        
        VelocityProfileGenerator cfg = new SipConfiguration(phone);
        cfg.setVelocityEngine(TestHelper.getVelocityEngine());
        
        CharArrayWriter out = new CharArrayWriter();
        cfg.generateProfile(phone.getSipTemplate(), out);       
        
        InputStream expectedPhoneStream = getClass().getResourceAsStream("expected-sip.cfg.xml");
        Reader expectedXml = new InputStreamReader(expectedPhoneStream);            
        Reader generatedXml = new CharArrayReader(out.toCharArray());

        // helpful debug
        System.out.println(new String(out.toCharArray()));

        Diff phoneDiff = new Diff(expectedXml, generatedXml);
        assertXMLEqual(phoneDiff, true);
        expectedPhoneStream.close();        
    }
}
