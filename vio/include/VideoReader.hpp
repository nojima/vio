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
    /// �����ǂݍ��ނ��߂̃N���X
    class VideoReader {
    public:
        /// fileName ���J���D
        /// @return ���������ꍇ�� VideoReader �̃C���X�^���X��Ԃ��D���s�����ꍇ�� null ��Ԃ��D
        static std::unique_ptr<VideoReader> Open(const std::string& fileName);

        /// ����̃t���[������Ԃ��D
        int64_t GetFrameCount() const { return mFormatContext->streams[mStreamIndex]->nb_frames; }

        /// ����̃t�H�[�}�b�g��Ԃ��D
        AVPixelFormat GetPixelFormat() const { return mCodecContext->pix_fmt; }

        /// �摜�̕���Ԃ��D
        int GetWidth() const { return mCodecContext->width; }

        /// �摜�̍�����Ԃ��D
        int GetHeight() const { return mCodecContext->height; }

        /// Read()���ԋp����摜�̃t�H�[�}�b�g��Ԃ��D
        AVPixelFormat GetOutputPixelFormat() const { return mOutputPixelFormat; }

        /// Read()���ԋp����摜�̃t�H�[�}�b�g��ݒ肷��D
        bool SetOutputPixelFormat(AVPixelFormat format);

        /// ���̃t���[����ǂݍ���ŕԂ��D
        /// ���̊֐��͓����̃o�b�t�@�ւ̎Q�Ƃ�Ԃ����߁C������x���̊֐����Ăяo���ƈȑO�̌Ăяo���œ���ꂽ�摜�͔j�󂳂��D
        /// @return ����̏I�[�ɒB�����ꍇ�� null ��Ԃ��D�����łȂ��ꍇ�� AVFrame ���w���|�C���^��Ԃ��D
        AVFrame* ReadNextFrame();

        /// �w�肵���t���[���ɃV�[�N����D
        /// @return �V�[�N�ł����ꍇ�� true, �����łȂ��ꍇ�� false.
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
