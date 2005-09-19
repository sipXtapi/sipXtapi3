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

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

import junit.framework.TestCase;

import org.dbunit.Assertion;
import org.dbunit.dataset.IDataSet;
import org.dbunit.dataset.ITable;
import org.dbunit.dataset.ReplacementDataSet;
import org.sipfoundry.sipxconfig.TestHelper;
import org.sipfoundry.sipxconfig.setting.Group;
import org.sipfoundry.sipxconfig.setting.Setting;
import org.springframework.context.ApplicationContext;
import org.springframework.orm.hibernate3.HibernateTemplate;
import org.springframework.orm.hibernate3.support.HibernateDaoSupport;

public class CoreContextImplTestDb extends TestCase {

    private static final int NUM_USERS = 10;
    private CoreContextImpl m_core;

    protected void setUp() throws Exception {
        ApplicationContext app = TestHelper.getApplicationContext();
        m_core = (CoreContextImpl) app.getBean(CoreContextImpl.CONTEXT_BEAN_NAME);
        TestHelper.cleanInsert("ClearDb.xml");
    }

    public void testLoadByUserName() throws Exception {
        TestHelper.cleanInsertFlat("common/UserSearchSeed.xml");

        assertNotNull(m_core.loadUserByUserName("userseed5"));
        assertNull(m_core.loadUserByUserName("wont find this guy"));
    }

    public void testLoadByAlias() throws Exception {
        TestHelper.cleanInsertFlat("common/UserSearchSeed.xml");

        User user = m_core.loadUserByAlias("2");
        assertNotNull(user);
        assertEquals("userseed2", user.getUserName());
        user = m_core.loadUserByAlias("two");
        assertNotNull(user);
        assertEquals("userseed2", user.getUserName());
        user = m_core.loadUserByAlias("5");
        assertNotNull(user);
        assertEquals("userseed5", user.getUserName());
        assertNull(m_core.loadUserByAlias("666"));
        assertNull(m_core.loadUserByAlias("userseed2"));
    }

    public void testRemoveAlias() throws Exception {
        TestHelper.cleanInsert("ClearDb.xml");
        TestHelper.cleanInsertFlat("common/UserSearchSeed.xml");
        
        HibernateTemplate hibernate = ((HibernateDaoSupport) m_core).getHibernateTemplate();
        List names = hibernate.loadAll(UserName.class);
        int numAliases = names.size();
        
        // Remove one alias => check that the alias got cleaned out of the names table,
        // not just removed from the User and orphaned in the names table
        User user = m_core.loadUserByAlias("2");
        user.removeAlias("2");
        hibernate.saveOrUpdate(user);
        names = hibernate.loadAll(UserName.class);
        assertEquals(1, numAliases - names.size());
    }

    public void testLoadUserByUserNameOrAlias() throws Exception {
        TestHelper.cleanInsertFlat("common/UserSearchSeed.xml");

        User user = m_core.loadUserByUserNameOrAlias("2");
        assertNotNull(user);
        assertEquals("userseed2", user.getUserName());

        user = m_core.loadUserByUserNameOrAlias("two");
        assertNotNull(user);
        assertEquals("userseed2", user.getUserName());

        user = m_core.loadUserByUserNameOrAlias("5");
        assertNotNull(user);
        assertEquals("userseed5", user.getUserName());

        assertNull(m_core.loadUserByUserNameOrAlias("666"));

        user = m_core.loadUserByUserNameOrAlias("userseed2");
        assertNotNull(user);
        assertEquals("userseed2", user.getUserName());

        // There was a bug earlier where only users who had aliases could be loaded.
        // Test that a user without aliases can be loaded.
        user = m_core.loadUserByUserNameOrAlias("userwithnoaliases");
        assertNotNull(user);
        assertEquals("userwithnoaliases", user.getUserName());
    }

    public void testCheckForDuplicateNameOrAlias() throws Exception {
        TestHelper.cleanInsertFlat("common/UserSearchSeed.xml");
        final String UNIQUE_NAME = "uniqueNameThatDoesntExistInTheUniverseOrBeyond";
        
        // Check that a user with a unique name won't collide with any existing users
        User user = new User();
        user.setUserName(UNIQUE_NAME);
        assertNull(m_core.checkForDuplicateNameOrAlias(user));
        
        // Check that a user with a duplicate name is found to be a duplicate
        user.setUserName("userseed4");
        assertEquals("userseed4", m_core.checkForDuplicateNameOrAlias(user));
        
        // Check that a user with an alias that matches an existing username
        // is found to be a duplicate
        user.setUserName(UNIQUE_NAME);
        user.setAliasesString("userseed6");
        assertEquals("userseed6", m_core.checkForDuplicateNameOrAlias(user));
        
        // Check that a user with an alias that matches an existing alias
        // is found to be a duplicate
        user.setAliasesString("two");
        assertEquals("two", m_core.checkForDuplicateNameOrAlias(user));
        
        // Check that duplicate checking doesn't get confused by multiple collisions
        // with both usernames and aliases
        user.setUserName("userseed4");
        user.setAliasesString("two,1");
        assertNotNull(m_core.checkForDuplicateNameOrAlias(user));
        
        // Check that collisions internal to the user are caught.
        // Note that duplicates within the aliases list are ignored.
        final String WASAWUSU = "wasawusu";
        user.setUserName(WASAWUSU);
        user.setAliasesString(WASAWUSU);
        assertEquals(WASAWUSU, m_core.checkForDuplicateNameOrAlias(user));
        user.setUserName(UNIQUE_NAME);
        user.setAliasesString(WASAWUSU + "," + WASAWUSU);
        assertNull(m_core.checkForDuplicateNameOrAlias(user));        
    }
    
    public void testSearchByUserName() throws Exception {
        TestHelper.cleanInsertFlat("common/UserSearchSeed.xml");

        User template = new User();
        template.setUserName("userseed");
        List users = m_core.loadUserByTemplateUser(template);

        assertEquals(6, users.size());
    }

    public void testSearchByAlias() throws Exception {
        TestHelper.cleanInsertFlat("common/UserSearchSeed.xml");

        User template = new User();
        template.setUserName("two");
        List users = m_core.loadUserByTemplateUser(template);
        assertEquals(1, users.size());
        User user = (User) users.get(0);
        assertEquals("userseed2", user.getUserName());
    }

    public void testSearchByAliasNotAllowed() throws Exception {
        TestHelper.cleanInsertFlat("common/UserSearchSeed.xml");

        User template = new User();
        template.setUserName("two");

        // Pass in false to turn off matching on aliases, so search should return nothing
        List users = m_core.loadUserByTemplateUser(template, false);
        assertEquals(0, users.size());
    }

    public void testSearchByFirstName() throws Exception {
        TestHelper.cleanInsertFlat("common/UserSearchSeed.xml");

        User template = new User();
        template.setFirstName("user4");
        List users = m_core.loadUserByTemplateUser(template);
        assertEquals(1, users.size());
        User user = (User) users.get(0);
        assertEquals("userseed4", user.getUserName());
    }

    public void testSearchByLastName() throws Exception {
        TestHelper.cleanInsertFlat("common/UserSearchSeed.xml");

        User template = new User();
        template.setLastName("seed5");
        List users = m_core.loadUserByTemplateUser(template);
        assertEquals(1, users.size());
        User user = (User) users.get(0);
        assertEquals("userseed5", user.getUserName());
    }

    // Test that we are doing a logical "AND" not "OR". There is a user with
    // the specified first name, and a different user with the specified last
    // name. Because of the "AND" we should get no matches.
    public void testSearchByFirstAndLastName() throws Exception {
        TestHelper.cleanInsertFlat("common/UserSearchSeed.xml");

        User template = new User();
        template.setFirstName("user4");
        template.setLastName("seed5");
        List users = m_core.loadUserByTemplateUser(template);
        assertEquals(0, users.size());
    }

    public void testSearchFormBlank() throws Exception {
        TestHelper.cleanInsertFlat("common/UserSearchSeed.xml");

        User template = new User();
        template.setFirstName("");
        List users = m_core.loadUserByTemplateUser(template);

        assertEquals(NUM_USERS, users.size());
    }

    public void testLoadUsers() throws Exception {
        TestHelper.cleanInsertFlat("common/UserSearchSeed.xml");

        List users = m_core.loadUsers();

        // Check that we have the expected number of users
        assertEquals(NUM_USERS, users.size());
    }

    public void testDeleteUsers() throws Exception {
        TestHelper.cleanInsertFlat("common/UserSearchSeed.xml");

        // Check that we have the expected number of users
        ITable usersTable = TestHelper.getConnection().createDataSet().getTable("users");
        assertEquals(NUM_USERS, usersTable.getRowCount());

        // Delete two users
        List usersToDelete = new ArrayList();
        usersToDelete.add(new Integer(1001));
        usersToDelete.add(new Integer(1002));
        m_core.deleteUsers(usersToDelete);

        // We should have reduced the user count by two
        usersTable = TestHelper.getConnection().createDataSet().getTable("users");
        assertEquals(NUM_USERS - 2, usersTable.getRowCount());
    }

    public void testLoadGroups() throws Exception {
        TestHelper.cleanInsert("ClearDb.xml");
        TestHelper.insertFlat("common/UserGroupSeed.xml");
        List groups = m_core.getUserGroups();
        assertEquals(1, groups.size());
        Group group = (Group) groups.get(0);
        assertEquals("SeedUserGroup1", group.getName());
    }

    public void testGetUserSettingModel() {
        Setting model = m_core.getUserSettingsModel();
        assertNotNull(model.getSetting("permission"));
    }

    public void testAliases() throws Exception {
        Collection userAliases = m_core.getAliasMappings();
        assertEquals(0, userAliases.size());

        TestHelper.cleanInsertFlat("common/TestUserSeed.xml");

        userAliases = m_core.getAliasMappings();
        assertEquals(1, userAliases.size());
    }

    public void testClear() throws Exception {
        TestHelper.cleanInsertFlat("common/TestUserSeed.xml");
        m_core.clear();
        ITable t = TestHelper.getConnection().createDataSet().getTable("users");
        assertEquals(0, t.getRowCount());
    }

    public void testCreateAdminGroupAndInitialUserTask() throws Exception {
        TestHelper.cleanInsert("ClearDb.xml");
        m_core.createAdminGroupAndInitialUserTask();

        User admin = m_core.loadUserByUserName("superadmin");
        Group adminGroup = (Group) admin.getGroups().iterator().next();
        IDataSet expectedDs = TestHelper
                .loadDataSetFlat("common/CreateAdminAndInitialUserExpected.xml");
        ReplacementDataSet expectedRds = new ReplacementDataSet(expectedDs);
        expectedRds.addReplacementObject("[user_id]", admin.getId());
        expectedRds.addReplacementObject("[user_name_id]", admin.getUserNameObject().getId());
        expectedRds.addReplacementObject("[group_id]", adminGroup.getId());
        expectedRds.addReplacementObject("[weight]", adminGroup.getWeight());
        expectedRds.addReplacementObject("[null]", null);

        IDataSet actualDs = TestHelper.getConnection().createDataSet();

        Assertion.assertEquals(expectedRds.getTable("users"), actualDs.getTable("users"));
        Assertion.assertEquals(expectedRds.getTable("group_storage"), actualDs
                .getTable("group_storage"));
        Assertion.assertEquals(expectedRds.getTable("setting_value"), actualDs
                .getTable("setting_value"));
    }
    
    public void testCheckForDuplicateString() {
        // An empty collection should have no duplicates
        List strings = new ArrayList();
        assertNull(m_core.checkForDuplicateString(strings));
        
        // Still no duplicates
        strings.add(new String("a"));
        strings.add(new String("b"));
        strings.add(new String("c"));
        assertNull(m_core.checkForDuplicateString(strings));
        
        // Now we have a duplicate
        strings.add(new String("b"));
        assertEquals("b", m_core.checkForDuplicateString(strings));
        
        // Try multiple duplicates (result is indeterminate but non-null)
        strings.add(new String("c"));
        assertNotNull(m_core.checkForDuplicateString(strings));        
    }
    
    public void testCountUsers() throws Exception {
        TestHelper.cleanInsertFlat("common/UserSearchSeed.xml");
        assertEquals(10, m_core.getUserCount());
    }
    
    public void testLoadUserPage() throws Exception {
        TestHelper.cleanInsertFlat("common/SampleUsersSeed.xml");
        Collection page = m_core.loadUsersByPage(0, 2, User.USER_NAME_PROPERTY, true);
        assertEquals(2, page.size());
        User u = (User) page.iterator().next();
        assertEquals("alpha", u.getUserName());
        
        Collection next = m_core.loadUsersByPage(2, 2, User.USER_NAME_PROPERTY, true);
        assertEquals(2, next.size());
        User nextUser = (User) next.iterator().next();
        assertEquals("charlie", nextUser.getUserName());
        
        page = m_core.loadUsersByPage(0, 2, User.LAST_NAME_PROPERTY, true);
        assertEquals(2, page.size());
        nextUser = (User) page.iterator().next();
        assertEquals("elephant", nextUser.getUserName());
        
        next = m_core.loadUsersByPage(2, 2, User.FIRST_NAME_PROPERTY, false);
        assertEquals(2, next.size());
        nextUser = (User) next.iterator().next();
        assertEquals("gogo", nextUser.getUserName());
    }

    public void testLoadUserPageDescending() throws Exception {
        TestHelper.cleanInsertFlat("common/SampleUsersSeed.xml");
        // expect third user from bottom
        Collection page = m_core.loadUsersByPage(2, 2, User.USER_NAME_PROPERTY, false);
        User u = (User) page.iterator().next();
        assertEquals("horatio", u.getUserName());
    }
}
