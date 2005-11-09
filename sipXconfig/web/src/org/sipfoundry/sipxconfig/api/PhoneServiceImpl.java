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
package org.sipfoundry.sipxconfig.api;

import java.rmi.RemoteException;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;

import org.sipfoundry.sipxconfig.phone.PhoneContext;
import org.sipfoundry.sipxconfig.phone.PhoneModel;
import org.sipfoundry.sipxconfig.setting.Group;
import org.sipfoundry.sipxconfig.setting.SettingDao;

public class PhoneServiceImpl implements PhoneService {
    
    private PhoneContext m_context;
    
    private SettingDao m_settingDao;
    
    private PhoneBuilder m_builder;

    public void setPhoneContext(PhoneContext context) {
        m_context = context;
    }
    
    public void setPhoneBuilder(PhoneBuilder builder) {
        m_builder = builder;
    }

    public void addPhone(AddPhone addPhone) throws RemoteException {        
        Phone apiPhone = addPhone.getPhone(); 
        PhoneModel model = requireModelId(apiPhone.getModelId());
        org.sipfoundry.sipxconfig.phone.Phone phone = m_context.newPhone(model);
        m_builder.fromApi(apiPhone, phone);
        String[] groups = addPhone.getGroup();
        String resourceId = org.sipfoundry.sipxconfig.common.User.GROUP_RESOURCE_ID;
        for (int i = 0; groups != null && i < groups.length; i++) {
            Group g = m_settingDao.getGroupCreateIfNotFound(resourceId, groups[i]);
            phone.addGroup(g);
        }
        m_context.storePhone(phone);
    }

    public FindPhoneResponse findPhone(FindPhone findPhone) throws RemoteException {
        FindPhoneResponse response = new FindPhoneResponse();        
        org.sipfoundry.sipxconfig.phone.Phone[] otherPhones = phoneSearch(findPhone.getSearch());
        Phone[] arrayOfPhones = (Phone[]) ApiBeanUtil.toApiArray(m_builder, otherPhones, Phone.class);
        response.setPhones(arrayOfPhones);
        
        return response;
    }
    
    org.sipfoundry.sipxconfig.phone.Phone[] phoneSearch(PhoneSearch search) {
        Collection phones = Collections.EMPTY_LIST;
        if (search == null) {
            phones = m_context.loadPhones();
        } else if (search.getBySerialNumber() != null) {
            Integer id = m_context.getPhoneIdBySerialNumber(search.getBySerialNumber());
            if (id != null) {
                org.sipfoundry.sipxconfig.phone.Phone phone = m_context.loadPhone(id);
                if (phone != null) {
                    phones = Collections.singleton(phone);
                }
            }
        } else if (search.getByGroup() != null) {
            String resourceId = org.sipfoundry.sipxconfig.phone.Phone.GROUP_RESOURCE_ID;
            Group g = m_settingDao.getGroupByName(resourceId, search.getByGroup());
            if (g != null) {
                phones = m_context.getPhonesByGroupId(g.getId());
            }
        } else if (search.getById() != null) {
            org.sipfoundry.sipxconfig.phone.Phone phone = m_context.loadPhone(search.getById());
            if (phone != null) {
                phones = Collections.singleton(phone);
            }
        }
        
        return (org.sipfoundry.sipxconfig.phone.Phone[])
            phones.toArray(new org.sipfoundry.sipxconfig.phone.Phone[phones.size()]);
    }

    public void editPhone(EditPhone editPhone) throws RemoteException {
        org.sipfoundry.sipxconfig.phone.Phone[] otherPhones = phoneSearch(editPhone.getSearch());
        for (int i = 0; i < otherPhones.length; i++) {
            ApiBeanUtil.setProperties(otherPhones[i], editPhone.getProperties());
            // TODO: lines and groups
            m_context.storePhone(otherPhones[i]);
        }
    }

    public PhoneModel requireModelId(String modelId) {
        PhoneModel model = PhoneModel.getModel(modelId);
        if (model == null) {
            throw new IllegalArgumentException("phone model doesn't exist: " + modelId);
        }
        return model;
    }

    public void setSettingDao(SettingDao settingDao) {
        m_settingDao = settingDao;
    }

    public void managePhone(ManagePhone managePhone) throws RemoteException {
        org.sipfoundry.sipxconfig.phone.Phone[] otherPhones = phoneSearch(managePhone.getSearch());
        if (Boolean.TRUE.equals(managePhone.getGenerateProfiles())) {
            m_context.generateProfilesAndRestart(Arrays.asList(otherPhones));            
        } else if (Boolean.TRUE.equals(managePhone.getRestart())) {
            m_context.restart(Arrays.asList(otherPhones));            
        } else {
            for (int i = 0; i < otherPhones.length; i++) {
                if (Boolean.TRUE.equals(managePhone.getDoDelete())) {
                    m_context.deletePhone(otherPhones[i]);
                }
            }
        }
    }
}
