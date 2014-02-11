/*
 * Copyright (c) 2007-2011 Intel Corporation. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL INTEL AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file va_vpp.h
 * \brief The video processing API
 *
 * This file contains the \ref api_vpp "Video processing API".
 */

#ifndef VA_VPP_H
#define VA_VPP_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup api_vpp Video processing API
 *
 * @{
 *
 * The video processing API uses the same paradigm as for decoding:
 * - Query for supported filters;
 * - Set up a video processing pipeline;
 * - Send video processing parameters through VA buffers.
 *
 * \section api_vpp_caps Query for supported filters
 *
 * Checking whether video processing is supported can be performed
 * with vaQueryConfigEntrypoints() and the profile argument set to
 * #VAProfileNone. If video processing is supported, then the list of
 * returned entry-points will include #VAEntrypointVideoProc.
 *
 * \code
 * VAEntrypoint *entrypoints;
 * int i, num_entrypoints, supportsVideoProcessing = 0;
 *
 * num_entrypoints = vaMaxNumEntrypoints();
 * entrypoints = malloc(num_entrypoints * sizeof(entrypoints[0]);
 * vaQueryConfigEntrypoints(va_dpy, VAProfileNone,
 *     entrypoints, &num_entrypoints);
 *
 * for (i = 0; !supportsVideoProcessing && i < num_entrypoints; i++) {
 *     if (entrypoints[i] == VAEntrypointVideoProc)
 *         supportsVideoProcessing = 1;
 * }
 * \endcode
 *
 * Then, the vaQueryVideoProcFilters() function is used to query the
 * list of video processing filters.
 *
 * \code
 * VAProcFilterType filters[VAProcFilterCount];
 * unsigned int num_filters = VAProcFilterCount;
 *
 * // num_filters shall be initialized to the length of the array
 * vaQueryVideoProcFilters(va_dpy, vpp_ctx, &filters, &num_filters);
 * \endcode
 *
 * Finally, individual filter capabilities can be checked with
 * vaQueryVideoProcFilterCaps().
 *
 * \code
 * VAProcFilterCap denoise_caps;
 * unsigned int num_denoise_caps = 1;
 * vaQueryVideoProcFilterCaps(va_dpy, vpp_ctx,
 *     VAProcFilterNoiseReduction,
 *     &denoise_caps, &num_denoise_caps
 * );
 *
 * VAProcFilterCapDeinterlacing deinterlacing_caps[VAProcDeinterlacingCount];
 * unsigned int num_deinterlacing_caps = VAProcDeinterlacingCount;
 * vaQueryVideoProcFilterCaps(va_dpy, vpp_ctx,
 *     VAProcFilterDeinterlacing,
 *     &deinterlacing_caps, &num_deinterlacing_caps
 * );
 * \endcode
 *
 * \section api_vpp_setup Set up a video processing pipeline
 *
 * A video processing pipeline buffer is created for each source
 * surface we want to process. However, buffers holding filter
 * parameters can be created once and for all. Rationale is to avoid
 * multiple creation/destruction chains of filter buffers and also
 * because filter parameters generally won't change frame after
 * frame. e.g. this makes it possible to implement a checkerboard of
 * videos where the same filters are applied to each video source.
 *
 * The general control flow is demonstrated by the following pseudo-code:
 * \code
 * // Create filters
 * VABufferID denoise_filter, deint_filter;
 * VABufferID filter_bufs[VAProcFilterCount];
 * unsigned int num_filter_bufs;
 *
 * for (i = 0; i < num_filters; i++) {
 *     switch (filters[i]) {
 *     case VAProcFilterNoiseReduction: {       // Noise reduction filter
 *         VAProcFilterParameterBuffer denoise;
 *         denoise.type  = VAProcFilterNoiseReduction;
 *         denoise.value = 0.5;
 *         vaCreateBuffer(va_dpy, vpp_ctx,
 *             VAProcFilterParameterBufferType, sizeof(denoise), 1,
 *             &denoise, &denoise_filter
 *         );
 *         filter_bufs[num_filter_bufs++] = denoise_filter;
 *         break;
 *     }
 *
 *     case VAProcFilterDeinterlacing:          // Motion-adaptive deinterlacing
 *         for (j = 0; j < num_deinterlacing_caps; j++) {
 *             VAProcFilterCapDeinterlacing * const cap = &deinterlacing_caps[j];
 *             if (cap->type != VAProcDeinterlacingMotionAdaptive)
 *                 continue;
 *
 *             VAProcFilterParameterBufferDeinterlacing deint;
 *             deint.type                   = VAProcFilterDeinterlacing;
 *             deint.algorithm              = VAProcDeinterlacingMotionAdaptive;
 *             vaCreateBuffer(va_dpy, vpp_ctx,
 *                 VAProcFilterParameterBufferType, sizeof(deint), 1,
 *                 &deint, &deint_filter
 *             );
 *             filter_bufs[num_filter_bufs++] = deint_filter;
 *         }
 *     }
 * }
 * \endcode
 *
 * Once the video processing pipeline is set up, the caller shall check the
 * implied capabilities and requirements with vaQueryVideoProcPipelineCaps().
 * This function can be used to validate the number of reference frames are
 * needed by the specified deinterlacing algorithm, the supported color
 * primaries, etc.
 * \code
 * // Create filters
 * VAProcPipelineCaps pipeline_caps;
 * VASurfaceID *forward_references;
 * unsigned int num_forward_references;
 * VASurfaceID *backward_references;
 * unsigned int num_backward_references;
 * VAProcColorStandardType in_color_standards[VAProcColorStandardCount];
 * VAProcColorStandardType out_color_standards[VAProcColorStandardCount];
 *
 * pipeline_caps.input_color_standards      = NULL;
 * pipeline_caps.num_input_color_standards  = ARRAY_ELEMS(in_color_standards);
 * pipeline_caps.output_color_standards     = NULL;
 * pipeline_caps.num_output_color_standards = ARRAY_ELEMS(out_color_standards);
 * vaQueryVideoProcPipelineCaps(va_dpy, vpp_ctx,
 *     filter_bufs, num_filter_bufs,
 *     &pipeline_caps
 * );
 *
 * num_forward_references  = pipeline_caps.num_forward_references;
 * forward_references      =
 *     malloc(num__forward_references * sizeof(VASurfaceID));
 * num_backward_references = pipeline_caps.num_backward_references;
 * backward_references     =
 *     malloc(num_backward_references * sizeof(VASurfaceID));
 * \endcode
 *
 * \section api_vpp_submit Send video processing parameters through VA buffers
 *
 * Video processing pipeline parameters are submitted for each source
 * surface to process. Video filter parameters can also change, per-surface.
 * e.g. the list of reference frames used for deinterlacing.
 *
 * \code
 * foreach (iteration) {
 *     vaBeginPicture(va_dpy, vpp_ctx, vpp_surface);
 *     foreach (surface) {
 *         VARectangle output_region;
 *         VABufferID pipeline_buf;
 *         VAProcPipelineParameterBuffer *pipeline_param;
 *
 *         vaCreateBuffer(va_dpy, vpp_ctx,
 *             VAProcPipelineParameterBuffer, sizeof(*pipeline_param), 1,
 *             NULL, &pipeline_buf
 *         );
 *
 *         // Setup output region for this surface
 *         // e.g. upper left corner for the first surface
 *         output_region.x     = BORDER;
 *         output_region.y     = BORDER;
 *         output_region.width =
 *             (vpp_surface_width - (Nx_surfaces + 1) * BORDER) / Nx_surfaces;
 *         output_region.height =
 *             (vpp_surface_height - (Ny_surfaces + 1) * BORDER) / Ny_surfaces;
 *
 *         vaMapBuffer(va_dpy, pipeline_buf, &pipeline_param);
 *         pipeline_param->surface              = surface;
 *         pipeline_param->surface_region       = NULL;
 *         pipeline_param->output_region        = &output_region;
 *         pipeline_param->output_background_color = 0;
 *         if (first surface to render)
 *             pipeline_param->output_background_color = 0xff000000; // black
 *         pipeline_param->filter_flags         = VA_FILTER_SCALING_HQ;
 *         pipeline_param->filters              = filter_bufs;
 *         pipeline_param->num_filters          = num_filter_bufs;
 *         vaUnmapBuffer(va_dpy, pipeline_buf);
 *
 *         // Update reference frames for deinterlacing, if necessary
 *         pipeline_param->forward_references      = forward_references;
 *         pipeline_param->num_forward_references  = num_forward_references_used;
 *         pipeline_param->backward_references     = backward_references;
 *         pipeline_param->num_backward_references = num_bacward_references_used;
 *
 *         // Apply filters
 *         vaRenderPicture(va_dpy, vpp_ctx, &pipeline_buf, 1);
 *     }
 *     vaEndPicture(va_dpy, vpp_ctx);
 * }
 * \endcode
 */

/** \brief Video filter types. */
typedef enum _VAProcFilterType {
    VAProcFilterNone = 0,
    /** \brief Noise reduction filter. */
    VAProcFilterNoiseReduction,
    /** \brief Deinterlacing filter. */
    VAProcFilterDeinterlacing,
    /** \brief Sharpening filter. */
    VAProcFilterSharpening,
    /** \brief Color balance parameters. */
    VAProcFilterColorBalance,
    /** \brief Deblocking filter. */
    VAProcFilterDeblocking,
    /** \brief Frame rate conversion. */
    VAProcFilterFrameRateConversion,
    /** \brief Skin Tone Enhancement. */
    VAProcFilterSkinToneEnhancement,
    /** \brief Total Color Correction. */
    VAProcFilterTotalColorCorrection,
    /** \brief Non-Linear Anamorphic Scaling. */
    VAProcFilterNonLinearAnamorphicScaling,
    /** \brief Image Stabilization. */
    VAProcFilterImageStabilization,
    /** \brief Number of video filters. */
    VAProcFilterCount
} VAProcFilterType;

/** \brief Deinterlacing types. */
typedef enum _VAProcDeinterlacingType {
    VAProcDeinterlacingNone = 0,
    /** \brief Bob deinterlacing algorithm. */
    VAProcDeinterlacingBob,
    /** \brief Weave deinterlacing algorithm. */
    VAProcDeinterlacingWeave,
    /** \brief Motion adaptive deinterlacing algorithm. */
    VAProcDeinterlacingMotionAdaptive,
    /** \brief Motion compensated deinterlacing algorithm. */
    VAProcDeinterlacingMotionCompensated,
    /** \brief Number of deinterlacing algorithms. */
    VAProcDeinterlacingCount
} VAProcDeinterlacingType;

/** \brief Color balance types. */
typedef enum _VAProcColorBalanceType {
    VAProcColorBalanceNone = 0,
    /** \brief Hue. */
    VAProcColorBalanceHue,
    /** \brief Saturation. */
    VAProcColorBalanceSaturation,
    /** \brief Brightness. */
    VAProcColorBalanceBrightness,
    /** \brief Contrast. */
    VAProcColorBalanceContrast,
    /** \brief Automatically adjusted saturation. */
    VAProcColorBalanceAutoSaturation,
    /** \brief Automatically adjusted brightness. */
    VAProcColorBalanceAutoBrightness,
    /** \brief Automatically adjusted contrast. */
    VAProcColorBalanceAutoContrast,
    /** \brief Number of color balance attributes. */
    VAProcColorBalanceCount
} VAProcColorBalanceType;

/** \brief Color standard types. */
typedef enum _VAProcColorStandardType {
    VAProcColorStandardNone = 0,
    /** \brief ITU-R BT.601. */
    VAProcColorStandardBT601,
    /** \brief ITU-R BT.709. */
    VAProcColorStandardBT709,
    /** \brief ITU-R BT.470-2 System M. */
    VAProcColorStandardBT470M,
    /** \brief ITU-R BT.470-2 System B, G. */
    VAProcColorStandardBT470BG,
    /** \brief SMPTE-170M. */
    VAProcColorStandardSMPTE170M,
    /** \brief SMPTE-240M. */
    VAProcColorStandardSMPTE240M,
    /** \brief Generic film. */
    VAProcColorStandardGenericFilm,
    /** \brief sRGB. */
    VAProcColorStandardSRGB,
    /** \brief stRGB. */
    VAProcColorStandardSTRGB,
    /** \brief xvYCC601. */
    VAProcColorStandardXVYCC601,
    /** \brief xvYCC709. */
    VAProcColorStandardXVYCC709,
    /** \brief ITU-R BT.2020. */
    VAProcColorStandardBT2020,
    /** \brief Number of color standards. */
    VAProcColorStandardCount
} VAProcColorStandardType;

/** \brief Total color correction types. */
typedef enum _VAProcTotalColorCorrectionType {
    VAProcTotalColorCorrectionNone = 0,
    /** \brief Red Saturation. */
    VAProcTotalColorCorrectionRed,
    /** \brief Green Saturation. */
    VAProcTotalColorCorrectionGreen,
    /** \brief Blue Saturation. */
    VAProcTotalColorCorrectionBlue,
    /** \brief Cyan Saturation. */
    VAProcTotalColorCorrectionCyan,
    /** \brief Magenta Saturation. */
    VAProcTotalColorCorrectionMagenta,
    /** \brief Yellow Saturation. */
    VAProcTotalColorCorrectionYellow,
    /** \brief Number of color correction attributes. */
    VAProcTotalColorCorrectionCount
} VAProcTotalColorCorrectionType;

/** \brief ImageStabilization Types. */
typedef enum _VAProcImageStabilizationType {
    VAProcImageStabilizationTypeNone = 0,
    /** \brief Mode Crop - crops the frame by the app provided percentage. */
    VAProcImageStabilizationTypeCrop,
    /** \brief Mode Crop Min Zoom - crops and then upscales the frame to half the black boundary. */
    VAProcImageStabilizationTypeMinZoom,
    /** \brief Mode Crop Full Zoom - crops and upscales the frame to original size. */
    VAProcImageStabilizationTypeFullZoom,
    /** \brief Number of Image Stabilization Type. */
    VAProcImageStabilizationTypeCount
} VAProcImageStabilizationType;

/** @name Video blending flags */
/**@{*/
/** \brief Global alpha blending. */
#define VA_BLEND_GLOBAL_ALPHA           0x0002
/** \brief Premultiplied alpha blending (RGBA surfaces only). */
#define VA_BLEND_PREMULTIPLIED_ALPHA    0x0008
/** \brief Luma color key (YUV surfaces only). */
#define VA_BLEND_LUMA_KEY               0x0010
/**@}*/

/** \brief Video blending state definition. */
typedef struct _VABlendState {
    /** \brief Video blending flags. */
    unsigned int        flags;
    /**
     * \brief Global alpha value.
     *
     * Valid if \flags has VA_BLEND_GLOBAL_ALPHA.
     * Valid range is 0.0 to 1.0 inclusive.
     */
    float               global_alpha;
    /**
     * \brief Minimum luma value.
     *
     * Valid if \flags has VA_BLEND_LUMA_KEY.
     * Valid range is 0.0 to 1.0 inclusive.
     * \ref min_luma shall be set to a sensible value lower than \ref max_luma.
     */
    float               min_luma;
    /**
     * \brief Maximum luma value.
     *
     * Valid if \flags has VA_BLEND_LUMA_KEY.
     * Valid range is 0.0 to 1.0 inclusive.
     * \ref max_luma shall be set to a sensible value larger than \ref min_luma.
     */
    float               max_luma;
} VABlendState;

/** @name Video pipeline flags */
/**@{*/
/** \brief Specifies whether to apply subpictures when processing a surface. */
#define VA_PROC_PIPELINE_SUBPICTURES    0x00000001
/**
 * \brief Specifies whether to apply power or performance
 * optimizations to a pipeline.
 *
 * When processing several surfaces, it may be necessary to prioritize
 * more certain pipelines than others. This flag is only a hint to the
 * video processor so that it can omit certain filters to save power
 * for example. Typically, this flag could be used with video surfaces
 * decoded from a secondary bitstream.
 */
#define VA_PROC_PIPELINE_FAST           0x00000002
/**@}*/

/** @name Video filter flags */
/**@{*/
/** \brief Specifies whether the filter shall be present in the pipeline. */
#define VA_PROC_FILTER_MANDATORY        0x00000001
/**@}*/

/** @name Pipeline end flags */
/**@{*/
/** \brief Specifies the pipeline is the last. */
#define VA_PIPELINE_FLAG_END		0x00000004
/**@}*/

/** @name Chroma Siting flag */
/**@{*/
#define VA_CHROMA_SITING_UNKNOWN              0x00000000
/** \brief Chroma samples are co-sited vertically on the top with the luma samples. */
#define VA_CHROMA_SITING_VERTICAL_TOP         0x00000001
/** \brief Chroma samples are not co-sited vertically with the luma samples. */
#define VA_CHROMA_SITING_VERTICAL_CENTER      0x00000002
/** \brief Chroma samples are co-sited vertically on the bottom with the luma samples. */
#define VA_CHROMA_SITING_VERTICAL_BOTTOM      0x00000003
/** \brief Chroma samples are co-sited horizontally on the left with the luma samples. */
#define VA_CHROMA_SITING_HORIZONTAL_LEFT      0x00000004
/** \brief Chroma samples are not co-sited horizontally with the luma samples. */
#define VA_CHROMA_SITING_HORIZONTAL_CENTER    0x00000008
/**@}*/

/** \brief Video processing pipeline capabilities. */
typedef struct _VAProcPipelineCaps {
    /** \brief Pipeline flags. See VAProcPipelineParameterBuffer::pipeline_flags. */
    unsigned int        pipeline_flags;
    /** \brief Extra filter flags. See VAProcPipelineParameterBuffer::filter_flags. */
    unsigned int        filter_flags;
    /** \brief Number of forward reference frames that are needed. */
    unsigned int        num_forward_references;
    /** \brief Number of backward reference frames that are needed. */
    unsigned int        num_backward_references;
    /** \brief List of color standards supported on input. */
    VAProcColorStandardType *input_color_standards;
    /** \brief Number of elements in \ref input_color_standards array. */
    unsigned int        num_input_color_standards;
    /** \brief List of color standards supported on output. */
    VAProcColorStandardType *output_color_standards;
    /** \brief Number of elements in \ref output_color_standards array. */
    unsigned int        num_output_color_standards;
    /**
     * \brief Rotation flags.
     *
     * For each rotation angle supported by the underlying hardware,
     * the corresponding bit is set in \ref rotation_flags. See
     * "Rotation angles" for a description of rotation angles.
     *
     * A value of 0 means the underlying hardware does not support any
     * rotation. Otherwise, a check for a specific rotation angle can be
     * performed as follows:
     *
     * \code
     * VAProcPipelineCaps pipeline_caps;
     * ...
     * vaQueryVideoProcPipelineCaps(va_dpy, vpp_ctx,
     *     filter_bufs, num_filter_bufs,
     *     &pipeline_caps
     * );
     * ...
     * if (pipeline_caps.rotation_flags & (1 << VA_ROTATION_xxx)) {
     *     // Clockwise rotation by xxx degrees is supported
     *     ...
     * }
     * \endcode
     */
    unsigned int        rotation_flags;
    /** \brief Blend flags. See "Video blending flags". */
    unsigned int        blend_flags;
    /**
     * \brief Mirroring flags.
     *
     * For each mirroring direction supported by the underlying hardware,
     * the corresponding bit is set in \ref mirror_flags. See
     * "Mirroring directions" for a description of mirroring directions.
     *
     */
    unsigned int        mirror_flags;
    /** \brief Number of additional output surfaces supported by the pipeline  */
    unsigned int        num_additional_outputs;
} VAProcPipelineCaps;

/** \brief Specification of values supported by the filter. */
typedef struct _VAProcFilterValueRange {
    /** \brief Minimum value supported, inclusive. */
    float               min_value;
    /** \brief Maximum value supported, inclusive. */
    float               max_value;
    /** \brief Default value. */
    float               default_value;
    /** \brief Step value that alters the filter behaviour in a sensible way. */
    float               step;
} VAProcFilterValueRange;

/**
 * \brief Video processing pipeline configuration.
 *
 * This buffer defines a video processing pipeline. As for any buffer
 * passed to \c vaRenderPicture(), this is a one-time usage model.
 * However, the actual filters to be applied are provided in the
 * \c filters field, so they can be re-used in other processing
 * pipelines.
 *
 * The target surface is specified by the \c render_target argument of
 * \c vaBeginPicture(). The general usage model is described as follows:
 * - \c vaBeginPicture(): specify the target surface that receives the
 *   processed output;
 * - \c vaRenderPicture(): specify a surface to be processed and composed
 *   into the \c render_target. Use as many \c vaRenderPicture() calls as
 *   necessary surfaces to compose ;
 * - \c vaEndPicture(): tell the driver to start processing the surfaces
 *   with the requested filters.
 *
 * If a filter (e.g. noise reduction) needs to be applied with different
 * values for multiple surfaces, the application needs to create as many
 * filter parameter buffers as necessary. i.e. the filter parameters shall
 * not change between two calls to \c vaRenderPicture().
 *
 * For composition usage models, the first surface to process will generally
 * use an opaque background color, i.e. \c output_background_color set with
 * the most significant byte set to \c 0xff. For instance, \c 0xff000000 for
 * a black background. Then, subsequent surfaces would use a transparent
 * background color.
 */
typedef struct _VAProcPipelineParameterBuffer {
    /**
     * \brief Source surface ID.
     *
     * ID of the source surface to process. If subpictures are associated
     * with the video surfaces then they shall be rendered to the target
     * surface, if the #VA_PROC_PIPELINE_SUBPICTURES pipeline flag is set.
     */
    VASurfaceID         surface;
    /**
     * \brief Region within the source surface to be processed.
     *
     * Pointer to a #VARectangle defining the region within the source
     * surface to be processed. If NULL, \c surface_region implies the
     * whole surface.
     */
    const VARectangle  *surface_region;
    /**
     * \brief Requested input color primaries.
     *
     * Color primaries are implicitly converted throughout the processing
     * pipeline. The video processor chooses the best moment to apply
     * this conversion. The set of supported color primaries primaries
     * for input shall be queried with vaQueryVideoProcPipelineCaps().
     */
    VAProcColorStandardType surface_color_standard;
    /**
     * \brief Region within the output surface.
     *
     * Pointer to a #VARectangle defining the region within the output
     * surface that receives the processed pixels. If NULL, \c output_region
     * implies the whole surface. 
     *
     * Note that any pixels residing outside the specified region will
     * be filled in with the \ref output_background_color.
     */
    const VARectangle  *output_region;
    /**
     * \brief Background color.
     *
     * Background color used to fill in pixels that reside outside of the
     * specified \ref output_region. The color is specified in ARGB format:
     * [31:24] alpha, [23:16] red, [15:8] green, [7:0] blue.
     *
     * Unless the alpha value is zero or the \ref output_region represents
     * the whole target surface size, implementations shall not render the
     * source surface to the target surface directly. Rather, in order to
     * maintain the exact semantics of \ref output_background_color, the
     * driver shall use a temporary surface and fill it in with the
     * appropriate background color. Next, the driver will blend this
     * temporary surface into the target surface.
     */
    unsigned int        output_background_color;
    /**
     * \brief Requested output color primaries.
     */
    VAProcColorStandardType output_color_standard;
    /**
     * \brief Pipeline filters. See video pipeline flags.
     *
     * Flags to control the pipeline, like whether to apply subpictures
     * or not, notify the driver that it can opt for power optimizations,
     * should this be needed.
     */
    unsigned int        pipeline_flags;
    /**
     * \brief Extra filter flags. See vaPutSurface() flags.
     *
     * Filter flags are used as a fast path, wherever possible, to use
     * vaPutSurface() flags instead of explicit filter parameter buffers.
     *
     * Allowed filter flags API-wise. Use vaQueryVideoProcPipelineCaps()
     * to check for implementation details:
     * - Bob-deinterlacing: \c VA_FRAME_PICTURE, \c VA_TOP_FIELD,
     *   \c VA_BOTTOM_FIELD. Note that any deinterlacing filter
     *   (#VAProcFilterDeinterlacing) will override those flags.
     * - Color space conversion: \c VA_SRC_BT601, \c VA_SRC_BT709,
     *   \c VA_SRC_SMPTE_240. 
     * - Scaling: \c VA_FILTER_SCALING_DEFAULT, \c VA_FILTER_SCALING_FAST,
     *   \c VA_FILTER_SCALING_HQ, \c VA_FILTER_SCALING_NL_ANAMORPHIC.
     * - Enable auto noise reduction: \c VA_FILTER_NOISEREDUCTION_AUTO.
     */
    unsigned int        filter_flags;
    /**
     * \brief Array of filters to apply to the surface.
     *
     * The list of filters shall be ordered in the same way the driver expects
     * them. i.e. as was returned from vaQueryVideoProcFilters().
     * Otherwise, a #VA_STATUS_ERROR_INVALID_FILTER_CHAIN is returned
     * from vaRenderPicture() with this buffer.
     *
     * #VA_STATUS_ERROR_UNSUPPORTED_FILTER is returned if the list
     * contains an unsupported filter.
     *
     * Note: no filter buffer is destroyed after a call to vaRenderPicture(),
     * only this pipeline buffer will be destroyed as per the core API
     * specification. This allows for flexibility in re-using the filter for
     * other surfaces to be processed.
     */
    VABufferID         *filters;
    /** \brief Actual number of filters. */
    unsigned int        num_filters;
    /** \brief Array of forward reference frames. */
    VASurfaceID        *forward_references;
    /** \brief Number of forward reference frames that were supplied. */
    unsigned int        num_forward_references;
    /** \brief Array of backward reference frames. */
    VASurfaceID        *backward_references;
    /** \brief Number of backward reference frames that were supplied. */
    unsigned int        num_backward_references;
    /**
     * \brief Rotation state. See rotation angles.
     *
     * The rotation angle is clockwise. There is no specific rotation
     * center for this operation. Rather, The source \ref surface is
     * first rotated by the specified angle and then scaled to fit the
     * \ref output_region.
     *
     * This means that the top-left hand corner (0,0) of the output
     * (rotated) surface is expressed as follows:
     * - \ref VA_ROTATION_NONE: (0,0) is the top left corner of the
     *   source surface -- no rotation is performed ;
     * - \ref VA_ROTATION_90: (0,0) is the bottom-left corner of the
     *   source surface ;
     * - \ref VA_ROTATION_180: (0,0) is the bottom-right corner of the
     *   source surface -- the surface is flipped around the X axis ;
     * - \ref VA_ROTATION_270: (0,0) is the top-right corner of the
     *   source surface.
     *
     * Check VAProcPipelineCaps::rotation_flags first prior to
     * defining a specific rotation angle. Otherwise, the hardware can
     * perfectly ignore this variable if it does not support any
     * rotation.
     */
    unsigned int        rotation_state;
    /**
     * \brief blending state. See "Video blending state definition".
     *
     * If \ref blend_state is NULL, then default operation mode depends
     * on the source \ref surface format:
     * - RGB: per-pixel alpha blending ;
     * - YUV: no blending, i.e override the underlying pixels.
     *
     * Otherwise, \ref blend_state is a pointer to a #VABlendState
     * structure that shall be live until vaEndPicture().
     *
     * Implementation note: the driver is responsible for checking the
     * blend state flags against the actual source \ref surface format.
     * e.g. premultiplied alpha blending is only applicable to RGB
     * surfaces, and luma keying is only applicable to YUV surfaces.
     * If a mismatch occurs, then #VA_STATUS_ERROR_INVALID_BLEND_STATE
     * is returned.
     */
    const VABlendState *blend_state;
    /**
     * \bried mirroring state. See "Mirroring directions".
     * 
     * Mirroring of an image can be performed either along the 
     * horizontal or vertical axis. It is assumed that the rotation
     * operation is always performed before the mirroring operation.
     */
    unsigned int      mirror_state;
    /** \brief Array of additional output surfaces. */
    VASurfaceID        *additional_outputs;
    /** \brief Number of additional output surfaces. */
    unsigned int        num_additional_outputs;
    /**
     * \brief Flag to indicate the input surface flag such as chroma-siting,
     * range flag and so on.
     *
     * The lower 4 bits are still used as chroma-siting flag
     * The range_flag bit is used to indicate that the range flag of color-space conversion.
     * -\ref VA_SOURCE_RANGE_FULL(Full range): Y/Cb/Cr is in [0, 255].It is
     *   mainly used for JPEG/JFIF formats. The combination with the BT601 flag
     *   means that JPEG/JFIF color-space conversion matrix is used.
     * -\ref VA_SOURCE_RANGE_FULL(Reduced range): Y is in [16, 235] and Cb/Cr
     *   is in [16, 240]. It is mainly used for the YUV<->RGB color-space
     *   conversion in SDTV/HDTV/UHDTV.
     */
    unsigned int        input_surface_flag;
} VAProcPipelineParameterBuffer;

/**
 * \brief Filter parameter buffer base.
 *
 * This is a helper structure used by driver implementations only.
 * Users are not supposed to allocate filter parameter buffers of this
 * type.
 */
typedef struct _VAProcFilterParameterBufferBase {
    /** \brief Filter type. */
    VAProcFilterType    type;
} VAProcFilterParameterBufferBase;

/**
 * \brief Default filter parametrization.
 *
 * Unless there is a filter-specific parameter buffer,
 * #VAProcFilterParameterBuffer is the default type to use.
 */
typedef struct _VAProcFilterParameterBuffer {
    /** \brief Filter type. */
    VAProcFilterType    type;
    /** \brief Value. */
    float               value;
} VAProcFilterParameterBuffer;

/** @name De-interlacing flags */
/**@{*/
/** 
 * \brief Bottom field first in the input frame. 
 * if this is not set then assumes top field first.
 */
#define VA_DEINTERLACING_BOTTOM_FIELD_FIRST	0x0001
/** 
 * \brief Bottom field used in deinterlacing. 
 * if this is not set then assumes top field is used.
 */
#define VA_DEINTERLACING_BOTTOM_FIELD		0x0002
/** 
 * \brief A single field is stored in the input frame. 
 * if this is not set then assumes the frame contains two interleaved fields.
 */
#define VA_DEINTERLACING_ONE_FIELD		0x0004
/** 
 * \brief Film Mode Detection is enabled. If enabled, driver performs inverse
 * of various pulldowns, such as 3:2 pulldown.
 * if this is not set then assumes FMD is disabled.
 */
#define VA_DEINTERLACING_FMD_ENABLE		0x0008
/**@}*/

/** \brief Deinterlacing filter parametrization. */
typedef struct _VAProcFilterParameterBufferDeinterlacing {
    /** \brief Filter type. Shall be set to #VAProcFilterDeinterlacing. */
    VAProcFilterType            type;
    /** \brief Deinterlacing algorithm. */
    VAProcDeinterlacingType     algorithm;
    /** \brief Deinterlacing flags. */
    unsigned int     		flags;
} VAProcFilterParameterBufferDeinterlacing;

/**
 * \brief Color balance filter parametrization.
 *
 * This buffer defines color balance attributes. A VA buffer can hold
 * several color balance attributes by creating a VA buffer of desired
 * number of elements. This can be achieved by the following pseudo-code:
 *
 * \code
 * enum { kHue, kSaturation, kBrightness, kContrast };
 *
 * // Initial color balance parameters
 * static const VAProcFilterParameterBufferColorBalance colorBalanceParams[4] =
 * {
 *     [kHue] =
 *         { VAProcFilterColorBalance, VAProcColorBalanceHue, 0.5 },
 *     [kSaturation] =
 *         { VAProcFilterColorBalance, VAProcColorBalanceSaturation, 0.5 },
 *     [kBrightness] =
 *         { VAProcFilterColorBalance, VAProcColorBalanceBrightness, 0.5 },
 *     [kSaturation] =
 *         { VAProcFilterColorBalance, VAProcColorBalanceSaturation, 0.5 }
 * };
 *
 * // Create buffer
 * VABufferID colorBalanceBuffer;
 * vaCreateBuffer(va_dpy, vpp_ctx,
 *     VAProcFilterParameterBufferType, sizeof(*pColorBalanceParam), 4,
 *     colorBalanceParams,
 *     &colorBalanceBuffer
 * );
 *
 * VAProcFilterParameterBufferColorBalance *pColorBalanceParam;
 * vaMapBuffer(va_dpy, colorBalanceBuffer, &pColorBalanceParam);
 * {
 *     // Change brightness only
 *     pColorBalanceBuffer[kBrightness].value = 0.75;
 * }
 * vaUnmapBuffer(va_dpy, colorBalanceBuffer);
 * \endcode
 */
typedef struct _VAProcFilterParameterBufferColorBalance {
    /** \brief Filter type. Shall be set to #VAProcFilterColorBalance. */
    VAProcFilterType            type;
    /** \brief Color balance attribute. */
    VAProcColorBalanceType      attrib;
    /**
     * \brief Color balance value.
     *
     * Special case for automatically adjusted attributes. e.g. 
     * #VAProcColorBalanceAutoSaturation,
     * #VAProcColorBalanceAutoBrightness,
     * #VAProcColorBalanceAutoContrast.
     * - If \ref value is \c 1.0 +/- \c FLT_EPSILON, the attribute is
     *   automatically adjusted and overrides any other attribute of
     *   the same type that would have been set explicitly;
     * - If \ref value is \c 0.0 +/- \c FLT_EPSILON, the attribute is
     *   disabled and other attribute of the same type is used instead.
     */
    float                       value;
} VAProcFilterParameterBufferColorBalance;

/** @name FRC Custom Rate types. */
/**@{*/
/** \brief 24p to 60p. */
#define VA_FRAME_RATE_CONVERSION_24p_60p    0x0001
/** \brief 30p to 60p. */
#define VA_FRAME_RATE_CONVERSION_30p_60p    0x0002
/**@}*/

/** \brief Frame rate conversion filter parametrization. */
typedef struct _VAProcFilterParamterBufferFrameRateConversion {
    /** \brief filter type. Shall be set to #VAProcFilterFrameRateConversion. */
    VAProcFilterType                    type;
    /** \brief FPS of input sequence. */
    unsigned int                        input_fps;
    /** \brief FPS of output sequence. */
    unsigned int                        output_fps;
    /** \brief Number of output frames in addition to the first output frame.
        \brief If num_output_frames returned from pipeline query is 0,
        \brief vaRenderPicture() will only produce one output frame with each call*/ 
    unsigned int                        num_output_frames;
    /** 
     * \brief Array to store output frames in addition to the first one. 
     * \brief The first output frame is stored in the render target from vaBeginPicture(). */
    VASurfaceID*                        output_frames;   
    /** \brief if frame repeat or not. 1: repeat 0: do not repeat */
    unsigned int                        repeat_frame;  
    /** \brief Counter within one complete FRC Cycle.
        \brief The counter would run from 0 to 4 for 24to60p in each cycle.
        \brief The counter would run from 0 to 1 for 30to60p in each cycle. */
    unsigned int                        cyclic_counter;
} VAProcFilterParameterBufferFrameRateConversion;

/** \brief Total color correction filter parametrization. */
typedef struct _VAProcFilterParameterBufferTotalColorCorrection {
    /** \brief Filter type. Shall be set to #VAProcFilterTotalColorCorrection. */
    VAProcFilterType                  type;
    /** \brief Color to correct. */
    VAProcTotalColorCorrectionType    attrib;
    /** \brief Color correction value. */
    float                             value;
} VAProcFilterParameterBufferTotalColorCorrection;

/** @name ImageStabilization Perf Types. */
/**@{*/
/** \brief Fast Mode. */
#define VA_IMAGE_STABILIZATION_PERF_TYPE_FAST       0x0001
 /** \brief Quality Mode. */
#define VA_IMAGE_STABILIZATION_PERF_TYPE_QUALITY    0x0002
/**@}*/

/** \brief Image Stabilization filter parametrization. */
typedef struct _VAProcFilterParameterBufferImageStabilization {
    /** \brief Filter type. Shall be set to #VAProcFilterImageStabilization. */
    VAProcFilterType                  type;
    /** \brief Image Stabilization Mode. */
    VAProcImageStabilizationType      mode;
    /** \brief Image Stabilization Crop percentage. */
    float                             crop;
    /** \brief Image Stabilization Perf type. */
    unsigned int                      perf_type;
} VAProcFilterParameterBufferImageStabilization;

/** \brief Non-Linear Anamorphic Scaling filter parametrization. */
typedef struct _VAProcFilterParameterBufferNonLinearAnamorphicScaling {
    /** \brief filter type. Shall be set to #VAProcFilterNonLinearAnamorphicScaling. */
    VAProcFilterType    type;
    /** \brief Vertical crop. */
    float               vertical_crop;
    /** \brief HLinear region. */
    float               horizontal_linear_region;
    /** \brief Non-linear crop. */
    float               nonlinear_crop;
} VAProcFilterParameterBufferNonLinearAnamorphicScaling;

/**
 * \brief Default filter cap specification (single range value).
 *
 * Unless there is a filter-specific cap structure, #VAProcFilterCap is the
 * default type to use for output caps from vaQueryVideoProcFilterCaps().
 */
typedef struct _VAProcFilterCap {
    /** \brief Range of supported values for the filter. */
    VAProcFilterValueRange      range;
} VAProcFilterCap;

/** \brief Capabilities specification for the deinterlacing filter. */
typedef struct _VAProcFilterCapDeinterlacing {
    /** \brief Deinterlacing algorithm. */
    VAProcDeinterlacingType     type;
} VAProcFilterCapDeinterlacing;

/** \brief Capabilities specification for the color balance filter. */
typedef struct _VAProcFilterCapColorBalance {
    /** \brief Color balance operation. */
    VAProcColorBalanceType      type;
    /** \brief Range of supported values for the specified operation. */
    VAProcFilterValueRange      range;
} VAProcFilterCapColorBalance;

/** \brief Capabilities specification for the Total Color Correction filter. */
typedef struct _VAProcFilterCapTotalColorCorrection {
    /** \brief Color to correct. */
    VAProcTotalColorCorrectionType    type;
    /** \brief Range of supported values for the specified color. */
    VAProcFilterValueRange            range;
} VAProcFilterCapTotalColorCorrection;

/** \brief Capabilities specification for the Image Stabilization filter. */
typedef struct _VAProcFilterCapImageStabilization {
    /** \brief IS modes supported. */
    VAProcImageStabilizationType       type[VAProcImageStabilizationTypeCount];
    /** \brief Range of supported values for crop ratio. */
    VAProcFilterValueRange             crop_range;
    /** \brief Maximum number of forward reference frames supported. */
    unsigned int                       max_forward_reference;
    /** \brief Maximum number of IS perf modes supported. */
    unsigned int                       perf_type;
} VAProcFilterCapImageStabilization;

/** \brief Capabilities specification for the Non-Linear Anamorphic Scaling filter. */
typedef struct _VAProcFilterCapNonLinearAnamorphicScaling {
    /** \brief Range of supported values for the vertical crop. */
    VAProcFilterValueRange      vertical_crop_range;
    /** \brief Range of supported values for the horizontal linear region. */
    VAProcFilterValueRange      horizontal_linear_region_range;
    /** \brief Range of supported values for the non-linear crop. */
    VAProcFilterValueRange      nonlinear_crop_range;
} VAProcFilterCapNonLinearAnamorphicScaling;

/** \brief Capabilities specification for the Frame Rate Conversion filter. */
typedef struct _VAProcFilterCapFrameRateConversion { 
    /** \brief Should be set to 1 if only supported rates are requested.
        \brief Set to 0 to get the rest of the caps for the particular custom rate */
    unsigned int                        bget_custom_rates;
    /** \brief FRC custom rates supported by the pipeline in the first query
        \brief App request caps for a custom rate in the second query */
    unsigned int                        frc_custom_rates;
    /** \brief FPS of input sequence. */
    unsigned int                        input_fps;
    /** \brief FPS of output sequence. */
    unsigned int                        output_fps;
    /** \brief Number of input frames. */   
    unsigned int                        input_frames;
    /** \brief Number of output frames. */
    unsigned int                        output_frames;
   /** \brief Set to 1 if interlaced input is supoorted. */
    unsigned int                        input_interlaced;
} VAProcFilterCapFrameRateConversion;

/**
 * \brief Queries video processing filters.
 *
 * This function returns the list of video processing filters supported
 * by the driver. The \c filters array is allocated by the user and
 * \c num_filters shall be initialized to the number of allocated
 * elements in that array. Upon successful return, the actual number
 * of filters will be overwritten into \c num_filters. Otherwise,
 * \c VA_STATUS_ERROR_MAX_NUM_EXCEEDED is returned and \c num_filters
 * is adjusted to the number of elements that would be returned if enough
 * space was available.
 *
 * The list of video processing filters supported by the driver shall
 * be ordered in the way they can be iteratively applied. This is needed
 * for both correctness, i.e. some filters would not mean anything if
 * applied at the beginning of the pipeline; but also for performance
 * since some filters can be applied in a single pass (e.g. noise
 * reduction + deinterlacing).
 *
 * @param[in] dpy               the VA display
 * @param[in] context           the video processing context
 * @param[out] filters          the output array of #VAProcFilterType elements
 * @param[in,out] num_filters the number of elements allocated on input,
 *      the number of elements actually filled in on output
 */
VAStatus
vaQueryVideoProcFilters(
    VADisplay           dpy,
    VAContextID         context,
    VAProcFilterType   *filters,
    unsigned int       *num_filters
);

/**
 * \brief Queries video filter capabilities.
 *
 * This function returns the list of capabilities supported by the driver
 * for a specific video filter. The \c filter_caps array is allocated by
 * the user and \c num_filter_caps shall be initialized to the number
 * of allocated elements in that array. Upon successful return, the
 * actual number of filters will be overwritten into \c num_filter_caps.
 * Otherwise, \c VA_STATUS_ERROR_MAX_NUM_EXCEEDED is returned and
 * \c num_filter_caps is adjusted to the number of elements that would be
 * returned if enough space was available.
 *
 * @param[in] dpy               the VA display
 * @param[in] context           the video processing context
 * @param[in] type              the video filter type
 * @param[out] filter_caps      the output array of #VAProcFilterCap elements
 * @param[in,out] num_filter_caps the number of elements allocated on input,
 *      the number of elements actually filled in output
 */
VAStatus
vaQueryVideoProcFilterCaps(
    VADisplay           dpy,
    VAContextID         context,
    VAProcFilterType    type,
    void               *filter_caps,
    unsigned int       *num_filter_caps
);

/**
 * \brief Queries video processing pipeline capabilities.
 *
 * This function returns the video processing pipeline capabilities. The
 * \c filters array defines the video processing pipeline and is an array
 * of buffers holding filter parameters.
 *
 * Note: the #VAProcPipelineCaps structure contains user-provided arrays.
 * If non-NULL, the corresponding \c num_* fields shall be filled in on
 * input with the number of elements allocated. Upon successful return,
 * the actual number of elements will be overwritten into the \c num_*
 * fields. Otherwise, \c VA_STATUS_ERROR_MAX_NUM_EXCEEDED is returned
 * and \c num_* fields are adjusted to the number of elements that would
 * be returned if enough space was available.
 *
 * @param[in] dpy               the VA display
 * @param[in] context           the video processing context
 * @param[in] filters           the array of VA buffers defining the video
 *      processing pipeline
 * @param[in] num_filters       the number of elements in filters
 * @param[in,out] pipeline_caps the video processing pipeline capabilities
 */
VAStatus
vaQueryVideoProcPipelineCaps(
    VADisplay           dpy,
    VAContextID         context,
    VABufferID         *filters,
    unsigned int        num_filters,
    VAProcPipelineCaps *pipeline_caps
);

/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* VA_VPP_H */
