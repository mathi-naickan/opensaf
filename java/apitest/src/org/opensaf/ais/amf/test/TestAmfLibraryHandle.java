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

package org.opensaf.ais.amf.test;

import java.util.Vector;

import junit.framework.Assert;
import junit.framework.TestCase;

import org.opensaf.ais.HandleImpl;
import org.opensaf.ais.test.Utils;
import org.saforum.ais.AisBadHandleException;
import org.saforum.ais.AisException;
import org.saforum.ais.AisInvalidParamException;
import org.saforum.ais.AisStatus;
import org.saforum.ais.AisVersionException;
import org.saforum.ais.Consts;
import org.saforum.ais.DispatchFlags;
import org.saforum.ais.Handle;
import org.saforum.ais.Version;
import org.saforum.ais.amf.AmfHandle;
import org.saforum.ais.amf.AmfHandleFactory;
import org.saforum.ais.amf.CleanupProxiedComponentCallback;
import org.saforum.ais.amf.CsiDescriptor;
import org.saforum.ais.amf.HaState;
import org.saforum.ais.amf.HealthcheckCallback;
import org.saforum.ais.amf.InstantiateProxiedComponentCallback;
import org.saforum.ais.amf.ProtectionGroupNotification;
import org.saforum.ais.amf.RemoveCsiCallback;
import org.saforum.ais.amf.SetCsiCallback;
import org.saforum.ais.amf.TerminateComponentCallback;
import org.saforum.ais.amf.TrackProtectionGroupCallback;

// CLASS DEFINITION
/**
 *
 * @author Nokia Siemens Networks
 * @author Jozsef BIRO
 */
public class TestAmfLibraryHandle extends TestCase implements
		HealthcheckCallback, SetCsiCallback, RemoveCsiCallback,
		TerminateComponentCallback, InstantiateProxiedComponentCallback,
		CleanupProxiedComponentCallback, TrackProtectionGroupCallback {
	// STATIC METHODS

	// CONSTANTS
	private final int ALLOWED_TIME_DIFF = 50; // (ms)

	// NOTE that Eclipse does not use this method...
	public static void main(String[] args) {
		junit.textui.TestRunner.run(TestAmfLibraryHandle.class);
	}

	private AisException aisExc;

	private AmfHandle amfLibHandle;

	/**
	 * A library handle without specified callbacks.
	 */
	private AmfHandle amfLibHandleNull;

	/**
	 * A library handle with all AMF callbacks implemented.
	 */
	private AmfHandle amfLibHandleAllOK;

	private Version b11Version;

	private boolean called_h_cb;

	private boolean called_sCSI_cb;

	private boolean called_rCSI_cb;

	private boolean called_tComp_cb;

	private boolean called_iPComp_cb;

	private boolean called_cPComp_cb;

	private boolean called_tPG_cb;

	AmfHandle.Callbacks callbackNull;
	AmfHandle.Callbacks callbackOK;

	// CONSTRUCTORS

	/**
	 * Constructor for TestAmfLibraryHandle.
	 *
	 * @param name
	 */
	public TestAmfLibraryHandle(String name) {
		super(name);
	}

	// INSTANCE METHODS

	public void setUp() throws Exception {
		System.out.println("JAVA TEST: SETTING UP NEXT TEST  for TestAmfLibraryHandle...");
		amfLibHandle = null;
		aisExc = null;
		b11Version = new Version((char) 'B', (short) 0x01, (short) 0x01);
		// preinitialized library handles
		// library without callbacks

		AmfHandleFactory amfHandleFactory = new AmfHandleFactory();

		callbackNull = new AmfHandle.Callbacks();
		callbackNull.cleanupProxiedComponentCallback = null;
		callbackNull.healthcheckCallback = null;
		callbackNull.instantiateProxiedComponentCallback = null;
		callbackNull.removeCsiCallback = null;
		callbackNull.setCsiCallback = null;
		callbackNull.terminateComponentCallback = null;
		callbackNull.trackProtectionGroupCallback = null;

		callbackOK = new AmfHandle.Callbacks();
		callbackOK.cleanupProxiedComponentCallback = this;
		callbackOK.healthcheckCallback = this;
		callbackOK.instantiateProxiedComponentCallback = this;
		callbackOK.removeCsiCallback = this;
		callbackOK.setCsiCallback = this;
		callbackOK.terminateComponentCallback = this;
		callbackOK.trackProtectionGroupCallback = this;

		// library without callbacks
		amfLibHandleNull = amfHandleFactory.initializeHandle(callbackNull,
				b11Version);

		// library with all callbacks
		amfLibHandleAllOK = amfHandleFactory.initializeHandle(callbackOK,
				b11Version);

		called_h_cb = false;
		called_sCSI_cb = false;
		called_rCSI_cb = false;
		called_tComp_cb = false;
		called_iPComp_cb = false;
		called_cPComp_cb = false;
		called_tPG_cb = false;
	}

	protected void tearDown() throws Exception {
		super.tearDown();
		System.out.println("JAVA TEST: CLEANING UP AFTER TEST for TestAmfLibraryHandle...");
		aisExc = null;
		// library without callbacks
		Utils.s_finalizeAmfLibraryHandle(amfLibHandle);
		amfLibHandle = null;
		// library without callbacks
		Utils.s_finalizeAmfLibraryHandle(amfLibHandleNull);
		amfLibHandleNull = null;
		// library with all callbacks
		Utils.s_finalizeAmfLibraryHandle(amfLibHandleAllOK);
		amfLibHandleAllOK = null;
	}

	// TEST METHODS

	/*
	 * Test method for 'ais.amf.AmfLibraryHandle.initializeHandle(AmfCallbacks,
	 * SVersion)'
	 */
	public final void testInitializeHandle_nullVersion() {
		System.out.println("JAVA TEST: Executing testInitializeHandle_nullVersion()...");
		try {
			AmfHandleFactory amfHandleFactory = new AmfHandleFactory();
			amfLibHandle = amfHandleFactory
					.initializeHandle(callbackNull, null);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(amfLibHandle);
		Assert.assertTrue(aisExc instanceof AisInvalidParamException);
	}

	/*
	 * Test method for 'ais.amf.AmfLibraryHandle.initializeHandle(AmfCallbacks,
	 * SVersion)'
	 */
	public final void testInitializeHandle_A11Version() {
		System.out.println("JAVA TEST: Executing testInitializeHandle_A11Version()...");
		Version _a11Version = new Version((char) 'A', (short) 0x01,
				(short) 0x01);
		try {
			AmfHandleFactory amfHandleFactory = new AmfHandleFactory();
			amfLibHandle = amfHandleFactory.initializeHandle(callbackNull,
					_a11Version);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(amfLibHandle);
		Assert.assertTrue(aisExc instanceof AisVersionException);
		Assert.assertEquals(_a11Version.releaseCode, (char) 'B');
		Assert.assertEquals(_a11Version.majorVersion, (short) 0x01);
		Assert.assertEquals(_a11Version.minorVersion, (short) 0x01);
	}

	/*
	 * Test method for 'ais.amf.AmfLibraryHandle.initializeHandle(AmfCallbacks,
	 * SVersion)'
	 */
	public final void testInitializeHandle_B11Version() {
		System.out.println("JAVA TEST: Executing testInitializeHandle_B11Version()...");
		try {
			AmfHandleFactory amfHandleFactory = new AmfHandleFactory();
			amfLibHandle = amfHandleFactory.initializeHandle(callbackNull,
					b11Version);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNotNull(amfLibHandle);
		Assert.assertNull(aisExc);
		Assert.assertEquals(b11Version.releaseCode, (char) 'B');
		Assert.assertEquals(b11Version.majorVersion, (short) 0x01);
		Assert.assertEquals(b11Version.minorVersion, (short) 0x01);
	}

	/*
	 * Test method for 'ais.amf.AmfLibraryHandle.initializeHandle(AmfCallbacks,
	 * SVersion)'
	 */
	public final void testInitializeHandle_B18Version() {
		System.out.println("JAVA TEST: Executing testInitializeHandle_B18Version()...");
		Version _b18Version = new Version((char) 'B', (short) 0x01,
				(short) 0x08);
		try {
			AmfHandleFactory amfHandleFactory = new AmfHandleFactory();
			amfLibHandle = amfHandleFactory.initializeHandle(callbackNull,
					_b18Version);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNotNull(amfLibHandle);
		Assert.assertNull(aisExc);
		Assert.assertEquals(_b18Version.releaseCode, (char) 'B');
		Assert.assertEquals(_b18Version.majorVersion, (short) 0x01);
		Assert.assertEquals(_b18Version.minorVersion, (short) 0x01);
	}

	/*
	 * Test method for 'ais.amf.AmfLibraryHandle.initializeHandle(AmfCallbacks,
	 * SVersion)'
	 */
	public final void testInitializeHandle_B21Version() {
		System.out.println("JAVA TEST: Executing testInitializeHandle_B21Version()...");
		Version _b21Version = new Version((char) 'B', (short) 0x02,
				(short) 0x01);
		try {
			AmfHandleFactory amfHandleFactory = new AmfHandleFactory();
			amfLibHandle = amfHandleFactory.initializeHandle(callbackNull,
					_b21Version);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(amfLibHandle);
		Assert.assertTrue(aisExc instanceof AisVersionException);
		Assert.assertEquals(_b21Version.releaseCode, (char) 'B');
		Assert.assertEquals(_b21Version.majorVersion, (short) 0x01);
		Assert.assertEquals(_b21Version.minorVersion, (short) 0x01);
	}

	/*
	 * Test method for 'ais.amf.AmfLibraryHandle.initializeHandle(AmfCallbacks,
	 * SVersion)'
	 */
	public final void testInitializeHandle_C11Version() {
		System.out.println("JAVA TEST: Executing testInitializeHandle_C11Version()...");
		Version _c11Version = new Version((char) 'C', (short) 0x01,
				(short) 0x01);
		try {
			AmfHandleFactory amfHandleFactory = new AmfHandleFactory();
			amfLibHandle = amfHandleFactory.initializeHandle(callbackNull,
					_c11Version);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(amfLibHandle);
		Assert.assertTrue(aisExc instanceof AisVersionException);
		Assert.assertEquals(_c11Version.releaseCode, (char) 'B');
		Assert.assertEquals(_c11Version.majorVersion, (short) 0x01);
		Assert.assertEquals(_c11Version.minorVersion, (short) 0x01);
	}

	public final void testInitializeHandle_TwoHandles() {
		System.out.println("JAVA TEST: Executing testInitializeHandle_TwoHandles()...");
		AmfHandle _secondHandle = null;

		AmfHandleFactory amfHandleFactory = new AmfHandleFactory();

		// first library instance
		try {
			amfLibHandle = amfHandleFactory.initializeHandle(callbackNull,
					b11Version);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNotNull(amfLibHandle);
		Assert.assertNull(aisExc);
		Assert.assertEquals(b11Version.releaseCode, (char) 'B');
		Assert.assertEquals(b11Version.majorVersion, (short) 0x01);
		Assert.assertEquals(b11Version.minorVersion, (short) 0x01);
		// second library instance
		try {
			_secondHandle = amfHandleFactory.initializeHandle(callbackNull,
					b11Version);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNotNull(_secondHandle);
		Assert.assertNull(aisExc);
		Assert.assertEquals(b11Version.releaseCode, (char) 'B');
		Assert.assertEquals(b11Version.majorVersion, (short) 0x01);
		Assert.assertEquals(b11Version.minorVersion, (short) 0x01);
		Assert.assertNotSame(amfLibHandle, _secondHandle);
	}

	public final void testInitializeHandle_AllCallbackVariations() {
		System.out.println("JAVA TEST: Executing testInitializeHandle_AllCallbackVariations()...");

		// Vector of invalid init combinations
		Vector<Integer> v = new Vector<Integer>();

    	for (int i = 0; i <= 127; i++) {
			try {
				System.out.println("******************* Round " + i);

				AmfHandleFactory amfHandleFactory = new AmfHandleFactory();

				AmfHandle.Callbacks callback = new AmfHandle.Callbacks();

				callback.cleanupProxiedComponentCallback = 		((i & 1) != 0)?this:null;
				callback.healthcheckCallback = 					((i & 2) != 0)?this:null;
				callback.instantiateProxiedComponentCallback = 	((i & 4) != 0)?this:null;
				callback.removeCsiCallback = 					((i & 8) != 0)?this:null;
				callback.setCsiCallback = 						((i & 16) != 0)?this:null;
				callback.terminateComponentCallback = 			((i & 32) != 0)?this:null;
				callback.trackProtectionGroupCallback = 		((i & 64) != 0)?this:null;

				b11Version = new Version((char) 'B', (short) 0x01, (short) 0x01);

				AmfHandle localAmfLibHandle = amfHandleFactory.initializeHandle(callback,
						b11Version);

				localAmfLibHandle.finalizeHandle();

			} catch (AisException e) {
				aisExc = e;
				v.add(new Integer(i));
			}
		}

    	if (v.size() != 0) {
        	System.out.println("Invalid callback variations:");
        	for (Integer integer : v) {
        		String s = "Callbacks = ";

        		s = (((integer & 1) != 0)?"cleanupProxiedComponentCallback":"") + ", " +
        		    (((integer & 2) != 0)?"healthcheckCallback":"") + ", " +
        		    (((integer & 4) != 0)?"instantiateProxiedComponentCallback":null) + ", " +
        		    (((integer & 8) != 0)?"removeCsiCallback":null) + ", " +
        		    (((integer & 16) != 0)?"setCsiCallback":null) + ", " +
        		    (((integer & 32) != 0)?"terminateComponentCallback":null) + ", " +
        		    (((integer & 64) != 0)?"trackProtectionGroupCallback":null);

				  System.out.println(s);

        	}
    	}

    	assertTrue(v.size() == 0);
	}


	/*
	 * Test method for 'ais.LibraryHandle.select()'
	 */
	public final void testSelect_InvalidParam() {
		System.out.println("JAVA TEST: Executing testSelect_InvalidParam()...");
		// first library instance
		Assert.assertNotNull(amfLibHandleNull);
		// 1: null array
		Handle[] _selectedHandleArray = null;
		try {
			_selectedHandleArray = HandleImpl.select(null);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(_selectedHandleArray);
		Assert.assertTrue(aisExc instanceof AisInvalidParamException);
		// 1: zero-length array
		aisExc = null;
		Handle[] _handleArray = new Handle[0];
		try {
			_selectedHandleArray = HandleImpl.select(_handleArray);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(_selectedHandleArray);
		Assert.assertTrue(aisExc instanceof AisInvalidParamException);
		// 1: zero-length array
		aisExc = null;
		_handleArray = new Handle[1];
		_handleArray[0] = null;
		try {
			_selectedHandleArray = HandleImpl.select(_handleArray);
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
		System.out.println("JAVA TEST: Executing testSelectNow_NoRegisteredCB()...");
		// first library instance
		Assert.assertNotNull(amfLibHandleNull);
		Handle[] _handleArray = new Handle[1];
		_handleArray[0] = amfLibHandleNull;
		Handle[] _selectedHandleArray = _handleArray;
		try {
			_selectedHandleArray = HandleImpl.selectNow(_handleArray);
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
		//doTestForSelectTimeout(40 * Consts.SA_TIME_ONE_MILLISECOND);
		doTestForSelectTimeout(100 * Consts.SA_TIME_ONE_MILLISECOND);
		doTestForSelectTimeout(500 * Consts.SA_TIME_ONE_MILLISECOND);
		// doTestForSelectTimeout( 2 * Consts.SA_TIME_ONE_SECOND );
		// doTestForSelectTimeout( 10 * Consts.SA_TIME_ONE_SECOND );
	}

	/*
	 * Test method for 'ais.amf.AmfLibraryHandle.hasPendingCallback()'
	 */
	public final void testHasPendingCallback_NoRegisteredCB() {
		System.out.println("JAVA TEST: Executing testHasPendingCallbackNoRegisteredCB()()...");
		// first library instance
		boolean _pending = false;
		Assert.assertNotNull(amfLibHandleNull);
		try {
			_pending = amfLibHandleNull.hasPendingCallback();
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertFalse(_pending); // because no registered callback
		Assert.assertNull(aisExc);
	}

	/*
	 * Test method for 'ais.amf.AmfLibraryHandle.hasPendingCallback()'
	 */
	public final void testHasPendingCallback_NoCB() {
		System.out.println("JAVA TEST: Executing testHasPendingCallbackNoCB()...");
		// first library instance
		boolean _pending = true;
		Assert.assertNotNull(amfLibHandleAllOK);
		try {
			_pending = amfLibHandleAllOK.hasPendingCallback();
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertFalse(_pending); // because no pending callback
		Assert.assertNull(aisExc);
	}

	/*
	 * Test method for 'ais.amf.AmfLibraryHandle.hasPendingCallback()'
	 */
	public final void testHasPendingCallback_Timeout() {
		System.out.println("JAVA TEST: Executing testHasPendingCallbackTimeout()...");
		//doTestForHasPendingCallbackTimeout(10 * Consts.SA_TIME_ONE_MILLISECOND);
		//doTestForHasPendingCallbackTimeout(40 * Consts.SA_TIME_ONE_MILLISECOND);
		doTestForHasPendingCallbackTimeout(100 * Consts.SA_TIME_ONE_MILLISECOND);
		doTestForHasPendingCallbackTimeout(500 * Consts.SA_TIME_ONE_MILLISECOND);
		// doTestForHasPendingCallbackTimeout( 2 * Consts.SA_TIME_ONE_SECOND );
		// doTestForHasPendingCallbackTimeout( 10 * Consts.SA_TIME_ONE_SECOND );
	}

	/*
	 * Test method for 'ais.amf.AmfLibraryHandle.dispatch(int)'
	 */
	public final void testDispatch_NoRegisteredCB() {
		System.out.println("JAVA TEST: Executing testDispatchNoRegisteredCB()()...");
		System.out.println("ENUM TEST (DISPATCH_ONE): "
				+ DispatchFlags.DISPATCH_ONE.ordinal());
		System.out.println("ENUM TEST (DISPATCH_ALL): "
				+ DispatchFlags.DISPATCH_ALL.ordinal());
		System.out.println("ENUM TEST (DISPATCH_BLOCKING): "
				+ DispatchFlags.DISPATCH_BLOCKING.ordinal());
		// first library instance
		Assert.assertNotNull(amfLibHandleNull);
		try {
			amfLibHandleNull.dispatch(DispatchFlags.DISPATCH_ONE);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertFalse(called_h_cb); // because no registered callback
		Assert.assertFalse(called_sCSI_cb); // because no registered callback
		Assert.assertFalse(called_rCSI_cb); // because no registered callback
		Assert.assertFalse(called_tComp_cb); // because no registered callback
		Assert.assertFalse(called_iPComp_cb); // because no registered callback
		Assert.assertFalse(called_cPComp_cb); // because no registered callback
		Assert.assertFalse(called_tPG_cb); // because no registered callback
		Assert.assertNull(aisExc);
	}

	/*
	 * Test method for 'ais.amf.AmfLibraryHandle.dispatch(int)'
	 */
	public final void testDispatch_NoCB() {
		System.out.println("JAVA TEST: Executing testDispatchNoCB()()...");
		// first library instance
		Assert.assertNotNull(amfLibHandleAllOK);
		try {
			amfLibHandleAllOK.dispatch(DispatchFlags.DISPATCH_ONE);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertFalse(called_h_cb); // because no pending callback
		Assert.assertFalse(called_sCSI_cb); // because no pending callback
		Assert.assertFalse(called_rCSI_cb); // because no pending callback
		Assert.assertFalse(called_tComp_cb); // because no pending callback
		Assert.assertFalse(called_iPComp_cb); // because no pending callback
		Assert.assertFalse(called_cPComp_cb); // because no pending callback
		Assert.assertFalse(called_tPG_cb); // because no pending callback
		Assert.assertNull(aisExc);
	}

	/*
	 * Test method for 'ais.amf.AmfLibraryHandle.dispatchBlocking(long)'
	 */
	public final void testDispatchBlocking_Timeout() {
		System.out.println("JAVA TEST: Executing testDispatchBlockingTimeout()...");
		doTestForDispatchBlockingTimeout(15 * Consts.SA_TIME_ONE_MILLISECOND);
		doTestForDispatchBlockingTimeout(50 * Consts.SA_TIME_ONE_MILLISECOND);
		doTestForDispatchBlockingTimeout(90 * Consts.SA_TIME_ONE_MILLISECOND);
		doTestForDispatchBlockingTimeout(700 * Consts.SA_TIME_ONE_MILLISECOND);
		// doTestForDispatchBlockingTimeout( 3 * Consts.SA_TIME_ONE_SECOND );
		// doTestForDispatchBlockingTimeout( 8 * Consts.SA_TIME_ONE_SECOND );
	}

	/*
	 * Test method for 'ais.amf.AmfLibraryHandle.finalizeHandle()'
	 */
	public final void testFinalizeHandle() {
		System.out.println("JAVA TEST: Executing testFinalizeHandle()...");
		// create library handle
		try {
			AmfHandleFactory amfHandleFactory = new AmfHandleFactory();
			amfLibHandle = amfHandleFactory.initializeHandle(callbackNull,
					b11Version);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNotNull(amfLibHandle);
		Assert.assertNull(aisExc);
		// finalize library handle
		try {
			amfLibHandle.finalizeHandle();
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNotNull(amfLibHandle);
		Assert.assertNull(aisExc);
		// test other handle operations!
		// try to finalize an already finalized library handle
		try {
			amfLibHandle.finalizeHandle();
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertTrue(aisExc instanceof AisBadHandleException);
		// try to check pending callbacks for an already finalized library
		// handle
		aisExc = null;
		boolean _pending = false;
		try {
			_pending = amfLibHandle.hasPendingCallback();
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertFalse(_pending); // because not set
		Assert.assertTrue(aisExc instanceof AisBadHandleException);
		// try to dispatch for an already finalized library handle
		aisExc = null;
		try {
			amfLibHandle.dispatch(DispatchFlags.DISPATCH_ONE);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertFalse(called_h_cb);
		Assert.assertFalse(called_sCSI_cb);
		Assert.assertFalse(called_rCSI_cb);
		Assert.assertFalse(called_tComp_cb);
		Assert.assertFalse(called_iPComp_cb);
		Assert.assertFalse(called_cPComp_cb);
		Assert.assertFalse(called_tPG_cb);
		Assert.assertTrue(aisExc instanceof AisBadHandleException);
		// tear down...
		amfLibHandle = null;

	}

	// MISC INSTANCE METHODS

	/*
	 * Test method for 'ais.amf.AmfLibraryHandle.hasPendingCallback()'
	 */
	public final void doTestForHasPendingCallbackTimeout(long saTimeout) {
		System.out.println("JAVA TEST: Executing doTestForHasPendingCallbackTimeout( "
						+ saTimeout + " )...");
		// first library instance
		boolean _pending = true;
		long _before = 0;
		long _after = 0;
		aisExc = null;
		Assert.assertNotNull(amfLibHandleAllOK);
		try {
			_before = System.currentTimeMillis();
			_pending = amfLibHandleAllOK.hasPendingCallback(saTimeout);
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
	 * Test method for 'ais.amf.AmfLibraryHandle.hasPendingCallback()'
	 */
	public final void doTestForSelectTimeout(long saTimeout) {
		System.out.println("JAVA TEST: Executing doTestForSelectTimeout( "
				+ saTimeout + " )...");
		// first library instance
		long _before = 0;
		long _after = 0;
		aisExc = null;
		Assert.assertNotNull(amfLibHandleAllOK);
		Handle[] _handleArray = new Handle[1];
		_handleArray[0] = amfLibHandleAllOK;
		Handle[] _selectedHandleArray = _handleArray;
		try {
			_before = System.currentTimeMillis();
			_selectedHandleArray = HandleImpl.select(_handleArray, saTimeout);
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
	 * Test method for 'ais.amf.AmfLibraryHandle.hasPendingCallback()'
	 */
	public final void doTestForDispatchBlockingTimeout(long saTimeout) {
		System.out.println("JAVA TEST: Executing doTestForDispatchBlockingTimeout( "
						+ saTimeout + " )...");
		// first library instance
		long _before = 0;
		long _after = 0;
		aisExc = null;
		Assert.assertNotNull(amfLibHandleAllOK);
		try {
			_before = System.currentTimeMillis();
			amfLibHandleAllOK.dispatchBlocking(saTimeout);
			_after = System.currentTimeMillis();
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertFalse(called_h_cb);
		Assert.assertFalse(called_sCSI_cb);
		Assert.assertFalse(called_rCSI_cb);
		Assert.assertFalse(called_tComp_cb);
		Assert.assertFalse(called_iPComp_cb);
		Assert.assertFalse(called_cPComp_cb);
		Assert.assertFalse(called_tPG_cb);
		Assert.assertFalse(Utils.s_isDurationTooLong(_before, _after,
				saTimeout, ALLOWED_TIME_DIFF));
		Assert.assertFalse(Utils.s_isDurationTooShort(_before, _after,
				saTimeout, ALLOWED_TIME_DIFF));
	}

	/*
	 * @param invocation - @param componentName - @param healthcheckKey -
	 */
	public void healthcheckCallback(long invocation, String componentName,
			byte[] healthcheckKey) {
		System.out
				.println("JAVA TEST CALLBACK: Executing healthcheckCallback()...");
		called_h_cb = true;
	}

	public void setCsiCallback(long invocation, String componentName,
			HaState haState, CsiDescriptor csiDescriptor) {
		System.out.println("JAVA TEST CALLBACK: Executing setCsiCallback()...");
		called_sCSI_cb = true;
	}

	public void removeCsiCallback(long invocation, String componentName,
			String csiName, CsiDescriptor.CsiFlags csiFlags) {
		System.out.println("JAVA TEST CALLBACK: Executing removeCsiCallback()...");
		called_rCSI_cb = true;
	}

	public void terminateComponentCallback(long invocation, String componentName) {
		System.out.println("JAVA TEST CALLBACK: Executing terminateComponentCallback()...");
		called_tComp_cb = true;
	}

	public void instantiateProxiedComponentCallback(long invocation,
			String proxiedCompName) {
		System.out.println("JAVA TEST CALLBACK: Executing instantiateProxiedComponentCallback()...");
		called_iPComp_cb = true;
	}

	public void cleanupProxiedComponentCallback(long invocation,
			String proxiedCompName) {
		System.out.println("JAVA TEST CALLBACK: Executing cleanupProxiedComponentCallback()...");
		called_cPComp_cb = true;
	}

	public void trackProtectionGroupCallback(String csiName,
			ProtectionGroupNotification[] notificationBuffer, int nMembers,
			AisStatus error) {
		System.out.println("JAVA TEST CALLBACK: Executing trackProtectionGroupCallback()...");
		called_tPG_cb = true;
	}

}
