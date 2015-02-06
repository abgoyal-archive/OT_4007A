

#ifndef USYSTEM_H
#define USYSTEM_H

#ifdef U_HIDE_SYSTEM_API

#    if U_DISABLE_RENAMING
#        define u_cleanup u_cleanup_SYSTEM_API_DO_NOT_USE
#        define u_setAtomicIncDecFunctions u_setAtomicIncDecFunctions_SYSTEM_API_DO_NOT_USE
#        define u_setMemoryFunctions u_setMemoryFunctions_SYSTEM_API_DO_NOT_USE
#        define u_setMutexFunctions u_setMutexFunctions_SYSTEM_API_DO_NOT_USE
#        define ucnv_setDefaultName ucnv_setDefaultName_SYSTEM_API_DO_NOT_USE
#        define uloc_getDefault uloc_getDefault_SYSTEM_API_DO_NOT_USE
#        define uloc_setDefault uloc_setDefault_SYSTEM_API_DO_NOT_USE
#    else
#        define u_cleanup_4_4 u_cleanup_SYSTEM_API_DO_NOT_USE
#        define u_setAtomicIncDecFunctions_4_4 u_setAtomicIncDecFunctions_SYSTEM_API_DO_NOT_USE
#        define u_setMemoryFunctions_4_4 u_setMemoryFunctions_SYSTEM_API_DO_NOT_USE
#        define u_setMutexFunctions_4_4 u_setMutexFunctions_SYSTEM_API_DO_NOT_USE
#        define ucnv_setDefaultName_4_4 ucnv_setDefaultName_SYSTEM_API_DO_NOT_USE
#        define uloc_getDefault_4_4 uloc_getDefault_SYSTEM_API_DO_NOT_USE
#        define uloc_setDefault_4_4 uloc_setDefault_SYSTEM_API_DO_NOT_USE
#    endif /* U_DISABLE_RENAMING */

#endif /* U_HIDE_SYSTEM_API */
#endif /* USYSTEM_H */

