#pragma once

#include <string>
#include <memory>
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

namespace vio
{
    /// 動画を書き出すためのクラス．
    /// 現在の実装では，動画は常に無圧縮AVIで出力される．
    class VideoWriter {
    public:
        /// 書き込み先を指定して VideoWriter を初期化する．
        /// @return 成功した場合は VideoWriter のインスタンスを返す．失敗した場合は null を返す．
        static std::unique_ptr<VideoWriter> Open(const std::string& fileName,
                                                 int width, int height,
                                                 AVPixelFormat pixelFormat);

        /// デストラクタ
        ~VideoWriter();

        /// ファイルを閉じる．
        /// この関数を呼ばなかった場合はデストラクタで自動的に呼ばれる．
        void Close();

        /// フレームを書き込む．
        /// @return 成功した場合は true,　失敗した場合は false を返す．
        bool WriteFrame(const AVFrame* frame);

    private:
        VideoWriter():
            mClosed(false), mFormat(nullptr), mStream(nullptr),
            mBufferSize(0), mFrameCount(0) {}

    private:
        bool mClosed;
        AVOutputFormat* mFormat;
        std::shared_ptr<AVFormatContext> mFormatContext;
        AVStream* mStream;
        std::shared_ptr<AVCodecContext> mCodecContext;
        std::shared_ptr<uint8_t> mBuffer;
        int mBufferSize;
        int64_t mFrameCount;
    };
}
