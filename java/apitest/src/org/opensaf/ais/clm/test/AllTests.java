/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2008 The OpenSAF Foundation
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. This file and program are licensed
 * under the GNU Lesser General Public License Version 2.1, February 1999.
 * The complete license can be accessed from the following location:
 * http://opensource.org/licenses/lgpl-license.php
 * See the Copying file included with the OpenSAF distribution for full
 * licensing terms.
 *
 * Author(s): Nokia Siemens Networks, OptXware Research & Development LLC.
 */

package org.opensaf.ais.clm.test;

import org.opensaf.ais.test.DispatchHandleTests;

import junit.framework.Test;
import junit.framework.TestSuite;

// CLASS DEFINITION
public class AllTests {

	public static void main(String[] args) {
		junit.textui.TestRunner.run(AllTests.suite());
	}

	public static Test suite() {
		TestSuite suite = new TestSuite("All tests for ais.clm.test");
		// $JUnit-BEGIN$
		suite.addTestSuite(TestClmLibraryHandle.class);
		suite.addTestSuite(TestClusterMembershipManager.class);
		suite.addTestSuite(TestClmMultiThreading.class);
		suite.addTestSuite(DispatchHandleTests.class);
		// $JUnit-END$
		return suite;
	}

}
