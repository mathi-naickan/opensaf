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

import org.saforum.ais.AisStatus;
import org.saforum.ais.clm.ClusterNode;
import org.saforum.ais.clm.GetClusterNodeCallback;

import junit.framework.Assert;

// CLASS DEFINITION
/**
 *
 * @author Nokia Siemens Networks
 * @author Jozsef BIRO
 */
public class GetClusterNodeCallbackForTest implements GetClusterNodeCallback {

	// INSTANCE FIELDS

	private boolean called;

	Thread cbThread;

	/**
	 * The value of the error parameter passed to getClusterNodeCallback.
	 */
	private AisStatus error;

	/**
	 * The value of the invocation parameter passed to getClusterNodeCallback.
	 */
	private long invocation;

	/**
	 * The value of the clusterNode parameter passed to getClusterNodeCallback.
	 *
	 */
	private ClusterNode clusterNode;

	// INSTANCE METHODS

	public void getClusterNodeCallback(long invocation,
			ClusterNode clusterNode, AisStatus error) {
		System.out.println("JAVA TEST CALLBACK: Executing getClusterNodeCallback( " + invocation + ", "
						+ clusterNode + ", " + error + " ) on " + this);
		called = true;
		cbThread = Thread.currentThread();
		this.invocation = invocation;
		this.clusterNode = clusterNode;
		this.error = error;
	}

	void assertCalled() {
		Assert.assertTrue(called);
		Assert.assertNotNull(cbThread);
		Assert.assertNotNull(clusterNode);
		Assert.assertTrue(error != null);
		// Assert.assertEquals( error, AisStatus.OK );
	}

	void assertNotCalled() {
		Assert.assertFalse(called);
		Assert.assertNull(cbThread);
		Assert.assertNull(clusterNode);
		Assert.assertTrue(error == null);
	}

	void reset() {
		called = false;
		cbThread = null;
		clusterNode = null;
		error = null;
	}

}
