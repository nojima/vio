#define _CRT_SECURE_NO_WARNINGS
#include "VideoReader.hpp"
#include "VideoWriter.hpp"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>
using namespace std;


// Portable Gray Map で書き出す．(PGMはGIMPなどで表示できる)
void WriteFramePGM(const string& filename, AVFrame* frame, int width, int height)
{
    FILE* f = fopen(filename.c_str(), "w");
    if (!f) return;

    fprintf(f, "P2\n%d %d\n255\n", width, height);

    uint8_t* pixels = frame->data[0];
    int linesize = frame->linesize[0];
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            fprintf(f, "%d\n", pixels[y * linesize + x]);
        }
    }

    fclose(f);
}

// Portable Pixel Map で書き出す．(PPMはGIMPなどで表示できる)
void WriteFramePPM(const string& filename, AVFrame* frame, int width, int height)
{
    FILE* f = fopen(filename.c_str(), "w");
    if (!f) return;

    fprintf(f, "P3\n%d %d\n255\n", width, height);

    uint8_t* pixels = frame->data[0];
    int linesize = frame->linesize[0];
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            uint8_t* pixel = pixels + y * linesize + 3 * x;
            fprintf(f, "%d %d %d\n", pixel[0], pixel[1], pixel[2]);
        }
    }

    fclose(f);
}

// 色を反転する．
void Negate(AVFrame* frame, int width, int height)
{
    uint8_t* pixels = frame->data[0];
    int linesize = frame->linesize[0];
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            uint8_t* pixel = pixels + y * linesize + 3 * x;
            for (int c = 0; c < 3; ++c) {
                pixel[c] = 255 - pixel[c];
            }
        }
    }
}

int main(int argc, char** argv)
{
    if (argc <= 1) {
        cout << "Usage: example.exe AVI_FILE" << endl;
        exit(EXIT_FAILURE);
    }

    string fileName = argv[1];
    std::unique_ptr<vio::VideoReader> reader = vio::VideoReader::Open(fileName);
    if (!reader) {
        cerr << "Error: failed to open video file at " << fileName << endl;
        exit(EXIT_FAILURE);
    }

    // 動画の情報を表示
    cout << "FileName: " << fileName << endl;
    cout << "FrameCount: " << reader->GetFrameCount() << endl;
    cout << "Width: " << reader->GetWidth() << endl;
    cout << "Height: " << reader->GetHeight() << endl;

    // 先頭の3フレームを8ビットグレー画像に変換してファイルに書き出す
    reader->SetOutputPixelFormat(PIX_FMT_GRAY8);    // 8ビットグレー画像で読み込む
    for (int i = 0; i < 3; ++i) {
        AVFrame* frame = reader->ReadNextFrame();   // 次のフレームを読み込む
        if (!frame) break;
        char filename[256];
        sprintf(filename, "frame-%d.pgm", i);
        WriteFramePGM(filename, frame, reader->GetWidth(), reader->GetHeight());
    }

    // 最後の3フレームを24ビットRGB画像に変換してファイルに書き出す
    reader->Seek(reader->GetFrameCount() - 4);      // 最後から3フレーム前に移動する
    reader->SetOutputPixelFormat(PIX_FMT_RGB24);    // 24ビットRGB画像で読み込む
    for (int i = 0; i < 3; ++i) {
        AVFrame* frame = reader->ReadNextFrame();
        if (!frame) break;
        char filename[256];
        sprintf(filename, "frame-%d.ppm", i);
        WriteFramePPM(filename, frame, reader->GetWidth(), reader->GetHeight());
    }

    // 動画を読み込んで別のファイルにそのまま出力する
    std::unique_ptr<vio::VideoWriter> writer(
        vio::VideoWriter::Open("out.avi",
                               reader->GetWidth(), reader->GetHeight(),
                               AV_PIX_FMT_BGR24));      // BGRにしないとRとBがひっくり返って出力される
    if (!writer) {
        cerr << "failed to open writer" << endl;
        exit(EXIT_FAILURE);
    }
    reader->Seek(0);
    reader->SetOutputPixelFormat(AV_PIX_FMT_BGR24);     // writerの設定に合わせる
    while (AVFrame* frame = reader->ReadNextFrame()) {
        Negate(frame, reader->GetWidth(), reader->GetHeight());     // 色反転
        writer->WriteFrame(frame);
    }
    writer->Close();    // ここでCloseしなくてもデストラクタでCloseされる．

    return EXIT_SUCCESS;
}
