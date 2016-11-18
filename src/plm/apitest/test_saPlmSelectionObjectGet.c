#include "plm/apitest/plmtest.h"
#include "osaf/saf/saAis.h"
#include "ntf/saf/saNtf.h"
#include "plm/saf/saPlm.h"

void saPlmSelectionObjectGet_01(void)
{
    safassert(saPlmInitialize(&plmHandle, NULL, &PlmVersion), SA_AIS_OK);
    rc = saPlmSelectionObjectGet(plmHandle, &selectionObject);
    test_validate(rc, SA_AIS_OK); 
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}

void saPlmSelectionObjectGet_02(void)
{
    SaPlmCallbacksT plms_cbks; 
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    rc = saPlmSelectionObjectGet(-1, &selectionObject);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}

void saPlmSelectionObjectGet_03(void)
{
    SaPlmCallbacksT plms_cbks; 
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    rc = saPlmSelectionObjectGet(plmHandle, NULL);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}

