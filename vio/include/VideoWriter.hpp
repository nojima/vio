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
    /// 動画を書き出すためのクラス
    class VideoWriter {
    public:
        /// 書き込み先を指定して VideoWriter を初期化する
        static std::unique_ptr<VideoWriter> Open(const std::string& fileName,
                                                 int width, int height,
                                                 AVPixelFormat pixelFormat);

        /// デストラクタ
        ~VideoWriter();

        /// ファイルを閉じる．
        /// この関数を呼ばなかった場合はデストラクタで自動的に呼ばれる．
        void Close();

        /// フレームを書き込む．
        bool WriteFrame(const AVFrame* frame);

    private:
        VideoWriter():
            mClosed(false), mFormat(nullptr),
            mBufferSize(0), mStream(nullptr), mFrameCount(0) {}

    private:
        bool mClosed;
        AVOutputFormat* mFormat;
        std::shared_ptr<AVFormatContext> mFormatContext;
        std::shared_ptr<uint8_t> mBuffer;
        int mBufferSize;
        std::shared_ptr<AVCodecContext> mCodecContext;
        AVStream* mStream;
        int64_t mFrameCount;
    };
}