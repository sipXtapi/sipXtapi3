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
package org.sipfoundry.sipxconfig.site;

import org.apache.tapestry.IRequestCycle;
import org.apache.tapestry.html.BasePage;
import org.sipfoundry.sipxconfig.admin.callgroup.CallGroupContext;
import org.sipfoundry.sipxconfig.admin.commserver.SipxProcessContext;
import org.sipfoundry.sipxconfig.admin.commserver.imdb.DataSet;
import org.sipfoundry.sipxconfig.admin.dialplan.DialPlanContext;
import org.sipfoundry.sipxconfig.common.CoreContext;
import org.sipfoundry.sipxconfig.common.User;
import org.sipfoundry.sipxconfig.components.TapestryUtils;
import org.sipfoundry.sipxconfig.gateway.GatewayContext;
import org.sipfoundry.sipxconfig.phone.PhoneContext;
import org.sipfoundry.sipxconfig.site.admin.commserver.RestartReminder;
import org.sipfoundry.sipxconfig.site.setting.EditGroup;

/**
 * TestPage page
 */
public abstract class TestPage extends BasePage {

    public static final String PAGE = "TestPage";

    public static final User TEST_USER = new User();
    public static final String TEST_USER_PIN = "1234";
    static {
        // makesure this name matches SiteTestHelper.java
        TEST_USER.setUserName("testuser");
        TEST_USER.setFirstName("Test");
        TEST_USER.setExtension("200");
        TEST_USER.setLastName("User");
    }

    public abstract DialPlanContext getDialPlanManager();

    public abstract GatewayContext getGatewayContext();

    public abstract PhoneContext getPhoneContext();

    public abstract CallGroupContext getCallGroupContext();

    public abstract CoreContext getCoreContext();

    public abstract SipxProcessContext getSipxProcessContext();

    public void resetDialPlans(IRequestCycle cycle_) {
        getDialPlanManager().clear();
        getGatewayContext().clear();
    }

    public void resetPhoneContext(IRequestCycle cycle_) {
        getPhoneContext().clear();
    }

    public void resetCallGroupContext(IRequestCycle cycle_) {
        getCallGroupContext().clear();
    }

    public void resetCoreContext(IRequestCycle cycle) {
        // need to reset all data that could potentially have a reference
        // to users
        resetDialPlans(cycle);
        resetPhoneContext(cycle);
        resetCallGroupContext(cycle);
        getCoreContext().clear();
        Visit visit = (Visit) getVisit();
        visit.logout();
    }

    public void newGroup(IRequestCycle cycle) {
        String resource = (String) TapestryUtils.assertParameter(String.class, cycle.getServiceParameters(), 0);
        EditGroup page = (EditGroup) cycle.getPage(EditGroup.PAGE);
        page.newGroup(resource, PAGE);
        cycle.activate(page);
    }

    public void goToRestartReminderPage(IRequestCycle cycle) {
        RestartReminder page = (RestartReminder) cycle.getPage(RestartReminder.PAGE);
        page.setNextPage(PAGE);
        cycle.activate(page);
    }

    public void toggleNavigation(IRequestCycle cycle_) {
        Visit visit = (Visit) getVisit();
        visit.setNavigationVisible(!visit.isNavigationVisible());
    }

    public void toggleAdmin(IRequestCycle cycle) {
        Visit visit = (Visit) getVisit();
        boolean admin = !visit.isAdmin();
        Integer userId = visit.getUserId();
        if (userId == null) {
            login(cycle);
        } else {
            visit.login(userId, admin);
        }
    }

    public void seedTestUser(IRequestCycle cycle_) {
        User user = new User();
        user.setUserName(TEST_USER.getUserName());
        user.setFirstName(TEST_USER.getFirstName());
        user.setLastName(TEST_USER.getLastName());
        user.setExtension(TEST_USER.getExtension());
        user.setPin(TEST_USER_PIN, getCoreContext().getAuthorizationRealm());
        getCoreContext().saveUser(user);
    }

    public void login(IRequestCycle cycle) {        
        CoreContext core = getCoreContext(); 
        User user = core.loadUserByUserName(TEST_USER.getUserName());
        if (user == null) {
            seedTestUser(cycle);
            user = core.loadUserByUserName(TEST_USER.getUserName());
        }
        Visit visit = (Visit) getVisit();
        visit.login(user.getId(), true);
    }

    public void generateDataSet(IRequestCycle cycle) {
        String setName = (String) TapestryUtils.assertParameter(String.class, cycle
                .getServiceParameters(), 0);
        SipxProcessContext sipxProcessContext = getSipxProcessContext();
        sipxProcessContext.generate(DataSet.getEnum(setName));
    }
}
