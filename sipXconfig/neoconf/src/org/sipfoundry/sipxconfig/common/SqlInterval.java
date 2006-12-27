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
package org.sipfoundry.sipxconfig.common;

import java.util.Calendar;

import org.postgresql.util.PGInterval;

/**
 *  To my surprise, postgres driver returns one of these object types in SQL results
 *  besides not being a standard SQL type, toString is not localized and in quite
 *  unpleasent format.
 */
public class SqlInterval implements Comparable {
    private static final Calendar ZERO = Calendar.getInstance();
    static {
        ZERO.setTimeInMillis(0);
    }

    private PGInterval m_pgInterval;

    public SqlInterval(PGInterval pgInterval) {
        m_pgInterval = pgInterval;
    }

    public long getMillisecs() {
        Calendar duration = (Calendar) ZERO.clone();
        m_pgInterval.add(duration);
        long millis = duration.getTimeInMillis();
        return millis;
    }

    public int compareTo(Object o) {
        if (o == null) {
            return -1;
        }

        long a = getMillisecs();
        long b = ((SqlInterval) o).getMillisecs();
        long cmp = a - b;
        return (cmp < 0 ? -1 : cmp > 0 ? 1 : 0);
    }

}
