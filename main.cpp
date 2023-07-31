#include <iostream>
#include <vector>
#include <windows.h>
#include <vlc/vlc.h>
#include "CodeCvt.h"

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

int main() {



    return 0;
}