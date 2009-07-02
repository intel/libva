
INTEL_G4I =			\
	exa_wm.g4i		\
	exa_wm_affine.g4i

INTEL_G4A =				\
	exa_sf.g4a			\
	exa_wm_xy.g4a			\
	exa_wm_src_affine.g4a		\
	exa_wm_src_sample_planar.g4a	\
	exa_wm_yuv_rgb.g4a		\
	exa_wm_blend_subpicture.g4a		\
	exa_wm_write.g4a

INTEL_G4B =				\
	exa_sf.g4b			\
	exa_wm_xy.g4b			\
	exa_wm_src_affine.g4b		\
	exa_wm_src_sample_planar.g4b	\
	exa_wm_yuv_rgb.g4b		\
	exa_wm_blend_subpicture.g4b		\
	exa_wm_write.g4b

EXTRA_DIST = $(INTEL_G4I)	\
	     $(INTEL_G4A)       \
	     $(INTEL_G4B)    

if HAVE_GEN4ASM

SUFFIXES = .g4a .g4b
.g4a.g4b:
	m4 $*.g4a > $*.g4m && intel-gen4asm -o $@ $*.g4m && rm $*.g4m

$(INTEL_G4B): $(INTEL_G4I)

BUILT_SOURCES= $(INTEL_G4B)

clean-local:
	-rm -f $(INTEL_G4B)
endif    
