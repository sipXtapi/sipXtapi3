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
package org.sipfoundry.sipxconfig.conference;

import java.text.MessageFormat;
import java.util.Collection;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;

import org.sipfoundry.sipxconfig.common.NamedObject;
import org.sipfoundry.sipxconfig.device.DeviceDefaults;
import org.sipfoundry.sipxconfig.setting.AbstractSettingVisitor;
import org.sipfoundry.sipxconfig.setting.BeanWithSettings;
import org.sipfoundry.sipxconfig.setting.Setting;
import org.sipfoundry.sipxconfig.setting.type.FileSetting;
import org.sipfoundry.sipxconfig.setting.type.SettingType;

public class Bridge extends BeanWithSettings implements NamedObject {
    public static final String BEAN_NAME = "conferenceBridge";
    
    public static final String CONFERENCES_PROP = "conferences";
    public static final String SERVICE_URI_PROP = "serviceUri";
    
    public static final String SIP_DOMAIN = "bridge-bridge/BOSTON_BRIDGE_SIP_DOMAIN";

    private boolean m_enabled;

    private String m_name;

    private String m_description;

    private Set m_conferences = new HashSet();

    private DeviceDefaults m_systemDefaults;

    private String m_audioDirectory;

    private int m_port;

    private String m_host;

    /** location - host:port of the admission server */
    private String m_admissionServer;

    public String getHost() {
        return m_host;
    }

    public void setHost(String host) {
        m_host = host;
    }

    public int getPort() {
        return m_port;
    }

    public void setPort(int port) {
        m_port = port;
    }

    public void addConference(Conference conference) {
        getConferences().add(conference);
        conference.setBridge(this);
    }

    public void addConferences(Collection conferences) {
        for (Iterator iter = conferences.iterator(); iter.hasNext();) {
            Conference conf = (Conference) iter.next();
            addConference(conf);
        }
    }

    public void removeConference(Conference conference) {
        conference.setBridge(null);
        getConferences().remove(conference);
    }

    public void setAdmissionServer(String admissionServer) {
        m_admissionServer = admissionServer;
    }

    public String getAdmissionServer() {
        return m_admissionServer;
    }

    // trivial get/set
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

    public Set getConferences() {
        return m_conferences;
    }

    public void setConferences(Set conferences) {
        m_conferences = conferences;
    }

    public void setSystemDefaults(DeviceDefaults systemDefaults) {
        m_systemDefaults = systemDefaults;
    }

    public void setAudioDirectory(String audioDirectory) {
        m_audioDirectory = audioDirectory;
    }

    protected void defaultSettings() {
        setSettingValue(SIP_DOMAIN, m_systemDefaults.getDomainName());
        getSettings().acceptVisitor(new AudioDirectorySetter());
    }

    private class AudioDirectorySetter extends AbstractSettingVisitor {
        public void visitSetting(Setting setting) {
            SettingType type = setting.getType();
            if (type instanceof FileSetting) {
                FileSetting fileType = (FileSetting) type;
                fileType.setDirectory(m_audioDirectory);
            }
        }
    }

    public String getServiceUri() {
        Object[] params = new Object[] {
            m_host, Integer.toString(m_port)
        };
        return MessageFormat.format("https://{0}:{1}/RPC2", params);
    }
}
