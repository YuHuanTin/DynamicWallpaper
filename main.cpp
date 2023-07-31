#include <iostream>
#include <vector>
#include <windows.h>
#include <fmt/format.h>
#include <SDL2/SDL.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#undef main

size_t g_iconAttributes = 100;

BOOL enumWindowsProc(HWND hwnd, LPARAM lparam) {
    HWND hDefView = FindWindowEx(hwnd, nullptr, "SHELLDLL_DefView", nullptr);
    if (hDefView != nullptr) {
        HWND hWorkerw = FindWindowEx(nullptr, hwnd, "WorkerW", nullptr);

        // 设置桌面图标透明度
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TOOLWINDOW);
        SetLayeredWindowAttributes(hwnd, 0, (int) (g_iconAttributes / 100 * 255), LWA_ALPHA);
        //UpdateLayeredWindow(hwnd, nullptr, nullptr, nullptr, nullptr, nullptr, 0, nullptr, 0);

        HWND *p = (HWND *) lparam;
        *p = hWorkerw;
        return FALSE;
    }
    return TRUE;
}

class SDL_Windows {
private:

public:
};

class FFMEPG_Player {
private:
    AVFormatContext *m_avFormatContext = nullptr;
    AVCodecContext  *m_avCodecContext  = nullptr;
    int             m_videoIndex       = -1;
    int             m_audioIndex       = -1;

public:
    bool openFromPath(const std::string &FilePath) {
        // 分配上下文
        m_avFormatContext = avformat_alloc_context();
        if (!m_avFormatContext) return false;

        // open file from disk
        int result = avformat_open_input(&m_avFormatContext, FilePath.data(), nullptr, nullptr);
        if (result) {
            fmt::println("failed avformat_open_input {}", result);
            return false;
        }

        // 读取音视频包，获取解码器相关的信息
        result = avformat_find_stream_info(m_avFormatContext, nullptr);
        if (result < 0) {
            fmt::println("failed avformat_find_stream_info {}", result);
            return false;
        }
        return true;
    }

    // 查找音视频流 && 打开视频解码器
    bool splitAVStream() {
        // 查找音视频流
        m_videoIndex = av_find_best_stream(m_avFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        m_audioIndex = av_find_best_stream(m_avFormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        if (m_videoIndex < 0 || m_audioIndex < 0) {
            fmt::println("m_videoIndex: {}\nm_audioIndex: {}", m_videoIndex, m_audioIndex);
            return false;
        }

        // 申请视频解码器
        m_avCodecContext = avcodec_alloc_context3(nullptr);
        if (!m_avCodecContext) {
            fmt::println("failed avcodec_alloc_context3");
            return false;
        }

        // 拷贝解码器参数
        int result = avcodec_parameters_to_context(m_avCodecContext, m_avFormatContext->streams[m_videoIndex]->codecpar);
        if (result < 0) {
            fmt::println("failed avcodec_parameters_to_context {}", result);
            return false;
        }

        // 查找解码器
        const AVCodec *avCodec = avcodec_find_decoder(m_avCodecContext->codec_id);
        if (!avCodec) {
            fmt::println("failed avcodec_find_decoder {}", (int) m_avCodecContext->codec_id);
            return false;
        }

        // 打开解码器，将解码器和解码器上下文关联
        result = avcodec_open2(m_avCodecContext, avCodec, nullptr);
        if (result < 0) {
            fmt::println("failed avcodec_open2 {}", result);
            return false;
        }
#ifdef _DEBUG
        av_dump_format(m_avFormatContext, 0, m_avFormatContext->url, 0);
#endif
        return true;
    }

    bool loopPlay() {
        AVPacket *packet = av_packet_alloc();

        int frameCount = 0;
        while (1) {
            ++frameCount;
            int result = av_read_frame(m_avFormatContext, packet);
            if (result) {
                fmt::println("failed av_read_frame {} and frameCount: {}", result, frameCount);
                break;
            }
            if (packet->stream_index == m_videoIndex) {
                fmt::println("video stream");



            } else if (packet->stream_index == m_audioIndex) {
                fmt::println("audio stream");
            } else {
                fmt::println("unknow stream");
            }

            // 释放包的内存
            av_packet_unref(packet);
        }
        return true;
    }

    ~FFMEPG_Player() {
        if (m_avCodecContext) {
            avcodec_free_context(&m_avCodecContext);
        }
        if (m_avFormatContext) {
            avformat_free_context(m_avFormatContext);
        }
    }
};

int main() {


    FFMEPG_Player player;
    player.openFromPath("D:\\OBS\\Video\\2022-01-28_13-04-24.mp4");
    player.splitAVStream();
//    player.loopPlay();

    return 0;
}
