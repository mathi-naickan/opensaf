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
import org.saforum.ais.AisException;
import org.saforum.ais.AisInvalidParamException;
import org.saforum.ais.AisNotExistException;
import org.saforum.ais.Version;
import org.saforum.ais.amf.AmfHandle;
import org.saforum.ais.amf.AmfHandleFactory;
import org.saforum.ais.amf.CsiManager;
import org.saforum.ais.amf.HaState;

import junit.framework.Assert;
import junit.framework.TestCase;

//CLASS DEFINITION
/**
 *
 * @author Nokia Siemens Networks
 * @author Jozsef BIRO
 */
public class TestCsiManager extends TestCase {

	// STATIC FIELDS
	private static String s_csiName1 = "safCsi=amfTestCSI_1,safSi=amfTestSI_1,safApp=AmfTestApp";
	private static String s_csiName2 = "safCsi=amfTestCSI_2,safSi=amfTestSI_2,safApp=AmfTestApp";
	private static String s_csiName3 = "safCsi=amfTestCSI_3,safSi=amfTestSI_3,safApp=AmfTestApp";

	private static String s_compName1 = "safComp=amfTestComp,safSu=amfTestSU1,safSg=amfTestSG,safApp=AmfTestApp";
	private static String s_compName2 = "safComp=amfTestComp,safSu=amfTestSU2,safSg=amfTestSG,safApp=AmfTestApp";
	private static HaState s_haState_comp1_csi1;
	private static HaState s_haState_comp1_csi2;
	private static HaState s_haState_comp1_csi3;
	private static HaState s_haState_comp2_csi1;
	private static HaState s_haState_comp2_csi2;
	private static HaState s_haState_comp2_csi3;

	// STATIC METHODS

	public static void main(String[] args) {
		junit.textui.TestRunner.run(TestCsiManager.class);
	}

	// INSTANCE FIELDS

	private AisException aisExc;

	/**
	 */
	private AmfHandle amfLibHandle;

	private Version b11Version;

	private CsiManager csiMgr;

	// CONSTRUCTORS

	public TestCsiManager(String arg0) {
		super(arg0);
	}

	// INSTANCE METHODS

	protected void setUp() throws Exception {
		super.setUp();
		System.out.println("JAVA TEST: SETTING UP NEXT TEST...");
		aisExc = null;
		b11Version = new Version((char) 'B', (short) 0x01, (short) 0x01);

		AmfHandleFactory amfHandleFactory = new AmfHandleFactory();

		AmfHandle.Callbacks callback = new AmfHandle.Callbacks();
		callback.cleanupProxiedComponentCallback = null;
		callback.healthcheckCallback = null;
		callback.instantiateProxiedComponentCallback = null;
		callback.removeCsiCallback = null;
		callback.setCsiCallback = null;
		callback.terminateComponentCallback = null;
		callback.trackProtectionGroupCallback = null;

		// library without callbacks
		amfLibHandle = amfHandleFactory.initializeHandle(
				callback, b11Version);
		csiMgr = amfLibHandle.getCsiManager();
	}

	protected void tearDown() throws Exception {
		super.tearDown();
		System.out.println("JAVA TEST: CLEANING UP AFTER TEST...");
		aisExc = null;
		// library without callbacks
		Utils.s_finalizeAmfLibraryHandle(amfLibHandle);
		amfLibHandle = null;
		csiMgr = null;
	}

	/*
	 * Test method for 'ais.amf.CsiManager.getHaState(String, String)'
	 */
	public final void testGetHaState_Exceptions() {
		System.out
				.println("JAVA TEST: Executing testGetHaState_Exceptions()...");
		// TEST CASE
		System.out
				.println("\nJAVA TEST: TEST CASE BEGIN try to get an haState for a null Component name: this should FAIL...");
		HaState _haState = null;
		try {
			_haState = csiMgr.getHaState(null, s_csiName1);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(_haState);
		Assert.assertNotNull(aisExc);
		Assert.assertTrue(aisExc instanceof AisInvalidParamException);
		aisExc = null;
		System.out
				.println("JAVA TEST: TEST CASE END tried to get an haState for a null Component name: this FAILED");
		// TEST CASE
		System.out
				.println("\nJAVA TEST: TEST CASE BEGIN try to get an haState for a non-existing Component name: this should FAIL...");
		_haState = null;
		try {
			_haState = csiMgr.getHaState("NotExistentComponent", s_csiName2);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(_haState);
		Assert.assertNotNull(aisExc);
		Assert.assertTrue(aisExc instanceof AisNotExistException);
		aisExc = null;
		System.out
				.println("JAVA TEST: TEST CASE END tried to get an haState for a non-existing Component name: this FAILED");
		// TEST CASE
		System.out
				.println("\nJAVA TEST: TEST CASE BEGIN try to get an haState for a null CSI name: this should FAIL...");
		_haState = null;
		try {
			_haState = csiMgr.getHaState(s_compName1, null);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(_haState);
		Assert.assertNotNull(aisExc);
		Assert.assertTrue(aisExc instanceof AisInvalidParamException);
		aisExc = null;
		System.out
				.println("JAVA TEST: TEST CASE END tried to get an haState for a null CSI name: this FAILED");
		// TEST CASE
		System.out
				.println("\nJAVA TEST: TEST CASE BEGIN try to get an haState for a non-existing CSI name: this should FAIL...");
		_haState = null;
		try {
			_haState = csiMgr.getHaState(s_compName2, "NotExistentCSI");
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(_haState);
		Assert.assertNotNull(aisExc);
		Assert.assertTrue(aisExc instanceof AisNotExistException);
		aisExc = null;
		System.out
				.println("JAVA TEST: TEST CASE END tried to get an haState for a non-existing CSI name: this FAILED");
	}

	/*
	 * Test method for 'ais.amf.CsiManager.getHaState(String, String)'
	 */
	public final void testGetHaState_Comp1_OK() {
		System.out.println("JAVA TEST: Executing testGetHaState_Comp1_OK()...");
		getHaState_Comp1();
	}

	/*
	 * Test method for 'ais.amf.CsiManager.getHaState(String, String)'
	 */
	public final void testGetHaState_Comp2_OK() {
		System.out.println("JAVA TEST: Executing testGetHaState_Comp2_OK()...");
		getHaState_Comp2();
	}

	/*
	 * Test method for 'ais.amf.CsiManager.getHaState(String, String)'
	 */
	public final void testGetHaState_BothComps_OK() {
		System.out
				.println("JAVA TEST: Executing testGetHaState_BothComps_OK()...");
		getHaState_Comp1();
		getHaState_Comp2();
		// additional assertions
		Assert.assertNotSame(s_haState_comp1_csi1, s_haState_comp2_csi1);
	}

	// PRIVATE

	/*
	 * Test method for 'ais.amf.CsiManager.getHaState(String, String)'
	 */
	public void getHaState_Comp1() {
		//
		try {
            System.out.println(java.util.Calendar.getInstance());
            System.out.println("Trying to get " + s_compName1 + " " + s_csiName1);
			s_haState_comp1_csi1 = csiMgr.getHaState(s_compName1, s_csiName1);
		} catch (AisException e) {
			aisExc = e;
            System.out.println(e);
            e.printStackTrace(System.out);
		}
		Assert.assertNull(aisExc);
		Assert.assertNotNull(s_haState_comp1_csi1);
		System.out.println("JAVA TEST: Ha state of component " + s_compName1
				+ " for csi " + s_csiName1 + " is " + s_haState_comp1_csi1);
		//
		try {
			s_haState_comp1_csi2 = csiMgr.getHaState(s_compName1, s_csiName2);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(aisExc);
		Assert.assertNotNull(s_haState_comp1_csi2);
		System.out.println("JAVA TEST: Ha state of component " + s_compName1
				+ " for csi " + s_csiName2 + " is " + s_haState_comp1_csi2);
		//
		try {
			s_haState_comp1_csi3 = csiMgr.getHaState(s_compName1, s_csiName3);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(aisExc);
		Assert.assertNotNull(s_haState_comp1_csi3);
		System.out.println("JAVA TEST: Ha state of component " + s_compName1
				+ " for csi " + s_csiName3 + " is " + s_haState_comp1_csi3);
		// Do some additional assertions
		// TODO: these are only true if cluster is not in a transient state
		Assert.assertSame(s_haState_comp1_csi1, s_haState_comp1_csi2);
		Assert.assertSame(s_haState_comp1_csi1, s_haState_comp1_csi3);
	}

	/*
	 * Test method for 'ais.amf.CsiManager.getHaState(String, String)'
	 */
	public void getHaState_Comp2() {
		//
		try {
			s_haState_comp2_csi1 = csiMgr.getHaState(s_compName2, s_csiName1);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(aisExc);
		Assert.assertNotNull(s_haState_comp2_csi1);
		System.out.println("JAVA TEST: Ha state of component " + s_compName2
				+ " for csi " + s_csiName1 + " is " + s_haState_comp2_csi1);
		//
		try {
			s_haState_comp2_csi2 = csiMgr.getHaState(s_compName2, s_csiName2);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(aisExc);
		Assert.assertNotNull(s_haState_comp2_csi2);
		System.out.println("JAVA TEST: Ha state of component " + s_compName2
				+ " for csi " + s_csiName2 + " is " + s_haState_comp2_csi2);
		//
		try {
			s_haState_comp2_csi3 = csiMgr.getHaState(s_compName2, s_csiName3);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(aisExc);
		Assert.assertNotNull(s_haState_comp2_csi3);
		System.out.println("JAVA TEST: Ha state of component " + s_compName2
				+ " for csi " + s_csiName3 + " is " + s_haState_comp2_csi3);
		// Do some additional assertions
		// TODO: these are only true if cluster is not in a transient state
		Assert.assertSame(s_haState_comp2_csi1, s_haState_comp2_csi2);
		Assert.assertSame(s_haState_comp2_csi1, s_haState_comp2_csi3);
	}

}
