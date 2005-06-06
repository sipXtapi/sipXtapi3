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
package org.sipfoundry.sipxconfig.common;

import java.io.Serializable;

import org.sipfoundry.sipxconfig.phone.PhoneContext;

/**
 * User that logs in, and base info for most lines
 */
public class User implements PrimaryKeySource, Serializable {

    private static final long serialVersionUID = 1L;

    private Integer m_id = PhoneContext.UNSAVED_ID;

    private String m_firstName;

    private Organization m_organization;

    private String m_password;
    
    private String m_pintoken;

    private Integer m_ugId = new Integer(1); //default group

    private Integer m_rcsId = new Integer(2); // 2='Complete User'

    private String m_lastName;

    private String m_displayId;

    private String m_extension;

    private String m_profileEncryptionKey;

    public String getPintoken() {
        return m_pintoken;
    }

    public void setPintoken(String pintoken) {
        m_pintoken = pintoken;
    }

    public Integer getId() {
        return m_id;
    }

    public void setId(Integer id) {
        m_id = id;
    }

    public String getFirstName() {
        return m_firstName;
    }

    public void setFirstName(String firstName) {
        m_firstName = firstName;
    }

    public String getPassword() {
        return m_password;
    }

    public void setPassword(String password) {
        m_password = password;
    }

    public Integer getUserGroupId() {
        return m_ugId;
    }

    public void setUserGroupId(Integer ugId) {
        m_ugId = ugId;
    }

    public Integer getRcsId() {
        return m_rcsId;
    }

    public void setRcsId(Integer rcsId) {
        m_rcsId = rcsId;
    }

    public String getLastName() {
        return m_lastName;
    }

    public void setLastName(String lastName) {
        m_lastName = lastName;
    }

    public String getDisplayId() {
        return m_displayId;
    }

    public void setDisplayId(String displayId) {
        m_displayId = displayId;
    }
    
    public String getDisplayName() {
        StringBuffer sb = new StringBuffer();
        delimAppend(sb, m_firstName, ' ');            
        delimAppend(sb, m_lastName, ' ');
        
        return sb.length() == 0 ? null : sb.toString();
    }
    
    private void delimAppend(StringBuffer sb, String s, char delim) {
        if (s != null) {
            if (sb.length() != 0) {
                sb.append(delim);                
            }
            sb.append(s);
        }
    }

    public String getExtension() {
        return m_extension;
    }

    public void setExtension(String extension) {
        m_extension = extension;
    }

    public String getProfileEncryptionKey() {
        return m_profileEncryptionKey;
    }

    public void setProfileEncryptionKey(String profileEncryptionKey) {
        m_profileEncryptionKey = profileEncryptionKey;
    }

    public Organization getOrganization() {
        return m_organization;
    }

    public void setOrganization(Organization organization) {
        m_organization = organization;
    }

    public Object getPrimaryKey() {
        return getId();
    }
}
