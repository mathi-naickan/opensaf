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

import java.nio.channels.SelectableChannel;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;

import junit.framework.Test;
import junit.framework.TestSuite;

import org.saforum.ais.AisException;
import org.saforum.ais.AisStatus;
import org.saforum.ais.DispatchFlags;
import org.saforum.ais.Version;
import org.saforum.ais.amf.AmfHandle;
import org.saforum.ais.amf.AmfHandleFactory;
import org.saforum.ais.amf.CleanupProxiedComponentCallback;
import org.saforum.ais.amf.ComponentRegistry;
import org.saforum.ais.amf.CsiDescriptor;
import org.saforum.ais.amf.HaState;
import org.saforum.ais.amf.HealthcheckCallback;
import org.saforum.ais.amf.InstantiateProxiedComponentCallback;
import org.saforum.ais.amf.ProtectionGroupNotification;
import org.saforum.ais.amf.RemoveCsiCallback;
import org.saforum.ais.amf.SetCsiCallback;
import org.saforum.ais.amf.TerminateComponentCallback;
import org.saforum.ais.amf.TrackProtectionGroupCallback;
import org.saforum.ais.amf.CsiDescriptor.CsiFlags;

public class AmfTestRunner implements SetCsiCallback, RemoveCsiCallback, HealthcheckCallback,
				TerminateComponentCallback, TrackProtectionGroupCallback,
				CleanupProxiedComponentCallback, InstantiateProxiedComponentCallback{

	private AmfHandle amfLibHandle;
	private Version b11Version;
	private ComponentRegistry compRegistry;
	private boolean endProgram = false;
	private boolean runTests = false;
	private Selector selector;

    public static void main( String[] args ) {
    	AmfTestRunner testRunner = new AmfTestRunner();
    	testRunner.doDispatching();
    }

    public AmfTestRunner() {
		System.out.println("AmfTestRunner Ver 2");

		// Initialize AMF handle
		try {
			AmfHandleFactory amfHandleFactory = new AmfHandleFactory();

			AmfHandle.Callbacks callback = new AmfHandle.Callbacks();

			callback.cleanupProxiedComponentCallback = null;
			callback.healthcheckCallback = null;
			callback.instantiateProxiedComponentCallback = null;
			callback.removeCsiCallback = this;
			callback.setCsiCallback = this;
			callback.terminateComponentCallback = this;
			callback.trackProtectionGroupCallback = null;

			b11Version = new Version((char) 'B', (short) 0x01, (short) 0x01);

			amfLibHandle = amfHandleFactory.initializeHandle(callback,
					b11Version);

			// Initialize ComponentRegistry
			compRegistry = amfLibHandle.getComponentRegistry();

			// Register component
			compRegistry.registerComponent();

			// Retrieve selectable channel
			selector = Selector.open();

			// Register channel in selector
			SelectableChannel localChannel = amfLibHandle.getSelectableChannel();
			localChannel.configureBlocking(false);
			localChannel.register(selector, SelectionKey.OP_READ);

		} catch (Exception e) {
			// Exit on error
			e.printStackTrace();
			System.exit(-1);
		}

		System.out.println("Initialization successful.");

	}

    void doDispatching() {
    	while (!endProgram) {
    		try {
    			int numSelected = selector.select();

    			//boolean hasPending = amfLibHandle.hasPendingCallback(10 * Consts.SA_TIME_ONE_SECOND);
    			if (numSelected > 0) amfLibHandle.dispatch(DispatchFlags.DISPATCH_ONE);

				if (runTests) {
					runTests = false;
					System.out.println("***************************************************************************");
					System.out.println("***************************************************************************");
					System.out.println("***************************************************************************");
					System.out.println("***************************************************************************");
					System.out.flush();

					// Run tests.
					junit.textui.TestRunner.run(suite());

					System.out.println("***************************************************************************");
					System.out.println("***************************************************************************");
					System.out.println("***************************************************************************");
					System.out.println("***************************************************************************");
					System.out.flush();

				}

			} catch (Exception e) {
				e.printStackTrace();
			}
    	}
    }

    public static Test suite() {

    	TestSuite _suite = new TestSuite( "AMF Tests for org.saforum.ais.amf package.");
        //$JUnit-BEGIN$

        // CLASS TestAmfLibraryHandle
    	/**/
        _suite.addTest(new TestAmfLibraryHandle("testInitializeHandle_nullVersion"));
        _suite.addTest(new TestAmfLibraryHandle("testInitializeHandle_A11Version"));
        _suite.addTest(new TestAmfLibraryHandle("testInitializeHandle_B11Version"));
        _suite.addTest(new TestAmfLibraryHandle("testInitializeHandle_B18Version"));
        _suite.addTest(new TestAmfLibraryHandle("testInitializeHandle_B21Version"));
        _suite.addTest(new TestAmfLibraryHandle("testInitializeHandle_C11Version"));
        _suite.addTest(new TestAmfLibraryHandle("testInitializeHandle_TwoHandles"));
        //_suite.addTest(new TestAmfLibraryHandle("testInitializeHandle_AllCallbackVariations"));
        _suite.addTest(new TestAmfLibraryHandle("testHasPendingCallback_NoRegisteredCB"));
        _suite.addTest(new TestAmfLibraryHandle("testHasPendingCallback_NoCB"));
        _suite.addTest(new TestAmfLibraryHandle("testHasPendingCallback_Timeout"));
        _suite.addTest(new TestAmfLibraryHandle("testDispatch_NoRegisteredCB"));
        _suite.addTest(new TestAmfLibraryHandle("testDispatch_NoCB"));
        _suite.addTest(new TestAmfLibraryHandle("testDispatchBlocking_Timeout"));
        _suite.addTest(new TestAmfLibraryHandle("testFinalizeHandle"));
        _suite.addTest(new TestAmfLibraryHandle("testSelect_Timeout"));
        _suite.addTest(new TestAmfLibraryHandle("testSelect_InvalidParam"));
        _suite.addTest(new TestAmfLibraryHandle("testSelectNow_NoRegisteredCB"));
        /**/
        // CLASS TestCsiManager

        /*_suite.addTestSuite( TestCsiManager.class );*/

        _suite.addTest(new TestCsiManager("testGetHaState_Exceptions"));
        _suite.addTest(new TestCsiManager("testGetHaState_Comp1_OK"));
        _suite.addTest(new TestCsiManager("testGetHaState_Comp2_OK"));
        _suite.addTest(new TestCsiManager("testGetHaState_BothComps_OK"));
        /**/

        // CLASS TestProtectionGroupManager

        //_suite.addTestSuite( TestProtectionGroupManager.class );
    	_suite.addTest(new TestProtectionGroupManager("testGetProtectionGroup_Simple"));
    	_suite.addTest(new TestProtectionGroupManager("testGetProtectionGroup_Equals"));
    	_suite.addTest(new TestProtectionGroupManager("test_InvalidCsiNameAsync"));
    	_suite.addTest(new TestProtectionGroupManager("test_InvalidCsiName"));
    	_suite.addTest(new TestProtectionGroupManager("testGetProtectionGroupAsync_NoCallback"));
    	_suite.addTest(new TestProtectionGroupManager("testGetProtectionGroupAsync_EqualsDispatchBlocking"));
    	_suite.addTest(new TestProtectionGroupManager("testStartProtectionGroupTrackingBZ_EqualsDispatchBlocking"));
    	_suite.addTest(new TestProtectionGroupManager("testStartProtectionGroupTrackingB_Equals"));
    	_suite.addTest(new TestProtectionGroupManager("testFinalizeHandle"));

    	//_suite.addTest(new TestProtectionGroupManager("testStartProtectionGroupTrackingB_WaitForever")); Out commented, waits forever


        //$JUnit-END$
        return _suite;
    }




    // Callbacks

	public void setCsiCallback(long invocation, String componentName,
			HaState haState, CsiDescriptor csiDescriptor) {
		try {
			if (haState == HaState.ACTIVE) runTests = true;
			amfLibHandle.response(invocation, AisStatus.OK);
		} catch (AisException e) {
			e.printStackTrace();
		}
	}

	public void removeCsiCallback(long invocation, String componentName,
			String csiName, CsiFlags csiFlags) {
		try {
			amfLibHandle.response(invocation, AisStatus.OK);
		} catch (AisException e) {
			e.printStackTrace();
		}
	}

	public void healthcheckCallback(long invocation, String componentName,
			byte[] healthcheckKey) {
		try {
			amfLibHandle.response(invocation, AisStatus.OK);
		} catch (AisException e) {
			e.printStackTrace();
		}
	}

	public void terminateComponentCallback(long invocation, String componentName) {
		try {
			amfLibHandle.response(invocation, AisStatus.OK);
		} catch (AisException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		endProgram = true;
	}

	public void trackProtectionGroupCallback(String csiName,
			ProtectionGroupNotification[] notificationBuffer, int members,
			AisStatus error) {

	}

	public void cleanupProxiedComponentCallback(long invocation,
			String proxiedCompName) {
	}

	public void instantiateProxiedComponentCallback(long invocation,
			String proxiedCompName) {
	}
}
