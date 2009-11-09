#ifndef _I965_STRUCTS_H_
#define _I965_STRUCTS_H_

struct i965_vfe_state 
{
    struct {
	unsigned int per_thread_scratch_space:4;
	unsigned int pad3:3;
	unsigned int extend_vfe_state_present:1;
	unsigned int pad2:2;
	unsigned int scratch_base:22;
    } vfe0;

    struct {
	unsigned int debug_counter_control:2;
	unsigned int children_present:1;
	unsigned int vfe_mode:4;
	unsigned int pad2:2;
	unsigned int num_urb_entries:7;
	unsigned int urb_entry_alloc_size:9;
	unsigned int max_threads:7;
    } vfe1;

    struct {
	unsigned int pad4:4;
	unsigned int interface_descriptor_base:28;
    } vfe2;
};

struct i965_vfe_state_ex 
{
    struct {
	unsigned int pad:8;
	unsigned int obj_id:24;
    } vfex0;

    struct {
	unsigned int residual_grf_offset:5;
	unsigned int pad0:3;
	unsigned int weight_grf_offset:5;
	unsigned int pad1:3;
	unsigned int residual_data_offset:8;
	unsigned int sub_field_present_flag:2;
	unsigned int residual_data_fix_offset:1;
	unsigned int pad2:5;
    }vfex1;

    struct {
	unsigned int remap_index_0:4;
	unsigned int remap_index_1:4;
	unsigned int remap_index_2:4;
	unsigned int remap_index_3:4;
	unsigned int remap_index_4:4;
	unsigned int remap_index_5:4;
	unsigned int remap_index_6:4;
	unsigned int remap_index_7:4;
    }remap_table0;

    struct {
	unsigned int remap_index_8:4;
	unsigned int remap_index_9:4;
	unsigned int remap_index_10:4;
	unsigned int remap_index_11:4;
	unsigned int remap_index_12:4;
	unsigned int remap_index_13:4;
	unsigned int remap_index_14:4;
	unsigned int remap_index_15:4;
    } remap_table1;

    struct {
	unsigned int scoreboard_mask:8;
	unsigned int pad:22;
	unsigned int type:1;
	unsigned int enable:1;
    } scoreboard0;

    struct {
	unsigned int ignore;
    } scoreboard1;

    struct {
	unsigned int ignore;
    } scoreboard2;

    unsigned int pad;
};

struct i965_vld_state 
{
    struct {
        unsigned int pad6:6;
        unsigned int scan_order:1;
        unsigned int intra_vlc_format:1;
        unsigned int quantizer_scale_type:1;
        unsigned int concealment_motion_vector:1;
        unsigned int frame_predict_frame_dct:1;
        unsigned int top_field_first:1;
        unsigned int picture_structure:2;
        unsigned int intra_dc_precision:2;
        unsigned int f_code_0_0:4;
        unsigned int f_code_0_1:4;
        unsigned int f_code_1_0:4;
        unsigned int f_code_1_1:4;
    } vld0;

    struct {
        unsigned int pad2:9;
        unsigned int picture_coding_type:2;
        unsigned int pad:21;
    } vld1;

    struct {
        unsigned int index_0:4;
        unsigned int index_1:4;
        unsigned int index_2:4;
        unsigned int index_3:4;
        unsigned int index_4:4;
        unsigned int index_5:4;
        unsigned int index_6:4;
        unsigned int index_7:4;
    } desc_remap_table0;

    struct {
        unsigned int index_8:4;
        unsigned int index_9:4;
        unsigned int index_10:4;
        unsigned int index_11:4;
        unsigned int index_12:4;
        unsigned int index_13:4;
        unsigned int index_14:4;
        unsigned int index_15:4;
    } desc_remap_table1;
};

struct i965_interface_descriptor 
{
    struct {
	unsigned int grf_reg_blocks:4;
	unsigned int pad:2;
	unsigned int kernel_start_pointer:26;
    } desc0;

    struct {
	unsigned int pad:7;
	unsigned int software_exception:1;
	unsigned int pad2:3;
	unsigned int maskstack_exception:1;
	unsigned int pad3:1;
	unsigned int illegal_opcode_exception:1;
	unsigned int pad4:2;
	unsigned int floating_point_mode:1;
	unsigned int thread_priority:1;
	unsigned int single_program_flow:1;
	unsigned int pad5:1;
	unsigned int const_urb_entry_read_offset:6;
	unsigned int const_urb_entry_read_len:6;
    } desc1;

    struct {
	unsigned int pad:2;
	unsigned int sampler_count:3;
	unsigned int sampler_state_pointer:27;
    } desc2;

    struct {
	unsigned int binding_table_entry_count:5;
	unsigned int binding_table_pointer:27;
    } desc3;
};

struct i965_surface_state 
{
    struct {
	unsigned int cube_pos_z:1;
	unsigned int cube_neg_z:1;
	unsigned int cube_pos_y:1;
	unsigned int cube_neg_y:1;
	unsigned int cube_pos_x:1;
	unsigned int cube_neg_x:1;
	unsigned int pad:3;
	unsigned int render_cache_read_mode:1;
	unsigned int mipmap_layout_mode:1;
	unsigned int vert_line_stride_ofs:1;
	unsigned int vert_line_stride:1;
	unsigned int color_blend:1;
	unsigned int writedisable_blue:1;
	unsigned int writedisable_green:1;
	unsigned int writedisable_red:1;
	unsigned int writedisable_alpha:1;
	unsigned int surface_format:9;
	unsigned int data_return_format:1;
	unsigned int pad0:1;
	unsigned int surface_type:3;
    } ss0;

    struct {
	unsigned int base_addr;
    } ss1;

    struct {
	unsigned int render_target_rotation:2;
	unsigned int mip_count:4;
	unsigned int width:13;
	unsigned int height:13;
    } ss2;

    struct {
	unsigned int tile_walk:1;
	unsigned int tiled_surface:1;
	unsigned int pad:1;
	unsigned int pitch:18;
	unsigned int depth:11;
    } ss3;

    struct {
	unsigned int pad:19;
	unsigned int min_array_elt:9;
	unsigned int min_lod:4;
    } ss4;

    struct {
	unsigned int pad:20;
	unsigned int y_offset:4;
	unsigned int pad2:1;
	unsigned int x_offset:7;
    } ss5;
};

struct thread0
{
    unsigned int pad0:1;
    unsigned int grf_reg_count:3; 
    unsigned int pad1:2;
    unsigned int kernel_start_pointer:26; 
};

struct thread1
{
    unsigned int ext_halt_exception_enable:1; 
    unsigned int sw_exception_enable:1; 
    unsigned int mask_stack_exception_enable:1; 
    unsigned int timeout_exception_enable:1; 
    unsigned int illegal_op_exception_enable:1; 
    unsigned int pad0:3;
    unsigned int depth_coef_urb_read_offset:6;	/* WM only */
    unsigned int pad1:2;
    unsigned int floating_point_mode:1; 
    unsigned int thread_priority:1; 
    unsigned int binding_table_entry_count:8; 
    unsigned int pad3:5;
    unsigned int single_program_flow:1; 
};

struct thread2
{
    unsigned int per_thread_scratch_space:4; 
    unsigned int pad0:6;
    unsigned int scratch_space_base_pointer:22; 
};

   
struct thread3
{
    unsigned int dispatch_grf_start_reg:4; 
    unsigned int urb_entry_read_offset:6; 
    unsigned int pad0:1;
    unsigned int urb_entry_read_length:6; 
    unsigned int pad1:1;
    unsigned int const_urb_entry_read_offset:6; 
    unsigned int pad2:1;
    unsigned int const_urb_entry_read_length:6; 
    unsigned int pad3:1;
};

struct i965_vs_unit_state
{
    struct thread0 thread0;
    struct thread1 thread1;
    struct thread2 thread2;
    struct thread3 thread3;
   
    struct {
        unsigned int pad0:10;
        unsigned int stats_enable:1; 
        unsigned int nr_urb_entries:7; 
        unsigned int pad1:1;
        unsigned int urb_entry_allocation_size:5; 
        unsigned int pad2:1;
        unsigned int max_threads:4; 
        unsigned int pad3:3;
    } thread4;   

    struct {
        unsigned int sampler_count:3; 
        unsigned int pad0:2;
        unsigned int sampler_state_pointer:27; 
    } vs5;

    struct {
        unsigned int vs_enable:1; 
        unsigned int vert_cache_disable:1; 
        unsigned int pad0:30;
    } vs6;
};

struct i965_gs_unit_state
{
   struct thread0 thread0;
   struct thread1 thread1;
   struct thread2 thread2;
   struct thread3 thread3;

   struct {
      unsigned int pad0:10;
      unsigned int stats_enable:1; 
      unsigned int nr_urb_entries:7; 
      unsigned int pad1:1;
      unsigned int urb_entry_allocation_size:5; 
      unsigned int pad2:1;
      unsigned int max_threads:1; 
      unsigned int pad3:6;
   } thread4;   
      
   struct {
      unsigned int sampler_count:3; 
      unsigned int pad0:2;
      unsigned int sampler_state_pointer:27; 
   } gs5;

   
   struct {
      unsigned int max_vp_index:4; 
      unsigned int pad0:26;
      unsigned int reorder_enable:1; 
      unsigned int pad1:1;
   } gs6;
};

struct i965_clip_unit_state
{
   struct thread0 thread0;
   struct thread1 thread1;
   struct thread2 thread2;
   struct thread3 thread3;

   struct {
      unsigned int pad0:9;
      unsigned int gs_output_stats:1; /* not always */
      unsigned int stats_enable:1; 
      unsigned int nr_urb_entries:7; 
      unsigned int pad1:1;
      unsigned int urb_entry_allocation_size:5; 
      unsigned int pad2:1;
      unsigned int max_threads:6; 	/* may be less */
      unsigned int pad3:1;
   } thread4;   
      
   struct {
      unsigned int pad0:13;
      unsigned int clip_mode:3; 
      unsigned int userclip_enable_flags:8; 
      unsigned int userclip_must_clip:1; 
      unsigned int pad1:1;
      unsigned int guard_band_enable:1; 
      unsigned int viewport_z_clip_enable:1; 
      unsigned int viewport_xy_clip_enable:1; 
      unsigned int vertex_position_space:1; 
      unsigned int api_mode:1; 
      unsigned int pad2:1;
   } clip5;
   
   struct {
      unsigned int pad0:5;
      unsigned int clipper_viewport_state_ptr:27; 
   } clip6;

   
   float viewport_xmin;  
   float viewport_xmax;  
   float viewport_ymin;  
   float viewport_ymax;  
};

struct i965_sf_unit_state
{
   struct thread0 thread0;
   struct {
      unsigned int pad0:7;
      unsigned int sw_exception_enable:1; 
      unsigned int pad1:3;
      unsigned int mask_stack_exception_enable:1; 
      unsigned int pad2:1;
      unsigned int illegal_op_exception_enable:1; 
      unsigned int pad3:2;
      unsigned int floating_point_mode:1; 
      unsigned int thread_priority:1; 
      unsigned int binding_table_entry_count:8; 
      unsigned int pad4:5;
      unsigned int single_program_flow:1; 
   } sf1;
   
   struct thread2 thread2;
   struct thread3 thread3;

   struct {
      unsigned int pad0:10;
      unsigned int stats_enable:1; 
      unsigned int nr_urb_entries:7; 
      unsigned int pad1:1;
      unsigned int urb_entry_allocation_size:5; 
      unsigned int pad2:1;
      unsigned int max_threads:6; 
      unsigned int pad3:1;
   } thread4;   

   struct {
      unsigned int front_winding:1; 
      unsigned int viewport_transform:1; 
      unsigned int pad0:3;
      unsigned int sf_viewport_state_offset:27; 
   } sf5;
   
   struct {
      unsigned int pad0:9;
      unsigned int dest_org_vbias:4; 
      unsigned int dest_org_hbias:4; 
      unsigned int scissor:1; 
      unsigned int disable_2x2_trifilter:1; 
      unsigned int disable_zero_pix_trifilter:1; 
      unsigned int point_rast_rule:2; 
      unsigned int line_endcap_aa_region_width:2; 
      unsigned int line_width:4; 
      unsigned int fast_scissor_disable:1; 
      unsigned int cull_mode:2; 
      unsigned int aa_enable:1; 
   } sf6;

   struct {
      unsigned int point_size:11; 
      unsigned int use_point_size_state:1; 
      unsigned int subpixel_precision:1; 
      unsigned int sprite_point:1; 
      unsigned int pad0:11;
      unsigned int trifan_pv:2; 
      unsigned int linestrip_pv:2; 
      unsigned int tristrip_pv:2; 
      unsigned int line_last_pixel_enable:1; 
   } sf7;
};

struct i965_sampler_state
{
   struct {
      unsigned int shadow_function:3; 
      unsigned int lod_bias:11; 
      unsigned int min_filter:3; 
      unsigned int mag_filter:3; 
      unsigned int mip_filter:2; 
      unsigned int base_level:5; 
      unsigned int pad:1;
      unsigned int lod_preclamp:1; 
      unsigned int border_color_mode:1; 
      unsigned int pad0:1;
      unsigned int disable:1; 
   } ss0;

   struct {
      unsigned int r_wrap_mode:3; 
      unsigned int t_wrap_mode:3; 
      unsigned int s_wrap_mode:3; 
      unsigned int pad:3;
      unsigned int max_lod:10; 
      unsigned int min_lod:10; 
   } ss1;

   
   struct {
      unsigned int pad:5;
      unsigned int border_color_pointer:27; 
   } ss2;
   
   struct {
      unsigned int pad:19;
      unsigned int max_aniso:3; 
      unsigned int chroma_key_mode:1; 
      unsigned int chroma_key_index:2; 
      unsigned int chroma_key_enable:1; 
      unsigned int monochrome_filter_width:3; 
      unsigned int monochrome_filter_height:3; 
   } ss3;
};

struct i965_wm_unit_state
{
   struct thread0 thread0;
   struct thread1 thread1;
   struct thread2 thread2;
   struct thread3 thread3;
   
   struct {
      unsigned int stats_enable:1; 
      unsigned int pad0:1;
      unsigned int sampler_count:3; 
      unsigned int sampler_state_pointer:27; 
   } wm4;
   
   struct {
      unsigned int enable_8_pix:1; 
      unsigned int enable_16_pix:1; 
      unsigned int enable_32_pix:1; 
      unsigned int pad0:7;
      unsigned int legacy_global_depth_bias:1; 
      unsigned int line_stipple:1; 
      unsigned int depth_offset:1; 
      unsigned int polygon_stipple:1; 
      unsigned int line_aa_region_width:2; 
      unsigned int line_endcap_aa_region_width:2; 
      unsigned int early_depth_test:1; 
      unsigned int thread_dispatch_enable:1; 
      unsigned int program_uses_depth:1; 
      unsigned int program_computes_depth:1; 
      unsigned int program_uses_killpixel:1; 
      unsigned int legacy_line_rast: 1; 
      unsigned int transposed_urb_read:1; 
      unsigned int max_threads:7; 
   } wm5;
   
   float global_depth_offset_constant;  
   float global_depth_offset_scale;   
};

struct i965_cc_viewport
{
   float min_depth;  
   float max_depth;  
};

struct i965_cc_unit_state
{
   struct {
      unsigned int pad0:3;
      unsigned int bf_stencil_pass_depth_pass_op:3; 
      unsigned int bf_stencil_pass_depth_fail_op:3; 
      unsigned int bf_stencil_fail_op:3; 
      unsigned int bf_stencil_func:3; 
      unsigned int bf_stencil_enable:1; 
      unsigned int pad1:2;
      unsigned int stencil_write_enable:1; 
      unsigned int stencil_pass_depth_pass_op:3; 
      unsigned int stencil_pass_depth_fail_op:3; 
      unsigned int stencil_fail_op:3; 
      unsigned int stencil_func:3; 
      unsigned int stencil_enable:1; 
   } cc0;

   
   struct {
      unsigned int bf_stencil_ref:8; 
      unsigned int stencil_write_mask:8; 
      unsigned int stencil_test_mask:8; 
      unsigned int stencil_ref:8; 
   } cc1;

   
   struct {
      unsigned int logicop_enable:1; 
      unsigned int pad0:10;
      unsigned int depth_write_enable:1; 
      unsigned int depth_test_function:3; 
      unsigned int depth_test:1; 
      unsigned int bf_stencil_write_mask:8; 
      unsigned int bf_stencil_test_mask:8; 
   } cc2;

   
   struct {
      unsigned int pad0:8;
      unsigned int alpha_test_func:3; 
      unsigned int alpha_test:1; 
      unsigned int blend_enable:1; 
      unsigned int ia_blend_enable:1; 
      unsigned int pad1:1;
      unsigned int alpha_test_format:1;
      unsigned int pad2:16;
   } cc3;
   
   struct {
      unsigned int pad0:5; 
      unsigned int cc_viewport_state_offset:27; 
   } cc4;
   
   struct {
      unsigned int pad0:2;
      unsigned int ia_dest_blend_factor:5; 
      unsigned int ia_src_blend_factor:5; 
      unsigned int ia_blend_function:3; 
      unsigned int statistics_enable:1; 
      unsigned int logicop_func:4; 
      unsigned int pad1:11;
      unsigned int dither_enable:1; 
   } cc5;

   struct {
      unsigned int clamp_post_alpha_blend:1; 
      unsigned int clamp_pre_alpha_blend:1; 
      unsigned int clamp_range:2; 
      unsigned int pad0:11;
      unsigned int y_dither_offset:2; 
      unsigned int x_dither_offset:2; 
      unsigned int dest_blend_factor:5; 
      unsigned int src_blend_factor:5; 
      unsigned int blend_function:3; 
   } cc6;

   struct {
      union {
	 float f;  
	 unsigned char ub[4];
      } alpha_ref;
   } cc7;
};

#endif /* _I965_STRUCTS_H_ */
