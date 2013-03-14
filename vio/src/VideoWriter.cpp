#include "VideoWriter.hpp"
extern "C" {
#include "libavutil/avstring.h"
}

std::unique_ptr<vio::VideoWriter> vio::VideoWriter::Open(const std::string& fileName,
                                                         int width, int height,
                                                         AVPixelFormat pixelFormat)
{
    // コンテナのフォーマットはAVI
    AVOutputFormat* format = av_guess_format("avi", fileName.c_str(), nullptr);
    if (!format)
        return nullptr;

    std::shared_ptr<AVFormatContext> formatContext(avformat_alloc_context(),
                                                   avformat_free_context);
    if (!formatContext)
        return nullptr;
    formatContext->oformat = format;
    av_strlcpy(formatContext->filename, fileName.c_str(), sizeof(formatContext->filename));

    AVStream* stream = av_new_stream(formatContext.get(), 0);
    if (!stream)
        return nullptr;

    CodecID codecId = AV_CODEC_ID_RAWVIDEO;         // コーデックはrawvideo (無圧縮)
    AVCodec* codec = avcodec_find_encoder(codecId);
    if (!codec)
        return nullptr;

    std::shared_ptr<AVCodecContext> codecContext(stream->codec, avcodec_close);
    codecContext->codec_id = codec->id;
    codecContext->codec_type = AVMEDIA_TYPE_VIDEO;
    codecContext->width = width;
    codecContext->height = height;
    codecContext->time_base.num = 1;
    codecContext->time_base.den = 30;

    codecContext->pix_fmt = pixelFormat;
    if (codec && codec->pix_fmts) {
        const PixelFormat* p = codec->pix_fmts;
        for (; *p != -1; ++p) {
            if (*p == codecContext->pix_fmt)
                break;
        }
        if (*p == -1)   // コーデックが指定したピクセルフォーマットをサポートしていない
            return nullptr;
    }
    if (format->flags & AVFMT_GLOBALHEADER)
        codecContext->flags |= CODEC_FLAG_GLOBAL_HEADER;

    if (avcodec_open(codecContext.get(), codec) != 0)
        return nullptr;

    if (!(format->flags & AVFMT_NOFILE)) {
        if (avio_open(&formatContext->pb, formatContext->filename, AVIO_FLAG_WRITE) < 0)
            return nullptr;
    }

    if (avformat_write_header(formatContext.get(), nullptr) < 0)
        return nullptr;

    int size = codecContext->width * codecContext->height * 4 + 1024;
    std::shared_ptr<uint8_t> buffer(static_cast<uint8_t*>(av_malloc(size)), av_free);

    std::unique_ptr<VideoWriter> writer(new VideoWriter());
    writer->mFormat = format;
    writer->mFormatContext = formatContext;
    writer->mStream = stream;
    writer->mCodecContext = codecContext;
    writer->mBuffer = buffer;
    writer->mBufferSize = size;
    return writer;
}

void vio::VideoWriter::Close()
{
    if (mClosed)
        return;

    av_write_trailer(mFormatContext.get());
    if (!(mFormat->flags & AVFMT_NOFILE)) {
        avio_close(mFormatContext->pb);
    }
    mClosed = true;
}

vio::VideoWriter::~VideoWriter()
{
    if (!mClosed)
        Close();
}

bool vio::VideoWriter::WriteFrame(const AVFrame* frame)
{
    if (mClosed)
        return false;

    AVPacket packet;
    av_init_packet(&packet);

    int outSize = avcodec_encode_video(mCodecContext.get(), mBuffer.get(),
                                       mBufferSize, frame);
    if (outSize == 0)
        return true;
    if (outSize < 0)
        return false;

    packet.stream_index = mStream->index;
    packet.data = mBuffer.get();
    packet.size = outSize;

    int64_t pts = mCodecContext->coded_frame->pts;
    packet.pts = av_rescale_q(pts != AV_NOPTS_VALUE ? pts : mFrameCount + 1,
                              mCodecContext->time_base,
                              mStream->time_base);
    if (mCodecContext->coded_frame->key_frame)
        packet.flags |= AV_PKT_FLAG_KEY;

    int ret = av_interleaved_write_frame(mFormatContext.get(), &packet);
    av_free_packet(&packet);

    if (ret == 0) {
        mFrameCount += 1;
        return true;
    } else {
        return false;
    }
}
