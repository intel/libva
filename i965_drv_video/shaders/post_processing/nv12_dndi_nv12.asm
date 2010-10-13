// Module name: NV12_DNDI_NV12
.kernel NV12_DNDI_NV12
.code

#define INC_DNDI
        
#include "SetupVPKernel.asm"
#include "Multiple_Loop_Head.asm"
#include "PL_DNDI_ALG_UVCopy_NV12.asm"
#include "Multiple_Loop.asm"

END_THREAD  // End of Thread

.end_code  

.end_kernel

// end of nv12_dndi_nv12.asm
