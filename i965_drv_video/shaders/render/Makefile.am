
INTEL_G4I =			\
	exa_wm.g4i		\
	exa_wm_affine.g4i

INTEL_G4A =				\
	exa_sf.g4a			\
	exa_wm_xy.g4a			\
	exa_wm_src_affine.g4a		\
	exa_wm_src_sample_argb.g4a	\
	exa_wm_src_sample_planar.g4a	\
	exa_wm_yuv_rgb.g4a		\
	exa_wm_write.g4a

INTEL_G4B =				\
	exa_sf.g4b			\
	exa_wm_xy.g4b			\
	exa_wm_src_affine.g4b		\
	exa_wm_src_sample_argb.g4b	\
	exa_wm_src_sample_planar.g4b	\
	exa_wm_yuv_rgb.g4b		\
	exa_wm_write.g4b

INTEL_G4B_GEN5 =				\
	exa_sf.g4b.gen5				\
	exa_wm_xy.g4b.gen5			\
	exa_wm_src_affine.g4b.gen5		\
	exa_wm_src_sample_argb.g4b.gen5	\
	exa_wm_src_sample_planar.g4b.gen5	\
	exa_wm_yuv_rgb.g4b.gen5			\
	exa_wm_write.g4b.gen5

EXTRA_DIST = $(INTEL_G4I)	\
	     $(INTEL_G4A)       \
	     $(INTEL_G4B)    	\
	     $(INTEL_G4B_GEN5)

INTEL_G6A =				\
	exa_wm_src_affine.g6a 		\
	exa_wm_src_sample_argb.g6a 	\
	exa_wm_src_sample_planar.g6a 	\
	exa_wm_write.g6a 		\
	exa_wm_yuv_rgb.g6a

INTEL_G6B =				\
	exa_wm_src_affine.g6b 		\
	exa_wm_src_sample_argb.g6b 	\
	exa_wm_src_sample_planar.g6b 	\
	exa_wm_write.g6b 		\
	exa_wm_yuv_rgb.g6b

INTEL_G7A =				\
	exa_wm_src_affine.g7a 		\
	exa_wm_src_sample_argb.g7a 	\
	exa_wm_src_sample_planar.g7a 	\
	exa_wm_write.g7a 		\
	exa_wm_yuv_rgb.g7a

INTEL_G7B =				\
	exa_wm_src_affine.g7b 		\
	exa_wm_src_sample_argb.g7b 	\
	exa_wm_src_sample_planar.g7b 	\
	exa_wm_write.g7b 		\
	exa_wm_yuv_rgb.g7b

if HAVE_GEN4ASM

SUFFIXES = .g4a .g4b .g6a .g6b .g7a .g7b
.g4a.g4b:
	m4 $*.g4a > $*.g4m && intel-gen4asm -o $@ $*.g4m && intel-gen4asm -g 5 -o $@.gen5 $*.g4m && rm $*.g4m

.g6a.g6b:
	m4 -I$(srcdir) -s $< > $*.g6m && intel-gen4asm -g 6 -o $@ $*.g6m && rm $*.g6m

.g7a.g7b:
	m4 -I$(srcdir) -s $< > $*.g7m && intel-gen4asm -g 7 -o $@ $*.g7m && rm $*.g7m

$(INTEL_G4B): $(INTEL_G4I)
$(INTEL_G6B): $(INTEL_G4I)
$(INTEL_G7B): $(INTEL_G4I)

BUILT_SOURCES= $(INTEL_G4B) $(INTEL_G6B) $(INTEL_G7B)

clean-local:
	-rm -f $(INTEL_G4B)
	-rm -f $(INTEL_G4B_GEN5)
	-rm -f $(INTEL_G6B)
	-rm -f $(INTEL_G7B)
endif    
