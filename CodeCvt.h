#ifndef DYNAMICWALLPAPER_CODECVT_H
#define DYNAMICWALLPAPER_CODECVT_H

#include <memory>
#include <string>
#include <windows.h>

namespace CodeCvt {
    using std::string;
    using std::wstring;
    using std::unique_ptr;

    string WstrToStr(const wstring &Src, UINT CodePage) {
        if (Src.empty()) return {};
        int len = WideCharToMultiByte(CodePage,
                                      0,
                                      Src.c_str(),
                                      (int) Src.length(),
                                      nullptr,
                                      0,
                                      nullptr,
                                      nullptr
        );
        if (len <= 0) throw std::runtime_error("len <= 0");
        string szResult(len, 0);
        WideCharToMultiByte(CodePage,
                            0,
                            Src.c_str(),
                            (int) Src.length(),
                            &szResult.at(0),
                            len,
                            nullptr,
                            nullptr
        );
        return szResult;
    }

    wstring StrToWstr(const string &Src, UINT CodePage) {
        if (Src.empty()) return {};
        int len = MultiByteToWideChar(CodePage,
                                      0,
                                      Src.c_str(),
                                      (int) Src.length(),
                                      nullptr,
                                      0
        );
        if (len <= 0) throw std::runtime_error("len <= 0");
        wstring wszResult(len, 0);
        MultiByteToWideChar(CodePage,
                            0,
                            Src.c_str(),
                            (int) Src.length(),
                            &wszResult.at(0),
                            len
        );
        return wszResult;
    }

    unique_ptr<char[]> WstrToStr(wchar_t *Src, UINT CodePage) {
        int len = WideCharToMultiByte(CodePage,
                                      0,
                                      Src,
                                      -1,
                                      nullptr,
                                      0,
                                      nullptr,
                                      nullptr
        );
        if (len <= 0) throw std::runtime_error("len <= 0");
        unique_ptr<char[]> Dst = std::make_unique<char[]>(len * sizeof(char));
        WideCharToMultiByte(CodePage,
                            0,
                            Src,
                            -1,
                            Dst.get(),
                            len,
                            nullptr,
                            nullptr
        );
        return Dst;
    }

    unique_ptr<wchar_t[]> StrToWstr(char *Src, UINT CodePage) {
        int len = MultiByteToWideChar(CodePage,
                                      0,
                                      Src,
                                      -1,
                                      nullptr,
                                      0
        );
        if (len <= 0) throw std::runtime_error("len <= 0");
        unique_ptr<wchar_t[]> Dst = std::make_unique<wchar_t[]>(len * sizeof(wchar_t));
        MultiByteToWideChar(CodePage,
                            0,
                            Src,
                            -1,
                            Dst.get(),
                            len
        );
        return Dst;
    }
}

#endif //DYNAMICWALLPAPER_CODECVT_H
