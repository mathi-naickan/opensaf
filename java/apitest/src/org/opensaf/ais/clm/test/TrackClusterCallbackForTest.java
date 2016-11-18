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

import org.opensaf.ais.test.*;
import org.saforum.ais.AisStatus;
import org.saforum.ais.clm.ClusterNotificationBuffer;
import org.saforum.ais.clm.TrackClusterCallback;
import org.saforum.ais.ChangeStep;
import org.saforum.ais.CorrelationIds;
import junit.framework.Assert;

// CLASS DEFINITION
/**
 *
 * @author Nokia Siemens Networks
 * @author Jozsef BIRO
 */
public class TrackClusterCallbackForTest implements TrackClusterCallback {

	// INSTANCE FIELDS

	int cbCount;

	boolean called;

	Thread cbThread;

	/**
	 * The value of the notificationBuffer parameter passed to
	 * trackClusterCallback.
	 */
	ClusterNotificationBuffer notificationBuffer;

	/**
	 * The value of the numberOfMembers parameter passed to
	 * trackClusterCallback.
	 */
	int numberOfMembers;

	/**
	 * The value of the error parameter passed to trackClusterCallback.
	 */
	AisStatus error;

	// INSTANCE METHODS

	public synchronized void trackClusterCallback(
			ClusterNotificationBuffer notificationBuffer, int numberOfMembers,
			AisStatus error) {
		System.out.println("JAVA TEST CALLBACK: Executing trackClusterCallback( "
				+ notificationBuffer + ", " + numberOfMembers + ", "
				+ error + " ) on " + this);
		Utils.s_printClusterNotificationBuffer(notificationBuffer,
				"The Cluster as returned by the callback: ");
		cbCount++;
		called = true;
		cbThread = Thread.currentThread();
		this.notificationBuffer = notificationBuffer;
		this.numberOfMembers = numberOfMembers;
		this.error = error;
	}
	public synchronized void trackClusterCallback(      
			ClusterNotificationBuffer notificationBuffer, int numberOfMembers,
			long invocation, String rootCauseEntity,
			CorrelationIds correlationIds, ChangeStep step,
			long timeSupervision, AisStatus error ){
		System.out.println("JAVA TEST CALLBACK: Executing trackClusterCallback( "
			+ notificationBuffer + ", " + numberOfMembers + ", " + invocation + 
			"," + rootCauseEntity + "," + correlationIds + "," + step + "," + 
			timeSupervision + "," + error + " ) on " + this);
		Utils.s_printClusterNotificationBuffer_4(notificationBuffer,
				"The Cluster as returned by the callback: ");

		cbCount++;
		called = true;
		cbThread = Thread.currentThread();
		this.notificationBuffer = notificationBuffer;
		this.numberOfMembers = numberOfMembers;
		this.error = error;
	}

	synchronized void assertCalled() {
		Assert.assertTrue(called);
		Assert.assertNotNull(cbThread);
		Assert.assertNotNull(notificationBuffer);
		Assert.assertTrue(numberOfMembers > 0);
		Assert.assertTrue(error.getValue() != 0);
		// Assert.assertEquals( error, AisErrorException.OK );
	}

	synchronized void assertNotCalled() {
		Assert.assertFalse(called);
		Assert.assertNull(cbThread);
		Assert.assertNull(notificationBuffer);
		Assert.assertTrue(numberOfMembers == 0);
		Assert.assertTrue(error.getValue() == 0);
	}

	synchronized void reset() {
		called = false;
		cbThread = null;
		notificationBuffer = null;
		numberOfMembers = 0;
		error = null;
	}

	synchronized void fullReset() {
		reset();
		cbCount = 0;
	}

}
