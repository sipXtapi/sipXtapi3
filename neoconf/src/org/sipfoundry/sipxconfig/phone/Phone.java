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
package org.sipfoundry.sipxconfig.phone;

import java.util.Collection;

import org.sipfoundry.sipxconfig.setting.Setting;

/**
 * Implement this to add support for new devices to the system
 */
public interface Phone {
    
    public void setPhoneMetaData(PhoneMetaData meta);
    
    public PhoneMetaData getPhoneMetaData();
    
    public String getDisplayLabel();
    
    public Setting getSettingModel();

    public Setting getSettings();

    public void generateProfiles();
    
    public void restart();
        
    public Collection getLines();
    
    public void addLine(Line line);
    
    public Line getLine(int position);
    
    public Collection getDeletedLines();
    
    /** you must explicitly call addLine */
    public Line createLine(LineMetaData lineMeta);

    /** implementation need to declare they implement PrimaryKeySource */
    public Object getPrimaryKey();
}
