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
package org.sipfoundry.sipxconfig.bulk.csv;

import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.List;

import junit.framework.TestCase;

public class CsvParserImplTest extends TestCase {
    public static final String[] NAMES = {
        "John", "Ringo", "Conference room", "John", "George"
    };

    public static final String[] DESCRIPTIONS = {
        "Room", "Green", "", "", ""
    };
    
    
    public void testParseLineEmpty() {
        CsvParserImpl parser = new CsvParserImpl();
        String[] row = parser.parseLine("");
        assertEquals(0, row.length);        
    }
    
    public void testParseLineNoQuotes() {
        CsvParserImpl parser = new CsvParserImpl();
        String[] row = parser.parseLine("a,bbb,c");
        assertEquals(3, row.length);
        assertEquals("a", row[0]);
        assertEquals("bbb", row[1]);
        assertEquals("c", row[2]);
    }
    
    public void testParseLineStartWithEmpty() {
        CsvParserImpl parser = new CsvParserImpl();
        String[] row = parser.parseLine(",a,bbb,ccc c");
        assertEquals(4, row.length);
        assertEquals("", row[0]);
        assertEquals("a", row[1]);
        assertEquals("bbb", row[2]);
        assertEquals("ccc c", row[3]);
    }
    
    public void testParseLineEndWithEmpty() {
        CsvParserImpl parser = new CsvParserImpl();
        String[] row = parser.parseLine("a,bbb,ccc c,");
        assertEquals(4, row.length);
        assertEquals("a", row[0]);
        assertEquals("bbb", row[1]);
        assertEquals("ccc c", row[2]);
        assertEquals("", row[3]);
    }
    
    public void testParseLineQuotes() {
        CsvParserImpl parser = new CsvParserImpl();
        String[] row = parser.parseLine("a,\"bbb\",c,\"\"");
        assertEquals(4, row.length);
        assertEquals("a", row[0]);
        assertEquals("bbb", row[1]);
        assertEquals("c", row[2]);
        assertEquals("", row[3]);
    }
    
    public void testParseLineQuotedFieldSeparator() {
        CsvParserImpl parser = new CsvParserImpl();
        String[] row = parser.parseLine("a,\"bb,b\",c");
        assertEquals(3, row.length);
        assertEquals("a", row[0]);
        assertEquals("bb,b", row[1]);
        assertEquals("c", row[2]);
    }
    
    public void testParseLineQuotedQuote() {
        CsvParserImpl parser = new CsvParserImpl();
        String[] row = parser.parseLine("a,\"b\"bb\",c");
        assertEquals(3, row.length);
        assertEquals("a", row[0]);
        assertEquals("b\"bb", row[1]);
        assertEquals("c", row[2]);
    }

    public void testParse() {
        assertEquals(NAMES.length, DESCRIPTIONS.length);
        
        CsvParser parser = new CsvParserImpl();
        InputStream cutsheet = getClass().getResourceAsStream("cutsheet.csv");
        List rows = parser.parse(new InputStreamReader(cutsheet));
        // there are 6 rows - we expect that the header row is always skipped
        assertEquals(NAMES.length, rows.size());
        for (int i = 0; i < NAMES.length; i++) {
            Object item = rows.get(i);
            assertTrue("row has to be a String array", item instanceof String[]);
            String[] row = (String[]) item;
            assertEquals(11, row.length);
            assertEquals(DESCRIPTIONS[i], row[10]);
            assertEquals(NAMES[i], row[3]);
        }
    }
}
