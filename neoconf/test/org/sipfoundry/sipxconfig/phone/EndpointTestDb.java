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

import junit.framework.TestCase;

import org.dbunit.Assertion;
import org.dbunit.dataset.IDataSet;
import org.dbunit.dataset.ITable;
import org.dbunit.dataset.ReplacementDataSet;
import org.sipfoundry.sipxconfig.TestHelper;
import org.sipfoundry.sipxconfig.vendor.Polycom;
import org.springframework.orm.hibernate.HibernateObjectRetrievalFailureException;



/**
 * You need to call 'ant reset-db-patch' which clears a lot of data in your
 * database. before calling running this test. 
 */
public class EndpointTestDb extends TestCase {
    
    private PhoneContext m_context;
    
    private Class m_class = EndpointTestDb.class;
        
    public void setUp() {
        m_context = (PhoneContext) TestHelper.getApplicationContext().getBean(
                PhoneContext.CONTEXT_BEAN_NAME);        
    }
    
    public void testSave() throws Exception {        
        TestHelper.cleanInsert(m_class, "ClearDb.xml");
        
        Endpoint e = new Endpoint();
        e.setPhoneId(Polycom.MODEL_300.getModelId());
        e.setSerialNumber("999123456");
        e.setName("unittest-sample phone1");
        m_context.storeEndpoint(e);
        
        ITable actual = TestHelper.getConnection().createDataSet().getTable("endpoint");

        IDataSet expectedDs = TestHelper.loadDataSetFlat(m_class, "SaveEndpointExpected.xml"); 
        ReplacementDataSet expectedRds = new ReplacementDataSet(expectedDs);
        expectedRds.addReplacementObject("[endpoint_id_1]", new Integer(e.getId()));
        expectedRds.addReplacementObject("[null]", null);
        
        ITable expected = expectedRds.getTable("endpoint");
                
        Assertion.assertEquals(expected, actual);
    }
    
    public void testLoadAndDelete() throws Exception {
        TestHelper.cleanInsertFlat(m_class, "EndpointSeed.xml");
        
        Endpoint e = m_context.loadEndpoint(1);
        assertEquals("999123456", e.getSerialNumber());
        
        int id = e.getId();        
        m_context.deleteEndpoint(e);
        try {
            m_context.loadEndpoint(id);
            fail();
        } catch (HibernateObjectRetrievalFailureException x) {
            assertTrue(true);
        }
    }

}
