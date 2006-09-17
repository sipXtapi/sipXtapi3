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
package org.sipfoundry.sipxconfig.admin.dialplan;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

import org.apache.commons.lang.ArrayUtils;
import org.sipfoundry.sipxconfig.admin.dialplan.config.Transform;
import org.sipfoundry.sipxconfig.common.BeanWithId;
import org.sipfoundry.sipxconfig.common.DataCollectionItem;
import org.sipfoundry.sipxconfig.common.DataCollectionUtil;
import org.sipfoundry.sipxconfig.common.NamedObject;
import org.sipfoundry.sipxconfig.gateway.Gateway;
import org.sipfoundry.sipxconfig.permission.Permission;
import org.sipfoundry.sipxconfig.permission.PermissionManager;

/**
 * DialingRule At some point it will be replaced by the IDialingRule interface or made abstract.
 */
public abstract class DialingRule extends BeanWithId implements IDialingRule, DataCollectionItem,
        NamedObject {

    private boolean m_enabled;
    private String m_name;
    private String m_description;
    private int m_position;
    private List<Gateway> m_gateways = new ArrayList<Gateway>();
    private PermissionManager m_permissionManager;

    public abstract String[] getPatterns();

    public abstract Transform[] getTransforms();

    public abstract DialingRuleType getType();

    protected Object clone() throws CloneNotSupportedException {
        DialingRule clone = (DialingRule) super.clone();
        clone.m_gateways = new ArrayList(m_gateways);
        return clone;
    }

    public String getDescription() {
        return m_description;
    }

    public void setDescription(String description) {
        m_description = description;
    }

    public boolean isEnabled() {
        return m_enabled;
    }

    public void setEnabled(boolean enabled) {
        m_enabled = enabled;
    }

    public String getName() {
        return m_name;
    }

    public void setName(String name) {
        m_name = name;
    }

    public List<Gateway> getGateways() {
        return m_gateways;
    }

    public void setGateways(List<Gateway> gateways) {
        m_gateways = gateways;
    }

    public List<Permission> getPermissions() {
        return Collections.emptyList();
    }

    /**
     * @return list of Gateway objects representing source hosts
     */
    public List getHosts() {
        return null;
    }

    public boolean addGateway(Gateway gateway) {
        boolean existed = !m_gateways.remove(gateway);
        m_gateways.add(gateway);
        return existed;
    }

    public void removeGateways(Collection selectedGateways) {
        for (Iterator i = selectedGateways.iterator(); i.hasNext();) {
            Integer id = (Integer) i.next();
            m_gateways.remove(new BeanWithId(id));
        }
    }

    /**
     * Called to give a dialing rules a chance to append itself to the list of rules used for
     * generating XML
     * 
     * Default implementation appends the rule if it is enabled. Rule can append some other rules.
     * 
     * @param rules
     */
    public void appendToGenerationRules(List<DialingRule> rules) {
        if (isEnabled()) {
            rules.add(this);
        }
    }

    /**
     * Returns the lis of gateways that can be added to this rule.
     * 
     * @param allGateways pool of all possible gateways
     * @return list of gateways that still can be assigned to this rule
     */
    public Collection<Gateway> getAvailableGateways(List<Gateway> allGateways) {
        Set<Gateway> gateways = new HashSet<Gateway>(allGateways);
        Collection<Gateway> ruleGateways = getGateways();
        gateways.removeAll(ruleGateways);
        return gateways;
    }

    public void moveGateways(Collection ids, int step) {
        DataCollectionUtil.moveByPrimaryKey(m_gateways, ids.toArray(), step, false);
    }

    public int getPosition() {
        return m_position;
    }

    public void setPosition(int position) {
        m_position = position;
    }

    public Object getPrimaryKey() {
        return getId();
    }

    /**
     * Attempts to apply standard transformations to patterns. It is used when generating
     * authorization rules For example 9xxxx -> 7{digits} results in 97xxxx
     * 
     * Default implementation inserts gateway specific prefixes.
     */
    public String[] getTransformedPatterns(Gateway gateway) {
        String[] patterns = getPatterns();
        String[] transformed = new String[patterns.length];
        for (int i = 0; i < patterns.length; i++) {
            transformed[i] = gateway.getCallPattern(patterns[i]);
        }
        return transformed;
    }

    public void setPermissionManager(PermissionManager permissionManager) {
        m_permissionManager = permissionManager;
    }

    protected Permission getPermission(String name) {
        if (m_permissionManager == null) {
            throw new IllegalStateException("Permission manager not configured.");
        }
        return m_permissionManager.getPermission(name);
    }
    
    public String[] getHostPatterns() {
        return ArrayUtils.EMPTY_STRING_ARRAY;
    }
}
