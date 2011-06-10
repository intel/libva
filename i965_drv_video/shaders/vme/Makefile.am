VME_CORE  = intra_frame.asm inter_frame.asm

INTEL_G6B = intra_frame.g6b inter_frame.g6b
INTEL_G6A = intra_frame.g6a inter_frame.g6a
INTEL_INC = gen6_vme_header.inc

INTEL_G7B = intra_frame.g7b inter_frame.g7b
INTEL_G7A = intra_frame.g7a inter_frame.g7a
INTEL_INC_GEN7 = gen7_vme_header.inc

EXTRA_DIST = $(INTEL_G6B)	\
	$(INTEL_G6A)		\
	$(INTEL_INC)		\
	$(INTEL_G7B)		\
	$(INTEL_G7A)		\
	$(INTEL_INC_GEN7)

if HAVE_GEN4ASM

SUFFIXES = .g6a .g6b .g7a .g7b
.g6a.g6b:
	m4 $*.g6a > $*.g6m && intel-gen4asm -g 6 -o $@ $*.g6m && rm $*.g6m

.g7a.g7b:
	m4 $*.g7a > $*.g7m && intel-gen4asm -g 7 -o $@ $*.g7m && rm $*.g7m

$(INTEL_G6B): $(INTEL_INC) $(VME_CORE)

$(INTEL_G7B): $(INTEL_INC_GEN7) $(VME_CORE)

BUILT_SOURCES= $(INTEL_G6B) $(INTEL_G7B)

clean-local:
	-rm -f $(INTEL_G6B)
	-rm -f $(INTEL_G7B)

endif    
