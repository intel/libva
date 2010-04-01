/* ------------------------------------------------------------------
 * Copyright (C) 2008 PacketVideo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * -------------------------------------------------------------------
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "VideoMIO"
#include <utils/Log.h>
#include <ui/ISurface.h>

#include "android_surface_output.h"
#include <media/PVPlayer.h>

#include "pvlogger.h"
#include "pv_mime_string_utils.h"
#include "oscl_snprintf.h"

#include "oscl_dll.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <va/va.h>
#include <va/va_x11.h>

#ifdef __cplusplus
} /* extern "C" */
#endif

// Define entry point for this DLL
OSCL_DLL_ENTRY_POINT_DEFAULT()

//The factory functions.
#include "oscl_mem.h"

using namespace android;

OSCL_EXPORT_REF AndroidSurfaceOutput::AndroidSurfaceOutput() :
    OsclTimerObject(OsclActiveObject::EPriorityNominal, "androidsurfaceoutput")
{
    initData();

    iColorConverter = NULL;
    mInitialized = false;
    mPvPlayer = NULL;
    mEmulation = false;
    iEosReceived = false;
    mNumberOfFramesToHold = 2;
}

status_t AndroidSurfaceOutput::set(PVPlayer* pvPlayer, const sp<ISurface>& surface, bool emulation)
{
    mPvPlayer = pvPlayer;
    mEmulation = emulation;
    setVideoSurface(surface);
    return NO_ERROR;
}

status_t AndroidSurfaceOutput::setVideoSurface(const sp<ISurface>& surface)
{
    LOGV("setVideoSurface(%p)", surface.get());
    // unregister buffers for the old surface
    if (mSurface != NULL) {
        LOGV("unregisterBuffers from old surface");
        mSurface->unregisterBuffers();
    }
    mSurface = surface;
    // register buffers for the new surface
    if ((mSurface != NULL) && (mBufferHeap.heap != NULL)) {
        LOGV("registerBuffers from old surface");
        mSurface->registerBuffers(mBufferHeap);
    }
    return NO_ERROR;
}

void AndroidSurfaceOutput::initData()
{
    iVideoHeight = iVideoWidth = iVideoDisplayHeight = iVideoDisplayWidth = 0;
    iVideoFormat=PVMF_MIME_FORMAT_UNKNOWN;
    resetVideoParameterFlags();

    iCommandCounter=0;
    iLogger=NULL;
    iCommandResponseQueue.reserve(5);
    iWriteResponseQueue.reserve(5);
    iObserver=NULL;
    iLogger=NULL;
    iPeer=NULL;
    iState=STATE_IDLE;
    iIsMIOConfigured = false;
}

void AndroidSurfaceOutput::ResetData()
    //reset all data from this session.
{
    Cleanup();

    //reset all the received media parameters.
    iVideoFormatString="";
    iVideoFormat=PVMF_MIME_FORMAT_UNKNOWN;
    resetVideoParameterFlags();
    iIsMIOConfigured = false;
}

void AndroidSurfaceOutput::resetVideoParameterFlags()
{
    iVideoParameterFlags = VIDEO_PARAMETERS_INVALID;
}

bool AndroidSurfaceOutput::checkVideoParameterFlags()
{
    return (iVideoParameterFlags & VIDEO_PARAMETERS_MASK) == VIDEO_PARAMETERS_VALID;
}

/*
 * process the write response queue by sending writeComplete to the peer
 * (nominally the decoder node).
 *
 * numFramesToHold is the number of frames to be held in the MIO. During
 * playback, we hold the last frame which is used by SurfaceFlinger
 * to composite the final output.
 */
void AndroidSurfaceOutput::processWriteResponseQueue(int numFramesToHold)
{
    LOGV("processWriteResponseQueue: queued = %d, numFramesToHold = %d",
            iWriteResponseQueue.size(), numFramesToHold);
    while (iWriteResponseQueue.size() > numFramesToHold) {
        if (iPeer) {
            iPeer->writeComplete(iWriteResponseQueue[0].iStatus,
                    iWriteResponseQueue[0].iCmdId,
                    (OsclAny*)iWriteResponseQueue[0].iContext);
        }
        iWriteResponseQueue.erase(&iWriteResponseQueue[0]);
    }
}

void AndroidSurfaceOutput::Cleanup()
//cleanup all allocated memory and release resources.
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidSurfaceOutput::Cleanup() In"));
    while (!iCommandResponseQueue.empty())
    {
        if (iObserver)
            iObserver->RequestCompleted(PVMFCmdResp(iCommandResponseQueue[0].iCmdId, iCommandResponseQueue[0].iContext, iCommandResponseQueue[0].iStatus));
        iCommandResponseQueue.erase(&iCommandResponseQueue[0]);
    }

    processWriteResponseQueue(0);

    // We'll close frame buf and delete here for now.
    closeFrameBuf();

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidSurfaceOutput::Cleanup() Out"));
 }

OSCL_EXPORT_REF AndroidSurfaceOutput::~AndroidSurfaceOutput()
{
    Cleanup();
}


PVMFStatus AndroidSurfaceOutput::connect(PvmiMIOSession& aSession, PvmiMIOObserver* aObserver)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidSurfaceOutput::connect() called"));
    // Each Session could have its own set of Configuration parameters
    //in an array of structures and the session ID could be an index to that array.

    //currently supports only one session
    if (iObserver)
        return PVMFFailure;

    iObserver=aObserver;
    return PVMFSuccess;
}


PVMFStatus AndroidSurfaceOutput::disconnect(PvmiMIOSession aSession)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidSurfaceOutput::disconnect() called"));
    //currently supports only one session
    iObserver=NULL;
    return PVMFSuccess;
}


PvmiMediaTransfer* AndroidSurfaceOutput::createMediaTransfer(PvmiMIOSession& aSession, 
                                                        PvmiKvp* read_formats, int32 read_flags,
                                                        PvmiKvp* write_formats, int32 write_flags)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidSurfaceOutput::createMediaTransfer() called"));
    return (PvmiMediaTransfer*)this;
}

void AndroidSurfaceOutput::QueueCommandResponse(CommandResponse& aResp)
{
    //queue a command response and schedule processing.

    iCommandResponseQueue.push_back(aResp);

    //cancel any timer delay so the command response will happen ASAP.
    if (IsBusy())
        Cancel();

    RunIfNotReady();
}

PVMFCommandId AndroidSurfaceOutput::QueryUUID(const PvmfMimeString& aMimeType, 
                                        Oscl_Vector<PVUuid, OsclMemAllocator>& aUuids,
                                        bool aExactUuidsOnly, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidSurfaceOutput::QueryUUID() called"));

    OSCL_UNUSED_ARG(aMimeType);
    OSCL_UNUSED_ARG(aExactUuidsOnly);

    PVMFCommandId cmdid=iCommandCounter++;

    PVMFStatus status=PVMFFailure;
    int32 err ;
    OSCL_TRY(err, aUuids.push_back(PVMI_CAPABILITY_AND_CONFIG_PVUUID););
    if (err==OsclErrNone)
        status= PVMFSuccess;

    CommandResponse resp(status,cmdid,aContext);
    QueueCommandResponse(resp);
    return cmdid;
}


PVMFCommandId AndroidSurfaceOutput::QueryInterface(const PVUuid& aUuid, PVInterface*& aInterfacePtr, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidSurfaceOutput::QueryInterface() called"));

    PVMFCommandId cmdid=iCommandCounter++;

    PVMFStatus status=PVMFFailure;
    if(aUuid == PVMI_CAPABILITY_AND_CONFIG_PVUUID)
    {
        PvmiCapabilityAndConfig* myInterface = OSCL_STATIC_CAST(PvmiCapabilityAndConfig*,this);
        aInterfacePtr = OSCL_STATIC_CAST(PVInterface*, myInterface);
        status= PVMFSuccess;
    }
    else
    {
        status=PVMFFailure;
    }

    CommandResponse resp(status,cmdid,aContext);
    QueueCommandResponse(resp);
    return cmdid;
}


void AndroidSurfaceOutput::deleteMediaTransfer(PvmiMIOSession& aSession, PvmiMediaTransfer* media_transfer)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidSurfaceOutput::deleteMediaTransfer() called"));
    // This class is implementing the media transfer, so no cleanup is needed
}


PVMFCommandId AndroidSurfaceOutput:: Init(const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidSurfaceOutput::Init() called"));

    PVMFCommandId cmdid=iCommandCounter++;

    PVMFStatus status=PVMFFailure;

    switch(iState)
    {
    case STATE_LOGGED_ON:
        status=PVMFSuccess;
        iState=STATE_INITIALIZED;
        break;

    default:
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidSurfaceOutput::Invalid State"));
        status=PVMFErrInvalidState;
        break;
    }

    CommandResponse resp(status,cmdid,aContext);
    QueueCommandResponse(resp);
    return cmdid;
}

PVMFCommandId AndroidSurfaceOutput::Reset(const OsclAny* aContext)
{
    ResetData();
    PVMFCommandId cmdid=iCommandCounter++;
    CommandResponse resp(PVMFSuccess,cmdid,aContext);
    QueueCommandResponse(resp);
    return cmdid;
}


PVMFCommandId AndroidSurfaceOutput::Start(const OsclAny* aContext)
{
    iEosReceived = false;
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidSurfaceOutput::Start() called"));

    PVMFCommandId cmdid=iCommandCounter++;

    PVMFStatus status=PVMFFailure;

    switch(iState)
    {
    case STATE_INITIALIZED:
    case STATE_PAUSED:

        iState=STATE_STARTED;
        processWriteResponseQueue(0);
        status=PVMFSuccess;
        break;

    default:
        status=PVMFErrInvalidState;
        break;
    }

    CommandResponse resp(status,cmdid,aContext);
    QueueCommandResponse(resp);
    return cmdid;
}

// post the last video frame to refresh screen after pause
void AndroidSurfaceOutput::postLastFrame()
{
    // ignore if no surface or heap
    if ((mSurface == NULL) || (mBufferHeap.heap == NULL)) return;
    mSurface->postBuffer(mFrameBuffers[mFrameBufferIndex]);
}

PVMFCommandId AndroidSurfaceOutput::Pause(const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidSurfaceOutput::Pause() called"));

    PVMFCommandId cmdid=iCommandCounter++;

    PVMFStatus status=PVMFFailure;

    switch(iState)
    {
    case STATE_STARTED:

        iState=STATE_PAUSED;
        status=PVMFSuccess;

        // post last buffer to prevent stale data
        // if not configured, PVMFMIOConfigurationComplete is not sent
        // there should not be any media data.
    if(iIsMIOConfigured) { 
        postLastFrame();
        }
        break;

    default:
        status=PVMFErrInvalidState;
        break;
    }

    CommandResponse resp(status,cmdid,aContext);
    QueueCommandResponse(resp);
    return cmdid;
}


PVMFCommandId AndroidSurfaceOutput::Flush(const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidSurfaceOutput::Flush() called"));

    PVMFCommandId cmdid=iCommandCounter++;

    PVMFStatus status=PVMFFailure;

    switch(iState)
    {
    case STATE_STARTED:

        iState=STATE_INITIALIZED;
        status=PVMFSuccess;
        break;

    default:
        status=PVMFErrInvalidState;
        break;
    }

    CommandResponse resp(status,cmdid,aContext);
    QueueCommandResponse(resp);
    return cmdid;
}

PVMFCommandId AndroidSurfaceOutput::DiscardData(const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidSurfaceOutput::DiscardData() called"));

    PVMFCommandId cmdid=iCommandCounter++;

    //this component doesn't buffer data, so there's nothing
    //needed here.

    PVMFStatus status=PVMFSuccess;
    processWriteResponseQueue(0);

    CommandResponse resp(status,cmdid,aContext);
    QueueCommandResponse(resp);
    return cmdid;
}

PVMFCommandId AndroidSurfaceOutput::DiscardData(PVMFTimestamp aTimestamp, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidSurfaceOutput::DiscardData() called"));

    PVMFCommandId cmdid=iCommandCounter++;

    aTimestamp = 0;

    //this component doesn't buffer data, so there's nothing
    //needed here.

    PVMFStatus status=PVMFSuccess;
    processWriteResponseQueue(0);

    CommandResponse resp(status,cmdid,aContext);
    QueueCommandResponse(resp);
    return cmdid;
}

PVMFCommandId AndroidSurfaceOutput::Stop(const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidSurfaceOutput::Stop() called"));

    PVMFCommandId cmdid=iCommandCounter++;

    PVMFStatus status=PVMFFailure;

    switch(iState)
    {
    case STATE_STARTED:
    case STATE_PAUSED:

#ifdef PERFORMANCE_MEASUREMENTS_ENABLED
        // FIXME: This should be moved to OMAP library
    PVOmapVideoProfile.MarkEndTime();
    PVOmapVideoProfile.PrintStats();
    PVOmapVideoProfile.Reset();
#endif

        iState=STATE_INITIALIZED;
        status=PVMFSuccess;
        break;

    default:
        status=PVMFErrInvalidState;
        break;
    }

    CommandResponse resp(status,cmdid,aContext);
    QueueCommandResponse(resp);
    return cmdid;
}

PVMFCommandId AndroidSurfaceOutput::CancelAllCommands(const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidSurfaceOutput::CancelAllCommands() called"));

    PVMFCommandId cmdid=iCommandCounter++;

    //commands are executed immediately upon being received, so
    //it isn't really possible to cancel them.

    PVMFStatus status=PVMFSuccess;

    CommandResponse resp(status,cmdid,aContext);
    QueueCommandResponse(resp);
    return cmdid;
}

PVMFCommandId AndroidSurfaceOutput::CancelCommand(PVMFCommandId aCmdId, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidSurfaceOutput::CancelCommand() called"));

    PVMFCommandId cmdid=iCommandCounter++;

    //commands are executed immediately upon being received, so
    //it isn't really possible to cancel them.

    //see if the response is still queued.
    PVMFStatus status=PVMFFailure;
    for (uint32 i=0;i<iCommandResponseQueue.size();i++)
    {
        if (iCommandResponseQueue[i].iCmdId==aCmdId)
        {
            status=PVMFSuccess;
            break;
        }
    }

    CommandResponse resp(status,cmdid,aContext);
    QueueCommandResponse(resp);
    return cmdid;
}

void AndroidSurfaceOutput::ThreadLogon()
{
    if(iState==STATE_IDLE)
    {
        iLogger = PVLogger::GetLoggerObject("PVOmapVideo");
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidSurfaceOutput::ThreadLogon() called"));
        AddToScheduler();
        iState=STATE_LOGGED_ON;
    }
}


void AndroidSurfaceOutput::ThreadLogoff()
{   
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidSurfaceOutput::ThreadLogoff() called"));
    
    if(iState!=STATE_IDLE)
    {
        RemoveFromScheduler();
        iLogger=NULL;
        iState=STATE_IDLE;
    }
}


void AndroidSurfaceOutput::setPeer(PvmiMediaTransfer* aPeer)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidSurfaceOutput::setPeer() called"));
    // Set the observer 
    iPeer = aPeer;
}


void AndroidSurfaceOutput::useMemoryAllocators(OsclMemAllocator* write_alloc)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidSurfaceOutput::useMemoryAllocators() called"));
    //not supported.
}

// This routine will determine whether data can be accepted in a writeAsync
// call and if not, will return true;
bool AndroidSurfaceOutput::CheckWriteBusy(uint32 aSeqNum)
{
    // for all other cases, accept data now.
    return false;
}

PVMFCommandId AndroidSurfaceOutput::writeAsync(uint8 aFormatType, int32 aFormatIndex, uint8* aData, uint32 aDataLen,
                                        const PvmiMediaXferHeader& data_header_info, OsclAny* aContext)
{
    // Do a leave if MIO is not configured except when it is an EOS
    if (!iIsMIOConfigured
            &&
            !((PVMI_MEDIAXFER_FMT_TYPE_NOTIFICATION == aFormatType)
              && (PVMI_MEDIAXFER_FMT_INDEX_END_OF_STREAM == aFormatIndex)))
    {
        LOGE("data is pumped in before MIO is configured");
        OSCL_LEAVE(OsclErrInvalidState);
        return -1;
    }

    uint32 aSeqNum=data_header_info.seq_num;
    PVMFTimestamp aTimestamp=data_header_info.timestamp;
    uint32 flags=data_header_info.flags;

    if (aSeqNum < 6)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
            (0,"AndroidSurfaceOutput::writeAsync() seqnum %d ts %d context %d",aSeqNum,aTimestamp,aContext));

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
            (0,"AndroidSurfaceOutput::writeAsync() Format Type %d Format Index %d length %d",aFormatType,aFormatIndex,aDataLen));
    }

    PVMFStatus status=PVMFFailure;

    switch(aFormatType)
    {
    case PVMI_MEDIAXFER_FMT_TYPE_COMMAND :
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
            (0,"AndroidSurfaceOutput::writeAsync() called with Command info."));
        //ignore
        status= PVMFSuccess;
        break;

    case PVMI_MEDIAXFER_FMT_TYPE_NOTIFICATION :
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
            (0,"AndroidSurfaceOutput::writeAsync() called with Notification info."));
        switch(aFormatIndex)
        {
        case PVMI_MEDIAXFER_FMT_INDEX_END_OF_STREAM:
            iEosReceived = true;
            break;
        default:
            break;
        }
        //ignore
        status= PVMFSuccess;
        break;

    case PVMI_MEDIAXFER_FMT_TYPE_DATA :
        switch(aFormatIndex)
        {
        case PVMI_MEDIAXFER_FMT_INDEX_FMT_SPECIFIC_INFO:
            //format-specific info contains codec headers.
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                (0,"AndroidSurfaceOutput::writeAsync() called with format-specific info."));

            if (iState<STATE_INITIALIZED)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                    (0,"AndroidSurfaceOutput::writeAsync: Error - Invalid state"));
                status=PVMFErrInvalidState;
            }
            else
            {
                status= PVMFSuccess;
            }

            break;

        case PVMI_MEDIAXFER_FMT_INDEX_DATA:
            //data contains the media bitstream.

            //Verify the state
            if (iState!=STATE_STARTED)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                    (0,"AndroidSurfaceOutput::writeAsync: Error - Invalid state"));
                status=PVMFErrInvalidState;
            }
            else
            {

                //printf("V WriteAsync { seq=%d, ts=%d }\n", data_header_info.seq_num, data_header_info.timestamp);

                // Call playback to send data to IVA for Color Convert
                status = writeFrameBuf(aData, aDataLen, data_header_info);

                PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                   (0,"AndroidSurfaceOutput::writeAsync: Playback Progress - frame %d",iFrameNumber++));
            }
            break;

        default:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                (0,"AndroidSurfaceOutput::writeAsync: Error - unrecognized format index"));
            status= PVMFFailure;
            break;
        }
        break;

    default:
        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
            (0,"AndroidSurfaceOutput::writeAsync: Error - unrecognized format type"));
        status= PVMFFailure;
        break;
    }

    //Schedule asynchronous response
    PVMFCommandId cmdid=iCommandCounter++;
    WriteResponse resp(status,cmdid,aContext,aTimestamp);
    iWriteResponseQueue.push_back(resp);
    RunIfNotReady();

    return cmdid;
}

void AndroidSurfaceOutput::writeComplete(PVMFStatus aStatus, PVMFCommandId  write_cmd_id, OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidSurfaceOutput::writeComplete() called"));
    //won't be called since this component is a sink.
}


PVMFCommandId  AndroidSurfaceOutput::readAsync(uint8* data, uint32 max_data_len, OsclAny* aContext,
                                            int32* formats, uint16 num_formats)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidSurfaceOutput::readAsync() called"));
    //read not supported.
    OsclError::Leave(OsclErrNotSupported);
    return -1;
}


void AndroidSurfaceOutput::readComplete(PVMFStatus aStatus, PVMFCommandId  read_cmd_id, int32 format_index,
                                    const PvmiMediaXferHeader& data_header_info, OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidSurfaceOutput::readComplete() called"));
    //won't be called since this component is a sink.
}


void AndroidSurfaceOutput::statusUpdate(uint32 status_flags)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidSurfaceOutput::statusUpdate() called"));
    //won't be called since this component is a sink.
}


void AndroidSurfaceOutput::cancelCommand(PVMFCommandId  command_id)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidSurfaceOutput::cancelCommand() called"));

    //the purpose of this API is to cancel a writeAsync command and report
    //completion ASAP.

    //in this implementation, the write commands are executed immediately
    //when received so it isn't really possible to cancel.
    //just report completion immediately.
    processWriteResponseQueue(0);
}

void AndroidSurfaceOutput::cancelAllCommands()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidSurfaceOutput::cancelAllCommands() called"));

    //the purpose of this API is to cancel all writeAsync commands and report
    //completion ASAP.

    //in this implementaiton, the write commands are executed immediately 
    //when received so it isn't really possible to cancel.
    //just report completion immediately.

    for (uint32 i=0;i<iWriteResponseQueue.size();i++)
    {
        //report completion
        if (iPeer)
            iPeer->writeComplete(iWriteResponseQueue[i].iStatus,iWriteResponseQueue[i].iCmdId,(OsclAny*)iWriteResponseQueue[i].iContext);
        iWriteResponseQueue.erase(&iWriteResponseQueue[i]);
    }
}

void AndroidSurfaceOutput::setObserver (PvmiConfigAndCapabilityCmdObserver* aObserver)
{
    OSCL_UNUSED_ARG(aObserver);
    //not needed since this component only supports synchronous capability & config
    //APIs.
}

PVMFStatus AndroidSurfaceOutput::getParametersSync(PvmiMIOSession aSession, PvmiKeyType aIdentifier,
                                              PvmiKvp*& aParameters, int& num_parameter_elements,
                                              PvmiCapabilityContext aContext)
{
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);
    aParameters=NULL;

    // This is a query for the list of supported formats.
    if(pv_mime_strcmp(aIdentifier, INPUT_FORMATS_CAP_QUERY) == 0)
    {
        aParameters=(PvmiKvp*)oscl_malloc(sizeof(PvmiKvp));
        if (aParameters == NULL) return PVMFErrNoMemory;
        aParameters[num_parameter_elements++].value.pChar_value=(char*) PVMF_MIME_YUV420;

        return PVMFSuccess;
    }

    //unrecognized key.
    return PVMFFailure;
}

PVMFStatus AndroidSurfaceOutput::releaseParameters(PvmiMIOSession aSession, PvmiKvp* aParameters, int num_elements)
{
    //release parameters that were allocated by this component.
    if (aParameters)
    {
        oscl_free(aParameters);
        return PVMFSuccess;
    }
    return PVMFFailure;
}

void AndroidSurfaceOutput ::createContext(PvmiMIOSession aSession, PvmiCapabilityContext& aContext)
{
    OsclError::Leave(OsclErrNotSupported);
}

void AndroidSurfaceOutput::setContextParameters(PvmiMIOSession aSession, PvmiCapabilityContext& aContext,
                                           PvmiKvp* aParameters, int num_parameter_elements)
{
    OsclError::Leave(OsclErrNotSupported);
}

void AndroidSurfaceOutput::DeleteContext(PvmiMIOSession aSession, PvmiCapabilityContext& aContext)
{
    OsclError::Leave(OsclErrNotSupported);
}


void AndroidSurfaceOutput::setParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters,
                                        int num_elements, PvmiKvp * & aRet_kvp)
{
    OSCL_UNUSED_ARG(aSession);

    aRet_kvp = NULL;

    LOGV("setParametersSync");
    for (int32 i=0;i<num_elements;i++)
    {
        //Check against known video parameter keys...
        if (pv_mime_strcmp(aParameters[i].key, MOUT_VIDEO_FORMAT_KEY) == 0)
        {
            iVideoFormatString=aParameters[i].value.pChar_value;
            iVideoFormat=iVideoFormatString.get_str();
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                (0,"AndroidSurfaceOutput::setParametersSync() Video Format Key, Value %s",iVideoFormatString.get_str()));
        }
        else if (pv_mime_strcmp(aParameters[i].key, MOUT_VIDEO_WIDTH_KEY) == 0)
        {
            iVideoWidth=(int32)aParameters[i].value.uint32_value;
            iVideoParameterFlags |= VIDEO_WIDTH_VALID;
            LOGV("iVideoWidth=%d", iVideoWidth);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                (0,"AndroidSurfaceOutput::setParametersSync() Video Width Key, Value %d",iVideoWidth));
        }
        else if (pv_mime_strcmp(aParameters[i].key, MOUT_VIDEO_HEIGHT_KEY) == 0)
        {
            iVideoHeight=(int32)aParameters[i].value.uint32_value;
            iVideoParameterFlags |= VIDEO_HEIGHT_VALID;
            LOGV("iVideoHeight=%d", iVideoHeight);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                (0,"AndroidSurfaceOutput::setParametersSync() Video Height Key, Value %d",iVideoHeight));
        }
        else if (pv_mime_strcmp(aParameters[i].key, MOUT_VIDEO_DISPLAY_HEIGHT_KEY) == 0)
        {
            iVideoDisplayHeight=(int32)aParameters[i].value.uint32_value;
            iVideoParameterFlags |= DISPLAY_HEIGHT_VALID;
            LOGV("iVideoDisplayHeight=%d", iVideoDisplayHeight);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                (0,"AndroidSurfaceOutput::setParametersSync() Video Display Height Key, Value %d",iVideoDisplayHeight));
        }
        else if (pv_mime_strcmp(aParameters[i].key, MOUT_VIDEO_DISPLAY_WIDTH_KEY) == 0)
        {
            iVideoDisplayWidth=(int32)aParameters[i].value.uint32_value;
            iVideoParameterFlags |= DISPLAY_WIDTH_VALID;
            LOGV("iVideoDisplayWidth=%d", iVideoDisplayWidth);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                (0,"AndroidSurfaceOutput::setParametersSync() Video Display Width Key, Value %d",iVideoDisplayWidth));
        }
        else if (pv_mime_strcmp(aParameters[i].key, MOUT_VIDEO_SUBFORMAT_KEY) == 0)
        {
            iVideoSubFormat=aParameters[i].value.pChar_value;
            iVideoParameterFlags |= VIDEO_SUBFORMAT_VALID;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0,"AndroidSurfaceOutput::setParametersSync() Video SubFormat Key, Value %s",iVideoSubFormat.getMIMEStrPtr()));

LOGV("VIDEO SUBFORMAT SET TO %s\n",iVideoSubFormat.getMIMEStrPtr());
        }
        else
        {
            //if we get here the key is unrecognized.

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                (0,"AndroidSurfaceOutput::setParametersSync() Error, unrecognized key = %s", aParameters[i].key));

            //set the return value to indicate the unrecognized key
            //and return.
            aRet_kvp = &aParameters[i];
            return;
        }
    }
    uint32 mycache = iVideoParameterFlags ;
    // if initialization is complete, update the app display info
    if( checkVideoParameterFlags() )
    initCheck();
    iVideoParameterFlags = mycache;

    // when all necessary parameters are received, send 
    // PVMFMIOConfigurationComplete event to observer
    if(!iIsMIOConfigured && checkVideoParameterFlags() )
    {
        iIsMIOConfigured = true;
        if(iObserver)
        {
            iObserver->ReportInfoEvent(PVMFMIOConfigurationComplete);
        }
    }
}

PVMFCommandId AndroidSurfaceOutput::setParametersAsync(PvmiMIOSession aSession, PvmiKvp* aParameters,
                                                  int num_elements, PvmiKvp*& aRet_kvp, OsclAny* context)
{
    OsclError::Leave(OsclErrNotSupported);
    return -1;
}

uint32 AndroidSurfaceOutput::getCapabilityMetric (PvmiMIOSession aSession)
{
    return 0;
}

PVMFStatus AndroidSurfaceOutput::verifyParametersSync (PvmiMIOSession aSession, PvmiKvp* aParameters, int num_elements)
{
    OSCL_UNUSED_ARG(aSession);

    // Go through each parameter
    for (int32 i=0; i<num_elements; i++) {
        char* compstr = NULL;
        pv_mime_string_extract_type(0, aParameters[i].key, compstr);
        if (pv_mime_strcmp(compstr, _STRLIT_CHAR("x-pvmf/media/format-type")) == 0) {
            if (pv_mime_strcmp(aParameters[i].value.pChar_value, PVMF_MIME_YUV420) == 0) {
                return PVMFSuccess;
            }
            else {
                return PVMFErrNotSupported;
            }
        }
    }
    return PVMFSuccess;
}

//
// Private section
//

void AndroidSurfaceOutput::Run()
{
    //send async command responses
    while (!iCommandResponseQueue.empty())
    {
        if (iObserver)
            iObserver->RequestCompleted(PVMFCmdResp(iCommandResponseQueue[0].iCmdId, iCommandResponseQueue[0].iContext, iCommandResponseQueue[0].iStatus));
        iCommandResponseQueue.erase(&iCommandResponseQueue[0]);
    }

    //send async write completion
    if (iEosReceived) {
        LOGV("Flushing buffers after EOS");
        processWriteResponseQueue(0);
    } else {
        processWriteResponseQueue(1);
    }
}

// create a frame buffer for software codecs
OSCL_EXPORT_REF bool AndroidSurfaceOutput::initCheck()
{

    // initialize only when we have all the required parameters
    if (!checkVideoParameterFlags())
        return mInitialized;

    // release resources if previously initialized
    closeFrameBuf();

    // reset flags in case display format changes in the middle of a stream
    resetVideoParameterFlags();

    // copy parameters in case we need to adjust them
    int displayWidth = iVideoDisplayWidth;
    int displayHeight = iVideoDisplayHeight;
    int frameWidth = iVideoWidth;
    int frameHeight = iVideoHeight;
    int frameSize;

    // RGB-565 frames are 2 bytes/pixel
    displayWidth = (displayWidth + 1) & -2;
    displayHeight = (displayHeight + 1) & -2;
    frameWidth = (frameWidth + 1) & -2;
    frameHeight = (frameHeight + 1) & -2;
    frameSize = frameWidth * frameHeight * 2;

    // create frame buffer heap and register with surfaceflinger
    sp<MemoryHeapBase> heap = new MemoryHeapBase(frameSize * kBufferCount);
    if (heap->heapID() < 0) {
        LOGE("Error creating frame buffer heap");
        return false;
    }
    
    mBufferHeap = ISurface::BufferHeap(displayWidth, displayHeight,
            frameWidth, frameHeight, PIXEL_FORMAT_RGB_565, heap);
    mSurface->registerBuffers(mBufferHeap);

    // create frame buffers
    for (int i = 0; i < kBufferCount; i++) {
        mFrameBuffers[i] = i * frameSize;
    }

    // initialize software color converter
    iColorConverter = ColorConvert16::NewL();
    iColorConverter->Init(displayWidth, displayHeight, frameWidth, displayWidth, displayHeight, displayWidth, CCROTATE_NONE);
    iColorConverter->SetMemHeight(frameHeight);
    iColorConverter->SetMode(1);

    LOGV("video = %d x %d", displayWidth, displayHeight);
    LOGV("frame = %d x %d", frameWidth, frameHeight);
    LOGV("frame #bytes = %d", frameSize);

    // register frame buffers with SurfaceFlinger
    mFrameBufferIndex = 0;
    mInitialized = true;
    mPvPlayer->sendEvent(MEDIA_SET_VIDEO_SIZE, iVideoDisplayWidth, iVideoDisplayHeight);

    return mInitialized;
}

OSCL_EXPORT_REF PVMFStatus AndroidSurfaceOutput::writeFrameBuf(uint8* aData, uint32 aDataLen, const PvmiMediaXferHeader& data_header_info)
{
    // post to SurfaceFlinger
    if ((mSurface != NULL) && (mBufferHeap.heap != NULL)) {
        if (++mFrameBufferIndex == kBufferCount) mFrameBufferIndex = 0;
#if 0
	{
		uint32 *data_pointer = reinterpret_cast<uint32*>(aData);
		LOGE("the surfaceid is %x, location %x..\n", *data_pointer, data_pointer);
		LOGE("the vadisplay is %x, location %x \n", *(data_pointer + 1), data_pointer + 1);
	}
        iColorConverter->Convert(aData, static_cast<uint8*>(mBufferHeap.heap->base()) + mFrameBuffers[mFrameBufferIndex]);
        mSurface->postBuffer(mFrameBuffers[mFrameBufferIndex]);
#endif
	uint32 *data_pointer = reinterpret_cast<uint32*>(aData);
//	VASurfaceID surface_id =
//	reinterpret_cast<VASurfaceID>(*(data_pointer + 1));
	VASurfaceID surface_id = *(data_pointer);
	VADisplay va_display = reinterpret_cast<VADisplay>(*(data_pointer + 1));
	int data_len;

	LOGE("the surfaceid is %x\n", surface_id);
	LOGE("the vadisplay is %x\n", va_display);
#if 0
	vaPutSurface(va_display, surface_id, 0,
			0, 0, iVideoWidth, iVideoHeight,
			0, 0, iVideoDisplayWidth, iVideoDisplayHeight,
			NULL, 0, 0);
#else
	vaPutSurfaceBuf(va_display, surface_id, 0,
			static_cast<uint8*>(mBufferHeap.heap->base()) + mFrameBuffers[mFrameBufferIndex],
			&data_len, 0, 0, iVideoWidth, iVideoHeight,
			0, 0, iVideoDisplayWidth, iVideoDisplayHeight,
			NULL, 0, 0);
        mSurface->postBuffer(mFrameBuffers[mFrameBufferIndex]);
#endif
    }
    return PVMFSuccess;
}

OSCL_EXPORT_REF void AndroidSurfaceOutput::closeFrameBuf()
{
    LOGV("closeFrameBuf");
    if (!mInitialized) return;

    mInitialized = false;
    if (mSurface.get()) {
        LOGV("unregisterBuffers");
        mSurface->unregisterBuffers();
    }

    // free frame buffers
    LOGV("free frame buffers");
    for (int i = 0; i < kBufferCount; i++) {
        mFrameBuffers[i] = 0;
    }

    // free heaps
    LOGV("free frame heap");
    mBufferHeap.heap.clear();

    // free color converter
    if (iColorConverter != 0)
    {
        LOGV("free color converter");
        delete iColorConverter;
        iColorConverter = 0;
    }
}

OSCL_EXPORT_REF bool AndroidSurfaceOutput::GetVideoSize(int *w, int *h) {

    *w = iVideoDisplayWidth;
    *h = iVideoDisplayHeight;
    return iVideoDisplayWidth != 0 && iVideoDisplayHeight != 0;
}
