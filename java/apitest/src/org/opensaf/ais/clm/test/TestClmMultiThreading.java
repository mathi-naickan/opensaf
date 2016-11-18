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

import java.util.*;

import org.opensaf.ais.test.*;
import org.saforum.ais.AisException;
import org.saforum.ais.Consts;
import org.saforum.ais.DispatchFlags;
import org.saforum.ais.Version;
import org.saforum.ais.clm.ClmHandle;
import org.saforum.ais.clm.ClmHandleFactory;
import org.saforum.ais.clm.ClusterMembershipManager;
import org.saforum.ais.clm.ClusterNode;
import org.saforum.ais.clm.ClusterNotificationBuffer;
import java.io.InputStream;
import java.util.Properties;

import junit.framework.*;

// CLASS DEFINITION
/**
 *
 * @author Nokia Siemens Networks
 * @author Jozsef BIRO
 */
public class TestClmMultiThreading extends TestCase {

	// STATIC METHODS

	public static void main(String[] args) {
		junit.textui.TestRunner.run(TestClmMultiThreading.class);
	}

	// CONSTANTS
	private final int ALLOWED_TIME_DIFF = 50; // (ms)

	// INSTANCE FIELDS

	private AisException aisExc;

	/**
	 * A library handle with both callbacks implemented.
	 */
	private ClmHandle clmLibHandle;

	private GetClusterNodeCallbackForTest getClusterNodeCB;

	private TrackClusterCallbackForTest trackClusterCB;

	private Version b11Version;

	private ClusterMembershipManager clmManager;

	private ClusterNode clusterNode;

	private ClusterNotificationBuffer notificationBuffer;

	ClmHandle.Callbacks callbackOKOK;

	private Properties properties = new Properties();

	private char releaseCode;

	private short majorVersion;
	
	private short minorVersion;     

	// CONSTRUCTORs

	public TestClmMultiThreading(String name) {
		super(name);
	}

	// INSTANCE METHODS

	protected void setUp() throws Exception {
		super.setUp();
		System.out.println("JAVA TEST: SETTING UP NEXT TEST...");
		aisExc = null;
		try{
			InputStream in = this.getClass().getClassLoader().getResourceAsStream("org/opensaf/ais/version.properties");
			properties.load(in);
			releaseCode = properties.getProperty("releaseCode").trim().charAt(0);
			majorVersion = new Short(properties.getProperty("majorVersion").trim()).shortValue();
			minorVersion = new Short(properties.getProperty("minorVersion").trim()).shortValue();
			System.out.println("releaseCode:"+ releaseCode +majorVersion +  minorVersion);
			in.close();
		}
		catch(Exception e){
			System.out.println(e.getMessage());
		}

		b11Version = new Version(releaseCode, majorVersion, minorVersion);
		// library with both callbacks
		getClusterNodeCB = new GetClusterNodeCallbackForTest();
		trackClusterCB = new TrackClusterCallbackForTest();

		ClmHandleFactory clmHandleFactory = new ClmHandleFactory();

		callbackOKOK = new ClmHandle.Callbacks();
		callbackOKOK.getClusterNodeCallback = getClusterNodeCB;
		callbackOKOK.trackClusterCallback = trackClusterCB;

		clmLibHandle = clmHandleFactory.initializeHandle(callbackOKOK,
				b11Version);
		clmManager = clmLibHandle.getClusterMembershipManager();
		// callback parameters
		notificationBuffer = null;
		clusterNode = null;
	}

	protected void tearDown() throws Exception {
		super.tearDown();
		System.out.println("JAVA TEST: CLEANING UP AFTER TEST...");
		aisExc = null;
		// library with both callbacks
		Utils.s_finalizeClmLibraryHandle(clmLibHandle);
		clmLibHandle = null;
		clmManager = null;
		getClusterNodeCB = null;
		trackClusterCB = null;
	}

	/*
	 * Test method for ...
	 */
	public void testHasPendingCallback_Threads() {
		System.out.println("JAVA TEST: testHasPendingCallback_Threads()...");
		// 1st call, no pending callbacks:
		callAndAssertHasPendingCallback(2, false);
		// call getClusterAsync()
		TestClusterMembershipManager.s_callGetClusterAsync(clmManager);
		// 2nd call, a single pending callback indicated by all checks:
		callAndAssertHasPendingCallback(4, true);
		// dispatch callback for getClusterAsync...
		CallDispatch_DispatchOne _d = new CallDispatch_DispatchOne();
		_d.run();
		_d.assertOK();
		trackClusterCB.assertCalled();
		getClusterNodeCB.assertNotCalled();
		// 3rd call, no pending callbacks:
		callAndAssertHasPendingCallback(3, false);
	}

	/*
	 * Test method for ...
	 */
	public void testHasPendingCallbackTimeout_Threads() {
		System.out.println("JAVA TEST: Executing testHasPendingCallbackTimeout_Threads()...");
		// 1st call, no pending callbacks:
		callAndAssertHasPendingCallbackTimeout(2,
				100 * Consts.SA_TIME_ONE_MILLISECOND, false);
		// 2nd call, a single pending callback indicated by both checks:
		CallHasPendingCallbackTimeout[] _c = new CallHasPendingCallbackTimeout[3];
		// create elements
		callHasPendingCallbackTimeout(_c, Consts.SA_TIME_ONE_SECOND);
		// call getClusterAsync()
		TestClusterMembershipManager.s_callGetClusterAsync(clmManager);
		// now check what the threads have done:
		assertHasPendingCallbackTimeout(_c, true);
		// dispatch callback for getClusterAsync...
		CallDispatch_DispatchOne _d = new CallDispatch_DispatchOne();
		_d.run();
		_d.assertOK();
		Assert.assertTrue(trackClusterCB.called);
		//trackClusterCB.assertCalled();
		getClusterNodeCB.assertNotCalled();
		// 3rd call, a single pending callback indicated by both checks:
		callAndAssertHasPendingCallbackTimeout(2,
				100 * Consts.SA_TIME_ONE_MILLISECOND, false);
	}

	/*
	 * Test method for ...
	 */
	public void testDispatchBlocking_Threads2() {
		System.out.println("JAVA TEST: Executing testDispatchBlocking_Threads2()...");
		doTestDispatchBlocking(2);
	}

	/*
	 * Test method for ...
	 */
	public void testDispatchBlocking_Threads3() {
		System.out.println("JAVA TEST: Executing testDispatchBlocking_Threads()...");
		doTestDispatchBlocking(3);
	}

	/*
	 * Test method for ...
	 */
	public void testDispatchBlocking_Threads6() {
		System.out.println("JAVA TEST: Executing testDispatchBlocking_Threads6()...");
		doTestDispatchBlocking(6);
	}

	/*
	 * Test method for ...
	 */
	public void testDispatchBlocking_Threads9() {
		System.out.println("JAVA TEST: Executing testDispatchBlocking_Threads9()...");
		doTestDispatchBlocking(9);
	}

	// MISC METHODS

	/*
	 * Test method for ...
	 */
	public void doTestDispatchBlocking(int nThreads) {
		System.out.println("JAVA TEST: Executing doTestDispatchBlocking( "
				+ nThreads + " )...");
		//
		// create elements
		CallDispatchBlocking[] _allDispatchCalls = new CallDispatchBlocking[nThreads];
		callDispatchBlocking(_allDispatchCalls);
		// assert that the dispatch threads are running
		assertDispatchBlocking(_allDispatchCalls, false);
		// call getClusterAsync()
		TestClusterMembershipManager.s_callGetClusterAsync(clmManager);
		// now check what the threads have done:
		
		CallDispatchBlocking[] _finishedDispatchCalls = null;
		_finishedDispatchCalls = checkDispatchBlockingFinished(
				_allDispatchCalls, 500);
		System.out.println("JAVA TEST: " + _finishedDispatchCalls.length
				+ " dispatch calls have returned out of "
				+ _allDispatchCalls.length);
		Assert.assertTrue(_finishedDispatchCalls.length <= _allDispatchCalls.length);
		Assert.assertTrue(_finishedDispatchCalls.length >= 1);
		// assert that the finished dispatch threads have properly finished
		assertDispatchBlocking(_finishedDispatchCalls, true);
		// assert that the proper callback has been called once
		for (CallDispatchBlocking cdb : _allDispatchCalls) {
			try {
				cdb.dispatchThread.join(100);
			} catch (InterruptedException e) {
			}
		}
		trackClusterCB.assertCalled();
		Assert.assertEquals(trackClusterCB.cbCount, 1);
		assertDispatchBlocking_ThreadOK(_finishedDispatchCalls,
				trackClusterCB.cbThread);
		trackClusterCB.fullReset();
		// assert that the other callback has not been called
		getClusterNodeCB.assertNotCalled();
		
	}

	/*
	 * Test method for ...
	 */
	private void callAndAssertHasPendingCallback(int nThreads,
			boolean expectedResult) {
		System.out.println("JAVA TEST: Executing callAndAssertHasPendingCallback( "
				+ nThreads + ", " + expectedResult + " )...");
		CallHasPendingCallback[] _c = new CallHasPendingCallback[nThreads];
		// create elements
		for (int _idx = 0; _idx < nThreads; _idx++) {
			_c[_idx] = new CallHasPendingCallback();
		}
		// call methods
		for (int _idx = 0; _idx < nThreads; _idx++) {
			(new Thread(_c[_idx])).start();
		}
		// assert result
		if (expectedResult) {
			for (int _idx = 0; _idx < nThreads; _idx++) {
				_c[_idx].assertPending();
			}
		} else {
			for (int _idx = 0; _idx < nThreads; _idx++) {
				_c[_idx].assertNotPending();
			}
		}
	}

	/*
	 * Test method for ...
	 */
	private void callAndAssertHasPendingCallbackTimeout(int nThreads,
			long timeout, boolean expectedResult) {
		System.out.println("JAVA TEST: Executing callAndAssertHasPendingCallbackTimeout( "
				+ nThreads
				+ ", "
				+ timeout
				+ ", "
				+ expectedResult
				+ " )...");
		CallHasPendingCallbackTimeout[] _c = new CallHasPendingCallbackTimeout[nThreads];
		// create elements
		callHasPendingCallbackTimeout(_c, timeout);
		// assert result
		assertHasPendingCallbackTimeout(_c, expectedResult);
	}

	/*
	 * Test method for ...
	 */
	private void callHasPendingCallbackTimeout(
			CallHasPendingCallbackTimeout[] c, long timeout) {
		// create elements
		for (int _idx = 0; _idx < c.length; _idx++) {
			c[_idx] = new CallHasPendingCallbackTimeout(timeout);
		}
		// call methods
		for (int _idx = 0; _idx < c.length; _idx++) {
			(new Thread(c[_idx])).start();
		}
	}

	/*
	 * Test method for ...
	 */
	private void assertHasPendingCallbackTimeout(
			CallHasPendingCallbackTimeout[] c, boolean expectedResult) {
		System.out.println("JAVA TEST: Executing assertHasPendingCallbackTimeout()...");
		if (expectedResult) {
			for (int _idx = 0; _idx < c.length; _idx++) {
				c[_idx].assertPending();
			}
		} else {
			for (int _idx = 0; _idx < c.length; _idx++) {
				c[_idx].assertNotPending();
			}
		}
	}

	/*
	 * Test method for ...
	 */
	private void callDispatchBlocking(CallDispatchBlocking[] c) {
		// create elements
		for (int _idx = 0; _idx < c.length; _idx++) {
			c[_idx] = new CallDispatchBlocking();
		}
		// call methods
		for (int _idx = 0; _idx < c.length; _idx++) {
			(new Thread(c[_idx])).start();
		}
	}

	/*
	 * Test method for ...
	 */
	private CallDispatchBlocking[] checkDispatchBlockingFinished(
			CallDispatchBlocking[] c, int timeout_ms) {
		System.out.println("JAVA TEST: Executing checkDispatchBlockingFinished()...");
		Set<CallDispatchBlocking> _finishedSet = new HashSet<CallDispatchBlocking>();
		for (int _idx = 0; _idx < c.length; _idx++) {
			if (c[_idx].checkFinishedTimeout(timeout_ms)) {
				_finishedSet.add(c[_idx]);
			}
		}
		return _finishedSet.toArray(new CallDispatchBlocking[1]);
	}

	/*
	 * Test method for ...
	 */
	private void assertDispatchBlocking(CallDispatchBlocking[] c,
			boolean expectedFinished) {
		System.out.println("JAVA TEST: Executing assertDispatchBlocking()...");
		if (expectedFinished) {
			for (int _idx = 0; _idx < c.length; _idx++) {
				c[_idx].assertFinished();
			}
		} else {
			for (int _idx = 0; _idx < c.length; _idx++) {
				c[_idx].assertNotFinished();
			}
		}
	}

	/*
	 * Test method for ...
	 */
	private void assertDispatchBlocking_ThreadOK(CallDispatchBlocking[] c,
			Thread t) {
		System.out.println("JAVA TEST: Executing assertDispatchBlocking_ThreadOK()...");
		Set<Thread> _threadSet = new HashSet<Thread>();
		for (int _idx = 0; _idx < c.length; _idx++) {
			Assert.assertTrue(_threadSet.add(c[_idx].dispatchThread));
		}
		Assert.assertTrue(_threadSet.contains(t));

	}

	// INNER CLASSES

	/**
     	 *
     	 */
	private class CallHasPendingCallback implements Runnable {

		// INSTANCE FIELDS

		boolean isFinished = false;

		boolean isPendingCallback = false;

		AisException aisExc = null;

		// INSTANCE METHODS

		public synchronized void run() {
			try {
				isPendingCallback = clmLibHandle.hasPendingCallback();
				System.out.println("JAVA TEST: hasPendingCallback() by " + this
						+ " returned " + isPendingCallback);
			} catch (AisException e) {
				aisExc = e;
			}
			isFinished = true;
			notify();
		}

		synchronized void assertPending() {
			while (!isFinished) {
				try {
					wait();
				} catch (InterruptedException e) {
				}
			}
			Assert.assertTrue(isPendingCallback);
			Assert.assertNull(aisExc);
		}

		synchronized void assertNotPending() {
			while (!isFinished) {
				try {
					wait();
				} catch (InterruptedException e) {
				}
			}
			Assert.assertFalse(isPendingCallback);
			Assert.assertNull(aisExc);
		}

	}

	/**
     *
     */
	private class CallHasPendingCallbackTimeout implements Runnable {

		// INSTANCE FIELDS

		boolean isFinished = false;

		boolean isPendingCallback = false;

		AisException aisExc = null;

		long before;
		long after;
		long timeout;

		// INSTANCE METHODS

		/**
		 * @param timeout
		 */
		CallHasPendingCallbackTimeout(long timeout) {
			this.timeout = timeout;
		}

		public synchronized void run() {
			try {
				before = System.currentTimeMillis();
				isPendingCallback = clmLibHandle.hasPendingCallback(timeout);
				after = System.currentTimeMillis();
				System.out.println("JAVA TEST: hasPendingCallback( " + timeout
						+ " ) by " + this + " returned " + isPendingCallback);
			} catch (AisException e) {
				aisExc = e;
			}
			isFinished = true;
			notify();
		}

		synchronized void assertPending() {
			while (!isFinished) {
				try {
					wait();
				} catch (InterruptedException e) {
				}
			}
			Assert.assertTrue(isPendingCallback);
			Assert.assertFalse(Utils.s_isDurationTooLong(before, after,
					timeout, ALLOWED_TIME_DIFF));
			Assert.assertNull(aisExc);
		}

		synchronized void assertNotPending() {
			while (!isFinished) {
				try {
					wait();
				} catch (InterruptedException e) {
				}
			}
			Assert.assertFalse(isPendingCallback);
			Assert.assertFalse(Utils.s_isDurationTooLong(before, after,
					timeout, ALLOWED_TIME_DIFF));
			Assert.assertFalse(Utils.s_isDurationTooShort(before, after,
					timeout, ALLOWED_TIME_DIFF));
			Assert.assertNull(aisExc);
		}

	}

	/**
     *
     */
	private class CallDispatch_DispatchOne implements Runnable {

		// INSTANCE FIELDS

		boolean isFinished = false;

		AisException aisExc = null;

		// INSTANCE METHODS

		public synchronized void run() {
			try {
				clmLibHandle.dispatch(DispatchFlags.DISPATCH_ONE);
				System.out.println("JAVA TEST: dispatch() by " + this
						+ " returned ");
			} catch (AisException e) {
				aisExc = e;
			}
			isFinished = true;
			notify();
		}

		synchronized void assertOK() {
			while (!isFinished) {
				try {
					wait();
				} catch (InterruptedException e) {
				}
			}
			Assert.assertNull(aisExc);
		}

	}

	/**
     *
     */
	private class CallDispatchBlocking implements Runnable {

		// INSTANCE FIELDS

		boolean isFinished;

		AisException aisExc;

		Thread dispatchThread;

		// CONSTRUCTOR

		public CallDispatchBlocking() {
			isFinished = false;
		}

		// INSTANCE METHODS

		public synchronized void run() {
			try {
				dispatchThread = Thread.currentThread();
				// wait one second to receive the callback
				clmLibHandle.dispatchBlocking(Consts.SA_TIME_ONE_MILLISECOND * 1000);
				System.out.println("JAVA TEST: dispatch() by " + this
						+ " returned ");
			} catch (AisException e) {
				aisExc = e;
			}
			isFinished = true;
			notify();
		}

		synchronized void assertFinished() {
			while (!isFinished) {
				try {
					wait();
				} catch (InterruptedException e) {
				}
			}
			Assert.assertNotNull(dispatchThread);
			Assert.assertFalse(dispatchThread.equals(Thread.currentThread()));
			Assert.assertNull(aisExc);
		}

		boolean checkFinishedTimeout(int timeout_ms) {
			if (!isFinished) {
				try {
					System.out.println("JAVA TEST: Sleeping " + timeout_ms + " milliseconds.");
					Thread.sleep(timeout_ms);
				} catch (InterruptedException e) {
				}
			}
			return isFinished;
		}

		void assertNotFinished() {
			Assert.assertFalse(isFinished);
		}

	}

}
