/*
 * Copyright (c) 2017 Intel Corporation. All Rights Reserved.
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
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "va_str.h"

#define TOSTR(enumCase) case enumCase: return #enumCase

const char *vaProfileStr(VAProfile profile)
{
    switch (profile) {
    TOSTR(VAProfileNone);
    TOSTR(VAProfileMPEG2Simple);
    TOSTR(VAProfileMPEG2Main);
    TOSTR(VAProfileMPEG4Simple);
    TOSTR(VAProfileMPEG4AdvancedSimple);
    TOSTR(VAProfileMPEG4Main);
    TOSTR(VAProfileH264Main);
    TOSTR(VAProfileH264High);
    TOSTR(VAProfileVC1Simple);
    TOSTR(VAProfileVC1Main);
    TOSTR(VAProfileVC1Advanced);
    TOSTR(VAProfileH263Baseline);
    TOSTR(VAProfileH264ConstrainedBaseline);
    TOSTR(VAProfileJPEGBaseline);
    TOSTR(VAProfileVP8Version0_3);
    TOSTR(VAProfileH264MultiviewHigh);
    TOSTR(VAProfileH264StereoHigh);
    TOSTR(VAProfileHEVCMain);
    TOSTR(VAProfileHEVCMain10);
    TOSTR(VAProfileVP9Profile0);
    TOSTR(VAProfileVP9Profile1);
    TOSTR(VAProfileVP9Profile2);
    TOSTR(VAProfileVP9Profile3);
    TOSTR(VAProfileHEVCMain12);
    TOSTR(VAProfileHEVCMain422_10);
    TOSTR(VAProfileHEVCMain422_12);
    TOSTR(VAProfileHEVCMain444);
    TOSTR(VAProfileHEVCMain444_10);
    TOSTR(VAProfileHEVCMain444_12);
    TOSTR(VAProfileHEVCSccMain);
    TOSTR(VAProfileHEVCSccMain10);
    TOSTR(VAProfileHEVCSccMain444);
    default: break;
    }
    return "<unknown profile>";
}


const char *vaEntrypointStr(VAEntrypoint entrypoint)
{
    switch (entrypoint) {
    TOSTR(VAEntrypointVLD);
    TOSTR(VAEntrypointIZZ);
    TOSTR(VAEntrypointIDCT);
    TOSTR(VAEntrypointMoComp);
    TOSTR(VAEntrypointDeblocking);
    TOSTR(VAEntrypointEncSlice);
    TOSTR(VAEntrypointEncPicture);
    TOSTR(VAEntrypointEncSliceLP);
    TOSTR(VAEntrypointVideoProc);
    TOSTR(VAEntrypointFEI);
    TOSTR(VAEntrypointStats);
    }
    return "<unknown entrypoint>";
}

const char *vaConfigAttribTypeStr(VAConfigAttribType configAttribType)
{
    switch (configAttribType) {
    TOSTR(VAConfigAttribRTFormat);
    TOSTR(VAConfigAttribSpatialResidual);
    TOSTR(VAConfigAttribSpatialClipping);
    TOSTR(VAConfigAttribIntraResidual);
    TOSTR(VAConfigAttribEncryption);
    TOSTR(VAConfigAttribRateControl);
    TOSTR(VAConfigAttribDecSliceMode);
    TOSTR(VAConfigAttribEncPackedHeaders);
    TOSTR(VAConfigAttribEncInterlaced);
    TOSTR(VAConfigAttribEncMaxRefFrames);
    TOSTR(VAConfigAttribEncMaxSlices);
    TOSTR(VAConfigAttribEncSliceStructure);
    TOSTR(VAConfigAttribEncMacroblockInfo);
    TOSTR(VAConfigAttribEncJPEG);
    TOSTR(VAConfigAttribEncQualityRange);
    TOSTR(VAConfigAttribEncSkipFrame);
    TOSTR(VAConfigAttribEncROI);
    TOSTR(VAConfigAttribEncRateControlExt);
    TOSTR(VAConfigAttribFEIFunctionType);
    TOSTR(VAConfigAttribFEIMVPredictors);
    TOSTR(VAConfigAttribDecJPEG);
    TOSTR(VAConfigAttribMaxPictureWidth);
    TOSTR(VAConfigAttribMaxPictureHeight);
    TOSTR(VAConfigAttribEncQuantization);
    TOSTR(VAConfigAttribEncIntraRefresh);
    TOSTR(VAConfigAttribProcessingRate);
    TOSTR(VAConfigAttribEncDirtyRect);
    TOSTR(VAConfigAttribEncParallelRateControl);
    TOSTR(VAConfigAttribEncDynamicScaling);
    TOSTR(VAConfigAttribDecProcessing);
    TOSTR(VAConfigAttribFrameSizeToleranceSupport);
    TOSTR(VAConfigAttribEncTileSupport);
    TOSTR(VAConfigAttribCustomRoundingControl);
    TOSTR(VAConfigAttribQPBlockSize);
    TOSTR(VAConfigAttribStats);
    case VAConfigAttribTypeMax: break;
    }
    return "<unknown config attribute type>";
}

const char *vaBufferTypeStr(VABufferType bufferType)
{
    switch (bufferType) {
    TOSTR(VAPictureParameterBufferType);
    TOSTR(VAIQMatrixBufferType);
    TOSTR(VABitPlaneBufferType);
    TOSTR(VASliceGroupMapBufferType);
    TOSTR(VASliceParameterBufferType);
    TOSTR(VASliceDataBufferType);
    TOSTR(VAMacroblockParameterBufferType);
    TOSTR(VAResidualDataBufferType);
    TOSTR(VADeblockingParameterBufferType);
    TOSTR(VAImageBufferType);
    TOSTR(VAProtectedSliceDataBufferType);
    TOSTR(VAQMatrixBufferType);
    TOSTR(VAHuffmanTableBufferType);
    TOSTR(VAProbabilityBufferType);
    TOSTR(VAEncCodedBufferType);
    TOSTR(VAEncSequenceParameterBufferType);
    TOSTR(VAEncPictureParameterBufferType);
    TOSTR(VAEncSliceParameterBufferType);
    TOSTR(VAEncPackedHeaderParameterBufferType);
    TOSTR(VAEncPackedHeaderDataBufferType);
    TOSTR(VAEncMiscParameterBufferType);
    TOSTR(VAEncMacroblockParameterBufferType);
    TOSTR(VAEncMacroblockMapBufferType);
    TOSTR(VAProcPipelineParameterBufferType);
    TOSTR(VAProcFilterParameterBufferType);
    TOSTR(VAEncQPBufferType);
    TOSTR(VAEncFEIMVBufferType);
    TOSTR(VAEncFEIMBCodeBufferType);
    TOSTR(VAEncFEIDistortionBufferType);
    TOSTR(VAEncFEIMBControlBufferType);
    TOSTR(VAEncFEIMVPredictorBufferType);
    TOSTR(VAEncMacroblockDisableSkipMapBufferType);
    TOSTR(VADecodeStreamoutBufferType);
    TOSTR(VAStatsStatisticsParameterBufferType);
    TOSTR(VAStatsStatisticsBufferType);
    TOSTR(VAStatsStatisticsBottomFieldBufferType);
    TOSTR(VAStatsMVBufferType);
    TOSTR(VAStatsMVPredictorBufferType);
    TOSTR(VAEncFEICTBCmdBufferType);
    TOSTR(VAEncFEICURecordBufferType);
    case VABufferTypeMax: break;
    }
    return "<unknown buffer type>";
}

#undef TOSTR
