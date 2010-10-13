// Module name: NV12_SCALING_NV12
.kernel NV12_SCALING_NV12
.code

#define INC_SCALING
        
#include "SetupVPKernel.asm"
#include "Multiple_Loop_Head.asm"
#include "PL2_Scaling.asm"
#include "PL16x8_PL8x4.asm"        
#include "PL8x4_Save_NV12.asm"
#include "Multiple_Loop.asm"

END_THREAD  // End of Thread

.end_code  

.end_kernel

// end of nv12_scaling_nv12.asm
