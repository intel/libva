INTEL_G6B = intra_frame.g6b inter_frame.g6b

EXTRA_DIST = $(INTEL_G6B)

if HAVE_GEN4ASM

SUFFIXES = .asm .g6b
.asm.g6b:
	m4 $*.asm > $*.g6m && intel-gen4asm -g 6 -o $@ $*.g6m && rm $*.g6m

BUILT_SOURCES= $(INTEL_G6B)

clean-local:
	-rm -f $(INTEL_G6B)

endif    
