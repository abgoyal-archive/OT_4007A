#ifndef _AF_COEF_H
#define _AF_COEF_H

#include "MediaTypes.h"

    typedef struct
    {
        MINT32  i4AFS_STEP_MIN_ENABLE;
        MINT32  i4AFS_STEP_MIN_NORMAL;
        MINT32  i4AFS_STEP_MIN_MACRO;
        MINT32  i4AFC_FAIL_STOP;
        
    } AF_COEF_T;

	AF_COEF_T get_AF_Coef();
	
#endif /* _AF_COEF_H */

