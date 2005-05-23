/*
 * 
 * 
 * Copyright (C) 2005 SIPfoundry Inc.
 * Licensed by SIPfoundry under the LGPL license.
 * 
 * Copyright (C) 2005 Pingtel Corp.
 * Licensed to SIPfoundry under a Contributor Agreement.
 * 
 * $
 */
package org.sipfoundry.sipxconfig.site.admin;

import junit.framework.Test;
import net.sourceforge.jwebunit.WebTestCase;

import org.sipfoundry.sipxconfig.site.SiteTestHelper;

public class BackupPageTestUi extends WebTestCase {
    public static Test suite() throws Exception {
        return SiteTestHelper.webTestSuite(BackupPageTestUi.class);
    }

    public void setUp() {
        getTestContext().setBaseUrl(SiteTestHelper.getBaseUrl());
        SiteTestHelper.home(getTester());
        clickLink("BackupPage");
    }    
    
    /**
     * Does not check if backup was successful - just checks if no Tapestry exceptions show up
     */
    public void testBackupNow() {
        SiteTestHelper.assertNoException(getTester());
        clickButton("backup:now");
        SiteTestHelper.assertNoException(getTester());
    }

    /**
     * Does not check if backup was successful - just checks if no Tapestry exceptions show up
     */
    public void testOk() {
        SiteTestHelper.assertNoException(getTester());
        checkCheckbox("checkVoicemail");
        checkCheckbox("checkConfigs");
        checkCheckbox("checkDatabase");
        selectOption("limitCount", "10");
        checkCheckbox("dailyScheduleEnabled");
        selectOption("dailyScheduledDay", "Wednesday");
        setFormElement("dailyScheduledTime", "3:24 AM");
        clickButton("backup:ok");        
        SiteTestHelper.assertNoException(getTester());
        assertCheckboxSelected("checkVoicemail");
        assertCheckboxSelected("checkConfigs");
        assertCheckboxSelected("checkDatabase");
        assertOptionEquals("limitCount", "10");
        assertCheckboxSelected("dailyScheduleEnabled");
        assertOptionEquals("dailyScheduledDay", "Wednesday");
        assertFormElementEquals("dailyScheduledTime", "3:24 AM");
    }
}
