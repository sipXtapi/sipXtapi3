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
package org.sipfoundry.sipxconfig.site;

import org.apache.tapestry.IRequestCycle;
import org.apache.tapestry.html.BasePage;

import org.sipfoundry.sipxconfig.admin.dialplan.DialPlan;
import org.sipfoundry.sipxconfig.admin.dialplan.DialPlanManager;

/**
 * Tapestry Page support for editing and creating new phone endpoints
 */
public abstract class EditDialPlan extends BasePage {
    private static final String LIST_PAGE = "ListDialPlans";

    // virtual properties
    public abstract DialPlanManager getDialPlanManager();

    public abstract DialPlan getDialPlan();

    public abstract void setDialPlan(DialPlan plan);

    public abstract void setAddMode(boolean b);

    public abstract boolean getAddMode();

    public void save(IRequestCycle cycle) {
        DialPlanManager manager = getDialPlanManager();
        DialPlan dialPlan = getDialPlan();
        if (getAddMode()) {
            manager.addDialPlan(dialPlan);
        } else {
            manager.updateDialPlan(dialPlan);
        }
        cycle.activate(LIST_PAGE);
    }

    public void cancel(IRequestCycle cycle) {
        cycle.activate(LIST_PAGE);
    }

}
