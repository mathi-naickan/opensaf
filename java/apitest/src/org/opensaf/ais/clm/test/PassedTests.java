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

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;


// CLASS DEFINITION
public class PassedTests {

    // NOTE that Eclipse does not use this method...
    public static void main( String[] args ) {
        junit.textui.TestRunner.run( PassedTests.suite() );
    }

    public static Test suite() {
        TestSuite _suite = new TestSuite( "Passed tests for ais.clm.test" );
        //$JUnit-BEGIN$

        // CLASS TestClmLibraryHandle
        /*
        try {
            _suite.addTestSuite( Class.forName("ais.clm.test.TestClmLibraryHandle") );
        } catch ( ClassNotFoundException e ) {
        }
        */
        
        _suite.addTest(new TestClmLibraryHandle("testInitializeHandle_nullVersion"));
        _suite.addTest(new TestClmLibraryHandle("testInitializeHandle_A11Version"));
        _suite.addTest(new TestClmLibraryHandle("testInitializeHandle_B11Version"));
        _suite.addTest(new TestClmLibraryHandle("testInitializeHandle_B18Version"));
        _suite.addTest(new TestClmLibraryHandle("testInitializeHandle_B21Version"));
        _suite.addTest(new TestClmLibraryHandle("testInitializeHandle_C11Version"));
        _suite.addTest(new TestClmLibraryHandle("testInitializeHandle_TwoHandles"));
        _suite.addTest(new TestClmLibraryHandle("testSelect_InvalidParam"));
        _suite.addTest(new TestClmLibraryHandle("testSelectNow_NoRegisteredCB"));
        _suite.addTest(new TestClmLibraryHandle("testSelect_Timeout"));
        _suite.addTest(new TestClmLibraryHandle("testHasPendingCallback_NoRegisteredCB"));
        _suite.addTest(new TestClmLibraryHandle("testHasPendingCallback_NoCB"));
        _suite.addTest(new TestClmLibraryHandle("testHasPendingCallback_Timeout"));
        _suite.addTest(new TestClmLibraryHandle("testDispatch_NoRegisteredCB"));
        _suite.addTest(new TestClmLibraryHandle("testDispatch_NoCB"));
        _suite.addTest(new TestClmLibraryHandle("testDispatchBlocking_Timeout"));
        // TODO currently not passing, AP for myself
        _suite.addTest(new TestClmLibraryHandle("testFinalizeHandle")); 

        // CLASS TestClusterMembershipManager
        _suite.addTestSuite( org.opensaf.ais.clm.test.TestClusterMembershipManager.class );
        /* 
        _suite.addTest( new TestClusterMembershipManager("testGetCluster_Simple"));
        _suite.addTest( new TestClusterMembershipManager("testGetClusterZ_Simple"));
        _suite.addTest( new TestClusterMembershipManager("testGetCluster_Equals"));
        _suite.addTest( new TestClusterMembershipManager("testGetClusterZ_Equals"));
        _suite.addTest( new TestClusterMembershipManager("testGetClusterAsync_NoCallback"));
        _suite.addTest( new TestClusterMembershipManager("testGetClusterAsync_EqualsDispatchBlocking"));
        _suite.addTest( new TestClusterMembershipManager("testStartClusterTrackingBZ_EqualsDispatchBlocking"));
        _suite.addTest( new TestClusterMembershipManager("testStartClusterTrackingB_Equals"));
        _suite.addTest( new TestClusterMembershipManager("testGetClusterAsyncZ_NoCallback"));
        _suite.addTest( new TestClusterMembershipManager("testGetClusterAsyncZ_EqualsDispatchBlocking"));
        _suite.addTest( new TestClusterMembershipManager("testgetClusterAsyncThenStartTracking_EqualsDispatchBlocking"));
        _suite.addTest( new TestClusterMembershipManager("testgetClusterAsyncThenStartTrackingTFBI_EqualsDispatchBlocking"));       
        _suite.addTest( new TestClusterMembershipManager("testgetClusterThenStartTracking_Equals"));
        _suite.addTest( new TestClusterMembershipManager("testgetClusterThenStartTrackingBZTS_Equals"));
        _suite.addTest( new TestClusterMembershipManager("testGetClusterNode_LocalEquals"));
        _suite.addTest( new TestClusterMembershipManager("testGetClusterNode_LocalTimeout"));
        _suite.addTest( new TestClusterMembershipManager("testGetClusterNode_InvalidNodeId"));
        _suite.addTest( new TestClusterMembershipManager("testGetClusterNodeAsync_NoCallback"));
        _suite.addTest( new TestClusterMembershipManager("testGetClusterNodeAsync_EqualsSleep"));
        _suite.addTest( new TestClusterMembershipManager("testGetClusterNodeAsync_EqualsPoll"));
        _suite.addTest( new TestClusterMembershipManager("testGetClusterNodeAsync_EqualsPollWithTimeout"));
        _suite.addTest( new TestClusterMembershipManager("testGetClusterNodeAsync_EqualsDispatchBlocking"));
        _suite.addTest( new TestClusterMembershipManager("testGetClusterNodeAsync_EqualsDispatchWithTimeout"));
        _suite.addTest( new TestClusterMembershipManager("testFinalizeHandle"));
        */

        // CLASS TestClmMultiThreading
        _suite.addTestSuite( org.opensaf.ais.clm.test.TestClmMultiThreading.class );
        /*
        _suite.addTest( new TestClmMultiThreading("testHasPendingCallback_Threads"));
        _suite.addTest( new TestClmMultiThreading("testHasPendingCallbackTimeout_Threads"));
        _suite.addTest( new TestClmMultiThreading("testDispatchBlocking_Threads2"));
        _suite.addTest( new TestClmMultiThreading("testDispatchBlocking_Threads3"));
        _suite.addTest( new TestClmMultiThreading("testDispatchBlocking_Threads6"));
        _suite.addTest( new TestClmMultiThreading("testDispatchBlocking_Threads9"));
        */

        //$JUnit-END$
        return _suite;
    }

}
