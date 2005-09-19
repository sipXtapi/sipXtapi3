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

import java.util.Collection;
import java.util.Set;

import junit.framework.TestCase;

import org.dbunit.Assertion;
import org.dbunit.dataset.IDataSet;
import org.dbunit.dataset.ITable;
import org.dbunit.dataset.ReplacementDataSet;
import org.sipfoundry.sipxconfig.TestHelper;
import org.sipfoundry.sipxconfig.setting.Group;
import org.sipfoundry.sipxconfig.setting.Setting;
import org.sipfoundry.sipxconfig.setting.SettingDao;
import org.springframework.context.ApplicationContext;

public class UserTestDb extends TestCase {

    private CoreContext core;
    
    private SettingDao settingDao;
    
    private Integer userId = new Integer(1000);

    protected void setUp() throws Exception {
        ApplicationContext app = TestHelper.getApplicationContext(); 
        core = (CoreContext) app.getBean(CoreContext.CONTEXT_BEAN_NAME);        
        settingDao = (SettingDao) app.getBean(SettingDao.CONTEXT_NAME);
    }

    public void testLoadUser() throws Exception {
        TestHelper.cleanInsert("ClearDb.xml");
        TestHelper.insertFlat("common/TestUserSeed.xml");
        
        User user = core.loadUser(userId);
        assertEquals(userId, user.getPrimaryKey());
        assertEquals(userId, user.getId());
    }
    
    public void testSave() throws Exception {
        TestHelper.cleanInsert("ClearDb.xml");
        User user = new User();
        user.setUserName("userid");
        user.setFirstName("FirstName");
        user.setLastName("LastName");
        user.setPintoken("password");
        user.setSipPassword("sippassword");
        user.addAlias("1234");
        core.saveUser(user);

        IDataSet expectedDs = TestHelper.loadDataSetFlat("common/SaveUserExpected.xml");
        ReplacementDataSet expectedRds = new ReplacementDataSet(expectedDs);
        expectedRds.addReplacementObject("[user_id]", user.getId());
        expectedRds.addReplacementObject("[user_name_id]", user.getUserNameObject().getId());
        expectedRds.addReplacementObject("[null]", null);

        ITable expected = expectedRds.getTable("users");
        ITable actual = TestHelper.getConnection().createQueryTable("users",
                "select u.* from users u inner join user_name un using (user_name_id) where un.name = 'userid'");

        Assertion.assertEquals(expected, actual);
    }
    
    public void testUserGroups() throws Exception {
        TestHelper.cleanInsert("ClearDb.xml");
        TestHelper.insertFlat("common/UserGroupSeed.xml");
        User user = core.loadUser(new Integer(1001));
        Set groups = user.getGroups();
        assertEquals(1, groups.size());
    }
    
    public void testUserSettings() throws Exception {
        TestHelper.cleanInsert("ClearDb.xml");
        TestHelper.insertFlat("common/UserGroupSeed.xml");
        User user = core.loadUser(new Integer(1001));
        Setting settings = user.getSettings();        
        assertNotNull(settings);
    }
    
    public void testGroupMembers() throws Exception {
        TestHelper.cleanInsert("ClearDb.xml");
        TestHelper.insertFlat("common/UserGroupSeed.xml");
        Group group = settingDao.getGroup(new Integer(1001));
        Collection users = core.getGroupMembers(group);
        assertEquals(1, users.size());
        User actualUser = core.loadUser(new Integer(1001));
        User expectedUser = (User) users.iterator().next();
        assertEquals(actualUser.getDisplayName(), expectedUser.getDisplayName());
    }
    
    public void testDeleteUserGroups() throws Exception {
        TestHelper.cleanInsert("ClearDb.xml");
        TestHelper.insertFlat("common/UserGroupSeed.xml");
        Group group = settingDao.getGroup(new Integer(1001));
        settingDao.deleteGroup(group);
        // link table references removed
        ITable actual = TestHelper.getConnection().createDataSet().getTable("user_group");
        assertEquals(0, actual.getRowCount());
    }
}
