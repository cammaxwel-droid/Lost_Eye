#define UNICODE
#define _UNICODE
#define NOMINMAX
#include "FolderVideos.h"
#include <fstream>
#include <iostream>

namespace fs=std::filesystem;
int wmain() {
    int failures=0,checks=0,skips=0;
    auto check=[&](const char* name,bool passed){++checks;if(!passed)++failures;std::cout<<(passed?"PASS ":"FAIL ")<<name<<'\n';};
    auto skip=[&](const char* name){++skips;std::cout<<"SKIP "<<name<<" (Windows did not allow creating the test link)\n";};
    try {
        wchar_t temp[32768]{};const DWORD size=GetTempPathW(static_cast<DWORD>(std::size(temp)),temp);
        if(!size||size>=std::size(temp))throw std::runtime_error("Cannot find temporary directory");
        const auto root=fs::path(temp)/(L"LostEye Folder Tests "+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));
        if(!fs::create_directory(root))throw std::runtime_error("Temporary test directory already exists");
        const auto folder=root/L"opened folder",sibling=root/L"sibling",empty=root/L"empty",single=root/L"single";
        fs::create_directory(folder);fs::create_directory(sibling);fs::create_directory(empty);fs::create_directory(single);
        auto touch=[](const fs::path& path){std::ofstream out(path,std::ios::binary);out<<"test fixture, not a playable video";if(!out)throw std::runtime_error("Cannot create test fixture");};
        for(const auto* name:{L"clip1.MP4",L"clip2.mp4",L"clip10.mp4",L"clip20.MKV",L"clip30.webm",L"clip3.txt"})touch(folder/name);
        touch(root/L"clip11.mp4");touch(sibling/L"clip11.mp4");touch(single/L"only.mp4");
        fs::create_directory(folder/L"clip4.mp4");touch(folder/L"clip4.mp4"/L"clip5.mp4");
        fs::create_directory(folder/L"child");touch(folder/L"child"/L"only.mp4");
        auto adjacent=[&](const fs::path& file,int direction){return LostEyeFiles::AdjacentVideo(file.wstring(),direction);};
        auto is=[&](const fs::path& file,int direction,const fs::path& expected){const auto result=adjacent(file,direction);return result.error.empty()&&result.path==expected.wstring();};
        auto boundary=[&](const fs::path& file,int direction){const auto result=adjacent(file,direction);return result.path.empty()&&result.error.empty();};
        check("Next uses natural filename order",is(folder/L"clip1.MP4",1,folder/L"clip2.mp4"));
        check("Previous opens preceding video",is(folder/L"clip2.mp4",-1,folder/L"clip1.MP4"));
        check("Next skips non-video files and nested folders",is(folder/L"clip2.mp4",1,folder/L"clip10.mp4"));
        check("Next never enters parent or sibling folders",is(folder/L"clip10.mp4",1,folder/L"clip20.MKV"));
        check("Video extensions are case insensitive",is(folder/L"clip20.MKV",1,folder/L"clip30.webm"));
        check("First video does not wrap backwards",boundary(folder/L"clip1.MP4",-1));
        check("Last video does not wrap forwards",boundary(folder/L"clip30.webm",1));
        check("A different-case current path matches existing file",is(folder/L"CLIP2.MP4",1,folder/L"clip10.mp4"));
        check("Deleted current file retains forward filename anchor",is(folder/L"clip3.mp4",1,folder/L"clip10.mp4"));
        check("Deleted current file retains backward filename anchor",is(folder/L"clip3.mp4",-1,folder/L"clip2.mp4"));
        check("Empty folder returns a boundary without failure",boundary(empty/L"missing.mp4",1));
        check("Single-video folder stays at its end",boundary(single/L"only.mp4",1));
        check("Single-video folder stays at its beginning",boundary(single/L"only.mp4",-1));
        check("Explicitly opened child folder never moves up",boundary(folder/L"child"/L"only.mp4",-1));
        check("Missing folder reports a readable error",!adjacent(root/L"missing"/L"video.mp4",1).error.empty());
        check("Invalid navigation direction reports an error",!adjacent(folder/L"clip2.mp4",0).error.empty());
        check("Empty current path reports an error",!LostEyeFiles::AdjacentVideo(L"",1).error.empty());
        touch(folder/L"clip40_日本.mp4");
        check("Unicode filenames are returned intact",is(folder/L"clip30.webm",1,folder/L"clip40_日本.mp4"));
        const auto fileLink=folder/L"clip3.mp4";
        if(CreateSymbolicLinkW(fileLink.c_str(),(sibling/L"clip11.mp4").c_str(),SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE)) {
            check("Linked video files outside the folder are excluded",is(folder/L"clip2.mp4",1,folder/L"clip10.mp4"));
        } else skip("Linked video files outside the folder are excluded");
        const auto directoryLink=folder/L"clip12.mp4",alias=root/L"linked folder";
        if(CreateSymbolicLinkW(directoryLink.c_str(),sibling.c_str(),SYMBOLIC_LINK_FLAG_DIRECTORY|SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE)) {
            check("Linked directories are never enumerated",is(folder/L"clip10.mp4",1,folder/L"clip20.MKV"));
        } else skip("Linked directories are never enumerated");
        if(CreateSymbolicLinkW(alias.c_str(),folder.c_str(),SYMBOLIC_LINK_FLAG_DIRECTORY|SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE)) {
            check("Navigation refuses a linked parent directory",!adjacent(alias/L"clip2.mp4",1).error.empty());
            check("Navigation refuses a linked ancestor directory",!adjacent(alias/L"child"/L"only.mp4",1).error.empty());
        } else {skip("Navigation refuses a linked parent directory");skip("Navigation refuses a linked ancestor directory");}
        std::wcout<<L"Fixtures retained at "<<root.wstring()<<L'\n';
    }catch(const std::exception& error) {check("Test setup/execution",false);std::cerr<<error.what()<<'\n';}
    std::cout<<"CHECKS "<<checks<<"\nSKIPS "<<skips<<"\nFAILURES "<<failures<<'\n';
    return failures?1:0;
}
