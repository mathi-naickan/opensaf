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

import org.opensaf.ais.test.Utils;
import org.saforum.ais.AisBadHandleException;
import org.saforum.ais.AisException;
import org.saforum.ais.AisInitException;
import org.saforum.ais.AisInvalidParamException;
import org.saforum.ais.AisNotExistException;
import org.saforum.ais.AisStatus;
import org.saforum.ais.DispatchFlags;
import org.saforum.ais.TrackFlags;
import org.saforum.ais.Version;
import org.saforum.ais.amf.AmfHandle;
import org.saforum.ais.amf.AmfHandleFactory;
import org.saforum.ais.amf.ProtectionGroupManager;
import org.saforum.ais.amf.ProtectionGroupMember;
import org.saforum.ais.amf.ProtectionGroupNotification;
import org.saforum.ais.amf.TrackProtectionGroupCallback;

import junit.framework.*;

// CLASS DEFINITION
/**
 *
 * @author Nokia Siemens Networks
 * @author Jozsef BIRO
 */
public class TestProtectionGroupManager extends TestCase implements
		TrackProtectionGroupCallback {

	// STATIC METHODS

	public static void main(String[] args) {
		junit.textui.TestRunner.run(TestProtectionGroupManager.class);
	}

	/**
	 * @param pgManager
	 *            TODO
	 * @param printMsg
	 *            TODO
	 * @return TODO
	 */
	static ProtectionGroupNotification[] s_callGetProtectionGroup(
			String csiName, ProtectionGroupManager pgManager, String printMsg) {
		AisException _aisExc = null;
		ProtectionGroupNotification[] _pgNotificationArray = null;
		try {
			_pgNotificationArray = pgManager.getProtectionGroup(csiName);
		} catch (AisException e) {
			_aisExc = e;
		}
		Assert.assertNotNull(_pgNotificationArray);
		Utils.s_printPGNotificationArray(_pgNotificationArray, printMsg);
		Assert.assertNull(_aisExc);
		return _pgNotificationArray;
	}

	/**
	 * @param pgManager
	 *            TODO
	 *
	 */
	static void s_callGetProtectionGroupAsync(String csiName,
			ProtectionGroupManager pgManager) {
		AisException _aisExc = null;
		try {
			pgManager.getProtectionGroupAsync(csiName);
		} catch (AisException e) {
			_aisExc = e;
		}
		Assert.assertNull(_aisExc);
	}

	// INSTANCE FIELDS

	private AisException aisExc;

	/**
	 * A library handle without specified callbacks.
	 */
	private AmfHandle amfLibHandleNull;

	/**
	 * A library handle with trackProtectionGroupCallback implementation.
	 */
	private AmfHandle amfLibHandleOK;

	private Version b11Version;

	private ProtectionGroupManager pgManagerNull;

	private ProtectionGroupManager pgManagerOK;

	private ProtectionGroupNotification[] notificationArray;

	private String csiName1 = "safCsi=amfTestCSI_1,safSi=amfTestSI_1,safApp=AmfTestApp";
	private String csiName2 = "safCsi=amfTestCSI_2,safSi=amfTestSI_2,safApp=AmfTestApp";
	private String csiName3 = "safCsi=amfTestCSI_3,safSi=amfTestSI_3,safApp=AmfTestApp";
	private String csiNameCurrent;

	private boolean called_tPG_cb;

	private String tPG_cb_csiName;

	/**
	 * The value of the notificationArray parameter passed to
	 * trackClusterCallback.
	 */
	private ProtectionGroupNotification[] tPG_cb_notificationArray;

	/**
	 * The value of the numberOfMembers parameter passed to
	 * trackClusterCallback.
	 */
	private int tPG_cb_numberOfMembers;

	/**
	 * The value of the error parameter passed to trackClusterCallback.
	 */
	private AisStatus tPG_cb_error;

	/**
	 */
	private boolean tPG_cb_print;

	// CONSTRUCTORS

	/**
	 * @param name
	 */
	public TestProtectionGroupManager(String name) {
		super(name);
	}

	// INSTANCE METHODS

	protected void setUp() throws Exception {
		super.setUp();
		System.out.println("JAVA TEST: SETTING UP NEXT TEST...");
		aisExc = null;
		b11Version = new Version((char) 'B', (short) 0x01, (short) 0x01);

		AmfHandleFactory amfHandleFactory = new AmfHandleFactory();

		AmfHandle.Callbacks callbackNull = new AmfHandle.Callbacks();
		callbackNull.cleanupProxiedComponentCallback = null;
		callbackNull.healthcheckCallback = null;
		callbackNull.instantiateProxiedComponentCallback = null;
		callbackNull.removeCsiCallback = null;
		callbackNull.setCsiCallback = null;
		callbackNull.terminateComponentCallback = null;
		callbackNull.trackProtectionGroupCallback = null;

		AmfHandle.Callbacks callbackOk = new AmfHandle.Callbacks();
		callbackOk.cleanupProxiedComponentCallback = null;
		callbackOk.healthcheckCallback = null;
		callbackOk.instantiateProxiedComponentCallback = null;
		callbackOk.removeCsiCallback = null;
		callbackOk.setCsiCallback = null;
		callbackOk.terminateComponentCallback = null;
		callbackOk.trackProtectionGroupCallback = this;


		// library without callbacks
		amfLibHandleNull = amfHandleFactory.initializeHandle(callbackNull,
				b11Version);
		pgManagerNull = amfLibHandleNull.getProtectionGroupManager();
		// library with
		amfLibHandleOK = amfHandleFactory.initializeHandle(callbackOk,
				b11Version);
		pgManagerOK = amfLibHandleOK.getProtectionGroupManager();
		// library without callbacks
		// callback parameters
		called_tPG_cb = false;
		tPG_cb_csiName = null;
		tPG_cb_notificationArray = null;
		tPG_cb_numberOfMembers = 0;
		//
		notificationArray = null;
	}

	protected void tearDown() throws Exception {
		super.tearDown();
		System.out.println("JAVA TEST: CLEANING UP AFTER TEST...");
		aisExc = null;
		// library without callbacks
		Utils.s_finalizeAmfLibraryHandle(amfLibHandleNull);
		amfLibHandleNull = null;
		pgManagerNull = null;
		// library with 1 callback:
		Utils.s_finalizeAmfLibraryHandle(amfLibHandleOK);
		amfLibHandleOK = null;
		pgManagerOK = null;
	}

	/*
	 * Test method for 'ais.amf.ProtectionGroupManager.getProtectionGroup()'
	 * Needs to be removed due to malfunctioning of OpenSAF. ticket #202
	 */
	public final void testGetProtectionGroup_Simple() {
		System.out
				.println("JAVA TEST: Executing testGetProtectionGroup_Simple()...");
		csiNameCurrent = csiName1;
		notificationArray = s_callGetProtectionGroup(csiNameCurrent,
				pgManagerOK,
				"JAVA TEST: Result of 1st protection group query: ");
	}
	/*
	 * Test method for 'ais.amf.ProtectionGroupManager.getProtectionGroup()'
	 * Needs to be removed due to malfunctioning of OpenSAF. ticket #202
	 */
	public final void testGetProtectionGroup_Equals() {
		System.out
				.println("JAVA TEST: Executing testGetProtectionGroup_Equals()...");
		csiNameCurrent = csiName2;
		// 1st call
		notificationArray = s_callGetProtectionGroup(csiNameCurrent,
				pgManagerOK, "JAVA TEST: Result of 1st cluster query: ");
		// 2nd call
		ProtectionGroupNotification[] _notificationArray2;
		_notificationArray2 = s_callGetProtectionGroup(csiNameCurrent,
				pgManagerOK, "JAVA TEST: Result of 2nd cluster query: ");
		// check equality
		comparePGNotificationArrays(notificationArray, _notificationArray2);
		assertPGNotificationArray(notificationArray);
		assertPGNotificationArray(_notificationArray2);
	}

	/*
	 * Test method for
	 * 'ais.amf.ProtectionGroupManager.getProtectionGroupAsync()'
	 */
	public final void test_InvalidCsiNameAsync() {
		System.out.println("JAVA TEST: Executing test_InvalidCsiNameAsync()...");
		aisExc = null;
		// csi name invalid
		try {
			pgManagerOK.getProtectionGroupAsync("InvalidCsiName");
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNotNull(aisExc);
		Assert.assertTrue(aisExc instanceof AisNotExistException);

		aisExc = null;
		// csi name invalid
		try {
			pgManagerOK.getProtectionGroupAsync(null);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNotNull(aisExc);
		Assert.assertTrue(aisExc instanceof AisInvalidParamException);
}

	/*
	 * Test method for
	 * 'ais.amf.ProtectionGroupManager.getProtectionGroup()'
	 * Needs to be removed due to malfunctioning of OpenSAF. ticket #202
	 */
	public final void test_InvalidCsiName() {
		System.out.println("JAVA TEST: Executing test_InvalidCsiName()...");
		aisExc = null;
		// csi name null
		ProtectionGroupNotification[] _notificationArray = null;
		try {
			_notificationArray = pgManagerOK.getProtectionGroup(null);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNotNull(aisExc);
		Assert.assertTrue(aisExc instanceof AisInvalidParamException);
		Assert.assertNull(_notificationArray);
		aisExc = null;
	}

	/*
	 * Test method for
	 * 'ais.amf.ProtectionGroupManager.getProtectionGroupAsync()'
	 */
	public final void testGetProtectionGroupAsync_NoCallback() {
		// 1st call
		System.out
				.println("JAVA TEST: Executing testGetProtectionGroupNodeAsync_NoCallback()...");
		csiNameCurrent = csiName3;
		try {
			pgManagerNull.getProtectionGroupAsync(csiNameCurrent);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNotNull(aisExc);
		Assert.assertTrue(aisExc instanceof AisInitException);
	}

	/*
	 * Test method for
	 * 'ais.amf.ProtectionGroupManager.getProtectionGroupNodeAsync(long, int)'
	 * Needs to be removed due to malfunctioning of OpenSAF. ticket #202
	 *
	 */
	public final void testGetProtectionGroupAsync_EqualsDispatchBlocking() {
		System.out
				.println("JAVA TEST: Executing testGetProtectionGroupAsync_EqualsDispatchBlocking()...");
		csiNameCurrent = csiName3;
		// sync call
		notificationArray = s_callGetProtectionGroup(csiNameCurrent,
				pgManagerOK, "JAVA TEST: Result of SYNC cluster query: ");
		// async call
		s_callGetProtectionGroupAsync(csiNameCurrent, pgManagerOK);
		// blocking dispatch
		try {
			amfLibHandleOK.dispatchBlocking(); // MAY BLOCK FOREVER!!!
		} catch (AisException e) {
			aisExc = e;
		}
		// check callback
		assert_tPG_cb_OK();
		// check equality
		comparePGNotificationArrays(notificationArray, tPG_cb_notificationArray);
		assertPGNotificationArray(notificationArray);
		assertPGNotificationArray(tPG_cb_notificationArray);
	}


	/*
	 * Test method for
	 * 'ais.amf.ProtectionGroupManager.startProtectionGroupTracking(byte,boolean)'
	 * public final void testStartProtectionGroupTrackingBZ_InvalidTrackFlags() {
	 * System.out.println( "JAVA TEST: Executing
	 * testStartProtectionGroupTrackingB_InvalidTrackFlags()..." ); // 1st try:
	 * try{ pgManagerOK.startProtectionGroupTracking( (byte) 8, false ); }
	 * catch( AisException e ){ aisExc = e; } Assert.assertTrue( aisExc
	 * instanceof AisBadFlagsException ); aisExc = null; // 2nd try: try{
	 * pgManagerOK.startProtectionGroupTracking( (byte) ( 8 |
	 * TrackFlags.TRACK_CURRENT ), false ); } catch( AisException e ){
	 * aisExc = e; } Assert.assertTrue( aisExc instanceof AisBadFlagsException );
	 * aisExc = null; // 3rd try: try{ pgManagerOK.startProtectionGroupTracking(
	 * TrackFlags.TRACK_CURRENT, false ); } catch( AisException e ){ aisExc =
	 * e; } Assert.assertTrue( aisExc instanceof AisBadFlagsException ); }
	 */

	/*
	 * Test method for
	 * 'ais.amf.ProtectionGroupManager.startProtectionGroupTracking(byte)'
	 * public final void testStartProtectionGroupTrackingB_InvalidTrackFlags() {
	 * System.out.println( "JAVA TEST: Executing
	 * testStartProtectionGroupTrackingBN_InvalidTrackFlags()..." ); // 1st try:
	 * try{ pgManagerOK.startProtectionGroupTracking( (byte) 8, true ); } catch(
	 * AisException e ){ aisExc = e; } Assert.assertTrue( aisExc instanceof
	 * AisBadFlagsException ); aisExc = null; // 2nd try: try{
	 * pgManagerOK.startProtectionGroupTracking( (byte) ( 8 |
	 * TrackFlags.TRACK_CURRENT ), true ); } catch( AisException e ){
	 * aisExc = e; } Assert.assertTrue( aisExc instanceof AisBadFlagsException );
	 * aisExc = null; // 3rd try: try{ pgManagerOK.startProtectionGroupTracking(
	 * TrackFlags.TRACK_CURRENT, true ); } catch( AisException e ){ aisExc =
	 * e; } Assert.assertTrue( aisExc instanceof AisBadFlagsException ); }
	 */

	/*
	 * Test method for
	 * 'ais.amf.ProtectionGroupManager.stopProtectionGroupTracking()' public
	 * final void testStopProtectionGroupTracking() { // TODO Auto-generated
	 * method stub
	 *  }
	 */

	/*
	 * static SProtectionGroupNotification[] s_callGetProtectionGroup(
	 * ProtectionGroupManager pgManager, String printMsg ){ AisException
	 * _aisExc = null; SProtectionGroupNotification[] _notificationArray = null;
	 * try{ _notificationArray = pgManager.getProtectionGroup(); } catch(
	 * AisException e ){ _aisExc = e; } Assert.assertNotNull(
	 * _notificationArray ); Utils.s_printNotificationArray( _notificationArray,
	 * printMsg ); Assert.assertNull( _aisExc ); return _notificationArray; }
	 */

	/*
	 * Test method for
	 * 'ais.amf.ProtectionGroupManager.startProtectionGroupTracking(byte,boolean)'
	 * Needs to be removed due to malfunctioning of OpenSAF. ticket #202
	 */
	public final void testStartProtectionGroupTrackingBZ_EqualsDispatchBlocking() {
		System.out
				.println("JAVA TEST: Executing testStartProtectionGroupTrackingBZ_EqualsDispatchBlocking()...");
		csiNameCurrent = csiName1;
		// sync call
		notificationArray = s_callGetProtectionGroup(csiNameCurrent,
				pgManagerOK, "JAVA TEST: Result of SYNC cluster query: ");
		// start tracking and ask for async amf info
		try {
			pgManagerOK.startProtectionGroupTracking(csiNameCurrent,
					TrackFlags.TRACK_CHANGES);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(aisExc);
		// blocking dispatch
		try {
			amfLibHandleOK.dispatchBlocking(); // MAY BLOCK FOREVER!!!
		} catch (AisException e) {
			aisExc = e;
		}
		// check callback
		assert_tPG_cb_OK();
		// check equality
		comparePGNotificationArrays(notificationArray, tPG_cb_notificationArray);
		assertPGNotificationArray(notificationArray);
		assertPGNotificationArray(tPG_cb_notificationArray);
		// stop tracking
		try {
			pgManagerOK.stopProtectionGroupTracking(csiNameCurrent);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(aisExc);
	}

	/*
	 * Test method for
	 * 'ais.amf.ProtectionGroupManager.startProtectionGroupTracking(byte,boolean)'
	 * Needs to be removed due to malfunctioning of OpenSAF. ticket #202
	 */
	public final void testStartProtectionGroupTrackingB_Equals() {
		System.out
				.println("JAVA TEST: Executing testStartProtectionGroupTrackingB_Equals()...");
		csiNameCurrent = csiName2;
		// sync call
		notificationArray = s_callGetProtectionGroup(csiNameCurrent,
				pgManagerOK, "JAVA TEST: Result of SYNC cluster query: ");
		// start tracking and ask for sync amf info
		ProtectionGroupNotification[] _notificationArray = null;
		try {
			_notificationArray = pgManagerOK.getProtectionGroupThenStartTracking(
					csiNameCurrent, TrackFlags.TRACK_CHANGES);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNotNull(_notificationArray);
		Utils
				.s_printPGNotificationArray(_notificationArray,
						"JAVA TEST: Result of startProtectionGroupTracking(SYNC) query: ");
		Assert.assertNull(aisExc);
		// check equality
		comparePGNotificationArrays(notificationArray, _notificationArray);
		assertPGNotificationArray(notificationArray);
		assertPGNotificationArray(_notificationArray);
		// stop tracking
		try {
			pgManagerOK.stopProtectionGroupTracking(csiNameCurrent);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(aisExc);
	}

	/*
	 * Test method for 'ais.amf.AmfLibraryHandle.finalizeHandle()'
	 */
	public final void testFinalizeHandle() {
		System.out.println("JAVA TEST: Executing testFinalizeHandle()...");
		// finalize library handle
		try {
			amfLibHandleOK.finalizeHandle();
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNotNull(amfLibHandleOK);
		Assert.assertNull(aisExc);
		// test other handle operations!
		// getProtectionGroup
		ProtectionGroupNotification[] _notificationArray = null;
		try {
			_notificationArray = pgManagerOK.getProtectionGroup(csiName1);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(_notificationArray);
		Assert.assertTrue(aisExc instanceof AisBadHandleException);
		aisExc = null;
		// startProtectionGroupTracking
		_notificationArray = null;
		try {
			_notificationArray = pgManagerOK.getProtectionGroupThenStartTracking(
					csiName2, TrackFlags.TRACK_CHANGES);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertTrue(aisExc instanceof AisBadHandleException);
		aisExc = null;
		// getProtectionGroupAsync
		try {
			pgManagerOK.getProtectionGroupAsync(csiName3);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(_notificationArray);
		Assert.assertTrue(aisExc instanceof AisBadHandleException);
		aisExc = null;
		// getProtectionGroupAsync
		try {
			pgManagerOK.getProtectionGroupAsync(csiName1);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertTrue(aisExc instanceof AisBadHandleException);
		aisExc = null;
		// startProtectionGroupTracking
		try {
			pgManagerOK.getProtectionGroupAsyncThenStartTracking(csiName2,
					TrackFlags.TRACK_CHANGES);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertTrue(aisExc instanceof AisBadHandleException);
		aisExc = null;
		// stopProtectionGroupTracking
		try {
			pgManagerOK.stopProtectionGroupTracking(csiName3);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertTrue(aisExc instanceof AisBadHandleException);
		aisExc = null;
		// tear down...
		amfLibHandleOK = null;
	}

	// MISC INSTANCE METHODS

	/*
	 * Test method for
	 * 'ais.amf.ProtectionGroupManager.startProtectionGroupTracking(byte,boolean)'
	 */
	public final void testStartProtectionGroupTrackingB_WaitForever() {
		System.out
				.println("JAVA TEST: Executing testStartProtectionGroupTrackingB_WaitForever()...");
		csiNameCurrent = csiName1;
		// 1st try:
		try {
			pgManagerOK.getProtectionGroupAsyncThenStartTracking(csiNameCurrent,
					TrackFlags.TRACK_CHANGES);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(aisExc);
		// blocking dispatch
		try {
			tPG_cb_print = true;
			while (true) {
				System.out.println("JAVA TEST: Calling dispatch()...");
				amfLibHandleOK.dispatch(DispatchFlags.DISPATCH_BLOCKING); // WILL
																			// BLOCK
																			// FOREVER!!!
			}
		} catch (AisException e) {
			aisExc = e;
		}
		/*
		 */
	}

	/**
	 *
	 */
	private void assert_tPG_cb_OK() {
		Assert.assertTrue(called_tPG_cb);
		Assert.assertNotNull(tPG_cb_notificationArray);
		Assert.assertEquals(tPG_cb_numberOfMembers,
				tPG_cb_notificationArray.length);
		Assert.assertEquals(tPG_cb_error, AisStatus.OK);
		printTPGcbParameters("JAVA TEST: Result of ASYNC cluster query: ");
		Assert.assertNull(aisExc);
		called_tPG_cb = false;
	}

	/**
	 *
	 */
	private void comparePGNotificationArrays(
			ProtectionGroupNotification[] notArray1,
			ProtectionGroupNotification[] notArray2) {
		Assert.assertNotSame(notArray1, notArray2);
		Assert.assertEquals(notArray1.length, notArray2.length);
		for (int _idx = 0; _idx < notArray1.length; _idx++) {
			Assert.assertSame(notArray1[_idx].change, notArray2[_idx].change);
			compareProtectionGroupMembers(notArray1[_idx].member,
					notArray2[_idx].member);
		}
	}

	/**
	 *
	 */
	private void compareProtectionGroupMembers(
			ProtectionGroupMember pgMember1, ProtectionGroupMember pgMember2) {
		Assert.assertNotSame(pgMember1, pgMember2);
		Assert.assertEquals(pgMember1.rank, pgMember2.rank);
		Assert.assertSame(pgMember1.haState, pgMember2.haState);
		Assert.assertEquals(pgMember1.componentName, pgMember2.componentName);
	}

	/**
	 *
	 */
	private void assertPGNotificationArray(
			ProtectionGroupNotification[] notArray) {
		for (int _idx = 0; _idx < notArray.length; _idx++) {
		}
	}

	public void trackProtectionGroupCallback(String csiName,
			ProtectionGroupNotification[] notificationArray,
			int numberOfMembers, AisStatus error) {
		System.out
				.println("JAVA TEST CALLBACK: Executing trackProtectionGroupCallback( "
						+ csiName
						+ ", "
						+ notificationArray
						+ ", "
						+ numberOfMembers + ", " + error + " )...");
		called_tPG_cb = true;
		tPG_cb_csiName = csiName;
		tPG_cb_notificationArray = notificationArray;
		tPG_cb_numberOfMembers = numberOfMembers;
		tPG_cb_error = error;
		if (tPG_cb_print) {
			printTPGcbParameters("JAVA TEST CALLBACK: ProtectionGroup Info in callback");
		}
	}

	private void printTPGcbParameters(String msg) {
		System.out.println(msg);
		System.out.println("JAVA TEST: CSI name = " + tPG_cb_csiName);
		System.out.println("JAVA TEST: number of members = "
				+ tPG_cb_numberOfMembers);
		System.out.println("JAVA TEST: error = " + tPG_cb_error);
		if (tPG_cb_notificationArray != null)
			Utils.s_printPGNotificationArray(tPG_cb_notificationArray,
					"JAVA TEST: ProtectionGroup info: ");
	}

}
