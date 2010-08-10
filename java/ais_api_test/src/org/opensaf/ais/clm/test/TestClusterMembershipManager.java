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
import org.saforum.ais.AisException;
import org.saforum.ais.AisInitException;
import org.saforum.ais.AisInvalidParamException;
import org.saforum.ais.AisNotExistException;
import org.saforum.ais.AisStatus;
import org.saforum.ais.AisTimeoutException;
import org.saforum.ais.Consts;
import org.saforum.ais.DispatchFlags;
import org.saforum.ais.TrackFlags;
import org.saforum.ais.Version;
import org.saforum.ais.clm.ClmHandle;
import org.saforum.ais.clm.ClmHandleFactory;
import org.saforum.ais.clm.ClusterMembershipManager;
import org.saforum.ais.clm.ClusterNode;
import org.saforum.ais.clm.ClusterNotificationBuffer;
import org.saforum.ais.clm.GetClusterNodeCallback;
import org.saforum.ais.clm.NodeAddressIPv4;
import org.saforum.ais.clm.TrackClusterCallback;

import junit.framework.*;


// CLASS DEFINITION
/**
 *
 * @author Nokia Siemens Networks
 * @author Jozsef BIRO
 */
public class TestClusterMembershipManager extends TestCase implements
		GetClusterNodeCallback, TrackClusterCallback {

    // STATIC FIELDS

    public static final long GET_CLUSTER_NODE_TIMEOUT = ( 50 * Consts.SA_TIME_ONE_MICROSECOND );

    private static long s_invocation = 123;

    private static int INVALID_NODE_ID = 133;

    // STATIC METHODS

    public static void main(String[] args) {
		junit.textui.TestRunner.run(TestClusterMembershipManager.class);
	}

    /**
     * @param clmManager A Clm manager
     * @param printMsg Message to be printed before the data
     * @return Result of the getCluster() call
     */
    static ClusterNotificationBuffer s_callGetCluster(
			ClusterMembershipManager clmManager, String printMsg) {
		AisException _aisExc = null;
		ClusterNotificationBuffer _notificationBuffer = null;
		try {
			_notificationBuffer = clmManager.getCluster();
		} catch (AisException e) {
			_aisExc = e;
		}
		Assert.assertNotNull(_notificationBuffer);
		Utils.s_printClusterNotificationBuffer(_notificationBuffer, printMsg);
		Assert.assertNull(_aisExc);
		return _notificationBuffer;
	}

    /**
     * @param clmManager A Clm manager
     *
     */
    static void s_callGetClusterAsync(ClusterMembershipManager clmManager) {
		AisException _aisExc = null;
		try {
			clmManager.getClusterAsync();
		} catch (AisException e) {
			_aisExc = e;
		}
		Assert.assertNull(_aisExc);
	}

    /**
     * @param clmManager A Clm manager
     * @param timeout Timeout value for getClusterNode() in usecs
     * @param printMsg Message to be printed before the data
     * @return Resul of getClcusterNode() call
     */
    private ClusterNode s_callGetClusterNodeLocal(
			ClusterMembershipManager clmManager, long timeout, String printMsg) {
		AisException _aisExc = null;
		ClusterNode _cNode = null;
		try {
			_cNode = clmManager.getClusterNode(ClusterNode.LOCAL_NODE_ID,
					timeout);
		} catch (AisException e) {
			_aisExc = e;
		}
		Assert.assertNotNull(_cNode);
		Utils.s_printClusterNode(_cNode, printMsg);
		Assert.assertTrue(_cNode.nodeAddress instanceof NodeAddressIPv4);
		Assert.assertNull(_aisExc);
		return _cNode;
	}

    /**
     * @param clmManager A Clm manager
     *
     */
    static void s_callGetClusterNodeAsyncLocal(
			ClusterMembershipManager clmManager) {
		AisException _aisExc = null;
		TestClusterMembershipManager.s_invocation++;
		try {
			clmManager.getClusterNodeAsync(
					TestClusterMembershipManager.s_invocation,
					ClusterNode.LOCAL_NODE_ID);
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

    private ClusterMembershipManager clmManagerNullNull;

    private ClusterMembershipManager clmManagerNullOK;

    private ClusterMembershipManager clmManagerOKNull;

    private ClusterMembershipManager clmManagerOKOK;

    private ClusterNode clusterNode;

    private ClusterNotificationBuffer notificationBuffer;

    private boolean called_gCN_cb;

    /**
     * The value of the error parameter passed to getClusterNodeCallback.
     */
    private AisStatus gCN_cb_error;

    /**
     * The value of the invocation parameter passed to getClusterNodeCallback.
     */
    private long gCN_cb_invocation;

    /**
     * The value of the clusterNode parameter passed to getClusterNodeCallback.
     *
     */
    private ClusterNode gCN_cb_clusterNode;

    private boolean called_tC_cb;

    /**
     * The value of the notificationBuffer parameter passed to trackClusterCallback.
     */
    private ClusterNotificationBuffer tC_cb_notificationBuffer;

    /**
     * The value of the numberOfMembers parameter passed to trackClusterCallback.
     */
    private int tC_cb_numberOfMembers;

    /**
     * The value of the error parameter passed to trackClusterCallback.
     */
    private AisStatus tC_cb_error;

    /**
     */
    private boolean tC_cb_print;

    ClmHandle.Callbacks callbackNullNull;
    ClmHandle.Callbacks callbackNullOK;
    ClmHandle.Callbacks callbackOKNull;
    ClmHandle.Callbacks callbackOKOK;

    // CONSTRUCTORs

    /**
     * @param name
     */
    public TestClusterMembershipManager(String name) {
		super(name);
	}

    // INSTANCE METHODS

    protected void setUp() throws Exception {
		super.setUp();
		System.out.println("JAVA TEST: SETTING UP NEXT TEST...");
		aisExc = null;
		b11Version = new Version((char) 'B', (short) 0x01, (short) 0x01);

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

		// library without callbacks
		clmLibHandleNullNull = clmHandleFactory.initializeHandle(
				callbackNullNull, b11Version);
		clmManagerNullNull = clmLibHandleNullNull.getClusterMembershipManager();
		// library with
		clmLibHandleNullOK = clmHandleFactory.initializeHandle(callbackNullOK,
				b11Version);
		clmManagerNullOK = clmLibHandleNullOK.getClusterMembershipManager();
		// library without callbacks
		clmLibHandleOKNull = clmHandleFactory.initializeHandle(callbackOKNull,
				b11Version);
		clmManagerOKNull = clmLibHandleOKNull.getClusterMembershipManager();
		// library with both callbacks
		clmLibHandleOKOK = clmHandleFactory.initializeHandle(callbackOKOK,
				b11Version);
		clmManagerOKOK = clmLibHandleOKOK.getClusterMembershipManager();
		// callback parameters
		called_gCN_cb = false;
		called_tC_cb = false;
		gCN_cb_invocation = 0;
		gCN_cb_clusterNode = null;
		tC_cb_notificationBuffer = null;
		tC_cb_numberOfMembers = 0;
		notificationBuffer = null;
		clusterNode = null;
	}

    protected void tearDown() throws Exception {
		super.tearDown();
		System.out.println("JAVA TEST: CLEANING UP AFTER TEST...");
		aisExc = null;
		// library without callbacks
		Utils.s_finalizeClmLibraryHandle(clmLibHandleNullNull);
		clmLibHandleNullNull = null;
		clmManagerNullNull = null;
		// library with 1 callback:
		Utils.s_finalizeClmLibraryHandle(clmLibHandleOKNull);
		clmLibHandleOKNull = null;
		clmManagerOKNull = null;
		// library with 1 callback:
		Utils.s_finalizeClmLibraryHandle(clmLibHandleNullOK);
		clmLibHandleNullOK = null;
		clmManagerNullOK = null;
		// library with both callbacks
		Utils.s_finalizeClmLibraryHandle(clmLibHandleOKOK);
		clmLibHandleOKOK = null;
		clmManagerOKOK = null;
	}

    /*
     * Test method for 'ais.clm.ClusterMembershipManager.getCluster()'
     */
    public final void testGetCluster_Simple() {
		System.out.println("JAVA TEST: Executing testGetCluster_Simple()...");
		notificationBuffer = s_callGetCluster(
				// callbacks are required
				clmManagerOKOK, "JAVA TEST: Result of 1st cluster query: ");
	}

    /*
     * Test method for 'ais.clm.ClusterMembershipManager.getCluster()'
     */
    public final void testGetCluster_Equals() {
		System.out.println("JAVA TEST: Executing testGetCluster_Equals()...");
		// 1st call
		notificationBuffer = s_callGetCluster(
				// callbacks are required
				clmManagerOKOK, "JAVA TEST: Result of 1st cluster query: ");
		// 2nd call
		ClusterNotificationBuffer _notificationBuffer2;
		_notificationBuffer2 = s_callGetCluster(
				// callbacks are required
				clmManagerOKOK, "JAVA TEST: Result of 2nd cluster query: ");
		// check equality
		compareClusterNotificationBuffers(notificationBuffer,
				_notificationBuffer2);
		assertClusterNotificationBuffer(notificationBuffer);
		assertClusterNotificationBuffer(_notificationBuffer2);
	}

    /*
     * Test method for 'ais.clm.ClusterMembershipManager.getClusterAsync()'
     */
    public final void testGetClusterAsync_NoCallback() {
		// 1st call
		System.out.println("JAVA TEST: Executing testGetClusterNodeAsync_NoCallback()...");
		try {
			clmManagerNullNull.getClusterAsync();
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertTrue(aisExc instanceof AisInitException);
		// 2nd call
		aisExc = null;
		try {
			clmManagerOKNull.getClusterAsync();
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertTrue(aisExc instanceof AisInitException);
	}

    /*
     * Test method for 'ais.clm.ClusterMembershipManager.getClusterNodeAsync(long, int)'
     */
    public final void testGetClusterAsync_EqualsDispatchBlocking() {
        System.out.println( "JAVA TEST: Executing testGetClusterAsync_EqualsDispatchBlocking()..." );
        // sync call
		notificationBuffer = s_callGetCluster(
				// callbacks are required
				clmManagerOKOK, "JAVA TEST: Result of SYNC cluster query: ");
		// async call
		s_callGetClusterAsync(clmManagerNullOK);
		// blocking dispatch
		try {
			clmLibHandleNullOK.dispatchBlocking(); // MAY BLOCK FOREVER!!!
		} catch (AisException e) {
			aisExc = e;
		}
		// check callback
		assert_tC_cb_OK();
		// check equality
		compareClusterNotificationBuffers(notificationBuffer,
				tC_cb_notificationBuffer);
		assertClusterNotificationBuffer(notificationBuffer);
		assertClusterNotificationBuffer(tC_cb_notificationBuffer);
	}

    /*
     * Test method for 'ais.clm.ClusterMembershipManager.stopClusterTracking()'
    public final void testStopClusterTracking() {
        // TODO Auto-generated method stub

    }
     */

    /*static ClusterNotificationBuffer s_callGetCluster(
                                                       ClusterMembershipManager clmManager,
                                                       String printMsg ){
          AisException _aisExc = null;
          ClusterNotificationBuffer _notificationBuffer = null;
          try{
              _notificationBuffer = clmManager.getCluster();
          }
          catch( AisException e ){
              _aisExc = e;
          }
          Assert.assertNotNull( _notificationBuffer );
          Utils.s_printNotificationBuffer( _notificationBuffer, printMsg );
          Assert.assertNull( _aisExc );
          return _notificationBuffer;
      }*/

    /*
     * Test method for 'ais.clm.ClusterMembershipManager.startClusterTracking(byte,boolean)'
     */
    public final void testStartClusterTrackingBZ_EqualsDispatchBlocking() {
        System.out.println( "JAVA TEST: Executing testStartClusterTrackingBZ_EqualsDispatchBlocking()..." );
        System.out.println( "ENUM TEST (TRACK_CAHNGES): " + TrackFlags.TRACK_CHANGES.ordinal() );
        System.out.println( "ENUM TEST (TRACK_CHANGES_ONLY): " + TrackFlags.TRACK_CHANGES_ONLY.ordinal() );
        // sync call
		notificationBuffer = s_callGetCluster(
				// callbacks are required
				clmManagerOKOK, "JAVA TEST: Result of SYNC cluster query: ");
		// start tracking and ask for async clm info
		try {
			clmManagerOKOK.getClusterAsyncThenStartTracking(TrackFlags.TRACK_CHANGES);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(aisExc);
		// blocking dispatch
		try {
			clmLibHandleOKOK.dispatchBlocking(); // MAY BLOCK FOREVER!!!
		} catch (AisException e) {
			aisExc = e;
		}
		// check callback
		assert_tC_cb_OK();
		// check equality
		compareClusterNotificationBuffers(notificationBuffer,
				tC_cb_notificationBuffer);
		assertClusterNotificationBuffer(notificationBuffer);
		assertClusterNotificationBuffer(tC_cb_notificationBuffer);
		// stop tracking
		try {
			clmManagerOKOK.stopClusterTracking();
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(aisExc);
    }

    /*
     * Test method for 'ais.clm.ClusterMembershipManager.startClusterTracking(byte,boolean)'
     */
    public final void testStartClusterTrackingB_Equals() {
        System.out.println( "JAVA TEST: Executing testStartClusterTrackingB_Equals()..." );
        // sync call
		notificationBuffer = s_callGetCluster(
				// callbacks are required
				clmManagerOKOK, "JAVA TEST: Result of SYNC cluster query: ");
		// start tracking and ask for sync clm info
		ClusterNotificationBuffer _notificationBuffer = null;
		try {
			_notificationBuffer = clmManagerOKOK
					.getClusterThenStartTracking(TrackFlags.TRACK_CHANGES);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNotNull(_notificationBuffer);
		Utils.s_printClusterNotificationBuffer(_notificationBuffer,
				"JAVA TEST: Result of startClusterTracking(SYNC) query: ");
		Assert.assertNull(aisExc);
		// check equality
		compareClusterNotificationBuffers(notificationBuffer,
				_notificationBuffer);
		assertClusterNotificationBuffer(notificationBuffer);
		assertClusterNotificationBuffer(_notificationBuffer);
		// stop tracking
		try {
			clmManagerOKOK.stopClusterTracking();
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(aisExc);
    }

    /*
     * Test method for 'ais.clm.ClusterMembershipManager.getClusterNode(int, long)'
     */
    public final void testGetClusterNode_LocalEquals() {
        System.out.println( "JAVA TEST: Executing testGetClusterNode_LocalEquals()..." );
        // 1st call
		clusterNode = s_callGetClusterNodeLocal(clmManagerNullNull,
				GET_CLUSTER_NODE_TIMEOUT,
				"JAVA TEST: Result of 1st cluster node query: ");
		// 2nd call
		ClusterNode _clusterNode2 = s_callGetClusterNodeLocal(
				clmManagerNullNull, GET_CLUSTER_NODE_TIMEOUT,
				"JAVA TEST: Result of 2nd cluster node query: ");
		// check equality
		compareClusterNodes(clusterNode, _clusterNode2);
		// 3rd call
		ClusterNode _clusterNode3 = s_callGetClusterNodeLocal(clmManagerOKOK,
				GET_CLUSTER_NODE_TIMEOUT,
				"JAVA TEST: Result of 3rd cluster node query: ");
		// check equality
		compareClusterNodes(clusterNode, _clusterNode3);
    }

    /*
     * Test method for 'ais.clm.ClusterMembershipManager.getClusterNode(int, long)'
     */
    public final void testGetClusterNode_LocalTimeout() {
        System.out
				.println("JAVA TEST: Executing testGetClusterNode_LocalTimeout()...");
		try {
			clusterNode = clmManagerNullNull.getClusterNode(
					ClusterNode.LOCAL_NODE_ID, 0);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(clusterNode);
		Assert.assertTrue(aisExc instanceof AisTimeoutException);
    }

    /*
     * Test method for 'ais.clm.ClusterMembershipManager.getClusterNode(int, long)'
     */
    public final void testGetClusterNode_InvalidNodeId() {
        System.out.println( "JAVA TEST: Executing testGetClusterNode_InvalidNodeId()..." );
        try {
			clusterNode = clmManagerNullNull.getClusterNode(INVALID_NODE_ID, 1);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(clusterNode);
		Assert.assertTrue(aisExc instanceof AisNotExistException);
    }

    /*
     * Test method for 'ais.clm.ClusterMembershipManager.getClusterNodeAsync(long, int)'
     */
    public final void testGetClusterNodeAsync_NoCallback() {
        // 1st call
        System.out.println( "JAVA TEST: Executing testGetClusterNodeAsync_NoCallback()..." );
        try {
			clmManagerNullNull
					.getClusterNodeAsync(1, ClusterNode.LOCAL_NODE_ID);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertTrue(aisExc instanceof AisInitException);
		// 2nd call
		aisExc = null;
		try {
			clmManagerNullOK.getClusterNodeAsync(1, ClusterNode.LOCAL_NODE_ID);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertTrue(aisExc instanceof AisInitException);
    }

    /*
     * Test method for 'ais.clm.ClusterMembershipManager.getClusterNodeAsync(long, int)'
     */
    public final void testGetClusterNodeAsync_EqualsSleep() {
        System.out.println( "JAVA TEST: Executing testGetClusterNodeAsync_EqualsSleep()..." );
        // sync call
		clusterNode = s_callGetClusterNodeLocal(clmManagerNullNull,
				GET_CLUSTER_NODE_TIMEOUT,
				"JAVA TEST: Result of SYNC cluster node query: ");
		// async call
		s_callGetClusterNodeAsyncLocal(clmManagerOKNull);

		// wait 50 milliseconds
		try {
			Thread.sleep(50);
		} catch (InterruptedException e) {
		}
		// call dispatch
		boolean _pending = false;
		try {
			_pending = clmLibHandleOKNull.hasPendingCallback();
			clmLibHandleOKNull.dispatch(DispatchFlags.DISPATCH_ONE);
		} catch (AisException e) {
			aisExc = e;
		}

		Assert.assertTrue(_pending);
		// check callback
		assert_gCN_cb_OK(TestClusterMembershipManager.s_invocation);
		// check equality
		compareClusterNodes(clusterNode, gCN_cb_clusterNode);
    }

    /*
     * Test method for 'ais.clm.ClusterMembershipManager.getClusterNodeAsync(long, int)'
     */
    public final void testGetClusterNodeAsync_EqualsPoll() {
        System.out.println( "JAVA TEST: Executing testGetClusterNodeAsync_EqualsPoll()..." );
        // sync call
		clusterNode = s_callGetClusterNodeLocal(clmManagerNullNull,
				GET_CLUSTER_NODE_TIMEOUT,
				"JAVA TEST: Result of SYNC cluster node query: ");
		// async call
		s_callGetClusterNodeAsyncLocal(clmManagerOKNull);
		// poll
		try {
			while (!clmLibHandleOKNull.hasPendingCallback()) {
				// MAY BLOCK FOREVER!!!
			}
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertNull(aisExc);
		// call dispatch
		try {
			clmLibHandleOKNull.dispatch(DispatchFlags.DISPATCH_ONE);
		} catch (AisException e) {
			aisExc = e;
		}
		// check callback
		assert_gCN_cb_OK(TestClusterMembershipManager.s_invocation);
		// check equality
		compareClusterNodes(clusterNode, gCN_cb_clusterNode);
    }

    /*
     * Test method for 'ais.clm.ClusterMembershipManager.getClusterNodeAsync(long, int)'
     */
    public final void testGetClusterNodeAsync_EqualsPollWithTimeout() {
        System.out.println( "JAVA TEST: Executing testGetClusterNodeAsync_EqualsPollWithTimeout()..." );
        // sync call
		clusterNode = s_callGetClusterNodeLocal(clmManagerNullNull,
				GET_CLUSTER_NODE_TIMEOUT,
				"JAVA TEST: Result of SYNC cluster node query: ");
		// async call
		s_callGetClusterNodeAsyncLocal(clmManagerOKNull);
		// poll with timeout
		boolean _pending = false;
		long _before = 0;
		long _after = 0;
		try {
			_before = System.currentTimeMillis();
			_pending = clmLibHandleOKNull
					.hasPendingCallback(GET_CLUSTER_NODE_TIMEOUT);
			_after = System.currentTimeMillis();
			clmLibHandleOKNull.dispatch(DispatchFlags.DISPATCH_ONE);
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertTrue(_pending);
		Assert.assertFalse(Utils.s_isDurationTooLong(_before, _after,
				GET_CLUSTER_NODE_TIMEOUT, 5));
		// check callback
		assert_gCN_cb_OK(TestClusterMembershipManager.s_invocation);
		// check equality
		compareClusterNodes(clusterNode, gCN_cb_clusterNode);
    }

    /*
     * Test method for 'ais.clm.ClusterMembershipManager.getClusterNodeAsync(long, int)'
     */
    public final void testGetClusterNodeAsync_EqualsDispatchBlocking() {
        System.out.println( "JAVA TEST: Executing testGetClusterNodeAsync_EqualsDispatchBlocking()..." );
        // sync call
		clusterNode = s_callGetClusterNodeLocal(clmManagerNullNull,
				GET_CLUSTER_NODE_TIMEOUT,
				"JAVA TEST: Result of SYNC cluster node query: ");
		// async call
		s_callGetClusterNodeAsyncLocal(clmManagerOKNull);
		// blocking dispatch
		try {
			clmLibHandleOKNull.dispatchBlocking(); // MAY BLOCK FOREVER!!!
		} catch (AisException e) {
			aisExc = e;
		}
		// check callback
		assert_gCN_cb_OK(TestClusterMembershipManager.s_invocation);
		// check equality
		compareClusterNodes(clusterNode, gCN_cb_clusterNode);
    }

    /*
     * Test method for 'ais.clm.ClusterMembershipManager.getClusterNodeAsync(long, int)'
     */
    public final void testGetClusterNodeAsync_EqualsDispatchWithTimeout() {
        System.out.println( "JAVA TEST: Executing testGetClusterNodeAsync_EqualsDispatchWithTimeout()..." );
        // sync call
		clusterNode = s_callGetClusterNodeLocal(clmManagerNullNull,
				GET_CLUSTER_NODE_TIMEOUT,
				"JAVA TEST: Result of SYNC cluster node query: ");
		// async call
		s_callGetClusterNodeAsyncLocal(clmManagerOKNull);
		// call dispatch with timeout
		long _before = 0;
		long _after = 0;
		try {
			_before = System.currentTimeMillis();
			clmLibHandleOKNull.dispatchBlocking(GET_CLUSTER_NODE_TIMEOUT);
			_after = System.currentTimeMillis();
		} catch (AisException e) {
			aisExc = e;
		}
		Assert.assertFalse(Utils.s_isDurationTooLong(_before, _after,
				GET_CLUSTER_NODE_TIMEOUT, 5));
		// check callback
		assert_gCN_cb_OK(TestClusterMembershipManager.s_invocation);
		// check equality
		compareClusterNodes(clusterNode, gCN_cb_clusterNode);
    }

    /*
     * Test method for 'ais.clm.ClmLibraryHandle.finalizeHandle()'
     *
    public final void testFinalizeHandle() {
        System.out.println( "JAVA TEST: Executing testFinalizeHandle()..." );
        // finalize library handle
        try{
            clmLibHandleOKOK.finalizeHandle();
        }
        catch( AisException e ){
            aisExc = e;
        }
        Assert.assertNotNull( clmLibHandleOKOK );
        Assert.assertNull( aisExc );
        // test other handle operations!
        // getCluster
        ClusterNotificationBuffer _notificationBuffer = null;
        try{
            _notificationBuffer = clmManagerOKOK.getCluster();
        }
        catch( AisException e ){
            aisExc = e;
        }
        Assert.assertNull( _notificationBuffer );
        Assert.assertTrue( aisExc instanceof AisBadHandleException );
        aisExc = null;
        // startClusterTracking
        _notificationBuffer = null;
        try{
            _notificationBuffer = clmManagerOKOK.startClusterTracking( TrackFlags.TRACK_CHANGES );
        }
        catch( AisException e ){
            aisExc = e;
        }
        Assert.assertTrue( aisExc instanceof AisBadHandleException );
        aisExc = null;
        // getClusterAsync
        try{
            clmManagerOKOK.getClusterAsync();
        }
        catch( AisException e ){
            aisExc = e;
        }
        Assert.assertNull( _notificationBuffer );
        Assert.assertTrue( aisExc instanceof AisBadHandleException );
        aisExc = null;
        // getClusterAsync
        try{
            clmManagerOKOK.getClusterAsync();
        }
        catch( AisException e ){
            aisExc = e;
        }
        Assert.assertTrue( aisExc instanceof AisBadHandleException );
        aisExc = null;
        // startClusterTracking
        try{
            clmManagerOKOK.startClusterTracking( TrackFlags.TRACK_CHANGES, false );
        }
        catch( AisException e ){
            aisExc = e;
        }
        Assert.assertTrue( aisExc instanceof AisBadHandleException );
        aisExc = null;
        // stopClusterTracking
        try{
            clmManagerOKOK.stopClusterTracking();
        }
        catch( AisException e ){
            aisExc = e;
        }
        Assert.assertTrue( aisExc instanceof AisBadHandleException );
        aisExc = null;
        // getClusterNode
        ClusterNode _cNode = null;
        try{
            _cNode = clmManagerOKOK.getClusterNode( ClusterNode.LOCAL_NODE_ID, GET_CLUSTER_NODE_TIMEOUT );
        }
        catch( AisException e ){
            aisExc = e;
        }
        Assert.assertNull( _cNode );
        Assert.assertTrue( aisExc instanceof AisBadHandleException );
        aisExc = null;
        // getClusterNodeAsync
        try{
            clmManagerOKOK.getClusterNodeAsync( 1, ClusterNode.LOCAL_NODE_ID );
        }
        catch( AisException e ){
            aisExc = e;
        }
        Assert.assertTrue( aisExc instanceof AisBadHandleException );
        // tear down...
        clmLibHandleOKOK = null;
    }
     */



    // MISC INSTANCE METHODS

    /*
     * Test method for 'ais.clm.ClusterMembershipManager.startClusterTracking(byte,boolean)'
     */
    public final void testStartClusterTrackingB_WaitForever() {
        System.out.println( "JAVA TEST: Executing testStartClusterTrackingB_WaitForever()..." );
        // 1st try:
        /*
        try{
            clmManagerOKOK.startClusterTracking( TrackFlags.TRACK_CHANGES, false );
        }
        catch( AisException e ){
            aisExc = e;
        }
        Assert.assertNull( aisExc );
        // blocking dispatch
        try{
            tC_cb_print = true;
            while( true ){
                System.out.println( "JAVA TEST: Calling dispatch()..." );
                clmLibHandleOKOK.dispatch( DispatchFlags.DISPATCH_BLOCKING ); // WILL BLOCK FOREVER!!!
            }
        }catch( AisException e ){
            aisExc = e;
        }
        */
    }

    /**
     *
     */
    private void assert_tC_cb_OK() {
        Assert.assertFalse(called_gCN_cb);
		Assert.assertTrue(called_tC_cb);
		Assert.assertNotNull(tC_cb_notificationBuffer);
		Assert.assertEquals(tC_cb_numberOfMembers,
				tC_cb_notificationBuffer.notifications.length);
		Assert.assertEquals(tC_cb_error, AisStatus.OK);
		printTCcbParameters("JAVA TEST: Result of ASYNC cluster query: ");
		Assert.assertNull(aisExc);
		called_tC_cb = false;
    }

    /**
     *
     */
    private void compareClusterNotificationBuffers(
			ClusterNotificationBuffer notBuf1, ClusterNotificationBuffer notBuf2) {
		Assert.assertNotSame(notBuf1, notBuf2);
		Assert.assertEquals(notBuf1.notifications.length,
				notBuf2.notifications.length);
		for (int _idx = 0; _idx < notBuf1.notifications.length; _idx++) {
			Assert.assertEquals(notBuf1.notifications[_idx].clusterChange,
					notBuf2.notifications[_idx].clusterChange);
			compareClusterNodes(notBuf1.notifications[_idx].clusterNode,
					notBuf2.notifications[_idx].clusterNode);
		}
	}

    private void assertClusterNotificationBuffer(
			ClusterNotificationBuffer notBuf) {
		for (int _idx = 0; _idx < notBuf.notifications.length; _idx++) {
			switch (notBuf.notifications[_idx].clusterChange) {
			case NO_CHANGE:
			case JOINED:
			case LEFT:
			case RECONFIGURED:
				break;
			default:
				Assert.fail("Invalid cluster change value...");
			}
		}
	}

    private void assert_gCN_cb_OK(long invocation) {
		Assert.assertTrue(called_gCN_cb);
		Assert.assertEquals(invocation, gCN_cb_invocation);
		Assert.assertFalse(called_tC_cb);
		Assert.assertNotNull(gCN_cb_clusterNode);
		printGCNcbParameters("JAVA TEST: Result of ASYNC cluster node query: ");
		Assert.assertTrue(gCN_cb_clusterNode.nodeAddress instanceof NodeAddressIPv4);
		Assert.assertNull(aisExc);
		called_gCN_cb = false;
	}

    private void compareClusterNodes(ClusterNode cNode1, ClusterNode cNode2) {
		Assert.assertNotSame(cNode1, cNode2);
		Assert.assertEquals(cNode1.nodeId, cNode2.nodeId);
		Assert.assertNotSame(cNode1.nodeAddress, cNode2.nodeAddress);
		Assert.assertEquals(cNode1.nodeAddress.toString(), cNode2.nodeAddress
				.toString());
		Assert.assertEquals(cNode1.nodeName, cNode2.nodeName);
		Assert.assertEquals(cNode1.member, cNode2.member);
		// TODO currently not passing, waiting for FS feedback
		// Assert.assertEquals( cNode1.bootTimestamp,
		// cNode2.bootTimestamp );
		Assert.assertEquals(cNode1.initialViewNumber, cNode2.initialViewNumber);
	}

    public void getClusterNodeCallback(long invocation,
			ClusterNode clusterNode, AisStatus error) {
		System.out.println("JAVA TEST CALLBACK: Executing getClusterNodeCallback()...");
		called_gCN_cb = true;
		gCN_cb_invocation = invocation;
		gCN_cb_clusterNode = clusterNode;
		gCN_cb_error = error;
	}

    public void trackClusterCallback(
			ClusterNotificationBuffer notificationBuffer, int numberOfMembers,
			AisStatus error) {
		System.out.println("JAVA TEST CALLBACK: Executing trackClusterCallback( "
				+ notificationBuffer + ", " + numberOfMembers + ", "
				+ error + " )...");
		called_tC_cb = true;
		tC_cb_notificationBuffer = notificationBuffer;
		tC_cb_numberOfMembers = numberOfMembers;
		tC_cb_error = error;
		if (tC_cb_print) {
			printTCcbParameters("JAVA TEST CALLBACK: Cluster Info in callback");
		}
	}

    private void printGCNcbParameters( String msg ){
        System.out.println( msg );
        System.out.println("JAVA TEST: invocation = " + gCN_cb_invocation );
        Utils.s_printClusterNode( gCN_cb_clusterNode, "JAVA TEST: ClusterNode info: " );
        System.out.println("JAVA TEST: error = " + gCN_cb_error );
    }

    private void printTCcbParameters( String msg ){
        System.out.println( msg );
        System.out.println("JAVA TEST: number of members = " + tC_cb_numberOfMembers );
        System.out.println("JAVA TEST: error = " + tC_cb_error );
        Utils.s_printClusterNotificationBuffer( tC_cb_notificationBuffer, "JAVA TEST: Cluster info: " );
    }



}
