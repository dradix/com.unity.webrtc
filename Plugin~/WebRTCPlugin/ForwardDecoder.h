#pragma once
#include <memory>
#include <string>
#include <vector>

#include "api/video/encoded_image.h"
#include "api/video/video_frame.h"
#include "api/video_codecs/video_codec.h"
#include "rtc_base/system/rtc_export.h"
#include "api/video_codecs/video_decoder.h"

#include "WebRTCPlugin.h"
namespace unity
{
    namespace webrtc
    {
        namespace webrtc = ::webrtc;
        class ForwardDecoder : public webrtc::VideoDecoder
        {
            DelegateNativeOnDecodeRequestFrame* OnFrameInternal = nullptr;
       
            std::unique_ptr<webrtc::VideoDecoder> OriginalRef;

        public:
            ForwardDecoder(std::unique_ptr<webrtc::VideoDecoder> Original,
                DelegateNativeOnDecodeRequestFrame* OnFrame
               ) {
                OriginalRef = std::move(Original);
                OnFrameInternal = OnFrame;
         
            }
            ~ForwardDecoder() {// OriginalRef->~VideoDecoder()};
            }

            int32_t InitDecode(const webrtc::VideoCodec* codec_settings,
                int32_t number_of_cores)
            {
                if (!OriginalRef)
                {
                    return -1;
                }
                return OriginalRef->InitDecode(codec_settings, number_of_cores);
            };

            int32_t Decode(const webrtc::EncodedImage& input_image,
                bool missing_frames,
                int64_t render_time_ms)
            {
                if (!OriginalRef)
                {
                    return -1;
                }

                if (OnFrameInternal != nullptr )
                {
                    if (*OnFrameInternal != nullptr)
                    {                        
                        if (input_image.GetEncodedData()->data() != nullptr)
                        {
                            (*OnFrameInternal)(input_image.GetEncodedData()->data(), input_image.GetEncodedData()->size(), input_image.ntp_time_ms_);
                        }                        
                    }
                }
                
                return OriginalRef->Decode(input_image, missing_frames, render_time_ms);
            };

            int32_t RegisterDecodeCompleteCallback(
                webrtc::DecodedImageCallback* callback)
            {
                if (!OriginalRef)
                {
                    return -1;
                }
                return OriginalRef->RegisterDecodeCompleteCallback(callback);
            };

            int32_t Release() override
            {
                if (!OriginalRef)
                {
                    return -1;
                }
                return OriginalRef->Release();
            };
            /*
            DecoderInfo GetDecoderInfo() const override
            {
                return OriginalRef->GetDecoderInfo();
            };*/


            bool PrefersLateDecoding() const override
            {
                if (!OriginalRef)
                {
                    return false;
                }
                return OriginalRef->PrefersLateDecoding();
            };
            // Deprecated, use GetDecoderInfo().implementation_name instead.
            const char* ImplementationName() const override
            {
                if (!OriginalRef)
                {
                    return nullptr;
                }
                return OriginalRef->ImplementationName();
            }
        };

    }
}
