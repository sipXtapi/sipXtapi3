/*
 * 
 * 
 * Copyright (C) 2006 SIPfoundry Inc.
 * Licensed by SIPfoundry under the LGPL license.
 * 
 * Copyright (C) 2006 Pingtel Corp.
 * Licensed to SIPfoundry under a Contributor Agreement.
 * 
 * $
 */
package org.sipfoundry.sipxconfig.phone;


/**
 * Information to describe an external line.
 */
public class LineInfo {
    private String m_registrationServer;
    private String m_registrationServerPort;
    private String m_userId;
    private String m_password;
    private String m_voiceMail;
    private String m_displayName;
    
    public String getDisplayName() {
        return m_displayName;
    }
    public void setDisplayName(String displayName) {
        m_displayName = displayName;
    }
    public String getPassword() {
        return m_password;
    }
    public void setPassword(String password) {
        m_password = password;
    }
    public String getRegistrationServer() {
        return m_registrationServer;
    }
    public void setRegistrationServer(String registrationServer) {
        m_registrationServer = registrationServer;
    }
    public String getUserId() {
        return m_userId;
    }
    public void setUserId(String userId) {
        m_userId = userId;
    }
    public String getVoiceMail() {
        return m_voiceMail;
    }
    public void setVoiceMail(String voiceMail) {
        m_voiceMail = voiceMail;
    }
    public String getRegistrationServerPort() {
        return m_registrationServerPort;
    }
    public void setRegistrationServerPort(String registrationServerPort) {
        m_registrationServerPort = registrationServerPort;
    }    
}
