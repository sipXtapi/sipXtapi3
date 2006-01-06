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
package org.sipfoundry.sipxconfig.common;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

/**
 * Daemon thread that sleeps most of the time in waitForWork method. When there is something to be
 * do it wakes up, sleeps some more and then does it.
 */
public abstract class LazyDaemon extends Thread {
    public static final Log LOG = LogFactory.getLog(LazyDaemon.class);
    private int m_sleepInterval;
    private volatile long m_workAddedTimestamp;

    public LazyDaemon(String name, int sleepInterval) {
        super(name);
        m_sleepInterval = sleepInterval;
        setPriority(Thread.MIN_PRIORITY);
        setDaemon(true);
    }

    public final void run() {
        try {
            boolean moreWork = true;
            while (moreWork) {
                waitForWork();
                long quietPeriod = 0;
                do {
                    sleep(m_sleepInterval);
                    quietPeriod = System.currentTimeMillis() - m_workAddedTimestamp;                    
                } while(quietPeriod < m_sleepInterval);                
                try {
                    moreWork = work();
                } catch (Throwable e) {
                    LOG.error(getName() + "exception in background task.", e);
                }
            }
        } catch (InterruptedException e) {
            LOG.error(getName() + "exiting due to exception.", e);
        }
    }
    
    public void workScheduled() {
        m_workAddedTimestamp = System.currentTimeMillis();        
    }

    /**
     * Overwrite to call wait function to suspend this thread until there is something to be done.
     * It's pefectly valid not to wait for anything - in that case this thread will still sleep
     * for m_sleepInterval before calling work
     * 
     * @throws InterruptedException
     */
    protected abstract void waitForWork() throws InterruptedException;

    /**
     * Overwrite to do something with low priority that needs to be done regularly (house cleaning
     * tasks).
     * 
     * @return moreWork - true if thread should keep on calling work, false to finish thread
     */
    protected abstract boolean work();
}
