#include <iostream>
#include <vector>
#include <windows.h>
#include <vlc/vlc.h>
#include "CodeCvt.h"

// 桌面图标透明度
double gIconAttributes = 100;

struct StreamManageT {
    std::unique_ptr<uint8_t[]> ptrFile;
    std::string                fileName;
    uint64_t                   fileSize = 0;
    uint64_t                   filePos  = 0;
};

class StreamManage {
private:
    std::vector<StreamManageT *> vStreamManageT;
public:
    void add(const std::string &rawFilePath) {
        std::string filePath = CodeCvt::WstrToStr(CodeCvt::StrToWstr(rawFilePath, CP_ACP), CP_ACP);
        std::cout << filePath << '\n';
        if (filePath.back() == '"')
            filePath.erase(filePath.end() - 1);
        if (filePath.front() == '"')
            filePath = filePath.substr(1, std::string::npos);

        std::string fileName;
        size_t      pos = 0;
        if ((pos = filePath.rfind('/')) == std::string::npos && (pos = filePath.rfind('\\')) == std::string::npos)
            throw std::runtime_error("illegal path");
        if (filePath.rfind('.') == std::string::npos)
            fileName = filePath.substr(pos + 1, std::string::npos);
        else
            fileName = filePath.substr(pos + 1, filePath.rfind('.') - pos - 1);

        // read file to memory
        std::FILE *fp = nullptr;
        if (fopen_s(&fp, filePath.c_str(), "rb") != 0 || fp == nullptr)
            throw std::runtime_error("failed open file");
        fseek(fp, 0, SEEK_END);
        auto fileSize = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        auto item = new StreamManageT;
        item->ptrFile  = std::make_unique<uint8_t[]>(fileSize);
        item->fileName = fileName;
        item->fileSize = fileSize;
        item->filePos  = 0;
        fread(item->ptrFile.get(), 1, fileSize, fp);
        fclose(fp);

        vStreamManageT.push_back(item);
    }

    void release() {
        for (auto &one: vStreamManageT)
            delete one;
    }

    std::vector<StreamManageT *> &getReference() {
        return vStreamManageT;
    }
};

class VlcListPlayer {
private:
    libvlc_instance_t          *inst            = nullptr;
    libvlc_media_list_t        *mediaList       = nullptr;
    libvlc_media_list_player_t *mediaListPlayer = nullptr;
    libvlc_media_player_t      *mediaPlayer     = nullptr;

    static int open_cb(void *opaque, void **datap, uint64_t *sizep) {
        auto *pStreamManageT = (StreamManageT *) opaque;
        *datap = pStreamManageT;
        *sizep = pStreamManageT->fileSize;
        return 0;
    }

    static ssize_t read_cb(void *opaque, unsigned char *buf, size_t len) {
        auto *pStreamManageT = (StreamManageT *) opaque;
        if (pStreamManageT->filePos + len == pStreamManageT->fileSize)
            return 0;
        else if (pStreamManageT->filePos + len > pStreamManageT->fileSize)
            return -1;

        memcpy(buf, pStreamManageT->ptrFile.get() + pStreamManageT->filePos, len);
        pStreamManageT->filePos += len;
        return len;
    }

    static int seek_cb(void *opaque, uint64_t offset) {
        auto *pStreamManageT = (StreamManageT *) opaque;
        pStreamManageT->filePos = offset;
        return 0;
    }

    static void close_cb(void *opaque) {
        auto *pStreamManageT = (StreamManageT *) opaque;
        pStreamManageT->filePos = 0;
    }

public:
    VlcListPlayer() {
        inst            = libvlc_new(0, nullptr);
        mediaListPlayer = libvlc_media_list_player_new(inst);
        mediaPlayer     = libvlc_media_player_new(inst);
    }

    bool addMedia(StreamManage &streamManage) {
        if (mediaList == nullptr) {
            mediaList = libvlc_media_list_new(inst);
        }
        for (auto &one: streamManage.getReference()) {
            libvlc_media_t *media = libvlc_media_new_callbacks(inst, open_cb, read_cb, seek_cb, close_cb, one);
            if (media == nullptr) throw std::runtime_error("failed load media");
            if (libvlc_media_list_add_media(mediaList, media) == -1) {
                libvlc_media_release(media);
                return false;
            }
            libvlc_media_release(media);
        }
        return true;
    }

    bool play(HWND videoHwnd = nullptr) {
        if (mediaPlayer == nullptr || mediaListPlayer == nullptr)
            return false;

        // 设置媒体播放器绑定的窗口句柄
        libvlc_media_player_set_hwnd(mediaPlayer, videoHwnd);
        libvlc_media_list_player_set_media_player(mediaListPlayer, mediaPlayer);
        libvlc_media_list_player_set_playback_mode(mediaListPlayer, libvlc_playback_mode_loop);

        // 播放
        libvlc_media_list_player_set_media_list(mediaListPlayer, mediaList);
        libvlc_media_list_player_play(mediaListPlayer);
        return true;
    }

    ~VlcListPlayer() {
        if (mediaPlayer != nullptr)
            libvlc_media_player_release(mediaPlayer);
        if (mediaListPlayer != nullptr)
            libvlc_media_list_player_release(mediaListPlayer);
        if (mediaList != nullptr)
            libvlc_media_list_release(mediaList);
        libvlc_release(inst);
    }
};

BOOL enumWindowsProc(HWND hwnd, LPARAM lparam) {
    HWND hDefView = FindWindowEx(hwnd, nullptr, "SHELLDLL_DefView", nullptr);
    if (hDefView != nullptr) {
        HWND hWorkerw = FindWindowEx(nullptr, hwnd, "WorkerW", nullptr);

        // 设置桌面图标透明度
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TOOLWINDOW);
        SetLayeredWindowAttributes(hwnd, 0, (int) (gIconAttributes / 100 * 255), LWA_ALPHA);
        //UpdateLayeredWindow(hwnd, nullptr, nullptr, nullptr, nullptr, nullptr, 0, nullptr, 0);

        HWND *p = (HWND *) lparam;
        *p = hWorkerw;
        return false;
    }
    return true;
}

int main() {
    setbuf(stdout, nullptr);

    StreamManage streamManage;
    std::string  s;
    HWND         hSecondWorker = nullptr;

    // 获取用户输入的参数
    {
        std::cout << "请输入要播放的壁纸路径:\n";
        std::cin >> s;
        streamManage.add(s);
    }

    // 是否需要更改图标透明度
    {
        std::cout << "是否需要更改图标透明度(如果需要更改请输入0-100的值):";
        std::cin >> s;
        long value = strtol(s.c_str(), nullptr, 10);
        if (0 <= value && value <= 100) {
            gIconAttributes = value;
            std::cout << "透明度已设置为: " << gIconAttributes << '\n';
        }
    }

    // 设置壁纸
    {
        HWND hProgman = FindWindow("Progman", nullptr);
        SendMessageTimeout(hProgman, 0x52C, 0, 0, 0, 10000, nullptr);
        EnumWindows(enumWindowsProc, (LPARAM) &hSecondWorker);
    }

    // 启动播放器
    {
        VlcListPlayer player;
        player.addMedia(streamManage);
        player.play(hSecondWorker);
        Sleep(INFINITE);
    }
    streamManage.release();
    return 0;
}