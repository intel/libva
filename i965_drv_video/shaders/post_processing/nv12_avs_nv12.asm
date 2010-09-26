// Module name: NV12_AVS_NV12
.kernel NV12_AVS_NV12
.code

#define INC_SCALING
        
#include "SetupVPKernel.asm"
#include "Multiple_Loop_Head.asm"
#include "PL2_AVS_IEF_16x8.asm"
#include "PL8x4_Save_NV12.asm"
#include "Multiple_Loop.asm"

END_THREAD  // End of Thread

.end_code  

.end_kernel

// end of nv12_avs_nv12.asm
