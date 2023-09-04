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
        TOSTR(VAProfileH264High10);
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
        TOSTR(VAProfileAV1Profile0);
        TOSTR(VAProfileAV1Profile1);
        TOSTR(VAProfileHEVCSccMain444_10);
        TOSTR(VAProfileProtected);
        TOSTR(VAProfileVVCMain10);
        TOSTR(VAProfileVVCMultilayerMain10);
    default:
        break;
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
        TOSTR(VAEntrypointProtectedTEEComm);
        TOSTR(VAEntrypointProtectedContent);
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
        TOSTR(VAConfigAttribMaxFrameSize);
        TOSTR(VAConfigAttribPredictionDirection);
        TOSTR(VAConfigAttribMultipleFrame);
        TOSTR(VAConfigAttribContextPriority);
        TOSTR(VAConfigAttribDecAV1Features);
        TOSTR(VAConfigAttribTEEType);
        TOSTR(VAConfigAttribTEETypeClient);
        TOSTR(VAConfigAttribProtectedContentCipherAlgorithm);
        TOSTR(VAConfigAttribProtectedContentCipherBlockSize);
        TOSTR(VAConfigAttribProtectedContentCipherMode);
        TOSTR(VAConfigAttribProtectedContentCipherSampleType);
        TOSTR(VAConfigAttribProtectedContentUsage);
        TOSTR(VAConfigAttribEncHEVCFeatures);
        TOSTR(VAConfigAttribEncHEVCBlockSizes);
        TOSTR(VAConfigAttribEncAV1);
        TOSTR(VAConfigAttribEncAV1Ext1);
        TOSTR(VAConfigAttribEncAV1Ext2);
        TOSTR(VAConfigAttribEncPerBlockControl);
        TOSTR(VAConfigAttribEncMaxTileRows);
        TOSTR(VAConfigAttribEncMaxTileCols);
    case VAConfigAttribTypeMax:
        break;
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
        TOSTR(VASubsetsParameterBufferType);
        TOSTR(VAContextParameterUpdateBufferType);
        TOSTR(VAProtectedSessionExecuteBufferType);
        TOSTR(VAEncryptionParameterBufferType);
        TOSTR(VAEncDeltaQpPerBlockBufferType);
        TOSTR(VAAlfBufferType);
        TOSTR(VALmcsBufferType);
        TOSTR(VASubPicBufferType);
        TOSTR(VATileBufferType);
        TOSTR(VASliceStructBufferType);
    case VABufferTypeMax:
        break;
    }
    return "<unknown buffer type>";
}

const char *vaStatusStr(VAStatus status)
{
    switch (status) {
        TOSTR(VA_STATUS_SUCCESS);
        TOSTR(VA_STATUS_ERROR_OPERATION_FAILED);
        TOSTR(VA_STATUS_ERROR_ALLOCATION_FAILED);
        TOSTR(VA_STATUS_ERROR_INVALID_DISPLAY);
        TOSTR(VA_STATUS_ERROR_INVALID_CONFIG);
        TOSTR(VA_STATUS_ERROR_INVALID_CONTEXT);
        TOSTR(VA_STATUS_ERROR_INVALID_SURFACE);
        TOSTR(VA_STATUS_ERROR_INVALID_BUFFER);
        TOSTR(VA_STATUS_ERROR_INVALID_IMAGE);
        TOSTR(VA_STATUS_ERROR_INVALID_SUBPICTURE);
        TOSTR(VA_STATUS_ERROR_ATTR_NOT_SUPPORTED);
        TOSTR(VA_STATUS_ERROR_MAX_NUM_EXCEEDED);
        TOSTR(VA_STATUS_ERROR_UNSUPPORTED_PROFILE);
        TOSTR(VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT);
        TOSTR(VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT);
        TOSTR(VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE);
        TOSTR(VA_STATUS_ERROR_SURFACE_BUSY);
        TOSTR(VA_STATUS_ERROR_FLAG_NOT_SUPPORTED);
        TOSTR(VA_STATUS_ERROR_INVALID_PARAMETER);
        TOSTR(VA_STATUS_ERROR_RESOLUTION_NOT_SUPPORTED);
        TOSTR(VA_STATUS_ERROR_UNIMPLEMENTED);
        TOSTR(VA_STATUS_ERROR_SURFACE_IN_DISPLAYING);
        TOSTR(VA_STATUS_ERROR_INVALID_IMAGE_FORMAT);
        TOSTR(VA_STATUS_ERROR_DECODING_ERROR);
        TOSTR(VA_STATUS_ERROR_ENCODING_ERROR);
        TOSTR(VA_STATUS_ERROR_INVALID_VALUE);
        TOSTR(VA_STATUS_ERROR_UNSUPPORTED_FILTER);
        TOSTR(VA_STATUS_ERROR_INVALID_FILTER_CHAIN);
        TOSTR(VA_STATUS_ERROR_HW_BUSY);
        TOSTR(VA_STATUS_ERROR_UNSUPPORTED_MEMORY_TYPE);
        TOSTR(VA_STATUS_ERROR_NOT_ENOUGH_BUFFER);
        TOSTR(VA_STATUS_ERROR_UNKNOWN);
    default:
        break;
    }
    return "unknown return value";
}

#undef TOSTR
