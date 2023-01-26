#include <vlc/vlc.h>
#include <winsock2.h>
#include <event.h>
#include <event2/http.h>
#include <iostream>
#include <thread>
#include "windows.h"
#include "CodeCvt.h"

void player(const std::string &filePath) {
    libvlc_instance_t     *inst = nullptr;
    libvlc_media_player_t *mp   = nullptr;
    libvlc_media_t        *m    = nullptr;

    libvlc_time_t length;
    int           width;
    int           height;

    inst = libvlc_new(0, nullptr);

    // 从url创建播放媒体
    m = libvlc_media_new_location(inst, filePath.c_str());

    // 设置循环播放
    libvlc_media_add_option(m, "--loop");
    libvlc_media_add_option(m, "--repeat");
    libvlc_media_add_option(m, "--advanced");

    /* Create a media player playing environement */
    mp = libvlc_media_player_new_from_media(m);

    /* No need to keep the media now */
    libvlc_media_release(m);

    // play the media_player
    libvlc_media_player_play(mp);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    //wait until the tracks are created
    length = libvlc_media_player_get_length(mp);
    width  = libvlc_video_get_width(mp);
    height = libvlc_video_get_height(mp);
    printf("Stream Duration: %ds\n", length / 1000);
    printf("Resolution: %d x %d\n", width, height);

    //Let it play
    std::this_thread::sleep_for(std::chrono::seconds(length - 1));

    // Stop playing
    libvlc_media_player_stop(mp);

    // Free the media_player
    libvlc_media_player_release(mp);

    libvlc_release(inst);
}
void http_cb(evhttp_request *request, void *userArgs) {
    //获取请求内容
    const char *url = evhttp_request_get_uri(request);
    printf("[+]url:%s\n", url);
    //获取请求类型
    std::string requestType;
    switch (evhttp_request_get_command(request)) {
        case EVHTTP_REQ_GET: {
            requestType = "GET";
            break;
        }
        case EVHTTP_REQ_POST: {
            requestType = "POST";
            break;
        }
        default:
            break;
    }

    printf("[+]request method:%s\n", requestType.c_str());
    //获取消息报头
    evkeyvalq     *headers = evhttp_request_get_input_headers(request);
    for (evkeyval *p       = headers->tqh_first; p != nullptr; p = p->next.tqe_next) {
        printf("[+]%s:%s\n", p->key, p->value);
    }
    //获取请求正文,GET为空,POST有表单信息
    evbuffer      *inputBuffer                                   = evhttp_request_get_input_buffer(request);
    char          buf[4096]{0};
    while (evbuffer_get_length(inputBuffer) > 0) {
        int n = evbuffer_remove(inputBuffer, buf, sizeof(buf) - 1);
        if (n > 0) {
            printf("[+]payload:%s\n", buf);
        }
    }

    //响应部分
    evkeyvalq   *outHeader = evhttp_request_get_output_headers(request);
    std::string path       = url;

    size_t pos = path.rfind('.');
    if (pos != std::string::npos) {
        std::string prefix = path.substr(pos + 1, path.length() - (pos + 1));
        if (prefix == "jpg" || prefix == "gif" || prefix == "png") {
            std::string tmp = "image/" + prefix;
            //添加协议头
            evhttp_add_header(outHeader, "Content-Type", tmp.c_str());
        } else if (prefix == "zip") {
            evhttp_add_header(outHeader, "Content-Type", "application/zip");
        } else if (prefix == "html") {
            evhttp_add_header(outHeader, "Content-Type", "text/html;charset=UTF8");
        } else if (prefix == "mp4") {
            evhttp_add_header(outHeader, "Content-Type", "video/mp4");
        }
    }

    //响应正文
    evbuffer *outputBuffer = evhttp_request_get_output_buffer(request);

    FILE *fp;
    fopen_s(&fp, "D:\\Steam\\steamapps\\workshop\\content\\431960\\2248135562\\开机待机动态.mp4", "rb");
    size_t bytesRead;
    while ((bytesRead = fread(buf, 1, sizeof(buf), fp)) > 0) {
        evbuffer_add(outputBuffer, buf, 4096);
    }
    fclose(fp);

    //evbuffer_add(outputBuffer,str, sizeof(str));
    evhttp_send_reply(request, HTTP_OK, "", outputBuffer);
}
void http() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    event_base *eventBase = event_base_new();
    //创建evhttp上下文
    evhttp     *evHttp    = evhttp_new(eventBase);
    //绑定端口和ip,0.0.0.0表示绑定任意网卡
    if (evhttp_bind_socket(evHttp, "0.0.0.0", 233) != 0) {
        //失败
    }
    //设定回调函数
    evhttp_set_gencb(evHttp, http_cb, nullptr);

    event_base_dispatch(eventBase);
    evhttp_free(evHttp);
    event_base_free(eventBase);
}
int main() {
    setbuf(stdout, nullptr);

    std::thread httpServ{http};
    httpServ.detach();

    std::cout << "evhttp init success\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::thread playerThread{player, "file:///D:/Steam/steamapps/workshop/content/431960/2248135562/%E5%BC%80%E6%9C%BA%E5%BE%85%E6%9C%BA%E5%8A%A8%E6%80%81.mp4"};

    playerThread.detach();
    std::this_thread::sleep_for(std::chrono::minutes(60));

    return 0;
}