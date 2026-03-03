#include "EcuAbsMcu.h"
#include "Rte_EcuAbs.h"
#include "IfxScu_reg.h"
#include "Mcu.h"
#include "Definition.h"

void EA_initMcu(void)
{
   (void) 0;
}

void EA_mainMcu(void)
{
    ST_DiagStatus stDiagStatus;
    Rte_Call_EcuAbs_rDcmCmd_getDiagStatus(&stDiagStatus);
    if(stDiagStatus.isEcuReset == TRUE)
    {
        Mcu_PerformReset();
    }
}
