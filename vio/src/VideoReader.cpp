#include "VideoReader.hpp"

namespace
{
    void InitializeFFmpeg()
    {
        av_register_all();
    }

    std::shared_ptr<AVFrame> CreateFrame()
    {
        return std::shared_ptr<AVFrame>(avcodec_alloc_frame(), av_free);
    }

    std::shared_ptr<uint8_t> CreateBuffer(AVPixelFormat format, int width, int height)
    {
        const int size = avpicture_get_size(format, width, height);
        return std::shared_ptr<uint8_t>(static_cast<uint8_t*>(av_malloc(size)),
                                        av_free);
    }

    std::shared_ptr<AVFormatContext> CreateFormatContext(const std::string& fileName)
    {
        AVFormatContext* formatContext = nullptr;
        if (avformat_open_input(&formatContext, fileName.c_str(), nullptr, nullptr) != 0)
            return nullptr;
        avformat_find_stream_info(formatContext, nullptr);
        return std::shared_ptr<AVFormatContext>(formatContext,
                [](AVFormatContext* p){ if (p) avformat_close_input(&p); });
    }

    int FindFirstVideoStreamIndex(AVFormatContext* formatContext)
    {
        for (int i = 0; i < (int)formatContext->nb_streams; ++i) {
            if (formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
                return i;
        }
        return -1;
    }

    AVCodec* FindAndOpenCodec(AVStream* stream)
    {
        AVCodecContext* codecContext = stream->codec;
        AVCodec* codec = avcodec_find_decoder(codecContext->codec_id);
        if (!codec) return nullptr;
        if (avcodec_open(codecContext, codec) != 0) return nullptr;
        return codec;
    }

    AVFrame* ConvertPixelFormat(
        AVPixelFormat format, AVCodecContext* codec, AVFrame* frame,
        AVFrame* convertedFrame, SwsContext* swsContext)
    {
        if (format != codec->pix_fmt) {
            sws_scale(swsContext, frame->data, frame->linesize, 0,
                      codec->height, convertedFrame->data, convertedFrame->linesize);
        } else {
            av_picture_copy(reinterpret_cast<AVPicture*>(convertedFrame),
                            reinterpret_cast<AVPicture*>(frame),
                            format, codec->width, codec->height);
        }
        convertedFrame->width = frame->width;
        convertedFrame->height = frame->height;
        convertedFrame->format = format;
        convertedFrame->pts = frame->pts;
        return convertedFrame;
    }
}

std::unique_ptr<vio::VideoReader> vio::VideoReader::Open(const std::string& fileName)
{
    std::unique_ptr<VideoReader> reader(new VideoReader());

    InitializeFFmpeg();
    
    reader->mFormatContext = CreateFormatContext(fileName);
    if (!reader->mFormatContext) return nullptr;

    reader->mStreamIndex = FindFirstVideoStreamIndex(reader->mFormatContext.get());
    if (reader->mStreamIndex == -1) return nullptr;

    AVStream* stream = reader->mFormatContext->streams[reader->mStreamIndex];
    if (!FindAndOpenCodec(stream)) return nullptr;

    reader->mCodecContext.reset(stream->codec, avcodec_close);
    reader->mFrame = CreateFrame();

    reader->SetOutputPixelFormat(reader->GetPixelFormat());

    return reader;
}

bool vio::VideoReader::SetOutputPixelFormat(AVPixelFormat format)
{
    if (format != PIX_FMT_NONE) {
        mConvertBuffer = CreateBuffer(format, GetWidth(), GetHeight());
        mConvertedFrame = CreateFrame();
        avpicture_fill(reinterpret_cast<AVPicture*>(mConvertedFrame.get()),
                       mConvertBuffer.get(), format, GetWidth(), GetHeight());

        mSwsContext.reset(
            sws_getContext(GetWidth(), GetHeight(), GetPixelFormat(),
                           GetWidth(), GetHeight(), format,
                           SWS_BILINEAR, nullptr, nullptr, nullptr),
            sws_freeContext);
        if (!mSwsContext)
            return false;
    }
    mOutputPixelFormat = format;
    return true;
}

AVFrame* vio::VideoReader::ReadNextFrame()
{
    const AVPixelFormat format =
        (GetOutputPixelFormat() == PIX_FMT_NONE) ? GetPixelFormat() : GetOutputPixelFormat();

    AVPacket packet;
    AVFrame* ret = nullptr;
    while (!ret && av_read_frame(mFormatContext.get(), &packet) >= 0) {
        if (packet.stream_index == mStreamIndex) {
            int finished;
            avcodec_decode_video2(mCodecContext.get(), mFrame.get(), &finished, &packet);
            if (finished) {
                ret = ConvertPixelFormat(format, mCodecContext.get(), mFrame.get(),
                                         mConvertedFrame.get(), mSwsContext.get());
            }
        }
        av_free_packet(&packet);
    }
    return ret;
}

bool vio::VideoReader::Seek(int64_t frame)
{
    int ret = avformat_seek_file(mFormatContext.get(), mStreamIndex,
                                 0, frame, frame, AVSEEK_FLAG_FRAME);
    if (ret < 0)
        return false;

    avcodec_flush_buffers(mCodecContext.get());
    return true;
}
