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

package org.opensaf.ais.test;

import java.io.IOException;
import java.nio.channels.SelectableChannel;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Random;
import java.util.Set;
import junit.framework.TestCase;
import org.opensaf.ais.SelectionObjectMediator;
import org.saforum.ais.AisBadHandleException;
import org.saforum.ais.AisException;
import org.saforum.ais.AisLibraryException;
import org.saforum.ais.AisNoMemoryException;
import org.saforum.ais.AisNoResourcesException;
import org.saforum.ais.AisStatus;
import org.saforum.ais.AisTimeoutException;
import org.saforum.ais.AisTryAgainException;
import org.saforum.ais.Consts;
import org.saforum.ais.DispatchFlags;
import org.saforum.ais.Version;
import org.saforum.ais.ChangeStep;
import org.saforum.ais.CorrelationIds;
import org.saforum.ais.clm.ClmHandle;
import org.saforum.ais.clm.ClmHandleFactory;
import org.saforum.ais.clm.ClusterMembershipManager;
import org.saforum.ais.clm.ClusterNode;
import org.saforum.ais.clm.ClusterNotificationBuffer;
import org.saforum.ais.clm.GetClusterNodeCallback;
import org.saforum.ais.clm.TrackClusterCallback;
import java.io.InputStream;
import java.util.Properties;

public class DispatchHandleTests extends TestCase implements
		GetClusterNodeCallback, TrackClusterCallback {

	private ClmHandle handle;
	private SelectionObjectMediator mediator;
	private boolean isGetClusterNodeCallbackCalled = false;
	private boolean isTrackClusterCallbackCalled;
	private Properties properties = new Properties();
	private char releaseCode;
	private short majorVersion;
	private short minorVersion;

	public static void main(String[] args) {
		junit.textui.TestRunner.run(DispatchHandleTests.class);
		System.out.println("End.");
	}

	/**
	 * Test getInstance() method.
	 */
	public final void testCreateMediator() {
		mediator = SelectionObjectMediator.getInstance();
		assertNotNull(mediator);
	}

	/**
	 * Test getSelectableChannel() method.
	 */
	public final void testActivateHandle() {
		SelectableChannel channel = null;
		try {
			channel = handle.getSelectableChannel();
		} catch (AisException e) {
			e.printStackTrace();
		}
		assertNotNull(channel);
	}

	/**
	 * Test if sensing and dispatching an event works well. Start an
	 * asynchronous invocation and wait for callback to appear on the
	 * SelectableChannel, then do the dispatch.
	 */
	public final void testDispatchEvents() {
		Exception ex = null;
		ClusterMembershipManager clmManager = handle
				.getClusterMembershipManager();
		isGetClusterNodeCallbackCalled = false;

		try {
			clmManager.getClusterAsync();
			while (!handle.hasPendingCallback(Consts.SA_TIME_ONE_SECOND * 10));

			SelectableChannel channel = handle.getSelectableChannel();
			channel.configureBlocking(false);
			Selector selector = Selector.open();
			channel.register(selector, SelectionKey.OP_READ);
			selector.select();

			handle.dispatch(DispatchFlags.DISPATCH_ALL);
		} catch (AisException e) {
			e.printStackTrace();
			ex = e;
		} catch (IOException e1) {
			e1.printStackTrace();
			ex = e1;
		}

		assertNull(ex);
		assertTrue(isTrackClusterCallbackCalled);
	}

	/**
	 * Test multiple handles.
	 */
	public final void testMultipleDispatchEvents() {
		Exception ex = null;
		List<ClmHandle> handles = new ArrayList<ClmHandle>();
		List<SelectableChannel> channels = new ArrayList<SelectableChannel>();
		List<SelectionKey> selectionKeys = new ArrayList<SelectionKey>();

		final int numTestHandles = 20;

		System.out
				.println("Testing multiple handle event dispatching. Num of handles is "
						+ numTestHandles);

		try {
			Version b11Version = new Version(releaseCode, majorVersion,
					minorVersion);
			ClmHandleFactory clmHandleFactory = new ClmHandleFactory();
			ClmHandle.Callbacks callbackNullNull = new ClmHandle.Callbacks();
			callbackNullNull.getClusterNodeCallback = null;
			callbackNullNull.trackClusterCallback = this;

			Selector selector = Selector.open();

			for (int i = 0; i < numTestHandles; i++) {
				System.out.println("Creating handle nr. " + i);
				System.out.flush();
				ClmHandle localHandle = clmHandleFactory.initializeHandle(
						callbackNullNull, b11Version);

				// Do the async call
				localHandle.getClusterMembershipManager().getClusterAsync();

				SelectableChannel localChannel = localHandle
						.getSelectableChannel();
				localChannel.configureBlocking(false);
				SelectionKey localSelectionKey = localChannel.register(
						selector, SelectionKey.OP_READ);

				handles.add(localHandle);
				channels.add(localChannel);
				selectionKeys.add(localSelectionKey);
			}

			isGetClusterNodeCallbackCalled = false;

			int numDispatch = 0;

			while (numDispatch < numTestHandles) {
				selector.select();

				Set<SelectionKey> selectedKeys = selector.selectedKeys();
				Iterator<SelectionKey> it = selectedKeys.iterator();
				while (it.hasNext()) {
					SelectionKey key = it.next(); 
					for (int i = 0; i < selectionKeys.size(); i++) {
						SelectionKey selectionKey = selectionKeys.get(i);
						if (selectionKey.equals(key)) {
							it.remove(); // Indicate that processing has been started
							handles.get(i).dispatch(DispatchFlags.DISPATCH_ALL);
							numDispatch++;
						}
					}
				}
			}

			for (ClmHandle handle : handles) {
				handle.finalizeHandle();
				System.out.flush();
			}
		} catch (AisException e) {
			e.printStackTrace();
			ex = e;
		} catch (IOException e1) {
			e1.printStackTrace();
			ex = e1;
		}

		assertNull(ex);
		assertTrue(isTrackClusterCallbackCalled);
	}

	private Object synchronizationMutex = new Object();
	private int numThreadRuns = 0;
	private int successfulThreadRuns = 0;

	private class ChannelDispatchThread implements Runnable,
			TrackClusterCallback {

		private int serialNum;

		public ChannelDispatchThread(int serialNum) {
			this.serialNum = serialNum;
		}

		public void run() {
			Exception ex = null;
			boolean dispatched = false;
			try {
				// Sleep for random length
				try {
					Thread.sleep(new Random().nextInt(1000));
				} catch (InterruptedException e) {
				}
				Version b11Version = new Version(releaseCode, majorVersion,
						minorVersion);
				ClmHandleFactory clmHandleFactory = new ClmHandleFactory();
				ClmHandle.Callbacks callbackNullNull = new ClmHandle.Callbacks();
				callbackNullNull.getClusterNodeCallback = null;
				callbackNullNull.trackClusterCallback = this;

				Selector selector = Selector.open();

				ClmHandle localHandle = clmHandleFactory.initializeHandle(
						callbackNullNull, b11Version);

				try {
					Thread.sleep(new Random().nextInt(100));
				} catch (InterruptedException e) {
				}
				// Do the async call
				localHandle.getClusterMembershipManager().getClusterAsync();

				SelectableChannel localChannel = localHandle
						.getSelectableChannel();
				localChannel.configureBlocking(false);
				SelectionKey localSelectionKey = localChannel.register(
						selector, SelectionKey.OP_READ);

				isGetClusterNodeCallbackCalled = false;

				try {
					Thread.sleep(new Random().nextInt(100));
				} catch (InterruptedException e) {
				}
				// Select and wait at most 1 second.
				int keysNum = selector.select(1000);

				if (keysNum == 1) {
					localHandle.dispatch(DispatchFlags.DISPATCH_ALL);
					dispatched = true;
				}

				localHandle.finalizeHandle();

			} catch (AisException e) {
				System.out.println("Exception in thread: " + serialNum);
				e.printStackTrace();
				ex = e;
			} catch (IOException e1) {
				e1.printStackTrace();
				ex = e1;
			}

			synchronized (synchronizationMutex) {
				numThreadRuns++;
				if (ex == null && dispatched)
					successfulThreadRuns++;
			}
		}

		public void trackClusterCallback(
				ClusterNotificationBuffer notificationBuffer,
				int numberOfMembers, AisStatus error) {
			System.out.println("JAVA TEST THREAD: threaded (" + serialNum
					+ ") tackClusterCallback");

		}
		public void trackClusterCallback(
			ClusterNotificationBuffer notificationBuffer, int numberOfMembers,
			long invocation, String rootCauseEntity,  CorrelationIds correlationIds,
			ChangeStep step, long timeSupervision, AisStatus error ){
			System.out.println("JAVA TEST THREAD: threaded (" + serialNum
					+ ") tackClusterCallback");
        	}
	}

	/**
	 * Test multiple concurrent handles.
	 */
	public final void testMultipleDispatchEvents_Threaded() {
		final int numTestHandles = 20;

		for (int i = 0; i < numTestHandles; i++) {
			ChannelDispatchThread t = new ChannelDispatchThread(i);
			new Thread(t).start();
		}

		// wait a bit
		try {
			Thread.sleep(3000);
		} catch (InterruptedException e) {
		}

		assertTrue(numThreadRuns == successfulThreadRuns);
	}

	/**
	 * Test remove handle method.
	 */
	public final void testRemoveHandle() {
		Exception ex = null;
		try {
			handle.finalizeHandle();
			handle.getSelectableChannel();
		} catch (AisLibraryException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (AisTimeoutException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (AisTryAgainException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (AisBadHandleException e) {
			ex = e;
		} catch (AisNoResourcesException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (AisNoMemoryException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} 
		assertNotNull(ex);
	}


	/**
	 * Test multiple callbacks on the same handle.
	 */
	public final void testMultipleCallbacksOnSameHandle() {
		Exception ex = null;
		ClusterMembershipManager clmManager = handle
				.getClusterMembershipManager();
		int callbacksNum = 4; // Execute 4 consecutive requests.
		isGetClusterNodeCallbackCalled = false;

		try {
			SelectableChannel channel = handle.getSelectableChannel();
			channel.configureBlocking(false);
			Selector selector = Selector.open();
			SelectionKey key = channel.register(selector, SelectionKey.OP_READ);

			while (callbacksNum != 0) {
				clmManager.getClusterAsync();

				System.out.println("selector.select() = " + selector.select());
				selector.selectedKeys().clear();
				handle.dispatch(DispatchFlags.DISPATCH_ONE);

				callbacksNum--;

			}

			int last = selector.select(100);
			
			// System.out.println("selector.select(1000) = " + last);
			// System.out.println("key.isReadable()= " + key.isReadable() + "selector.selectedKeys().size()= " + selector.selectedKeys().size());
			assertTrue(last == 0);

		} catch (AisException e) {
			e.printStackTrace();
			ex = e;
		} catch (IOException e1) {
			e1.printStackTrace();
			ex = e1;
		}

		assertNull(ex);
		assertTrue(isTrackClusterCallbackCalled);
	}

	@Override
	protected void setUp() throws Exception {
		super.setUp();
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
		Version b11Version = new Version(releaseCode, majorVersion , minorVersion);

		ClmHandleFactory clmHandleFactory = new ClmHandleFactory();
		ClmHandle.Callbacks callbackNullNull = new ClmHandle.Callbacks();
		callbackNullNull.getClusterNodeCallback = null;
		callbackNullNull.trackClusterCallback = this;
		handle = clmHandleFactory
				.initializeHandle(callbackNullNull, b11Version);
		testCreateMediator();
	}

	@Override
	protected void tearDown() throws Exception {
		super.tearDown();
		try {
			handle.finalizeHandle();
		} catch (AisBadHandleException e) {
			System.out.println("Handle has already been finalized.");
		}
		handle = null;
	}

	public void getClusterNodeCallback(long invocation,
			ClusterNode clusterNode, AisStatus error) {
		System.out.println("getClusterNodeCallback");
		isGetClusterNodeCallbackCalled = true;
	}

	public void trackClusterCallback(
			ClusterNotificationBuffer notificationBuffer, int numberOfMembers,
			AisStatus error) {
//		System.out.println("trackClusterCallback");
		isTrackClusterCallbackCalled = true;
	}

	public void trackClusterCallback(
			ClusterNotificationBuffer notificationBuffer, int numberOfMembers,
			long invocation, String rootCauseEntity,  CorrelationIds correlationIds,
			ChangeStep step, long timeSupervision, AisStatus error ){
		System.out.println("JAVA TEST CALLBACK: Executing trackClusterCallback("
				+ notificationBuffer + "," + numberOfMembers + "," + invocation 
				+ "," + rootCauseEntity + "," + correlationIds + "," + step 
				+ "," + timeSupervision + "," + error + ")");
                isTrackClusterCallbackCalled = true;
	}

}
