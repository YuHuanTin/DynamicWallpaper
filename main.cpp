#include <iostream>
#include <thread>
#include <vector>
#include <winsock2.h>
#include <windows.h>
#include <event.h>
#include <event2/http.h>
#include <vlc/vlc.h>
#include "CodeCvt.h"

// ����ͼ��͸����
double gIconAttributes = 100;

class VlcListPlayer {
private:
    libvlc_instance_t          *inst            = nullptr;
    libvlc_media_list_t        *mediaList       = nullptr;
    libvlc_media_list_player_t *mediaListPlayer = nullptr;
    libvlc_media_player_t      *mediaPlayer     = nullptr;
public:
    VlcListPlayer() {
        inst            = libvlc_new(0, nullptr);
        mediaListPlayer = libvlc_media_list_player_new(inst);
        mediaPlayer     = libvlc_media_player_new(inst);
    }

    bool addMedia(const std::string &mrl) {
        if (mediaList == nullptr) {
            mediaList = libvlc_media_list_new(inst);
        }
        libvlc_media_t *media = libvlc_media_new_location(inst, mrl.c_str());
        if (media == nullptr) {
            throw std::runtime_error("ý�����");
        }
        if (libvlc_media_list_add_media(mediaList, media) == -1) {
            libvlc_media_release(media);
            return false;
        }
        libvlc_media_release(media);
        return true;
    }

    bool play(HWND videoHwnd = nullptr) {
        if (mediaPlayer == nullptr || mediaListPlayer == nullptr)
            return false;

        // ����ý�岥�����󶨵Ĵ��ھ��
        libvlc_media_player_set_hwnd(mediaPlayer, videoHwnd);
        libvlc_media_list_player_set_media_player(mediaListPlayer, mediaPlayer);
        libvlc_media_list_player_set_playback_mode(mediaListPlayer, libvlc_playback_mode_loop);

        // ����
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

class StreamManage {
private:
    struct ManageT {
        std::vector<uint8_t> *ptrFile;
        std::string          fileName;
        uint64_t             hash;
    };
    std::vector<ManageT> vManageT;
public:
    void removeAllFile() {
        for (auto &i: vManageT)
            delete i.ptrFile;
    }

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
            throw std::runtime_error("���Ϸ���·��");
        if (filePath.rfind('.') == std::string::npos)
            fileName = filePath.substr(pos + 1, std::string::npos);
        else
            fileName = filePath.substr(pos + 1, filePath.rfind('.') - pos - 1);

        // read file to memory
        FILE *fp;
        if (fopen_s(&fp, filePath.c_str(), "rb") != 0) {
            throw std::runtime_error("���ļ�ʧ��");
        }
        fseek(fp, 0, SEEK_END);
        size_t fileSize = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        auto *ptrData = new std::vector<uint8_t>;
        ptrData->resize(fileSize);
        fread(ptrData->data(), 1, fileSize, fp);
        fclose(fp);

        vManageT.push_back({ptrData, fileName, std::hash<std::string>{}(fileName)});
    }

    // ��ȡ�ļ���mrl
    // ����http://ip:port/0000.xxx
    std::vector<std::string> requestMrl(short port) {
        std::vector<std::string> ret;
        for (auto                &i: vManageT) {
            ret.push_back({"http://127.0.0.1:" + std::to_string(port) + "/" + std::to_string(i.hash)});

        }
        return ret;
    }

    // ����ý������
    std::vector<uint8_t> *requestData(const std::string &request) {
        for (const auto &i: vManageT) {
            if (std::to_string(i.hash) == request)
                return i.ptrFile;
        }
        return nullptr;
    }
};

void evhttp_cb(evhttp_request *request, void *userArgs) {
    // ��ȡ��������
    const char *url = evhttp_request_get_uri(request);

    // ������Ӧͷ
    evkeyvalq *outputHeaders = evhttp_request_get_output_headers(request);
    evhttp_add_header(outputHeaders, "Content-Type", "video/mp4");

    // ������Ӧ����
    evbuffer *outputBuffer = evhttp_request_get_output_buffer(request);

    std::string file = url;
    file = file.substr(1, std::string::npos);

    StreamManage streamManage = *(StreamManage *) userArgs;
    auto         pData        = streamManage.requestData(file);
    if (pData != nullptr)
        evbuffer_add_reference(outputBuffer, pData->data(), pData->size(), nullptr, nullptr);

    evhttp_send_reply(request, HTTP_OK, "", outputBuffer);
}

void http(short port, StreamManage &streamManage) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    event_base *eventBase = event_base_new();
    evhttp *evHttp = evhttp_new(eventBase);
    if (evhttp_bind_socket(evHttp, "127.0.0.1", port) != 0) {
        std::cout << "evhttp_bind_socket failed\n";
        evhttp_free(evHttp);
        event_base_free(eventBase);
        return;
    }
    //�趨�ص�����
    evhttp_set_gencb(evHttp, evhttp_cb, &streamManage);
    event_base_dispatch(eventBase);
    evhttp_free(evHttp);
    event_base_free(eventBase);
}

BOOL enumWindowsProc(HWND hwnd, LPARAM lparam) {
    HWND hDefView = FindWindowEx(hwnd, nullptr, "SHELLDLL_DefView", nullptr);
    if (hDefView != nullptr) {
        HWND hWorkerw = FindWindowEx(nullptr, hwnd, "WorkerW", nullptr);

        // ��������ͼ��͸����
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
    short        httpPort = 233;
    HWND hSecondWorker = nullptr;

    // ��ȡ�û�����Ĳ���
    {
        std::cout << "������Ҫ���ŵı�ֽ·��:\n";
        std::cin >> s;
        streamManage.add(s);
    }

    // �Ƿ���Ҫ����ͼ��͸����
    {
        std::cout << "�Ƿ���Ҫ����ͼ��͸����(�����Ҫ����������0-100��ֵ):";
        std::cin >> s;
        long value = strtol(s.c_str(), nullptr, 10);
        if (0 <= value && value <= 100) {
            gIconAttributes = value;
            std::cout << "͸����������Ϊ: " << gIconAttributes << '\n';
        }
    }

    // ����http������
    {
        std::thread httpServ{http, httpPort, std::ref(streamManage)};
        httpServ.detach();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // ���ñ�ֽ
    {
        HWND hProgman = FindWindow("Progman", nullptr);
        SendMessageTimeout(hProgman, 0x52C, 0, 0, 0, 10000, nullptr);
        EnumWindows(enumWindowsProc, (LPARAM) &hSecondWorker);
    }

    // ����������
    {
        VlcListPlayer            player;
        std::vector<std::string> mediaList = streamManage.requestMrl(httpPort);
        for (const auto          &i: mediaList) {
            std::cout << "List: " << i << '\n';
            if (!player.addMedia(i))
                std::cout << "add failed";
        }
        player.play(hSecondWorker);
        Sleep(INFINITE);
    }
    streamManage.removeAllFile();

    return 0;
}