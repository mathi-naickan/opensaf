/*
 *
 * (C) Copyright 2009 The OpenSAF Foundation
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
 * Author(s): Ericsson AB
 *
 */

#ifndef SMF_SMFD_SMFSTEPTYPES_H_
#define SMF_SMFD_SMFSTEPTYPES_H_

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */

#include <string>

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   TYPE DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */

class SmfUpgradeStep;

//================================================================================
// Class SmfStepType
//================================================================================
///
/// Purpose: Base class for all step types.
///
class SmfStepType {
public:

        SmfStepType(SmfUpgradeStep * i_step);
        virtual ~SmfStepType();

	virtual bool execute() = 0;
	virtual bool rollback() = 0;

protected:
        SmfUpgradeStep * m_step;
};

//================================================================================
// Class SmfStepTypeSwInstall
//================================================================================
///
/// Purpose: Step type SW install.
///
class SmfStepTypeSwInstall : public SmfStepType {
public:
        SmfStepTypeSwInstall(SmfUpgradeStep * i_step) : SmfStepType(i_step) {}
        virtual ~SmfStepTypeSwInstall() {}

	virtual bool execute();
	virtual bool rollback();

private:
};

//================================================================================
// Class SmfStepTypeSwInstallAct
//================================================================================
///
/// Purpose: Step type SW install with activation.
///
class SmfStepTypeSwInstallAct : public SmfStepType {
public:
        SmfStepTypeSwInstallAct(SmfUpgradeStep * i_step) : SmfStepType(i_step) {}
        virtual ~SmfStepTypeSwInstallAct() {}

	virtual bool execute();
	virtual bool rollback();

private:
};

//================================================================================
// Class SmfStepTypeAuLock
//================================================================================
///
/// Purpose: Step type AU Lock.
///
class SmfStepTypeAuLock : public SmfStepType {
public:
        SmfStepTypeAuLock(SmfUpgradeStep * i_step) : SmfStepType(i_step) {}
        virtual ~SmfStepTypeAuLock() {}

	virtual bool execute();
	virtual bool rollback();

private:
};

//================================================================================
// Class SmfStepTypeAuLockAct
//================================================================================
///
/// Purpose: Step type AU Lock with activation.
///
class SmfStepTypeAuLockAct : public SmfStepType {
public:
        SmfStepTypeAuLockAct(SmfUpgradeStep * i_step) : SmfStepType(i_step) {}
        virtual ~SmfStepTypeAuLockAct() {}

	virtual bool execute();
	virtual bool rollback();

private:
};

//================================================================================
// Class SmfStepTypeAuRestart
//================================================================================
///
/// Purpose: Step type AU restart .
///
class SmfStepTypeAuRestart : public SmfStepType {
public:
        SmfStepTypeAuRestart(SmfUpgradeStep * i_step) : SmfStepType(i_step) {}
        virtual ~SmfStepTypeAuRestart() {}

	virtual bool execute();
	virtual bool rollback();

private:
};

//================================================================================
// Class SmfStepTypeAuRestartAct
//================================================================================
///
/// Purpose: Step type AU restart with activation .
///
class SmfStepTypeAuRestartAct : public SmfStepType {
public:
        SmfStepTypeAuRestartAct(SmfUpgradeStep * i_step) : SmfStepType(i_step) {}
        virtual ~SmfStepTypeAuRestartAct() {}

	virtual bool execute();
	virtual bool rollback();

private:
};

//================================================================================
// Class SmfStepTypeNodeReboot
//================================================================================
///
/// Purpose: Step type node reboot.
///
class SmfStepTypeNodeReboot : public SmfStepType {
public:
        SmfStepTypeNodeReboot(SmfUpgradeStep * i_step) : SmfStepType(i_step) {}
        virtual ~SmfStepTypeNodeReboot() {}

	virtual bool execute();
	virtual bool rollback();

private:
};

//================================================================================
// Class SmfStepTypeNodeRebootAct
//================================================================================
///
/// Purpose: Step type node reboot with activation.
///
class SmfStepTypeNodeRebootAct : public SmfStepType {
public:
        SmfStepTypeNodeRebootAct(SmfUpgradeStep * i_step) : SmfStepType(i_step) {}
        virtual ~SmfStepTypeNodeRebootAct() {}

	virtual bool execute();
	virtual bool rollback();

private:
};

//================================================================================
// Class SmfStepTypeClusterReboot
//================================================================================
///
/// Purpose: Step type cluster reboot.
///
class SmfStepTypeClusterReboot : public SmfStepType {
public:
        SmfStepTypeClusterReboot(SmfUpgradeStep * i_step) : SmfStepType(i_step) {}
        virtual ~SmfStepTypeClusterReboot() {}

	virtual bool execute();
	virtual bool rollback();

private:
};

//================================================================================
// Class SmfStepTypeClusterRebootAct
//================================================================================
///
/// Purpose: Step type cluster reboot with activation.
///
class SmfStepTypeClusterRebootAct : public SmfStepType {
public:
        SmfStepTypeClusterRebootAct(SmfUpgradeStep * i_step) : SmfStepType(i_step) {}
        virtual ~SmfStepTypeClusterRebootAct() {}

	virtual bool execute();
	virtual bool rollback();

private:
};

#endif  // SMF_SMFD_SMFSTEPTYPES_H_
