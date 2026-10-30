#pragma once
#include <windows.h>
#include <shlwapi.h>
#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace LostEyeFiles {
struct AdjacentResult {
    std::wstring path;
    std::wstring error;
};

namespace Detail {
inline bool VideoExtension(const std::filesystem::path& file) {
    const auto extension=file.extension().wstring();
    for(const auto* known:{L".mp4",L".m4v",L".mov",L".mkv",L".webm",L".avi",L".wmv",L".mpg",L".mpeg",L".ts",L".m2ts",L".3gp"})
        if(_wcsicmp(extension.c_str(),known)==0)return true;
    return false;
}

inline int CompareNames(const std::wstring& left,const std::wstring& right) {
    const int natural=StrCmpLogicalW(left.c_str(),right.c_str());
    if(natural)return natural;
    // Natural sorting can consider different spellings equal. Keep the order stable.
    const int folded=CompareStringOrdinal(left.c_str(),-1,right.c_str(),-1,TRUE);
    if(folded!=CSTR_EQUAL)return folded==CSTR_LESS_THAN?-1:1;
    const int exact=CompareStringOrdinal(left.c_str(),-1,right.c_str(),-1,FALSE);
    return exact==CSTR_EQUAL?0:exact==CSTR_LESS_THAN?-1:1;
}

inline bool PlainDirectory(std::filesystem::path directory) {
    // Do not navigate through a linked directory, including an ancestor junction.
    while(!directory.empty()) {
        const DWORD attributes=GetFileAttributesW(directory.c_str());
        if(attributes==INVALID_FILE_ATTRIBUTES||!(attributes&FILE_ATTRIBUTE_DIRECTORY)||(attributes&FILE_ATTRIBUTE_REPARSE_POINT))return false;
        const auto parent=directory.parent_path();
        // A UNC server root is not itself a directory; the share was checked above.
        if(parent==directory||parent==directory.root_path())break;
        directory=parent;
    }
    return true;
}

struct FindHandle {
    HANDLE value=INVALID_HANDLE_VALUE;
    ~FindHandle(){if(value!=INVALID_HANDLE_VALUE)FindClose(value);}
};
}

// The only search scope is the current path's parent. No recursion or wrapping.
// An empty path with no error means the first/last video has been reached.
inline AdjacentResult AdjacentVideo(const std::wstring& current,int direction) {
    try {
        if(current.empty()||(direction!=-1&&direction!=1))return {{},L"Choose a video before navigating its folder."};
        std::error_code error;
        const auto file=std::filesystem::absolute(std::filesystem::path(current),error).lexically_normal();
        if(error||!file.has_filename())return {{},L"This video's folder path is invalid."};
        const auto folder=file.parent_path();
        if(folder.empty()||!Detail::PlainDirectory(folder))return {{},L"This video's folder is unavailable or contains a directory link. Open a video directly from its original folder."};

        WIN32_FIND_DATAW entry{};
        Detail::FindHandle search;
        search.value=FindFirstFileExW((folder/L"*").c_str(),FindExInfoBasic,&entry,FindExSearchNameMatch,nullptr,0);
        if(search.value==INVALID_HANDLE_VALUE) {
            if(GetLastError()==ERROR_FILE_NOT_FOUND)return {};
            return {{},L"Windows couldn't read this video's folder."};
        }
        std::vector<std::wstring> names;
        do {
            if(!(entry.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))&&Detail::VideoExtension(entry.cFileName))names.emplace_back(entry.cFileName);
        }while(FindNextFileW(search.value,&entry));
        if(GetLastError()!=ERROR_NO_MORE_FILES)return {{},L"Windows couldn't finish reading this video's folder."};
        std::sort(names.begin(),names.end(),[](const auto& a,const auto& b){return Detail::CompareNames(a,b)<0;});

        const auto currentName=file.filename().wstring();
        auto found=std::find(names.begin(),names.end(),currentName);
        if(found==names.end())found=std::find_if(names.begin(),names.end(),[&](const auto& name){return CompareStringOrdinal(name.c_str(),-1,currentName.c_str(),-1,TRUE)==CSTR_EQUAL;});
        std::wstring selected;
        if(found!=names.end()) {
            if(direction>0) {if(++found!=names.end())selected=*found;}
            else if(found!=names.begin())selected=*--found;
        } else {
            // A removed current file still anchors navigation by its filename.
            for(const auto& name:names) {
                const int order=Detail::CompareNames(name,currentName);
                if(direction>0&&order>0){selected=name;break;}
                if(direction<0&&order<0)selected=name;
            }
        }
        if(selected.empty())return {};
        const auto next=folder/selected;
        const DWORD attributes=GetFileAttributesW(next.c_str());
        if(attributes==INVALID_FILE_ATTRIBUTES||(attributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)))return {{},L"The folder changed while choosing the next video. Try again."};
        return {next.wstring(),{}};
    }catch(...) {
        return {{},L"Windows couldn't read this video's folder."};
    }
}
}
