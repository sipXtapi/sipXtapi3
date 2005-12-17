package org.sipfoundry.sipxconfig;

import java.io.FileWriter;
import java.sql.SQLException;

import junit.framework.TestCase;

import org.dbunit.database.IDatabaseConnection;
import org.dbunit.dataset.IDataSet;
import org.dbunit.dataset.xml.FlatXmlWriter;
import org.springframework.dao.DataIntegrityViolationException;

/**
 * Special TestCase class that catches and prints additional info for SQL exceptions that may
 * happen during setUp, testXXX and tearDown.
 * 
 * Alternatively we could just throw e.getNextException, but we may want to preserve the original
 * exception.
 */
public class SipxDatabaseTestCase extends TestCase {
    public void runBare() throws Throwable {
        try {
            super.runBare();
        } catch (SQLException e) {
            dumpSqlExceptionMessages(e);
            throw e;
        } catch (DataIntegrityViolationException e) {
            if (e.getCause() instanceof SQLException) {
                dumpSqlExceptionMessages((SQLException) e.getCause());
            }
            throw e;
        }
    }

    private void dumpSqlExceptionMessages(SQLException e) {
        for (SQLException next = e; next != null; next = next.getNextException()) {
            System.err.println(next.getMessage());
        }
    }

    protected IDatabaseConnection getConnection() throws Exception {
        return TestHelper.getConnection();
    }

    protected void tearDown() throws Exception {
        super.tearDown();

        TestHelper.closeConnection();
    }

    protected void writeFlatXmlDataSet(FileWriter fileWriter) throws Exception {
        IDataSet set = getConnection().createDataSet();
        FlatXmlWriter writer = new FlatXmlWriter(fileWriter);
        writer.write(set);
    }
}
