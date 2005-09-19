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

import java.util.List;

import junit.framework.TestCase;

import org.apache.commons.lang.ObjectUtils;
import org.apache.commons.lang.StringUtils;
import org.sipfoundry.sipxconfig.TestHelper;
import org.sipfoundry.sipxconfig.admin.forwarding.AliasMapping;
import org.sipfoundry.sipxconfig.setting.Group;
import org.sipfoundry.sipxconfig.setting.Setting;

public class UserTest extends TestCase {
    private static final String USERNAME = "username";
    private static final String PIN = "pin";
    private static final String SIP_PASSWORD = "sip password";
    private static final String REALM = "sipfoundry.org";
    private static final String DOMAIN = "sipfoundry.org";
    private static final String ALIAS1 = "mambo";
    private static final String ALIAS2 = "tango";
    private static final String ALIAS3 = "hora";
    private static final String ALIASES_STRING = ALIAS1 + ", " + ALIAS2;
    
    public void testGetDisplayName() {
        User u = new User();
        assertNull(u.getDisplayName());
        u.setUserName("bob");
        assertNull(u.getDisplayName());
        u.setFirstName("First");
        assertEquals("First", u.getDisplayName());
        u.setLastName("Last");
        assertEquals("First Last", u.getDisplayName());
    }

    public void testGetUri() {
        User user = new User();
        user.setUserName(USERNAME);
        String uri = user.getUri("mycomp.com");

        assertEquals("sip:" + USERNAME + "@mycomp.com", uri);

        user.setLastName("Last");
        uri = user.getUri("mycomp.com");
        assertEquals("Last<sip:" + USERNAME + "@mycomp.com>", uri);

        user.setFirstName("First");
        uri = user.getUri("mycomp.com");
        assertEquals("First Last<sip:" + USERNAME + "@mycomp.com>", uri);
    }

    /** Test that setting a typical PIN yields expected results */
    public void testSetPin() throws Exception {
        checkSetPin(PIN);
    }

    /** Test that setting a null PIN yields expected results */
    public void testSetNullPin() throws Exception {
        checkSetPin(null);
    }
    
    private void checkSetPin(String pin) throws Exception {
        User user = new User();
        user.setUserName(USERNAME);
        user.setPin(pin, REALM);
        String pintoken = getPintoken(USERNAME, pin);
        assertEquals(pintoken, user.getPintoken());
    }

    public void testGetSipPasswordHash() throws Exception {
        User user = new User();
        user.setUserName(USERNAME);
        user.setSipPassword(SIP_PASSWORD);
        String hash = Md5Encoder.digestPassword(USERNAME, REALM, SIP_PASSWORD);

        assertEquals(hash, user.getSipPasswordHash(REALM));
    }

    public void testGetSipPasswordHashEmpty() throws Exception {
        User user = new User();
        user.setUserName(USERNAME);
        user.setSipPassword(null);
        String hash = Md5Encoder.digestPassword(USERNAME, REALM, "");

        assertEquals(hash, user.getSipPasswordHash(REALM));
    }

    public void testGetSipPasswordHashMd5() throws Exception {
        User user = new User();
        user.setUserName(USERNAME);
        String hash = Md5Encoder.digestPassword(USERNAME, REALM, "");
        user.setSipPassword(hash);

        String newHash = Md5Encoder.digestPassword(USERNAME, REALM, hash);

        assertFalse(hash.equals(newHash));
        assertEquals(newHash, user.getSipPasswordHash(REALM));
    }
    
    public void testGetAliases() {
        User user = createUserWithTwoAliases();
        assertEquals(ALIASES_STRING, user.getAliasesString());
        checkAliases(user, 2);
        
        user.setAliasesString(ALIASES_STRING);
        checkAliases(user, 2);
                
        List aliasMappings = user.getAliasMappings(DOMAIN);
        assertEquals(2, aliasMappings.size());
        AliasMapping alias = (AliasMapping) aliasMappings.get(0);
        assertEquals(ALIAS1 + "@" + DOMAIN, alias.getIdentity());
        final String CONTACT = "sip:" + USERNAME + "@" + DOMAIN;
        assertEquals(CONTACT, alias.getContact());
        alias = (AliasMapping) aliasMappings.get(1);
        assertEquals(ALIAS2 + "@" + DOMAIN, alias.getIdentity());
        assertEquals(CONTACT, alias.getContact());
    }
    
    private User createUserWithTwoAliases() {
        User user = new User();
        user.setUserName(USERNAME);        
        String[] aliases = new String[] {ALIAS1, ALIAS2};
        user.setAliases(aliases);
        return user;
    }
    
    public void testGetNames() {
        User user = createUserWithTwoAliases();
        String names[] = user.getNames();
        assertEquals(3, names.length);
        assertEquals(USERNAME, names[0]);
        assertEquals(ALIAS1, names[1]);
        assertEquals(ALIAS2, names[2]);        
    }
    
    private void checkAliases(User user, int numExpected) {
        String[] aliases = user.getAliases();
        assertEquals(numExpected, aliases.length);
        if (numExpected > 0) {
            assertEquals(ALIAS1, aliases[0]);
        }
        if (numExpected > 1) {
            assertEquals(ALIAS2, aliases[1]);
        }
        if (numExpected > 2) {
            assertEquals(ALIAS3, aliases[2]);
        }        
    }
    
    public void testGetEmptyAliases() {
        User user = new User();
        user.setUserName(USERNAME);
        List aliasMappings = user.getAliasMappings(DOMAIN);
        assertEquals(0, aliasMappings.size());
    }
    
    public void testAddAlias() {
        User user = createUserWithTwoAliases();
        user.addAlias(ALIAS3);
        String[] aliasesCheck = user.getAliases();
        assertEquals(3, aliasesCheck.length);
        assertEquals(ALIAS1, aliasesCheck[0]);
        assertEquals(ALIAS2, aliasesCheck[1]);        
        assertEquals(ALIAS3, aliasesCheck[2]);        
    }
    
    public void testAddAliases() {
        User user = new User();
        user.addAliases(new String[] {ALIAS1, ALIAS2, ALIAS3});
        checkAliases(user, 3);
    }

    public void testHasAlias() {
        User user = createUserWithTwoAliases();
        assertTrue(user.hasAlias(ALIAS1));
        assertTrue(user.hasAlias(ALIAS2));
        assertFalse(user.hasAlias(ALIAS3));
    }
    
    public void testHasPermission() {
        User user = new User();
        user.setSettingModel(TestHelper.loadSettings("user-settings.xml"));
        
        Group group = new Group();
        user.addGroup(group);
        
        String path = Permission.SUPERADMIN.getSettingPath();
        Setting superAdmin = user.getSettings().getSetting(path); 
        assertNotNull(superAdmin);
        assertFalse(user.hasPermission(Permission.SUPERADMIN));
    }

    
    private String getPintoken(String username, String pin) {
        pin = (String) ObjectUtils.defaultIfNull(pin, StringUtils.EMPTY);   // handle null pin
        return Md5Encoder.digestPassword(username, REALM, pin);
    }
}
