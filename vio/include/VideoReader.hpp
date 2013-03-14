#pragma once

#include <string>
#include <memory>
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}

namespace vio
{
    /// 動画を読み込むためのクラス
    class VideoReader {
    public:
        /// fileName を開く．
        /// @return 成功した場合は VideoReader のインスタンスを返す．失敗した場合は null を返す．
        static std::unique_ptr<VideoReader> Open(const std::string& fileName);

        /// 動画のフレーム数を返す．
        int64_t GetFrameCount() const { return mFormatContext->streams[mStreamIndex]->nb_frames; }

        /// 動画のフォーマットを返す．
        AVPixelFormat GetPixelFormat() const { return mCodecContext->pix_fmt; }

        /// 画像の幅を返す．
        int GetWidth() const { return mCodecContext->width; }

        /// 画像の高さを返す．
        int GetHeight() const { return mCodecContext->height; }

        /// Read()が返却する画像のフォーマットを返す．
        AVPixelFormat GetOutputPixelFormat() const { return mOutputPixelFormat; }

        /// Read()が返却する画像のフォーマットを設定する．
        bool SetOutputPixelFormat(AVPixelFormat format);

        /// 次のフレームを読み込んで返す．
        /// この関数は内部のバッファへの参照を返すため，もう一度この関数を呼び出すと以前の呼び出しで得られた画像は破壊される．
        /// @return 動画の終端に達した場合は null を返す．そうでない場合は AVFrame を指すポインタを返す．
        AVFrame* ReadNextFrame();

        /// 指定したフレームにシークする．
        /// @return シークできた場合は true, そうでない場合は false.
        bool Seek(int64_t frame);

    private:
        VideoReader(): mStreamIndex(-1), mOutputPixelFormat(PIX_FMT_NONE) {}

    private:
        int mStreamIndex;
        std::shared_ptr<AVFormatContext> mFormatContext;
        std::shared_ptr<AVCodecContext> mCodecContext;
        std::shared_ptr<AVFrame> mFrame;

        AVPixelFormat mOutputPixelFormat;
        std::shared_ptr<uint8_t> mConvertBuffer;
        std::shared_ptr<AVFrame> mConvertedFrame;
        std::shared_ptr<SwsContext> mSwsContext;
    };
}
