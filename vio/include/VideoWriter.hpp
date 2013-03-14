#pragma once

#include <string>
#include <memory>
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

namespace vio
{
    /// ����������o�����߂̃N���X
    class VideoWriter {
    public:
        /// �������ݐ���w�肵�� VideoWriter ������������D
        /// @return ���������ꍇ�� VideoWriter �̃C���X�^���X��Ԃ��D���s�����ꍇ�� null ��Ԃ��D
        static std::unique_ptr<VideoWriter> Open(const std::string& fileName,
                                                 int width, int height,
                                                 AVPixelFormat pixelFormat);

        /// �f�X�g���N�^
        ~VideoWriter();

        /// �t�@�C�������D
        /// ���̊֐����Ă΂Ȃ������ꍇ�̓f�X�g���N�^�Ŏ����I�ɌĂ΂��D
        void Close();

        /// �t���[�����������ށD
        /// @return ���������ꍇ�� true,�@���s�����ꍇ�� false ��Ԃ��D
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