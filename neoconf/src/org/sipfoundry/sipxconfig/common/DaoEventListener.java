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

import org.hibernate.type.Type;

/**
 * Any beans that implement this interface will be called when entities are saved
 * or deleted.  Dao's must declare DaoEventDispatcher (or any of it's subclasses)
 * as it's Spring-Hibernate interceptor for events to be sent
 */
public interface DaoEventListener {

    /**
     * Is called before the actual entity is deleted
     */
    public void onDelete(Object entity, Serializable id, Object[] state, String[] propertyNames,
            Type[] types);

    /**
     * Is called before the actual entity is saved
     */
    public boolean onSave(Object entity, Serializable id, Object[] state, String[] propertyNames,
            Type[] types);
}
