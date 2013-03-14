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
    /// ����������o�����߂̃N���X
    class VideoWriter {
    public:
        /// �������ݐ���w�肵�� VideoWriter ������������
        static std::unique_ptr<VideoWriter> Open(const std::string& fileName,
                                                 int width, int height,
                                                 AVPixelFormat pixelFormat);

        /// �f�X�g���N�^
        ~VideoWriter();

        /// �t�@�C�������D
        /// ���̊֐����Ă΂Ȃ������ꍇ�̓f�X�g���N�^�Ŏ����I�ɌĂ΂��D
        void Close();

        /// �t���[�����������ށD
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