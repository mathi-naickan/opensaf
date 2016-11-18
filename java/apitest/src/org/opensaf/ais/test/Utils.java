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

import junit.framework.Assert;
import org.saforum.ais.*;
import org.saforum.ais.amf.*;
import org.saforum.ais.clm.*;

// CLASS DEFINITION
/**
 *
 * @author Nokia Siemens Networks
 * @author Jozsef BIRO
 */
public class Utils {

	// STATIC METHODS

	public static boolean s_isDurationTooLong(long javaBefore, long javaAfter,
			long saTimeout, long toleratedDiff) {
		System.out.println("JAVA TEST: isDurationTooLong( " + javaBefore + ", "
				+ javaAfter + ", " + saTimeout + ", " + toleratedDiff
				+ " )...");
		long _javaPeriod = javaAfter - javaBefore;
		System.out.println("JAVA TEST: Java period in milliseconds: "
				+ _javaPeriod);
		System.out.println("JAVA TEST: timeout in milliseconds: "
				+ (saTimeout / Consts.SA_TIME_ONE_MILLISECOND));
		if (_javaPeriod > ((saTimeout / Consts.SA_TIME_ONE_MILLISECOND) + toleratedDiff)) {
			return true;
		}
		return false;
	}

	public static boolean s_isDurationTooShort(long javaBefore, long javaAfter,
			long saTimeout, long toleratedDiff) {
		System.out.println("JAVA TEST: isDurationTooShort( " + javaBefore
				+ ", " + javaAfter + ", " + saTimeout + ", " + toleratedDiff
				+ " )...");
		long _javaPeriod = javaAfter - javaBefore;
		System.out.println("JAVA TEST: Java period in milliseconds: "
				+ _javaPeriod);
		System.out.println("JAVA TEST: timeout in milliseconds: "
				+ (saTimeout / Consts.SA_TIME_ONE_MILLISECOND));
		if (_javaPeriod < ((saTimeout / Consts.SA_TIME_ONE_MILLISECOND) - toleratedDiff)) {
			return true;
		}
		return false;
	}

	/*
	 * public static String s_toString( HaState haState ){ if( ) }
	 */

	public static void s_finalizeClmLibraryHandle(ClmHandle clmLibH) {
		if (clmLibH != null) {
			try {
				System.out.println("JAVA TEST (Utils.java): About to call finalizeHandle() for clmLibH...");
				clmLibH.finalizeHandle();
				System.out.println("JAVA TEST (Utils.java): finalizeHandle() for clmLibH returned...");
			} catch (AisException e) {
				System.out.println("JAVA TEST (Utils.java): finalizeHandle() for clmLibH threw an exception...");
			}
		}
	}

//	 public static void s_finalizeCkptLibraryHandle(CkptHandle ckptLibHandle) {
//		if (ckptLibHandle != null) {
//			try {
//				ckptLibHandle.finalizeHandle();
//			} catch (AisException e) {
//			}
//		}
//	}

	public static void s_finalizeAmfLibraryHandle(AmfHandle amfLibHandle) {
		if (amfLibHandle != null) {
			try {
				amfLibHandle.finalizeHandle();
			} catch (AisException e) {
			}
		}
	}

	// Printing

	public static void s_printByteArrayAsString(byte[] byteArray, String msg) {
		if (byteArray == null) {
			System.out.println("JAVA TEST: " + msg + " is null ");
		} else {
			System.out.println("JAVA TEST: " + msg + " (length: "
					+ byteArray.length + "): >>"
					+ new String(byteArray) + "<<");
		}
	}

	// Printing for CLM

	public static void s_printClusterNode(ClusterNode cNode, String msg) {
		System.out.println(msg);
		System.out.println("JAVA TEST: \t\t\t.nodeId: " + cNode.nodeId);
		System.out.println("JAVA TEST: \t\t\t.nodeAddress: "
				+ cNode.nodeAddress);
		System.out.println("JAVA TEST: \t\t\t.nodeName: " + cNode.nodeName);
		System.out.println("JAVA TEST: \t\t\t.member: " + cNode.member);
		System.out.println("JAVA TEST: \t\t\t.bootTimestamp: "
				+ cNode.bootTimestamp);
		System.out.println("JAVA TEST: \t\t\t.initialViewNumber: "
				+ cNode.initialViewNumber);
	}

	public static void s_printClusterNode_4(ClusterNode cNode, String msg) {
		System.out.println(msg);
		System.out.println("JAVA TEST: \t\t\t.nodeId: " + cNode.nodeId);
		System.out.println("JAVA TEST: \t\t\t.nodeAddress: "
					+ cNode.nodeAddress);
		System.out.println("JAVA TEST: \t\t\t.nodeName: " + cNode.nodeName);
		System.out.println("JAVA TEST: \t\t\t.member: " + cNode.member);
		System.out.println("JAVA TEST: \t\t\t.executionEnvironment: " + cNode.executionEnvironment);
		System.out.println("JAVA TEST: \t\t\t.bootTimestamp: "
				 + cNode.bootTimestamp);
		System.out.println("JAVA TEST: \t\t\t.initialViewNumber: "
				+ cNode.initialViewNumber);
        }


	public static void s_printClusterNotificationBuffer(
			ClusterNotificationBuffer notBuf, String msg) {
		System.out.println(msg);
		System.out.println("JAVA TEST: \tClusterNotificationBuffer.viewNumber: "
				+ notBuf.viewNumber);
		for (int _idx = 0; _idx < notBuf.notifications.length; _idx++) {
			System.out.println("JAVA TEST: \tClusterNotificationBuffer.notifications["
					+ _idx + "]:");
			switch (notBuf.notifications[_idx].clusterChange) {
			case NO_CHANGE:
				System.out.println("JAVA TEST: \t\tclusterChange: NO_CHANGE");
				break;
			case JOINED:
				System.out.println("JAVA TEST: \t\tclusterChange: JOINED");
				break;
			case LEFT:
				System.out.println("JAVA TEST: \t\tclusterChange: LEFT");
				break;
			case RECONFIGURED:
				System.out.println("JAVA TEST: \t\tclusterChange: RECONFIGURED");
				break;
			default:
				System.out.println("JAVA TEST: \t\tclusterChange: INVALID VALUE!!! :-(");
				// Assert.assertTrue( false );
				break;
			}
			s_printClusterNode(notBuf.notifications[_idx].clusterNode,
					"JAVA TEST: \t\tclusterNode: ");
		}
	}
	
	public static void s_printClusterNotificationBuffer_4(
			ClusterNotificationBuffer notBuf, String msg) {
		System.out.println(msg);
		System.out.println("JAVA TEST: \tClusterNotificationBuffer.viewNumber: "
				+ notBuf.viewNumber);
		for (int _idx = 0; _idx < notBuf.notifications.length; _idx++) {
			System.out.println("JAVA TEST: \tClusterNotificationBuffer.notifications["
				+ _idx + "]:");
			switch (notBuf.notifications[_idx].clusterChange) {
				case NO_CHANGE:
						System.out.println("JAVA TEST: \t\tclusterChange: NO_CHANGE");
						break;
				case JOINED:
						System.out.println("JAVA TEST: \t\tclusterChange: JOINED");
						break;
				case LEFT:
						System.out.println("JAVA TEST: \t\tclusterChange: LEFT");
						break;
				case RECONFIGURED:
						System.out.println("JAVA TEST: \t\tclusterChange: RECONFIGURED");
						break;
				case UNLOCK:
						System.out.println("JAVA TEST: \t\tclusterChange: UNLOCK");
						break;
				case SHUTDOWN:
						System.out.println("JAVA TEST: \t\tclusterChange: SHUTDOWN");
						break;
				default:
					System.out.println("JAVA TEST: \t\tclusterChange: INVALID VALUE!!! :-(");
	                             // Assert.assertTrue( false );
						break;
			}
			s_printClusterNode_4(notBuf.notifications[_idx].clusterNode,
					"JAVA TEST: \t\tclusterNode: ");
		}
	}


	// Printing for AMF

	public static void s_printCsiDescriptor(CsiDescriptor csiDescriptor,
			HaState haState, String msg) {
		System.out.println(msg);
		System.out.println("JAVA TEST: \tCsiDescriptor.csiFlags: "
				+ csiDescriptor.csiFlags);
		if (csiDescriptor.csiFlags != CsiDescriptor.CsiFlags.TARGET_ALL) {
			Assert.assertNotNull(csiDescriptor.csiName);
			System.out.println("JAVA TEST: \tCsiDescriptor.csiName: "
					+ csiDescriptor.csiName);
		} else {
			Assert.assertNull(csiDescriptor.csiName);
		}
		// csi state descriptor
		if (haState == HaState.ACTIVE) {
			Assert.assertNotNull(csiDescriptor.csiStateDescriptor);
			Assert.assertTrue(csiDescriptor.csiStateDescriptor instanceof CsiActiveDescriptor);
			s_printActiveDescriptor(
					(CsiActiveDescriptor) csiDescriptor.csiStateDescriptor,
					"JAVA TEST: \tCsiDescriptor.activeDescriptor: ");
		} else if (haState == HaState.STANDBY) {
			Assert.assertNotNull(csiDescriptor.csiStateDescriptor);
			Assert.assertTrue(csiDescriptor.csiStateDescriptor instanceof CsiStandbyDescriptor);
			s_printStandbyDescriptor(
					(CsiStandbyDescriptor) csiDescriptor.csiStateDescriptor,
					"JAVA TEST: \tCsiDescriptor.standbyDescriptor: ");

		} else if ((haState == HaState.QUIESCING)
				|| (haState == HaState.QUIESCED)) {
			Assert.assertNull(csiDescriptor.csiStateDescriptor);
		} else {
			System.out.println("JAVA TEST: \t\thaState: INVALID VALUE!!! :-(");
			Assert.assertTrue(false);
		}
		// csi attributes
		if (csiDescriptor.csiFlags == CsiDescriptor.CsiFlags.ADD_ONE) {
			Assert.assertNotNull(csiDescriptor.csiAttribute);
			s_printCsiAttributeArray(csiDescriptor.csiAttribute,
					"JAVA TEST: \tCsiDescriptor.csiAttribute: ");
		} else {
			Assert.assertNull(csiDescriptor.csiAttribute);
		}
	}

	public static void s_printActiveDescriptor(
			CsiActiveDescriptor activeDescriptor, String msg) {
		System.out.println(msg);
		System.out.println("JAVA TEST: \t\ttransitionDescriptor: "
				+ activeDescriptor.transitionDescriptor);
		System.out.println("JAVA TEST: \t\tactiveComponentName: "
				+ activeDescriptor.activeComponentName);
	}

	public static void s_printStandbyDescriptor(
			CsiStandbyDescriptor standbyDescriptor, String msg) {
		System.out.println(msg);
		System.out.println("JAVA TEST: \t\tactiveComponentName: "
				+ standbyDescriptor.activeComponentName);
		System.out.println("JAVA TEST: \t\tstandbyRank: "
				+ standbyDescriptor.standbyRank);
	}

	public static void s_printCsiAttributeArray(
			CsiAttribute[] csiAttributeArray, String msg) {
		System.out.println(msg);
		System.out.println("JAVA TEST: \t\tlength: " + csiAttributeArray.length);
		for (int _idx = 0; _idx < csiAttributeArray.length; _idx++) {
			System.out.println("JAVA TEST: \t\tcsiAttribute[" + _idx + "] {"
					+ csiAttributeArray[_idx].name + "/"
					+ csiAttributeArray[_idx].value + "}");
		}
	}

	public static void s_printPGNotificationArray(
			ProtectionGroupNotification[] pgNotArray, String msg) {
		System.out.println(msg);
		System.out.println("JAVA TEST: \tProtectionGroupNotification[].length: "
				+ pgNotArray.length);
		for (int _idx = 0; _idx < pgNotArray.length; _idx++) {
			System.out.println("JAVA TEST: \tProtectionGroupNotification["
					+ _idx + "]:");
			System.out.println("JAVA TEST: \t\tchange: "
					+ pgNotArray[_idx].change);
			s_printProtectionGroupMember(pgNotArray[_idx].member,
					"JAVA TEST: \t\tmember: ");
		}
	}

	public static void s_printProtectionGroupMember(
			ProtectionGroupMember pgMember, String msg) {
		System.out.println(msg);
		System.out.println("JAVA TEST: \t\t\t.rank: " + pgMember.rank);
		System.out.println("JAVA TEST: \t\t\t.haState: " + pgMember.haState);
		System.out.println("JAVA TEST: \t\t\t.componentName: "
				+ pgMember.componentName);
	}

}
