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

    union {
        struct {
            unsigned int residual_grf_offset:5;
            unsigned int pad0:3;
            unsigned int weight_grf_offset:5;
            unsigned int pad1:3;
            unsigned int residual_data_offset:8;
            unsigned int sub_field_present_flag:2;
            unsigned int residual_data_fix_offset_flag:1;
            unsigned int pad2:5;
        } avc;
        
        unsigned int vc1;
    } vfex1;

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
	unsigned int mask:8;
	unsigned int pad:22;
	unsigned int type:1;
	unsigned int enable:1;
    } scoreboard0;

    struct {
        int delta_x0:4;
        int delta_y0:4;
        int delta_x1:4;
        int delta_y1:4;
        int delta_x2:4;
        int delta_y2:4;
        int delta_x3:4;
        int delta_y3:4;
    } scoreboard1;

    struct {
        int delta_x4:4;
        int delta_y4:4;
        int delta_x5:4;
        int delta_y5:4;
        int delta_x6:4;
        int delta_y6:4;
        int delta_x7:4;
        int delta_y7:4;
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
	unsigned int pad:2;
	unsigned int render_cache_read_mode:1;
	unsigned int cube_map_corner_mode:1;
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

struct i965_sampler_8x8
{
    struct {
        unsigned int pad0:16;
        unsigned int chroma_key_index:2;
        unsigned int chroma_key_enable:1;
        unsigned int pad1:8;
        unsigned int ief_filter_size:1;
        unsigned int ief_filter_type:1;
        unsigned int ief_bypass:1;
        unsigned int pad2:1;
        unsigned int avs_filter_type:1;
    } dw0;

    struct {
        unsigned int pad0:5;
        unsigned int sampler_8x8_state_pointer:27;
    } dw1;
    
    struct {
        unsigned int weak_edge_threshold:4;
        unsigned int strong_edge_threshold:4;
        unsigned int global_noise_estimation:8;
        unsigned int pad0:16;
    } dw2;

    struct {
        unsigned int r3x_coefficient:5;
        unsigned int pad0:1;
        unsigned int r3c_coefficient:5;
        unsigned int pad1:3;
        unsigned int gain_factor:6;
        unsigned int non_edge_weight:3;
        unsigned int pad2:1;
        unsigned int regular_weight:3;
        unsigned int pad3:1;
        unsigned int strong_edge_weight:3;
        unsigned int pad4:1;
    } dw3;

    struct {
        unsigned int pad0:2;
        unsigned int mr_boost:1;
        unsigned int mr_threshold:4;
        unsigned int steepness_boost:1;
        unsigned int steepness_threshold:4;
        unsigned int pad1:2;
        unsigned int r5x_coefficient:5;
        unsigned int pad2:1;
        unsigned int r5cx_coefficient:5;
        unsigned int pad3:1;
        unsigned int r5c_coefficient:5;
        unsigned int pad4:1;
    } dw4;

    struct {
        unsigned int pwl1_point_1:8;
        unsigned int pwl1_point_2:8;
        unsigned int pwl1_point_3:8;
        unsigned int pwl1_point_4:8;
    } dw5;

    struct {
        unsigned int pwl1_point_5:8;
        unsigned int pwl1_point_6:8;
        unsigned int pwl1_r3_bias_0:8;
        unsigned int pwl1_r3_bias_1:8;
    } dw6;

    struct {
        unsigned int pwl1_r3_bias_2:8;
        unsigned int pwl1_r3_bias_3:8;
        unsigned int pwl1_r3_bias_4:8;
        unsigned int pwl1_r3_bias_5:8;
    } dw7;

    struct {
        unsigned int pwl1_r3_bias_6:8;
        unsigned int pwl1_r5_bias_0:8;
        unsigned int pwl1_r5_bias_1:8;
        unsigned int pwl1_r5_bias_2:8;
    } dw8;

    struct {
        unsigned int pwl1_r5_bias_3:8;
        unsigned int pwl1_r5_bias_4:8;
        unsigned int pwl1_r5_bias_5:8;
        unsigned int pwl1_r5_bias_6:8;
    } dw9;

    struct {
        int pwl1_r3_slope_0:8;
        int pwl1_r3_slope_1:8;
        int pwl1_r3_slope_2:8;
        int pwl1_r3_slope_3:8;
    } dw10;

    struct {
        int pwl1_r3_slope_4:8;
        int pwl1_r3_slope_5:8;
        int pwl1_r3_slope_6:8;
        int pwl1_r5_slope_0:8;
    } dw11;

    struct {
        int pwl1_r5_slope_1:8;
        int pwl1_r5_slope_2:8;
        int pwl1_r5_slope_3:8;
        int pwl1_r5_slope_4:8;
    } dw12;

    struct {
        int pwl1_r5_slope_5:8;
        int pwl1_r5_slope_6:8;
        unsigned int limiter_boost:4;
        unsigned int pad0:4;
        unsigned int minimum_limiter:4;
        unsigned int maximum_limiter:4;
    } dw13;

    struct {
        unsigned int pad0:8;
        unsigned int clip_limiter:10;
        unsigned int pad1:14;
    } dw14;

    unsigned int dw15; /* Just a pad */
};

struct i965_sampler_8x8_coefficient
{
    struct {
        int table_0x_filter_c0:8;
        int table_0x_filter_c1:8;
        int table_0x_filter_c2:8;
        int table_0x_filter_c3:8;
    } dw0;

    struct {
        int table_0x_filter_c4:8;
        int table_0x_filter_c5:8;
        int table_0x_filter_c6:8;
        int table_0x_filter_c7:8;
    } dw1;

    struct {
        int table_0y_filter_c0:8;
        int table_0y_filter_c1:8;
        int table_0y_filter_c2:8;
        int table_0y_filter_c3:8;
    } dw2;

    struct {
        int table_0y_filter_c4:8;
        int table_0y_filter_c5:8;
        int table_0y_filter_c6:8;
        int table_0y_filter_c7:8;
    } dw3;

    struct {
        int pad0:16;
        int table_1x_filter_c2:8;
        int table_1x_filter_c3:8;
    } dw4;

    struct {
        int table_1x_filter_c4:8;
        int table_1x_filter_c5:8;
        int pad0:16;
    } dw5;

    struct {
        int pad0:16;
        int table_1y_filter_c2:8;
        int table_1y_filter_c3:8;
    } dw6;

    struct {
        int table_1y_filter_c4:8;
        int table_1y_filter_c5:8;
        int pad0:16;
    } dw7;
};

struct i965_sampler_8x8_state
{
    struct i965_sampler_8x8_coefficient coefficients[17];

    struct {
        unsigned int transition_area_with_8_pixels:3;
        unsigned int pad0:1;
        unsigned int transition_area_with_4_pixels:3;
        unsigned int pad1:1;
        unsigned int max_derivative_8_pixels:8;
        unsigned int max_derivative_4_pixels:8;
        unsigned int default_sharpness_level:8;
    } dw136;

    struct {
        unsigned int bit_field_name:1;
        unsigned int adaptive_filter_for_all_channel:1;
        unsigned int pad0:19;
        unsigned int bypass_y_adaptive_filtering:1;
        unsigned int bypass_x_adaptive_filtering:1;
        unsigned int pad1:9;
    } dw137;
};

struct i965_surface_state2
{
    struct {
        unsigned int surface_base_address;
    } ss0;

    struct {
        unsigned int cbcr_pixel_offset_v_direction:2;
        unsigned int pad0:4;
        unsigned int width:13;
        unsigned int height:13;
    } ss1;

    struct {
        unsigned int tile_walk:1;
        unsigned int tiled_surface:1;
        unsigned int half_pitch_for_chroma:1;
        unsigned int pitch:17;
        unsigned int pad0:2;
        unsigned int surface_object_control_data:4;
        unsigned int pad1:1;
        unsigned int interleave_chroma:1;
        unsigned int surface_format:4;
    } ss2;

    struct {
        unsigned int y_offset_for_cb:13;
        unsigned int pad0:3;
        unsigned int x_offset_for_cb:13;
        unsigned int pad1:3;
    } ss3;

    struct {
        unsigned int y_offset_for_cr:13;
        unsigned int pad0:3;
        unsigned int x_offset_for_cr:13;
        unsigned int pad1:3;
    } ss4;
};

struct i965_sampler_dndi
{
    struct {
        unsigned int denoise_asd_threshold:8;
        unsigned int denoise_history_delta:8;
        unsigned int denoise_maximum_history:8;
        unsigned int denoise_stad_threshold:8;
    } dw0;

    struct {
        unsigned int denoise_threshold_for_sum_of_complexity_measure:8;
        unsigned int denoise_moving_pixel_threshold:5;
        unsigned int stmm_c2:3;
        unsigned int low_temporal_difference_threshold:6;
        unsigned int pad0:2;
        unsigned int temporal_difference_threshold:6;
        unsigned int pad1:2;
    } dw1;

    struct {
        unsigned int block_noise_estimate_noise_threshold:8;
        unsigned int block_noise_estimate_edge_threshold:8; 
        unsigned int denoise_edge_threshold:8;
        unsigned int good_neighbor_threshold:8;
   } dw2;

    struct {
        unsigned int maximum_stmm:8;
        unsigned int multipler_for_vecm:6;
        unsigned int pad0:2;
        unsigned int blending_constant_across_time_for_small_values_of_stmm:8;
        unsigned int blending_constant_across_time_for_large_values_of_stmm:7;
        unsigned int stmm_blending_constant_select:1;
    } dw3;

    struct {
        unsigned int sdi_delta:8;
        unsigned int sdi_threshold:8;
        unsigned int stmm_output_shift:4;
        unsigned int stmm_shift_up:2;
        unsigned int stmm_shift_down:2;
        unsigned int minimum_stmm:8;
    } dw4;

    struct {
        unsigned int fmd_temporal_difference_threshold:8;
        unsigned int sdi_fallback_mode_2_constant:8;
        unsigned int sdi_fallback_mode_1_t2_constant:8;
        unsigned int sdi_fallback_mode_1_t1_constant:8;
    } dw5;

    struct {
        unsigned int dn_enable:1;
        unsigned int di_enable:1;
        unsigned int di_partial:1;
        unsigned int dndi_top_first:1;
        unsigned int dndi_stream_id:1;
        unsigned int dndi_first_frame:1;
        unsigned int progressive_dn:1;
        unsigned int pad0:1;
        unsigned int fmd_tear_threshold:6;
        unsigned int pad1:2;
        unsigned int fmd2_vertical_difference_threshold:8;
        unsigned int fmd1_vertical_difference_threshold:8;
    } dw6;

    struct {
        unsigned int pad0:8;
        unsigned int fmd_for_1st_field_of_current_frame:2;
        unsigned int pad1:6;
        unsigned int fmd_for_2nd_field_of_previous_frame:2;
        unsigned int vdi_walker_enable:1;
        unsigned int pad2:4;
        unsigned int column_width_minus1:9;
    } dw7;
};


struct gen6_blend_state
{
    struct {
        unsigned int dest_blend_factor:5;
        unsigned int source_blend_factor:5;
        unsigned int pad3:1;
        unsigned int blend_func:3;
        unsigned int pad2:1;
        unsigned int ia_dest_blend_factor:5;
        unsigned int ia_source_blend_factor:5;
        unsigned int pad1:1;
        unsigned int ia_blend_func:3;
        unsigned int pad0:1;
        unsigned int ia_blend_enable:1;
        unsigned int blend_enable:1;
    } blend0;

    struct {
        unsigned int post_blend_clamp_enable:1;
        unsigned int pre_blend_clamp_enable:1;
        unsigned int clamp_range:2;
        unsigned int pad0:4;
        unsigned int x_dither_offset:2;
        unsigned int y_dither_offset:2;
        unsigned int dither_enable:1;
        unsigned int alpha_test_func:3;
        unsigned int alpha_test_enable:1;
        unsigned int pad1:1;
        unsigned int logic_op_func:4;
        unsigned int logic_op_enable:1;
        unsigned int pad2:1;
        unsigned int write_disable_b:1;
        unsigned int write_disable_g:1;
        unsigned int write_disable_r:1;
        unsigned int write_disable_a:1;
        unsigned int pad3:1;
        unsigned int alpha_to_coverage_dither:1;
        unsigned int alpha_to_one:1;
        unsigned int alpha_to_coverage:1;
    } blend1;
};

struct gen6_color_calc_state
{
    struct {
        unsigned int alpha_test_format:1;
        unsigned int pad0:14;
        unsigned int round_disable:1;
        unsigned int bf_stencil_ref:8;
        unsigned int stencil_ref:8;
    } cc0;

    union {
        float alpha_ref_f;
        struct {
            unsigned int ui:8;
            unsigned int pad0:24;
        } alpha_ref_fi;
    } cc1;

    float constant_r;
    float constant_g;
    float constant_b;
    float constant_a;
};

struct gen6_depth_stencil_state
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
    } ds0;

    struct {
        unsigned int bf_stencil_write_mask:8;
        unsigned int bf_stencil_test_mask:8;
        unsigned int stencil_write_mask:8;
        unsigned int stencil_test_mask:8;
    } ds1;

    struct {
        unsigned int pad0:26;
        unsigned int depth_write_enable:1;
        unsigned int depth_test_func:3;
        unsigned int pad1:1;
        unsigned int depth_test_enable:1;
    } ds2;
};

struct gen6_interface_descriptor_data
{
    struct {
        unsigned int pad0:6;
        unsigned int kernel_start_pointer:26;
    } desc0;
    
    struct {
        unsigned int pad0:7;
        unsigned int software_exception_enable:1;
        unsigned int pad1:3;
        unsigned int maskstack_exception_enable:1;
        unsigned int pad2:1;
        unsigned int illegal_opcode_exception_enable:1;
        unsigned int pad3:2;
        unsigned int floating_point_mode:1;
        unsigned int thread_priority:1;
        unsigned int single_program_flow:1;
        unsigned int pad4:13;
    } desc1;

    struct {
        unsigned int pad0:2;
        unsigned int sampler_count:3;
        unsigned int sampler_state_pointer:27;
    } desc2;

    struct {
        unsigned int binding_table_entry_count:5;
        unsigned int binding_table_pointer:27;
    } desc3;

    struct {
        unsigned int constant_urb_entry_read_offset:16;
        unsigned int constant_urb_entry_read_length:16;
    } desc4;
    
    union {
        struct {
            unsigned int num_threads:8;
            unsigned int barrier_return_byte:8;
            unsigned int shared_local_memory_size:5;
            unsigned int barrier_enable:1;
            unsigned int rounding_mode:2;
            unsigned int barrier_return_grf_offset:8;
        } gen7;

        struct {
            unsigned int barrier_id:4;
            unsigned int pad0:28;
        } gen6;
    } desc5;

    struct {
        unsigned int cross_thread_constant_data_read_length:8;
        unsigned int pad0:24;
    } desc6;

    struct {
        unsigned int pad0;
    } desc7;
};

struct gen7_surface_state
{
    struct {
        unsigned int cube_pos_z:1;
        unsigned int cube_neg_z:1;
        unsigned int cube_pos_y:1;
        unsigned int cube_neg_y:1;
        unsigned int cube_pos_x:1;
        unsigned int cube_neg_x:1;
        unsigned int pad2:2;
        unsigned int render_cache_read_write:1;
        unsigned int pad1:1;
        unsigned int surface_array_spacing:1;
        unsigned int vert_line_stride_ofs:1;
        unsigned int vert_line_stride:1;
        unsigned int tile_walk:1;
        unsigned int tiled_surface:1;
        unsigned int horizontal_alignment:1;
        unsigned int vertical_alignment:2;
        unsigned int surface_format:9;     /**< BRW_SURFACEFORMAT_x */
        unsigned int pad0:1;
        unsigned int is_array:1;
        unsigned int surface_type:3;       /**< BRW_SURFACE_1D/2D/3D/CUBE */
    } ss0;

    struct {
        unsigned int base_addr;
    } ss1;

    struct {
        unsigned int width:14;
        unsigned int pad1:2;
        unsigned int height:14;
        unsigned int pad0:2;
    } ss2;

    struct {
        unsigned int pitch:18;
        unsigned int pad:3;
        unsigned int depth:11;
    } ss3;

    struct {
        unsigned int multisample_position_palette_index:3;
        unsigned int num_multisamples:3;
        unsigned int multisampled_surface_storage_format:1;
        unsigned int render_target_view_extent:11;
        unsigned int min_array_elt:11;
        unsigned int rotation:2;
        unsigned int pad0:1;
    } ss4;

    struct {
        unsigned int mip_count:4;
        unsigned int min_lod:4;
        unsigned int pad1:12;
        unsigned int y_offset:4;
        unsigned int pad0:1;
        unsigned int x_offset:7;
    } ss5;

    struct {
        unsigned int pad; /* Multisample Control Surface stuff */
    } ss6;

    struct {
        unsigned int resource_min_lod:12;
        unsigned int pad0:16;
        unsigned int alpha_clear_color:1;
        unsigned int blue_clear_color:1;
        unsigned int green_clear_color:1;
        unsigned int red_clear_color:1;
    } ss7;
};

struct gen7_sampler_state
{
   struct
   {
      unsigned int aniso_algorithm:1;
      unsigned int lod_bias:13;
      unsigned int min_filter:3;
      unsigned int mag_filter:3;
      unsigned int mip_filter:2;
      unsigned int base_level:5;
      unsigned int pad1:1;
      unsigned int lod_preclamp:1;
      unsigned int default_color_mode:1;
      unsigned int pad0:1;
      unsigned int disable:1;
   } ss0;

   struct
   {
      unsigned int cube_control_mode:1;
      unsigned int shadow_function:3;
      unsigned int pad:4;
      unsigned int max_lod:12;
      unsigned int min_lod:12;
   } ss1;

   struct
   {
      unsigned int pad:5;
      unsigned int default_color_pointer:27;
   } ss2;

   struct
   {
      unsigned int r_wrap_mode:3;
      unsigned int t_wrap_mode:3;
      unsigned int s_wrap_mode:3;
      unsigned int pad:1;
      unsigned int non_normalized_coord:1;
      unsigned int trilinear_quality:2;
      unsigned int address_round:6;
      unsigned int max_aniso:3;
      unsigned int chroma_key_mode:1;
      unsigned int chroma_key_index:2;
      unsigned int chroma_key_enable:1;
      unsigned int pad0:6;
   } ss3;
};

struct gen7_surface_state2
{
    struct {
        unsigned int surface_base_address;
    } ss0;

    struct {
        unsigned int cbcr_pixel_offset_v_direction:2;
        unsigned int picture_structure:2;
        unsigned int width:14;
        unsigned int height:14;
    } ss1;

    struct {
        unsigned int tile_walk:1;
        unsigned int tiled_surface:1;
        unsigned int half_pitch_for_chroma:1;
        unsigned int pitch:18;
        unsigned int pad0:1;
        unsigned int surface_object_control_data:4;
        unsigned int pad1:1;
        unsigned int interleave_chroma:1;
        unsigned int surface_format:4;
    } ss2;

    struct {
        unsigned int y_offset_for_cb:15;
        unsigned int pad0:1;
        unsigned int x_offset_for_cb:14;
        unsigned int pad1:2;
    } ss3;

    struct {
        unsigned int y_offset_for_cr:15;
        unsigned int pad0:1;
        unsigned int x_offset_for_cr:14;
        unsigned int pad1:2;
    } ss4;

    struct {
        unsigned int pad0;
    } ss5;

    struct {
        unsigned int pad0;
    } ss6;

    struct {
        unsigned int pad0;
    } ss7;
};

#endif /* _I965_STRUCTS_H_ */
