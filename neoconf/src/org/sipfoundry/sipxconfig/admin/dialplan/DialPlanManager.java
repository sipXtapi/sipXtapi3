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
package org.sipfoundry.sipxconfig.admin.dialplan;

import java.util.ArrayList;
import java.util.List;

/**
 * DialPlanManager
 * TODO: need interface and hibernate persistence implementation
 */
public class DialPlanManager {
    private List m_dialPlans = new ArrayList();
    private List m_gateways = new ArrayList();

    public List getDialPlans() {
        return m_dialPlans;
    }

    public void setDialPlans(List dialPlans) {
        m_dialPlans = dialPlans;
    }

    public List getGateways() {
        return m_gateways;
    }

    public void setGateways(List gateways) {
        m_gateways = gateways;
    }

    public boolean updateGateway(Gateway gateway) {
        if (m_gateways.remove(gateway)) {
            m_gateways.add(gateway);
            return true;
        }
        return false;
    }

    public boolean deleteGateway(Gateway gateway) {
        return m_gateways.remove(gateway);
    }

    public boolean addGateway(Gateway gateway) {
        if (!m_gateways.remove(gateway)) {
            m_gateways.add(gateway);
            return true;
        }
        return false;
    }

    public boolean addDialPlan(DialPlan dialPlan) {
        if (!m_dialPlans.remove(dialPlan)) {
            m_dialPlans.add(dialPlan);
            return true;
        }
        return false;
        
    }

    public boolean updateDialPlan(DialPlan dialPlan) {
        if (m_dialPlans.remove(dialPlan)) {
            m_dialPlans.add(dialPlan);
            return true;
        }
        return false;
    }
}
