#include "EcuAbsMcu.h"
#include "Rte_EcuAbs.h"
#include "IfxScu_reg.h"
#include "Mcu.h"
#include "Definition.h"

void EA_initMcu(void)
{
    FP_void_void entrypointApplicaton = (void*) MEMORY_ADDRESS_APPLICATION;
    if(SCU_RSTCON2.B.USRINFO != RESET_REASON_REPROGRAMMING_REQUEST)
    {
        entrypointApplicaton();
    }
    else
    {
        Mcal_ResetSafetyENDINIT_Timed(MCU_SAFETY_ENDINT_TIMEOUT);
        SCU_RSTCON2.B.USRINFO = RESET_REASON_NORMAL;
        Mcal_SetSafetyENDINIT_Timed();
    }
}

void EA_mainMcu(void)
{
    ST_DiagStatus stDiagStatus;
    Rte_Call_EcuAbs_rDcmCmd_getDiagStatus(&stDiagStatus);
    if((stDiagStatus.isEcuReset == TRUE) || (stDiagStatus.currentSession == DIAG_SESSION_DEFAULT))
    {
        Mcu_PerformReset();
    }
}
