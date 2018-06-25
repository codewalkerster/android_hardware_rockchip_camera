/*
 * Copyright (C) 2016-2017 Intel Corporation.
 * Copyright (c) 2017, Fuzhou Rockchip Electronics Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "InputFrameWorker"

#include "LogHelper.h"
#include "PerformanceTraces.h"
#include "InputFrameWorker.h"
#include "NodeTypes.h"
#include <sys/mman.h>

namespace android {
namespace camera2 {

InputFrameWorker::InputFrameWorker(int cameraId,
                camera3_stream_t* stream, std::vector<camera3_stream_t*>& outStreams,
                size_t pipelineDepth) :
                IDeviceWorker(cameraId),
                mStream(stream),
                mOutputStreams(outStreams),
                mNeedPostProcess(false),
                mPipelineDepth(pipelineDepth),
                mPostPipeline(new PostProcessPipeLine(this, cameraId))
{
        LOGI("@%s, instance(%p), mStream(%p)", __FUNCTION__, this, mStream);
}

InputFrameWorker::~InputFrameWorker()
{
    HAL_TRACE_CALL(CAM_GLBL_DBG_HIGH);
}

status_t
InputFrameWorker::stopWorker()
{
    mPostPipeline->flush();
    mPostPipeline->stop();
    mPostPipeline.reset();
    mProcessingInputBufs.clear();

    return OK;
}

status_t
InputFrameWorker::startWorker()
{
    return OK;
}

status_t
InputFrameWorker::notifyNewFrame(const std::shared_ptr<PostProcBuffer>& buf,
                                  const std::shared_ptr<ProcUnitSettings>& settings,
                                  int err)
{
    CameraStream *stream = buf->cambuf->getOwner();
    Camera3Request* request = buf->request;

    stream->captureDone(buf->cambuf, request);

    std::shared_ptr<CameraBuffer> camBuf = *mProcessingInputBufs.begin();

    if (camBuf.get()) {
        stream = camBuf->getOwner();
        stream->captureDone(camBuf, request);
        mProcessingInputBufs.erase(mProcessingInputBufs.begin());
    }

    return OK;
}

status_t InputFrameWorker::configure(std::shared_ptr<GraphConfig> &/*config*/)
{
    HAL_TRACE_CALL(CAM_GLBL_DBG_HIGH);

    FrameInfo sourceFmt;
    sourceFmt.width = mStream->width;
    sourceFmt.height = mStream->height;
    /* TODO: not used now, set to 0 */
    sourceFmt.size = 0;
    sourceFmt.format = mStream->format;
    sourceFmt.stride = mStream->width;

    std::vector<camera3_stream_t*>::iterator iter =
        mOutputStreams.begin();
    for (; iter != mOutputStreams.end(); iter++)
        if ((*iter)->stream_type == CAMERA3_STREAM_BIDIRECTIONAL)
            break;
    if (iter != mOutputStreams.begin() && iter != mOutputStreams.end())
        mOutputStreams.erase(iter);

    mPostPipeline->prepare(sourceFmt, mOutputStreams, mNeedPostProcess);
    mPostPipeline->start();

    return OK;
}

status_t InputFrameWorker::prepareRun(std::shared_ptr<DeviceMessage> msg)
{
    HAL_TRACE_CALL(CAM_GLBL_DBG_HIGH);
    mMsg = msg;
    status_t status = NO_ERROR;
    std::shared_ptr<CameraBuffer> inBuf;
    std::vector<std::shared_ptr<CameraBuffer>> outBufs;

    if (!mStream)
        return NO_ERROR;

    Camera3Request* request = mMsg->cbMetadataMsg.request;
    request->setSequenceId(-1);

    inBuf = findInputBuffer(request, mStream);
    outBufs = findOutputBuffers(request);
    if (inBuf.get() && outBufs.size() > 0) {
        // Work for mStream
        status = prepareBuffer(inBuf);
        if (status != NO_ERROR) {
            LOGE("prepare buffer error!");
            goto exit;
        }

        // If Input format is something else than
        // NV21 or Android flexible YCbCr 4:2:0, return
        if (inBuf->format() != HAL_PIXEL_FORMAT_YCrCb_420_SP &&
            inBuf->format() != HAL_PIXEL_FORMAT_YCbCr_420_888 &&
            inBuf->format() != HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED &&
            inBuf->format() != HAL_PIXEL_FORMAT_BLOB)  {
            LOGE("Bad format %d", inBuf->format());
            status = BAD_TYPE;
            goto exit;
        }

        for (auto buf : outBufs) {
            status = prepareBuffer(buf);
            if (status != NO_ERROR) {
                LOGE("prepare buffer error!");
                goto exit;
            }

            // If Input format is something else than
            // NV21 or Android flexible YCbCr 4:2:0, return
            if (buf->format() != HAL_PIXEL_FORMAT_YCrCb_420_SP &&
                buf->format() != HAL_PIXEL_FORMAT_YCbCr_420_888 &&
                buf->format() != HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED &&
                buf->format() != HAL_PIXEL_FORMAT_BLOB)  {
                LOGE("Bad format %d", buf->format());
                status = BAD_TYPE;
                goto exit;
            }
        }
        mProcessingInputBufs.push_back(inBuf);
    } else {
        LOGD("No work for this worker mStream: %p", mStream);
        return NO_ERROR;
    }

    LOGI("%s:%d:instance(%p), requestId(%d)", __FUNCTION__, __LINE__, this, request->getId());

exit:
    if (status < 0)
        returnBuffers();

    return status < 0 ? status : OK;
}

status_t InputFrameWorker::run()
{
    status_t status = NO_ERROR;
    HAL_TRACE_CALL(CAM_GLBL_DBG_HIGH);
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts); 

    // Update request sequence if needed
    Camera3Request* request = mMsg->cbMetadataMsg.request;

    ICaptureEventListener::CaptureMessage outMsg;
    outMsg.data.event.reqId = request->getId();
    outMsg.id = ICaptureEventListener::CAPTURE_MESSAGE_ID_EVENT;
    outMsg.data.event.type = ICaptureEventListener::CAPTURE_EVENT_SHUTTER;
    outMsg.data.event.timestamp.tv_sec = ts.tv_sec;
    outMsg.data.event.timestamp.tv_usec = ts.tv_nsec / 1000;
    outMsg.data.event.sequence = request->sequenceId();
    notifyListeners(&outMsg);

    LOGI("%s:%d:instance(%p), frame_id(%d), requestId(%d)", __FUNCTION__, __LINE__, this, request->sequenceId(), request->getId());

    return (status < 0) ? status : OK;
}

status_t InputFrameWorker::postRun()
{
    HAL_TRACE_CALL(CAM_GLBL_DBG_HIGH);

    status_t status = OK;
    CameraStream *stream;
    Camera3Request* request = nullptr;
    std::vector<std::shared_ptr<PostProcBuffer>> outBufs;
    std::shared_ptr<PostProcBuffer> postOutBuf;
    std::shared_ptr<PostProcBuffer> inBuf = std::make_shared<PostProcBuffer> ();
    std::vector<std::shared_ptr<CameraBuffer>> camBufs;
    std::shared_ptr<CameraBuffer> inCamBuf;
    int stream_type;

    if (mMsg == nullptr) {
        LOGE("Message null - Fix the bug");
        status = UNKNOWN_ERROR;
        goto exit;
    }

    request = mMsg->cbMetadataMsg.request;
    if (request == nullptr) {
        LOGE("No request provided for captureDone");
        status = UNKNOWN_ERROR;
        goto exit;
    }

    camBufs = findOutputBuffers(request);

    for(auto buf : camBufs) {
        postOutBuf = std::make_shared<PostProcBuffer> ();
        postOutBuf->cambuf = buf;
        postOutBuf->request = request;
        outBufs.push_back(postOutBuf);
        postOutBuf.reset();
    }

    inCamBuf = findInputBuffer(request, mStream);
    inBuf->cambuf = inCamBuf;
    inBuf->request = request;

    mPostPipeline->processFrame(inBuf, outBufs, mMsg->pMsg.processingSettings);

exit:
    /* Prevent from using old data */
    mMsg = nullptr;
    if (status != OK)
        returnBuffers();

    return status;
}

void InputFrameWorker::returnBuffers()
{
    if (!mMsg || !mMsg->cbMetadataMsg.request)
        return;

    Camera3Request* request = mMsg->cbMetadataMsg.request;
    std::shared_ptr<CameraBuffer> buffer;

    buffer = findInputBuffer(request, mStream);
    if (buffer.get() && buffer->isRegistered())
        buffer->getOwner()->captureDone(buffer, request);
    std::vector<std::shared_ptr<CameraBuffer>> outBufs =
        findOutputBuffers(request);
    for(auto buf : outBufs) {
        if (buf.get() && buf->isRegistered())
            buf->getOwner()->captureDone(buf, request);
    }
}

status_t
InputFrameWorker::prepareBuffer(std::shared_ptr<CameraBuffer>& buffer)
{
    CheckError((buffer.get() == nullptr), UNKNOWN_ERROR, "null buffer!");

    status_t status = NO_ERROR;
    if (!buffer->isLocked()) {
        status = buffer->lock();
        if (CC_UNLIKELY(status != NO_ERROR)) {
            LOGE("Could not lock the buffer error %d", status);
            return UNKNOWN_ERROR;
        }
    }
    status = buffer->waitOnAcquireFence();
    if (CC_UNLIKELY(status != NO_ERROR)) {
        LOGW("Wait on fence for buffer %p timed out", buffer.get());
    }
    return status;
}

std::shared_ptr<CameraBuffer>
InputFrameWorker::findInputBuffer(Camera3Request* request,
                              camera3_stream_t* stream)
{
    CheckError((request == nullptr || stream == nullptr), nullptr,
                "null request/stream!");

    CameraStream *s = nullptr;
    std::shared_ptr<CameraBuffer> buffer = nullptr;
    const std::vector<camera3_stream_buffer>* inputBufs =
                                        request->getInputBuffers();

    for (camera3_stream_buffer InputBuffer : *inputBufs) {
        s = reinterpret_cast<CameraStream *>(InputBuffer.stream->priv);
        if (s->getStream() == stream) {
            buffer = request->findBuffer(s, false);
            if (CC_UNLIKELY(buffer == nullptr)) {
                LOGW("buffer not found for stream");
            }
            break;
        }
    }

    if (buffer.get() == nullptr) {
        LOGI("No buffer for stream %p in req %d", stream, request->getId());
    }
    return buffer;
}

std::vector<std::shared_ptr<CameraBuffer>>
InputFrameWorker::findOutputBuffers(Camera3Request* request)
{
    CameraStream *s = nullptr;
    std::vector<std::shared_ptr<CameraBuffer>> buffers;
    std::shared_ptr<CameraBuffer> buf;
    const std::vector<camera3_stream_buffer>* outBufs =
        request->getOutputBuffers();

    for (camera3_stream_buffer OutputBuffer : *outBufs) {
        s = reinterpret_cast<CameraStream *>(OutputBuffer.stream->priv);
        buf = request->findBuffer(s, false);
        if (CC_UNLIKELY(buf == nullptr)) {
            LOGW("buffer not found for stream");
        }
        buffers.push_back(buf);
    }

    return buffers;
}

} /* namespace camera2 */
} /* namespace android */