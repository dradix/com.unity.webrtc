#include "pch.h"
#include "UnityVideoRenderer.h"

namespace unity {
    namespace webrtc {



        static void YUVBuffersToFrame(uint8_t* Destination, const uint8_t* YBuffer, const uint8_t* UBuffer, const  uint8_t* VBuffer, int width, int height)
        {
            //Allocate memory for  image buffer is Y = width*height, U = Y/4, V= Y/4
            //TODO: Check overflow of Buffers
            int YSize = width * height;

            memcpy_s(Destination, YSize * 3 / 2, YBuffer, YSize);
            memcpy_s(&Destination[YSize], YSize / 2, UBuffer, YSize / 4);
            memcpy_s(&Destination[YSize + YSize / 4], YSize / 4, VBuffer, YSize / 4);
        }
        UnityVideoRenderer::UnityVideoRenderer(uint32_t id) : m_id(id)
        {
            DebugLog("Create UnityVideoRenderer Id:%d", id);
        }

        UnityVideoRenderer::~UnityVideoRenderer()
        {
            DebugLog("Destory UnityVideoRenderer Id:%d", m_id);
            {
                std::unique_lock<std::mutex> lock(m_mutex);
            }
        }

        void UnityVideoRenderer::OnFrame(const webrtc::VideoFrame& frame)
        {
            rtc::scoped_refptr<webrtc::VideoFrameBuffer> frame_buffer = frame.video_frame_buffer();

            if (frame_buffer->type() == webrtc::VideoFrameBuffer::Type::kNative)
            {
                frame_buffer = frame_buffer->ToI420();
            }

            {
                std::unique_lock<std::mutex> lock(frameId_mutex);
                currentFrameId = (int)frame.ntp_time_ms();
            }
            SetFrameBuffer(frame_buffer);
        }

        uint32_t UnityVideoRenderer::GetId()
        {
            return m_id;
        }

        rtc::scoped_refptr<webrtc::VideoFrameBuffer> UnityVideoRenderer::GetFrameBuffer()
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (!lock.owns_lock())
            {
                return nullptr;
            }

            return m_frameBuffer;
        }

        void UnityVideoRenderer::SetFrameBuffer(rtc::scoped_refptr<webrtc::VideoFrameBuffer> buffer)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (!lock.owns_lock())
            {
                return;
            }

            m_frameBuffer = buffer;
        }

        void UnityVideoRenderer::ConvertVideoFrameToTextureAndWriteToBuffer(int width, int height, webrtc::VideoType format, DelegateNativeYUVCallback OnYUVFrame, void* SyncContext)
        {
            auto frame = GetFrameBuffer();
            int FrameId = 0;
            std::unique_lock<std::mutex> lock(frameId_mutex);
            {
                FrameId = currentFrameId;
            }
            if (frame == nullptr)
            {
                return;
            }

            rtc::scoped_refptr<webrtc::I420BufferInterface> i420_buffer;
            if (width == frame->width() && height == frame->height())
            {
                i420_buffer = frame->ToI420();
            }
            else
            {
                auto temp = webrtc::I420Buffer::Create(width, height);
                temp->ScaleFrom(*frame->ToI420());
                i420_buffer = temp;
            }

            size_t size = width * height * 4;
            if (tempBuffer.size() != size)
                tempBuffer.resize(size);
            if ((m_id ==0) && OnYUVFrame != nullptr)
            {
                uint8_t* Destination = new uint8_t[width * height * 3 / 2];
                YUVBuffersToFrame(Destination, i420_buffer->DataY(), i420_buffer->DataU(), i420_buffer->DataV(), width, height);
                if (Destination == nullptr)
                {
                    DebugLog("Destination buffer is nullptr");
                }
                else
                {
                    (*OnYUVFrame)(SyncContext, Destination, width * height * 3 / 2, currentFrameId);
                }
            }


            DebugLog("Stride  %d  %d %d", i420_buffer->StrideY(), i420_buffer->StrideU(), i420_buffer->StrideV());
            libyuv::ConvertFromI420(
                i420_buffer->DataY(), i420_buffer->StrideY(), i420_buffer->DataU(),
                i420_buffer->StrideU(), i420_buffer->DataV(), i420_buffer->StrideV(),
                tempBuffer.data(), 0, width, height,
                ConvertVideoType(format));
        }


    } // end namespace webrtc
} // end namespace unity
