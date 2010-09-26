
INTEL_G4I = 

INTEL_G4A = null.g4a

INTEL_G4B = null.g4b

INTEL_G4B_GEN5 = null.g4b.gen5

EXTRA_DIST = $(INTEL_G4I)	\
	     $(INTEL_G4A)       \
	     $(INTEL_G4B)    	\
	     $(INTEL_G4B_GEN5)

if HAVE_GEN4ASM

SUFFIXES = .g4a .g4b
.g4a.g4b:
	m4 $*.g4a > $*.g4m && intel-gen4asm -o $@ $*.g4m && intel-gen4asm -g 5 -o $@.gen5 $*.g4m && rm $*.g4m

$(INTEL_G4B): $(INTEL_G4I)

BUILT_SOURCES= $(INTEL_G4B)

clean-local:
	-rm -f $(INTEL_G4B)
	-rm -f $(INTEL_G4B_GEN5)
endif    
