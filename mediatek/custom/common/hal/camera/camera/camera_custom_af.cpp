
#include "camera_custom_af.h"

AF_COEF_T get_AF_Coef()
{
    AF_COEF_T sAFcoef;

    sAFcoef.i4AFS_STEP_MIN_ENABLE = 0;
    sAFcoef.i4AFS_STEP_MIN_MACRO = 4;
    sAFcoef.i4AFS_STEP_MIN_NORMAL = 4;
    sAFcoef.i4AFC_FAIL_STOP = 4;
    
	return sAFcoef;
}

