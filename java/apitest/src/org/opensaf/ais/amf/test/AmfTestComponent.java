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
import java.util.Set;

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

public class AmfTestComponent implements SetCsiCallback, RemoveCsiCallback, HealthcheckCallback,
				TerminateComponentCallback, TrackProtectionGroupCallback,
				CleanupProxiedComponentCallback, InstantiateProxiedComponentCallback{

	private AmfHandle amfLibHandle;
	private Version b11Version;
	private ComponentRegistry compRegistry;
	private boolean endProgram = false;
	private Selector selector;

	SelectableChannel globalChannel;

    public static void main( String[] args ) {
    	AmfTestComponent testComponent = new AmfTestComponent();
    	testComponent.doDispatching();
    }

    public AmfTestComponent() {
		System.out.println("AmfTestComponent Ver 1");

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
			globalChannel = amfLibHandle.getSelectableChannel();
			globalChannel.configureBlocking(false);
			globalChannel.register(selector, SelectionKey.OP_READ);

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
    			System.out.println("Dispatch loop " + System.currentTimeMillis());
    			Set<SelectionKey> keys = selector.selectedKeys();
    			//boolean hasPending = amfLibHandle.hasPendingCallback(10 * Consts.SA_TIME_ONE_SECOND);
    			if (keys.size() > 0) {
    				System.out.println("Dispatching.");
    				amfLibHandle.dispatch(DispatchFlags.DISPATCH_ONE);
    				keys.clear();
    			}

		} catch (Exception e) {
			e.printStackTrace();
		}
    	}
    }


    // Callbacks

	public void setCsiCallback(long invocation, String componentName,
			HaState haState, CsiDescriptor csiDescriptor) {
		try {
			System.out.println("CSI set called. CSI name=" + csiDescriptor.csiName);
			if (haState == HaState.ACTIVE) System.out.println("Activation");
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
