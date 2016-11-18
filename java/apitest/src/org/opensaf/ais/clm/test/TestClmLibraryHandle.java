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

import junit.framework.*;

import org.opensaf.ais.clm.ClmHandleImpl;
import org.opensaf.ais.test.*;
import org.saforum.ais.clm.*;
import org.saforum.ais.*;
// CLASS DEFINITION
/**
 *
 * @author Nokia Siemens Networks
 * @author Jozsef BIRO
 */
public class TestClmLibraryHandle extends TestCase implements
		GetClusterNodeCallback, TrackClusterCallback {
	// STATIC METHODS

	// CONSTANTS
	private final int ALLOWED_TIME_DIFF = 50; // (ms)

	// NOTE that Eclipse does not use this method...
	public static void main(String[] args) {
		junit.textui.TestRunner.run(TestClmLibraryHandle.class);
	}

	// INSTANCE FIELDS

	private AisException aisExc;

	private ClmHandleFactory clmHandleFactory = new ClmHandleFactory();

	private ClmHandle clmLibHandle;

	/**
	 * A library handle without specified callbacks.
	 */
	private ClmHandle clmLibHandleNullNull;

	/**
	 * A library handle with trackClusterCallback implementation.
	 */
	private ClmHandle clmLibHandleNullOK;

	/**
	 * A library handle with GetClusterNodeCallback implementation.
	 */
	private ClmHandle clmLibHandleOKNull;

	/**
	 * A library handle with both callbacks implemented.
	 */
	private ClmHandle clmLibHandleOKOK;

	private Version b11Version;

	private boolean called_gCN_cb;

	private boolean called_tC_cb;

	ClmHandle.Callbacks callbackNullNull;
	ClmHandle.Callbacks callbackNullOK;
	ClmHandle.Callbacks callbackOKNull;
	ClmHandle.Callbacks callbackOKOK;

	// CONSTRUCTORS

	/**
	 * Constructor for TestClmHandle.
	 *
	 * @param name
	 */
	public TestClmLibraryHandle(String name) {
		super(name);
	}

	// INSTANCE METHODS

	public void setUp() throws Exception {
		System.out.println("JAVA TEST: SETTING UP NEXT TEST...");
		clmLibHandle = null;
		aisExc = null;
		b11Version = new Version((char) 'B', (short) 0x01, (short) 0x01);

		// preinitialized library handles

		ClmHandleFactory clmHandleFactory = new ClmHandleFactory();

		callbackNullNull = new ClmHandle.Callbacks();
		callbackNullNull.getClusterNodeCallback = null;
		callbackNullNull.trackClusterCallback = null;

		callbackNullOK = new ClmHandle.Callbacks();
		callbackNullOK.getClusterNodeCallback = null;
		callbackNullOK.trackClusterCallback = this;

		callbackOKNull = new ClmHandle.Callbacks();
		callbackOKNull.getClusterNodeCallback = this;
		callbackOKNull.trackClusterCallback = null;

		callbackOKOK = new ClmHandle.Callbacks();
		callbackOKOK.getClusterNodeCallback = this;
		callbackOKOK.trackClusterCallback = this;

		clmLibHandleNullNull = clmHandleFactory.initializeHandle(
				callbackNullNull, b11Version);
		// library with
		clmLibHandleNullOK = clmHandleFactory.initializeHandle(callbackNullOK,
				b11Version);
		// library without callbacks
		clmLibHandleOKNull = clmHandleFactory.initializeHandle(callbackOKNull,
				b11Version);
		// library with both callbacks
		clmLibHandleOKOK = clmHandleFactory.initializeHandle(callbackOKOK,
				b11Version);
		called_gCN_cb = false;
		called_tC_cb = false;
	}

	protected void tearDown() throws Exception {
		super.tearDown();
		System.out.println("JAVA TEST: CLEANING UP AFTER TEST...");
		aisExc = null;
		// library without callbacks
		if (clmLibHandleNullNull != null) {
			clmLibHandleNullNull.finalizeHandle();
			clmLibHandleNullNull = null;
		}
		// library with 1 callback:
		if (clmLibHandleOKNull != null) {
			clmLibHandleOKNull.finalizeHandle();
			clmLibHandleOKNull = null;
		}
		// library with 1 callback:
		if (clmLibHandleNullOK != null) {
			clmLibHandleNullOK.finalizeHandle();
			clmLibHandleNullOK = null;
		}
		// library with both callbacks
		if (clmLibHandleOKOK != null) {
			clmLibHandleOKOK.finalizeHandle();
			clmLibHandleOKOK = null;
		}
		// library without callbacks
		if (clmLibHandle != null) {
			clmLibHandle.finalizeHandle();
			clmLibHandle = null;
		}
		System.out.println("JAVA TEST: CLEANING UP OK!");
	}

	/*
	 * Test method for 'ais.clm.ClmHandle.initializeHandle(ClmCallbacks,
	 * Version)'
	 */
	public final void testInitializeHandle_nullVersion() {
		System.out
				.println("JAVA TEST: Executing testInitializeHandle_nullVersion()...");
		try {
			clmLibHandle = clmHandleFactory.initializeHandle(callbackNullNull,
					null);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(clmLibHandle);
		Assert.assertTrue(aisExc instanceof AisInvalidParamException);
	}

	/*
	 * Test method for 'ais.clm.ClmHandle.initializeHandle(ClmCallbacks,
	 * Version)'
	 */
	public final void testInitializeHandle_A11Version() {
		System.out
				.println("JAVA TEST: Executing testInitializeHandle_A11Version()...");
		Version _a11Version = new Version((char) 'A', (short) 0x01,
				(short) 0x01);
		try {
			clmLibHandle = clmHandleFactory.initializeHandle(callbackNullNull,
					_a11Version);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(clmLibHandle);
		Assert.assertTrue(aisExc instanceof AisVersionException);
		Assert.assertEquals(_a11Version.releaseCode, (char) 'B');
		Assert.assertEquals(_a11Version.majorVersion, (short) 0x01);
		Assert.assertEquals(_a11Version.minorVersion, (short) 0x01);
	}

	/*
	 * Test method for 'ais.clm.ClmHandle.initializeHandle(ClmCallbacks,
	 * Version)'
	 */
	public final void testInitializeHandle_B11Version() {
		System.out
				.println("JAVA TEST: Executing testInitializeHandle_B11Version()...");
		try {
			clmLibHandle = clmHandleFactory.initializeHandle(callbackNullNull,
					b11Version);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNotNull(clmLibHandle);
		Assert.assertNull(aisExc);
		Assert.assertEquals(b11Version.releaseCode, (char) 'B');
		Assert.assertEquals(b11Version.majorVersion, (short) 0x01);
		Assert.assertEquals(b11Version.minorVersion, (short) 0x01);
	}

	/*
	 * Test method for 'ais.clm.ClmHandle.initializeHandle(ClmCallbacks,
	 * Version)'
	 */
	public final void testInitializeHandle_B18Version() {
		System.out
				.println("JAVA TEST: Executing testInitializeHandle_B18Version()...");
		Version _b18Version = new Version((char) 'B', (short) 0x01,
				(short) 0x08);
		try {
			clmLibHandle = clmHandleFactory.initializeHandle(callbackNullNull,
					_b18Version);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(clmLibHandle);
		Assert.assertEquals(_b18Version.releaseCode, (char) 'B');
		Assert.assertEquals(_b18Version.majorVersion, (short) 0x04);
		Assert.assertEquals(_b18Version.minorVersion, (short) 0x01);
		Assert.assertTrue(aisExc instanceof AisVersionException);
	}

	/*
	 * Test method for 'ais.clm.ClmHandle.initializeHandle(ClmCallbacks,
	 * Version)'
	 */
	public final void testInitializeHandle_B21Version() {
		System.out
				.println("JAVA TEST: Executing testInitializeHandle_B21Version()...");
		Version _b21Version = new Version((char) 'B', (short) 0x02,
				(short) 0x01);
		try {
			clmLibHandle = clmHandleFactory.initializeHandle(callbackNullNull,
					_b21Version);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNotNull(clmLibHandle);
		Assert.assertEquals(_b21Version.releaseCode, (char) 'B');
		Assert.assertEquals(_b21Version.majorVersion, (short) 0x04);
		Assert.assertEquals(_b21Version.minorVersion, (short) 0x01);
	}

	/*
         * Test method for 'ais.clm.ClmHandle.initializeHandle(ClmCallbacks,
         * Version)'
         */
	public final void testInitializeHandle_B51Version() {
		System.out
				.println("JAVA TEST: Executing testInitializeHandle_B51Version()...");
		Version _b51Version = new Version((char) 'B', (short) 0x05, (short) 0x01);
		try {
			clmLibHandle = clmHandleFactory.initializeHandle(callbackNullNull,
						_b51Version);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(clmLibHandle);
		Assert.assertTrue(aisExc instanceof AisVersionException);
		Assert.assertEquals(_b51Version.releaseCode, (char) 'B');
		Assert.assertEquals(_b51Version.majorVersion, (short) 0x04);
		Assert.assertEquals(_b51Version.minorVersion, (short) 0x01);
	}
	/*
	 * Test method for 'ais.clm.ClmHandle.initializeHandle(ClmCallbacks,
	 * Version)'
	 */
	public final void testInitializeHandle_C11Version() {
		System.out
				.println("JAVA TEST: Executing testInitializeHandle_C11Version()...");
		Version _c11Version = new Version((char) 'C', (short) 0x01,
				(short) 0x01);
		try {
			clmLibHandle = clmHandleFactory.initializeHandle(callbackNullNull,
					_c11Version);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(clmLibHandle);
		Assert.assertTrue(aisExc instanceof AisVersionException);
		Assert.assertEquals(_c11Version.releaseCode, (char) 'B');
		Assert.assertEquals(_c11Version.majorVersion, (short) 0x01);
		Assert.assertEquals(_c11Version.minorVersion, (short) 0x01);
	}

	public final void testInitializeHandle_TwoHandles() {
		System.out
				.println("JAVA TEST: Executing testInitializeHandle_TwoHandles()...");
		ClmHandle _secondHandle = null;
		// first library instance
		try {
			clmLibHandle = clmHandleFactory.initializeHandle(callbackNullNull,
					b11Version);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNotNull(clmLibHandle);
		Assert.assertNull(aisExc);
		Assert.assertEquals(b11Version.releaseCode, (char) 'B');
		Assert.assertEquals(b11Version.majorVersion, (short) 0x01);
		Assert.assertEquals(b11Version.minorVersion, (short) 0x01);
		// second library instance
		try {
			_secondHandle = clmHandleFactory.initializeHandle(callbackNullNull,
					b11Version);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNotNull(_secondHandle);
		Assert.assertNull(aisExc);
		Assert.assertEquals(b11Version.releaseCode, (char) 'B');
		Assert.assertEquals(b11Version.majorVersion, (short) 0x01);
		Assert.assertEquals(b11Version.minorVersion, (short) 0x01);
		Assert.assertNotSame(clmLibHandle, _secondHandle);
	}

	/*
	 * Test method for 'ais.LibraryHandle.select()'
	 */
	 public final void testSelect_InvalidParam() {
		 System.out.println("JAVATEST: Executing testSelect_InvalidParam()..."); // first library instance
		Assert.assertNotNull(clmLibHandleNullNull); // 1: null array
		Handle[] _selectedHandleArray = null;
		try {
			_selectedHandleArray = ClmHandleImpl.select(null);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(_selectedHandleArray);
		Assert.assertTrue(aisExc instanceof AisInvalidParamException); // 1: zero-length array
		aisExc = null;
		Handle[] _handleArray = new Handle[0];
		try {
			_selectedHandleArray = ClmHandleImpl.select(_handleArray);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(_selectedHandleArray);
		Assert.assertTrue(aisExc instanceof AisInvalidParamException); // 1: zero-length array
		aisExc = null;
		_handleArray = new Handle[1];
		_handleArray[0] = null;
		try {
			_selectedHandleArray = ClmHandleImpl.select(_handleArray);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(_selectedHandleArray); // because no registered callback
		Assert.assertTrue(aisExc instanceof AisInvalidParamException);
	}

	/*
	 * Test method for 'ais.LibraryHandle.selectNow()'
	 */
	public final void testSelectNow_NoRegisteredCB() {
		System.out.println("JAVA TEST: Executing testSelectNow_NoRegisteredCB()..."); // first library instance
		Assert.assertNotNull(clmLibHandleNullNull);
		Handle[] _handleArray = new Handle[1];
		_handleArray[0] = clmLibHandleNullNull;
		Handle[] _selectedHandleArray = _handleArray;
		try {
			_selectedHandleArray = ClmHandleImpl.selectNow(_handleArray);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(_selectedHandleArray); // because no registered callback
		Assert.assertNull(aisExc);
	}

	/*
	 * Test method for 'ais.LibraryHandle.select()'
	 */
	 public final void testSelect_Timeout() {
		System.out.println("JAVA TEST: Executing testSelect_Timeout()...");
		//doTestForSelectTimeout(10 * Consts.SA_TIME_ONE_MILLISECOND);
		doTestForSelectTimeout(40 * Consts.SA_TIME_ONE_MILLISECOND);
		doTestForSelectTimeout(100 * Consts.SA_TIME_ONE_MILLISECOND);
		doTestForSelectTimeout(500 * Consts.SA_TIME_ONE_MILLISECOND);
		//doTestForSelectTimeout( 2 * Consts.SA_TIME_ONE_SECOND );
		//doTestForSelectTimeout( 10 * Consts.SA_TIME_ONE_SECOND );
	}

	/*
	 * Test method for 'ais.clm.ClmHandle.hasPendingCallback()'
	 */
	public final void testHasPendingCallback_NoRegisteredCB() {
		System.out
				.println("JAVA TEST: Executing testHasPendingCallbackNoRegisteredCB()()...");
		// first library instance
		boolean _pending = false;
		Assert.assertNotNull(clmLibHandleNullNull);
		try {
			_pending = clmLibHandleNullNull.hasPendingCallback();
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertFalse(_pending); // because no registered callback
		Assert.assertNull(aisExc);
	}

	/*
	 * Test method for 'ais.clm.ClmHandle.hasPendingCallback()'
	 */
	public final void testHasPendingCallback_NoCB() {
		System.out
				.println("JAVA TEST: Executing testHasPendingCallbackNoCB()...");
		// first library instance
		boolean _pending = true;
		Assert.assertNotNull(clmLibHandleOKOK);
		try {
			_pending = clmLibHandleOKOK.hasPendingCallback();
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertFalse(_pending); // because no pending callback
		Assert.assertNull(aisExc);
	}

	/*
	 * Test method for 'ais.clm.ClmHandle.hasPendingCallback()'
	 */
	public final void testHasPendingCallback_Timeout() {
		System.out
				.println("JAVA TEST: Executing testHasPendingCallbackTimeout()...");
		doTestForHasPendingCallbackTimeout(40 * Consts.SA_TIME_ONE_MILLISECOND);
		doTestForHasPendingCallbackTimeout(100 * Consts.SA_TIME_ONE_MILLISECOND);
		doTestForHasPendingCallbackTimeout(500 * Consts.SA_TIME_ONE_MILLISECOND);
		//doTestForHasPendingCallbackTimeout( 2 * Consts.SA_TIME_ONE_SECOND );
		//doTestForHasPendingCallbackTimeout( 10 * Consts.SA_TIME_ONE_SECOND );
	}

	/*
	 * Test method for 'ais.clm.ClmHandle.dispatch(int)'
	 */
	public final void testDispatch_NoRegisteredCB() {
		System.out
				.println("JAVA TEST: Executing testDispatchNoRegisteredCB()()...");
		// first library instance
		Assert.assertNotNull(clmLibHandleNullNull);
		try {
			clmLibHandleNullNull.dispatch(DispatchFlags.DISPATCH_ONE);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertFalse(called_gCN_cb); // because no registered callback
		Assert.assertFalse(called_tC_cb); // because no registered callback
		Assert.assertNull(aisExc);
	}

	/*
	 * Test method for 'ais.clm.ClmHandle.dispatch(int)'
	 */
	public final void testDispatch_NoCB() {
		System.out.println("JAVA TEST: Executing testDispatchNoCB()()...");
		// first library instance
		Assert.assertNotNull(clmLibHandleOKOK);
		try {
			clmLibHandleOKOK.dispatch(DispatchFlags.DISPATCH_ONE);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertFalse(called_gCN_cb); // because no pending callback
		Assert.assertFalse(called_tC_cb);  // because no pending callback
		Assert.assertNull(aisExc);
	}

	/*
	 * Test method for 'ais.clm.ClmHandle.dispatchBlocking(long)'
	 */
	public final void testDispatchBlocking_Timeout() {
		System.out
				.println("JAVA TEST: Executing testDispatchBlockingTimeout()...");
		doTestForDispatchBlockingTimeout(15 * Consts.SA_TIME_ONE_MILLISECOND);
		doTestForDispatchBlockingTimeout(50 * Consts.SA_TIME_ONE_MILLISECOND);
		doTestForDispatchBlockingTimeout(90 * Consts.SA_TIME_ONE_MILLISECOND);
		doTestForDispatchBlockingTimeout(700 * Consts.SA_TIME_ONE_MILLISECOND);
		// doTestForDispatchBlockingTimeout( 3 * ETime.ONE_SECOND );
		// doTestForDispatchBlockingTimeout( 8 * ETime.ONE_SECOND );
	}

	/*
	 * Test method for 'ais.clm.ClmHandle.finalizeHandle()'
	 */
	public final void testFinalizeHandle() {
		System.out.println("JAVA TEST: Executing testFinalizeHandle()...");
		// create library handle
		try {
			System.out
					.println("JAVA TEST: About to call intializeHandle() for clmLibHandle...");
			clmLibHandle = clmHandleFactory.initializeHandle(callbackNullNull,
					b11Version);
			System.out
					.println("JAVA TEST: intializeHandle() for clmLibHandle returned...");
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNotNull(clmLibHandle);
		Assert.assertNull(aisExc);
		// finalize library handle
		try {
			System.out
					.println("JAVA TEST: About to call finalizeHandle() for clmLibHandle...");
			clmLibHandle.finalizeHandle();
			System.out
					.println("JAVA TEST: finalizeHandle() for clmLibHandle returned...");
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNotNull(clmLibHandle);
		Assert.assertNull(aisExc);
		// test other handle operations!
		// try to finalize an already finalized library handle
		try {
			System.out
					.println("JAVA TEST: About to call finalizeHandle() for clmLibHandle AGAIN...");
			clmLibHandle.finalizeHandle();
			System.out
					.println("JAVA TEST: SECOND CALL to finalizeHandle() for clmLibHandle returned...");
		} catch (AisException e) {
			aisExc = e;
			System.out
					.println("JAVA TEST: SECOND CALL to finalizeHandle() for clmLibHandle threw an exception...");
		}
		Assert.assertTrue(aisExc instanceof AisBadHandleException);
		// try to check pending callbacks for an already finalized library
		// handle
		aisExc = null;
		boolean _pending = false;
		try {
			System.out
					.println("JAVA TEST: About to call hasPendingCallback() for clmLibHandle...");
			_pending = clmLibHandle.hasPendingCallback();
			System.out
					.println("JAVA TEST: hasPendingCallback() for clmLibHandle returned...");
		} catch (AisException e) {
			aisExc = e;
			System.out
					.println("JAVA TEST: hasPendingCallback() for clmLibHandle threw an exception...");
		}
		Assert.assertFalse(_pending); // because not set
		Assert.assertTrue(aisExc instanceof AisBadHandleException);
		// try to dispatch for an already finalized library handle
		aisExc = null;
		try {
			System.out
					.println("JAVA TEST: About to call dispatch() for clmLibHandle...");
			clmLibHandle.dispatch(DispatchFlags.DISPATCH_ONE);
			System.out
					.println("JAVA TEST: dispatch() for clmLibHandle returned...");
		} catch (AisException e) {
			aisExc = e;
			System.out
					.println("JAVA TEST: dispatch() for clmLibHandle threw an exception...");
		}
		Assert.assertFalse(called_gCN_cb);
		Assert.assertFalse(called_tC_cb);
		Assert.assertTrue(aisExc instanceof AisBadHandleException);
		// try to finalize an already finalized library handle
		aisExc = null;
		try {
			System.out
					.println("JAVA TEST: About to call finalizeHandle() for clmLibHandle FOR THE THIRD TIME...");
			clmLibHandle.finalizeHandle();
			System.out
					.println("JAVA TEST: THIRD CALL to finalizeHandle() for clmLibHandle returned...");
		} catch (AisException e) {
			aisExc = e;
			System.out
					.println("JAVA TEST: SECOND CALL to finalizeHandle() for clmLibHandle threw an exception...");
		}
		Assert.assertTrue(aisExc instanceof AisBadHandleException);
		// tear down...
		clmLibHandle = null;

	}

	// MISC INSTANCE METHODS

	/*
	 * Test method for 'ais.clm.ClmHandle.hasPendingCallback()'
	 */
	public final void doTestForHasPendingCallbackTimeout(long saTimeout) {
		System.out
				.println("JAVA TEST: Executing doTestForHasPendingCallbackTimeout( "
						+ saTimeout + " )...");
		// first library instance
		boolean _pending = true;
		long _before = 0;
		long _after = 0;
		aisExc = null;
		Assert.assertNotNull(clmLibHandleOKOK);
		try {
			_before = System.currentTimeMillis();
			_pending = clmLibHandleOKOK.hasPendingCallback(saTimeout);
			_after = System.currentTimeMillis();
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertFalse(_pending); // because no pending callback
		Assert.assertNull(aisExc);
		Assert.assertFalse(Utils.s_isDurationTooLong(_before, _after,
				saTimeout, ALLOWED_TIME_DIFF));
		Assert.assertFalse(Utils.s_isDurationTooShort(_before, _after,
				saTimeout, ALLOWED_TIME_DIFF));
	}

	/*
	 * Test method for 'ais.LibraryHandle.select()'
	 */
	 public final void doTestForSelectTimeout(long saTimeout) {
		System.out.println("JAVA TEST: Executing doTestForSelectTimeout( "
				+ saTimeout + " )..."); // first library instance
		long _before = 0;
		long _after = 0;
		aisExc = null;
		Assert.assertNotNull(clmLibHandleOKOK);
		Handle[] _handleArray = new Handle[1];
		_handleArray[0] = clmLibHandleOKOK;
		Handle[] _selectedHandleArray = _handleArray;
		try {
			_before = System.currentTimeMillis();
			_selectedHandleArray = ClmHandleImpl.select(_handleArray, saTimeout);
			_after = System.currentTimeMillis();
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(_selectedHandleArray); // because no registered callback
		Assert.assertNull(aisExc);
		Assert.assertFalse(Utils.s_isDurationTooLong(_before, _after,
				saTimeout, ALLOWED_TIME_DIFF));
		Assert.assertFalse(Utils.s_isDurationTooShort(_before, _after,
				saTimeout, ALLOWED_TIME_DIFF));
	}

	/*
	 * Test method for 'ais.clm.ClmHandle.hasPendingCallback()'
	 */
	public final void doTestForDispatchBlockingTimeout(long saTimeout) {
		System.out
				.println("JAVA TEST: Executing doTestForDispatchBlockingTimeout( "
						+ saTimeout + " )...");
		// first library instance
		long _before = 0;
		long _after = 0;
		aisExc = null;
		Assert.assertNotNull(clmLibHandleOKOK);
		try {
			_before = System.currentTimeMillis();
			clmLibHandleOKOK.dispatchBlocking(saTimeout);
			_after = System.currentTimeMillis();
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertFalse(called_gCN_cb);
		Assert.assertFalse(called_tC_cb);
		Assert.assertFalse(Utils.s_isDurationTooLong(_before, _after,
				saTimeout, ALLOWED_TIME_DIFF));
		Assert.assertFalse(Utils.s_isDurationTooShort(_before, _after,
				saTimeout, ALLOWED_TIME_DIFF));
	}

	public void getClusterNodeCallback(long invocation,
			ClusterNode clusterNode, AisStatus error) {
		System.out
				.println("JAVA TEST CALLBACK: Executing getClusterNodeCallback()...");
		called_gCN_cb = true;
	}

	public void trackClusterCallback(
			ClusterNotificationBuffer notificationBuffer, int numberOfMembers,
			AisStatus error) {
		
		System.out.println("JAVA TEST CALLBACK: Executing trackClusterCallback( "
                                	+ notificationBuffer + ", " + numberOfMembers + ", "
                                	+ error + " )...");
	
		called_tC_cb = true;
	}
        public void trackClusterCallback(      ClusterNotificationBuffer notificationBuffer,
                                                                int numberOfMembers,
                                                                long invocation,
                                                                String rootCauseEntity,
                                                                CorrelationIds correlationIds,
                                                                ChangeStep step,
                                                                long timeSupervision,
                                                                AisStatus error ){

		System.out.println("JAVA TEST CALLBACK: Executing trackClusterCallback( "
                                + notificationBuffer + ", " + numberOfMembers + ", " + invocation + "," + rootCauseEntity +
                                "," + correlationIds + "," + step + "," + timeSupervision + ","
                                + error + " ) on " + this);

		called_tC_cb = true;
	}	

}
