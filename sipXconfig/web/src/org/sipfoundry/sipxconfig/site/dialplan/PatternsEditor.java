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
package org.sipfoundry.sipxconfig.site.dialplan;

import java.util.List;

import org.apache.tapestry.BaseComponent;
import org.apache.tapestry.IMarkupWriter;
import org.apache.tapestry.IRequestCycle;
import org.sipfoundry.sipxconfig.admin.dialplan.DialPattern;
import org.sipfoundry.sipxconfig.components.TapestryUtils;

/**
 * PatternsEditor - list of pattersn with ability to edit, remove and delete
 */
public abstract class PatternsEditor extends BaseComponent {
    public abstract boolean getAddPattern();

    public abstract void setAddPattern(boolean addPattern);

    public abstract int getIndexToRemove();

    public abstract void setIndexToRemove(int index);

    public abstract List getPatterns();

    public abstract int getIndex();

    public abstract int getSize();

    public abstract void setSize(int size);

    public boolean isLast() {
        List patterns = getPatterns();
        return getIndex() == patterns.size() - 1;
    }

    static final void setCollectionSize(List patterns, int newSize) {
        while (newSize < patterns.size()) {
            patterns.remove(patterns.size() - 1);
        }
        while (newSize > patterns.size()) {
            patterns.add(new DialPattern());
        }
    }

    public void sizeChanged() {
        List patterns = getPatterns();
        setCollectionSize(patterns, getSize());
    }

    /**
     * Process pattern adds/deletes.
     */
    protected void renderComponent(IMarkupWriter writer, IRequestCycle cycle) {
        List patterns = getPatterns();
        if (TapestryUtils.isRewinding(cycle, this)) {
            // reset components before rewind
            setIndexToRemove(-1);
            setAddPattern(false);
        } else {
            setSize(patterns.size());
        }
        super.renderComponent(writer, cycle);
        if (TapestryUtils.isRewinding(cycle, this) && TapestryUtils.isValid(cycle, this)) {
            if (getAddPattern()) {
                patterns.add(new DialPattern());
            }
            int indexToRemove = getIndexToRemove();
            if (indexToRemove >= 0) {
                patterns.remove(indexToRemove);
            }
        }
    }
}
