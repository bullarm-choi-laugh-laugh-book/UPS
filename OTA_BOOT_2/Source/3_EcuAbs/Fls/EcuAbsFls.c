#include "Definition.h"
#include "Rte_EcuAbs.h"
#include "FlsLoader.h"
#include "EcuAbsFls.h"
#include "EcuAbsWdg.h"

void EA_initFls(void)
{
    FlsLoader_Init(NULL);
}

void EA_erasePflashBlock(uint8 pflashArea, P_uint8 errorResult)
{
    Mcal_DisableAllInterrupts();
    EA_wdgTriggerUpdateSlow();
    if(pflashArea == PFLASH_AREA_APPICATION_PRIMARY)
    {
        *errorResult = (uint8_t)FlsLoader_Erase(FLSLOADER_PFLASH0_START_ADDRESS + FLSLOADER_PF_S23, 2u);
    }
    else if(pflashArea == PFLASH_AREA_APPICATION_SECONDARY)
    {
        *errorResult = (uint8_t)FlsLoader_Erase(FLSLOADER_PFLASH0_START_ADDRESS + FLSLOADER_PF_S25, 2u);
    }
    else
    {
        *errorResult = !FALSE;
    }
    Mcal_EnableAllInterrupts();
}
