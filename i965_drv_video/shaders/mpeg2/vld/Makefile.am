
INTEL_G4I = addidct.g4i               \
	    do_iq_intra.g4i           \
	    do_iq_non_intra.g4i       \
	    idct.g4i                  \
	    iq_intra.g4i              \
	    iq_non_intra.g4i          \
	    motion_field_uv.g4i       \
	    motion_field_y.g4i        \
	    motion_frame_uv.g4i       \
	    motion_frame_y.g4i        \
	    read_field_x0y0_uv.g4i    \
	    read_field_x0y0_y.g4i     \
	    read_field_x0y1_y.g4i     \
	    read_field_x1y0_y.g4i     \
	    read_field_x1y1_y.g4i     \
	    read_frame_x0y0_uv.g4i    \
	    read_frame_x0y0_y.g4i     \
	    read_frame_x0y1_y.g4i     \
	    read_frame_x1y0_y.g4i     \
	    read_frame_x1y1_y.g4i

INTEL_G4A = frame_intra.g4a 			\
	    frame_frame_pred_forward.g4a	\
	    frame_frame_pred_backward.g4a	\
	    frame_frame_pred_bidirect.g4a	\
	    frame_field_pred_forward.g4a	\
	    frame_field_pred_backward.g4a	\
	    frame_field_pred_bidirect.g4a	\
	    lib.g4a				\
	    field_intra.g4a			\
	    field_forward.g4a			\
	    field_forward_16x8.g4a		\
	    field_backward.g4a			\
	    field_backward_16x8.g4a		\
	    field_bidirect.g4a			\
	    field_bidirect_16x8.g4a		\
	    null.g4a

INTEL_G4B = frame_intra.g4b 			\
	    frame_frame_pred_forward.g4b	\
	    frame_frame_pred_backward.g4b	\
	    frame_frame_pred_bidirect.g4b	\
	    frame_field_pred_forward.g4b	\
	    frame_field_pred_backward.g4b	\
	    frame_field_pred_bidirect.g4b	\
	    lib.g4b				\
	    field_intra.g4b			\
	    field_forward.g4b			\
	    field_forward_16x8.g4b		\
	    field_backward.g4b			\
	    field_backward_16x8.g4b		\
	    field_bidirect.g4b			\
	    field_bidirect_16x8.g4b

INTEL_G4B_GEN5 = frame_intra.g4b.gen5 		\
	    frame_frame_pred_forward.g4b.gen5	\
	    frame_frame_pred_backward.g4b.gen5	\
	    frame_frame_pred_bidirect.g4b.gen5	\
	    frame_field_pred_forward.g4b.gen5	\
	    frame_field_pred_backward.g4b.gen5	\
	    frame_field_pred_bidirect.g4b.gen5	\
	    lib.g4b.gen5			\
	    field_intra.g4b.gen5		\
	    field_forward.g4b.gen5		\
	    field_forward_16x8.g4b.gen5		\
	    field_backward.g4b.gen5		\
	    field_backward_16x8.g4b.gen5	\
	    field_bidirect.g4b.gen5		\
	    field_bidirect_16x8.g4b.gen5

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
