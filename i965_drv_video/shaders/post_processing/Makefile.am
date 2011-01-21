
INTEL_G4I = 

INTEL_G4A = null.g4a

INTEL_G4B = null.g4b

INTEL_G4B_GEN5 = null.g4b.gen5

INTEL_PP_G4B_GEN5 = 		\
	nv12_avs_nv12.g4b.gen5	\
	nv12_dndi_nv12.g4b.gen5	\
	nv12_load_save_nv12.g4b.gen5	\
	nv12_scaling_nv12.g4b.gen5

INTEL_PP_ASM =				\
	Common/AYUV_Load_16x8.asm	\
	Common/IMC3_Load_8x4.asm	\
	Common/IMC3_Load_8x5.asm	\
	Common/IMC3_Load_9x5.asm	\
	Common/Init_All_Regs.asm	\
	Common/Multiple_Loop.asm	\
	Common/Multiple_Loop_Head.asm	\
	Common/NV11_Load_4x8.asm	\
	Common/NV11_Load_5x8.asm	\
	Common/NV12_Load_8x4.asm	\
	Common/NV12_Load_8x5.asm	\
	Common/NV12_Load_9x5.asm	\
	Common/P208_Load_8x8.asm	\
	Common/P208_Load_9x8.asm	\
	Common/PA_Load_8x8.asm	\
	Common/PA_Load_9x8.asm	\
	Common/PL16x8_PL8x4.asm	\
	Common/PL16x8_PL8x8.asm	\
	Common/PL4x8_Save_NV11.asm	\
	Common/PL5x8_PL16x8.asm	\
	Common/PL5x8_PL8x8.asm	\
	Common/PL8x4_Save_IMC3.asm	\
	Common/PL8x4_Save_NV12.asm	\
	Common/PL8x5_PL8x8.asm	\
	Common/PL8x8_PL8x4.asm	\
	Common/PL8x8_Save_P208.asm	\
	Common/PL8x8_Save_PA.asm	\
	Common/PL9x5_PL16x8.asm	\
	Common/PL9x8_PL16x8.asm	\
	Common/readSampler16x1.asm	\
	Common/RGB16x8_Save_RGB16.asm	\
	Common/RGB16x8_Save_RGB.asm	\
	Common/RGB16x8_Save_Y416.asm	\
	Common/RGB_Pack.asm	\
	Common/SetupVPKernel.asm	\
	Core_Kernels/AVS_SetupFirstBlock.asm	\
	Core_Kernels/AVS_SetupSecondBlock.asm	\
	Core_Kernels/DI_Hist_Save.asm	\
	Core_Kernels/DI_SAVE_PA.asm	\
	Core_Kernels/DNDI_COMMAND.asm	\
	Core_Kernels/DNDI_Hist_Save.asm	\
	Core_Kernels/PA_AVS_IEF_16x8.asm	\
	Core_Kernels/PA_AVS_IEF_8x4.asm	\
	Core_Kernels/PA_AVS_IEF_8x8.asm	\
	Core_Kernels/PA_AVS_IEF_Sample.asm	\
	Core_Kernels/PA_AVS_IEF_Unpack_16x8.asm	\
	Core_Kernels/PA_AVS_IEF_Unpack_8x4.asm	\
	Core_Kernels/PA_AVS_IEF_Unpack_8x8.asm	\
	Core_Kernels/PA_DN_ALG.asm	\
	Core_Kernels/PA_DNDI_ALG.asm	\
	Core_Kernels/PA_Scaling.asm	\
	Core_Kernels/PL2_AVS_IEF_16x8.asm	\
	Core_Kernels/PL2_AVS_IEF_8x4.asm	\
	Core_Kernels/PL2_AVS_IEF_8x8.asm	\
	Core_Kernels/PL2_AVS_IEF_Unpack_16x8.asm	\
	Core_Kernels/PL2_AVS_IEF_Unpack_8x4.asm	\
	Core_Kernels/PL2_AVS_IEF_Unpack_8x8.asm	\
	Core_Kernels/PL2_Scaling.asm	\
	Core_Kernels/PL3_AVS_IEF_16x8.asm	\
	Core_Kernels/PL3_AVS_IEF_8x4.asm	\
	Core_Kernels/PL3_AVS_IEF_8x8.asm	\
	Core_Kernels/PL3_AVS_IEF_Unpack_16x8.asm	\
	Core_Kernels/PL3_AVS_IEF_Unpack_8x4.asm	\
	Core_Kernels/PL3_AVS_IEF_Unpack_8x8.asm	\
	Core_Kernels/PL3_Scaling.asm	\
	Core_Kernels/PL_DN_ALG.asm	\
	Core_Kernels/PL_DNDI_ALG.asm	\
	Core_Kernels/PL_DNDI_ALG_UVCopy_NV11.asm	\
	Core_Kernels/PL_DNDI_ALG_UVCopy_NV12.asm	\
	Core_Kernels/PL_DNDI_ALG_UVCopy_P208.asm	\
	Core_Kernels/PL_DNDI_ALG_UVCopy_PL3.asm	\
	Core_Kernels/RGB_AVS_IEF_16x8.asm	\
	Core_Kernels/RGB_AVS_IEF_Unpack_16x8.asm	\
	Core_Kernels/RGB_AVS_IEF_Unscramble_16x8.asm	\
	Core_Kernels/RGB_Scaling.asm

EXTRA_DIST = $(INTEL_G4I)	\
	     $(INTEL_G4A)       \
	     $(INTEL_G4B)    	\
	     $(INTEL_G4B_GEN5)

if HAVE_GEN4ASM

SUFFIXES = .g4a .g4b
.g4a.g4b:
	m4 $*.g4a > $*.g4m && intel-gen4asm -o $@ $*.g4m && intel-gen4asm -g 5 -o $@.gen5 $*.g4m && rm $*.g4m

$(INTEL_G4B): $(INTEL_G4I)

$(INTEL_PP_G4B_GEN5): $(INTEL_PP_ASM)
	@_PP_TARGET=$@; \
	cpp -D DEV_ILK -I Common/ -I Core_Kernels $${_PP_TARGET/.g4b.gen5/.asm} > _pp0.asm; \
	../gpp.py _pp0.asm _pp1.asm;	\
	intel-gen4asm -a -o $@ -g 5 _pp1.asm; \
	rm _pp0.asm _pp1.asm

BUILT_SOURCES= $(INTEL_G4B) $(INTEL_PP_G4B_GEN5)

clean-local:
	-rm -f $(INTEL_G4B)
	-rm -f $(INTEL_G4B_GEN5)
	-rm -f $(INTEL_PP_G4B_GEN5)
endif    
