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
import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

/**
 * DialPlanManager TODO: need interface and hibernate persistence implementation
 */
public class DialPlanManager {
    private List m_gateways = new ArrayList();
    private FlexibleDialPlan m_flexDialPlan = new FlexibleDialPlan();

    public List getGateways() {
        return m_gateways;
    }

    public void setGateways(List gateways) {
        m_gateways = gateways;
    }

    public Gateway getGateway(Integer id) {
        if (null == id) {
            return null;
        }
        Object key = new Gateway(id);
        int i = m_gateways.indexOf(key);
        if (i < 0) {
            return null;
        }
        return (Gateway) m_gateways.get(i);
    }

    public boolean updateGateway(Integer id, Gateway gatewayData) {
        Gateway gateway = getGateway(id);
        if (null == gateway) {
            return false;
        }
        gateway.update(gatewayData);
        return true;
    }

    public boolean addGateway(Gateway gateway) {
        if (!m_gateways.remove(gateway)) {
            m_gateways.add(gateway);
            return true;
        }
        return false;
    }

    public void clear() {
        m_gateways.clear();
    }

    public boolean deleteGateway(Integer id) {
        return m_gateways.remove(new Gateway(id));
    }

    public void deleteGateways(Collection selectedRows) {
        for (Iterator i = selectedRows.iterator(); i.hasNext();) {
            Integer id = (Integer) i.next();
            deleteGateway(id);
        }
    }

    /**
     * Returns the list of gateways available for a specific rule
     * 
     * @param ruleId id of the rule for which gateways are checked
     * @return collection of available gateways
     */
    public Collection getAvailableGateways(Integer ruleId) {
        DialingRule rule = m_flexDialPlan.getRule(ruleId);
        if (null == rule) {
            return Collections.EMPTY_LIST;
        }
        Set gateways = new HashSet(getGateways());
        Collection ruleGateways = rule.getGateways();
        gateways.removeAll(ruleGateways);
        return gateways;
    }

    public FlexibleDialPlan getFlexDialPlan() {
        return m_flexDialPlan;
    }

    public void setFlexDialPlan(FlexibleDialPlan flexDialPlan) {
        m_flexDialPlan = flexDialPlan;
    }
}
