#define UNICODE
#define _UNICODE
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shobjidl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfmediaengine.h>
#include <mferror.h>
#include <d3d11.h>
#include <dwmapi.h>
#include <d3d10.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <propvarutil.h>
#include <atomic>
#include <thread>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <sstream>
#include "FolderVideos.h"
using Microsoft::WRL::ComPtr;
namespace fs = std::filesystem;
constexpr wchar_t WindowClass[] = L"LostEye.Native.0.3";
constexpr UINT WM_READY = WM_APP + 1, WM_MEDIA = WM_APP + 2, WM_START = WM_APP + 3;
enum ControlId { ID_OPEN=100, ID_BACK, ID_REWIND, ID_PLAY, ID_FORWARD, ID_SKIP, ID_LOOP, ID_MUTE, ID_FULL, ID_SEEK, ID_VOLUME };
static std::wstring ExePath() { wchar_t p[32768]{}; GetModuleFileNameW(nullptr,p,32768); return p; }
static void Check(HRESULT hr) { if(FAILED(hr)) throw hr; }
static std::wstring ErrorCode(HRESULT hr) { wchar_t b[32]{}; swprintf_s(b,L"0x%08X",static_cast<unsigned>(hr)); return b; }
static std::wstring TimeText(double seconds) {
    if(!std::isfinite(seconds)||seconds<0) seconds=0;
    const auto s=static_cast<unsigned long long>(seconds);
    wchar_t b[64]{};
    if(s>=3600) swprintf_s(b,L"%llu:%02llu:%02llu",s/3600,(s/60)%60,s%60);
    else swprintf_s(b,L"%llu:%02llu",s/60,s%60);
    return b;
}
struct Notify final : IMFMediaEngineNotify {
    std::atomic<ULONG> refs{1}; HWND window;
    explicit Notify(HWND h):window(h){}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out) override {
        if(!out) return E_POINTER; *out=nullptr;
        if(id==__uuidof(IUnknown)||id==__uuidof(IMFMediaEngineNotify)) { *out=this; AddRef(); return S_OK; } return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
    ULONG STDMETHODCALLTYPE Release() override { auto n=--refs; if(!n) delete this; return n; }
    HRESULT STDMETHODCALLTYPE EventNotify(DWORD event,DWORD_PTR p1,DWORD) override {
        // Never block a Media Foundation callback or call back into its engine here.
        if(event!=MF_MEDIA_ENGINE_EVENT_TIMEUPDATE) PostMessageW(window,WM_MEDIA,event,static_cast<LPARAM>(p1));
        return S_OK;
    }
};
struct Engine {
    ComPtr<ID3D11Device> device; ComPtr<ID3D11DeviceContext> context;
    ComPtr<IMFDXGIDeviceManager> manager; ComPtr<IDXGISwapChain1> swap;
    ComPtr<IMFMediaEngine> media; ComPtr<IMFMediaEngineEx> extended;
    bool started=false, hardware=false; std::wstring adapter; UINT width=1,height=1;
    unsigned long long frames=0; HRESULT lastRender=S_OK;
    ~Engine() { if(media) media->Shutdown(); extended.Reset(); media.Reset(); swap.Reset(); manager.Reset(); context.Reset(); device.Reset(); if(started) MFShutdown(); }
    void Initialize(HWND window,HWND video) {
        Check(MFStartup(MF_VERSION)); started=true;
        const UINT flags=D3D11_CREATE_DEVICE_VIDEO_SUPPORT|D3D11_CREATE_DEVICE_BGRA_SUPPORT;
        D3D_FEATURE_LEVEL level{};
        HRESULT hr=D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,flags,nullptr,0,D3D11_SDK_VERSION,&device,&level,&context);
        hardware=SUCCEEDED(hr);
        if(FAILED(hr)) Check(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,nullptr,0,D3D11_SDK_VERSION,&device,&level,&context));
        ComPtr<ID3D10Multithread> threading; Check(context.As(&threading)); threading->SetMultithreadProtected(TRUE);
        UINT token=0; Check(MFCreateDXGIDeviceManager(&token,&manager)); Check(manager->ResetDevice(device.Get(),token));
        ComPtr<IDXGIDevice> dxgi; Check(device.As(&dxgi)); ComPtr<IDXGIAdapter> gpu; Check(dxgi->GetAdapter(&gpu));
        DXGI_ADAPTER_DESC desc{}; gpu->GetDesc(&desc); adapter=desc.Description;
        ComPtr<IDXGIFactory2> factory; Check(gpu->GetParent(IID_PPV_ARGS(&factory)));
        RECT rect{}; GetClientRect(video,&rect); width=std::max(1L,rect.right); height=std::max(1L,rect.bottom);
        DXGI_SWAP_CHAIN_DESC1 spec{}; spec.Width=width; spec.Height=height; spec.Format=DXGI_FORMAT_B8G8R8A8_UNORM;
        spec.SampleDesc.Count=1; spec.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT; spec.BufferCount=2;
        spec.SwapEffect=DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; spec.Scaling=DXGI_SCALING_STRETCH; spec.AlphaMode=DXGI_ALPHA_MODE_IGNORE;
        Check(factory->CreateSwapChainForHwnd(device.Get(),video,&spec,nullptr,nullptr,&swap));
        factory->MakeWindowAssociation(window,DXGI_MWA_NO_ALT_ENTER);
        ComPtr<IMFAttributes> attributes; Check(MFCreateAttributes(&attributes,4));
        ComPtr<Notify> notify; notify.Attach(new Notify(window));
        Check(attributes->SetUnknown(MF_MEDIA_ENGINE_CALLBACK,notify.Get()));
        Check(attributes->SetUnknown(MF_MEDIA_ENGINE_DXGI_MANAGER,manager.Get()));
        Check(attributes->SetUINT32(MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT,DXGI_FORMAT_B8G8R8A8_UNORM));
        ComPtr<IMFMediaEngineClassFactory> mediaFactory;
        Check(CoCreateInstance(CLSID_MFMediaEngineClassFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&mediaFactory)));
        Check(mediaFactory->CreateInstance(0,attributes.Get(),&media)); Check(media.As(&extended));
        media->SetPreload(MF_MEDIA_ENGINE_PRELOAD_AUTOMATIC);
    }
    void Resize(UINT w,UINT h) {
        w=std::max(1u,w);h=std::max(1u,h);
        if(w==width&&h==height) return;
        context->OMSetRenderTargets(0,nullptr,nullptr); context->Flush();
        auto hr=swap->ResizeBuffers(0,w,h,DXGI_FORMAT_UNKNOWN,0);
        if(SUCCEEDED(hr)){width=w;height=h;} else lastRender=hr;
    }
    bool Render(bool force=false) {
        if(!media || !media->HasVideo()) return false;
        LONGLONG stamp=0; if(media->OnVideoStreamTick(&stamp)!=S_OK&&!force) return false;
        ComPtr<ID3D11Texture2D> texture; auto hr=swap->GetBuffer(0,IID_PPV_ARGS(&texture)); if(FAILED(hr)){lastRender=hr;return false;}
        DWORD vw=0,vh=0; media->GetVideoAspectRatio(&vw,&vh);
        RECT dst{0,0,static_cast<LONG>(width),static_cast<LONG>(height)};
        if(vw&&vh) {
            double ratio=static_cast<double>(vw)/vh;
            if(static_cast<double>(width)/height>ratio) { LONG w=static_cast<LONG>(height*ratio);dst.left=(static_cast<LONG>(width)-w)/2;dst.right=dst.left+w; }
            else { LONG h=static_cast<LONG>(width/ratio);dst.top=(static_cast<LONG>(height)-h)/2;dst.bottom=dst.top+h; }
        }
        ComPtr<ID3D11RenderTargetView> target; if(SUCCEEDED(device->CreateRenderTargetView(texture.Get(),nullptr,&target))) { const float black[4]={0,0,0,1};context->ClearRenderTargetView(target.Get(),black); }
        MFARGB border{0,0,0,255};
        hr=media->TransferVideoFrame(texture.Get(),nullptr,&dst,&border);
        if(SUCCEEDED(hr)) hr=swap->Present(1,0);
        lastRender=hr; if(SUCCEEDED(hr)){++frames;return true;} return false;
    }
#ifdef LOSTEYE_TEST
    bool FrameHasPixels() {
        ComPtr<ID3D11Texture2D> tex; if(FAILED(swap->GetBuffer(0,IID_PPV_ARGS(&tex)))) return false;
        // Explicitly transfer the current decoded frame, then read the GPU texture.
        RECT dst{0,0,static_cast<LONG>(width),static_cast<LONG>(height)}; MFARGB black{0,0,0,255};
        if(FAILED(media->TransferVideoFrame(tex.Get(),nullptr,&dst,&black))) return false;
        D3D11_TEXTURE2D_DESC d{};tex->GetDesc(&d);d.Usage=D3D11_USAGE_STAGING;d.BindFlags=0;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;d.MiscFlags=0;
        ComPtr<ID3D11Texture2D> copy;if(FAILED(device->CreateTexture2D(&d,nullptr,&copy))) return false;
        context->CopyResource(copy.Get(),tex.Get());D3D11_MAPPED_SUBRESOURCE map{};
        if(FAILED(context->Map(copy.Get(),0,D3D11_MAP_READ,0,&map))) return false;
        bool colored=false;for(UINT y=0;y<height;y+=16) for(UINT x=0;x<width;x+=16) {auto p=static_cast<BYTE*>(map.pData)+y*map.RowPitch+x*4;if(p[0]>24||p[1]>24||p[2]>24) colored=true;}
        context->Unmap(copy.Get(),0); return colored;
    }
#endif
};
struct Ready { std::mutex lock; bool alive=true; HRESULT hr=E_PENDING; std::unique_ptr<Engine> engine; };
static void SetRegistry(HKEY root,const std::wstring& key,const wchar_t* name,const std::wstring& value) {
    HKEY h{}; auto status=RegCreateKeyExW(root,key.c_str(),0,nullptr,0,KEY_SET_VALUE,nullptr,&h,nullptr);Check(HRESULT_FROM_WIN32(status));
    status=RegSetValueExW(h,name,0,REG_SZ,reinterpret_cast<const BYTE*>(value.c_str()),static_cast<DWORD>((value.size()+1)*sizeof(wchar_t)));RegCloseKey(h);Check(HRESULT_FROM_WIN32(status));
}
static const std::vector<std::wstring> Extensions={L".mp4",L".m4v",L".mov",L".mkv",L".webm",L".avi",L".wmv",L".mpg",L".mpeg",L".ts",L".m2ts",L".3gp"};
static void RegisterOpenWith(HKEY root,const std::wstring& exe) {
    const std::wstring app=L"Software\\Classes\\Applications\\Lost Eye.exe", prog=L"Software\\Classes\\LostEye.Video";
    const std::wstring cmd=L"\""+exe+L"\" \"%1\"", icon=L"\""+exe+L"\",0";
    SetRegistry(root,app,L"FriendlyAppName",L"Lost Eye"); SetRegistry(root,app,L"MultiSelectModel",L"Single");
    SetRegistry(root,app+L"\\shell\\open\\command",nullptr,cmd);SetRegistry(root,app+L"\\DefaultIcon",nullptr,icon);
    SetRegistry(root,prog,nullptr,L"Lost Eye video");SetRegistry(root,prog+L"\\shell\\open\\command",nullptr,cmd);SetRegistry(root,prog+L"\\DefaultIcon",nullptr,icon);
    const std::wstring caps=L"Software\\Lost Eye\\Capabilities";
    SetRegistry(root,caps,L"ApplicationName",L"Lost Eye");SetRegistry(root,caps,L"ApplicationDescription",L"A simple video player");SetRegistry(root,caps,L"ApplicationIcon",icon);
    for(const auto& ext:Extensions) {
        SetRegistry(root,app+L"\\SupportedTypes",ext.c_str(),L"");
        SetRegistry(root,caps+L"\\FileAssociations",ext.c_str(),L"LostEye.Video");
        HKEY key{};Check(HRESULT_FROM_WIN32(RegCreateKeyExW(root,(L"Software\\Classes\\"+ext+L"\\OpenWithProgids").c_str(),0,nullptr,0,KEY_SET_VALUE,nullptr,&key,nullptr)));
        auto status=RegSetValueExW(key,L"LostEye.Video",0,REG_NONE,nullptr,0);RegCloseKey(key);Check(HRESULT_FROM_WIN32(status));
    }
    SetRegistry(root,L"Software\\RegisteredApplications",L"Lost Eye",caps);
    SetRegistry(root,L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\Lost Eye.exe",nullptr,exe);
    // Never touch UserChoice or a file extension's default value.
}
static fs::path KnownFolder(REFKNOWNFOLDERID id) {
    PWSTR value=nullptr;Check(SHGetKnownFolderPath(id,0,nullptr,&value));fs::path result(value);CoTaskMemFree(value);return result;
}
static void WriteInstallResource(const fs::path& directory,const wchar_t* name,int id) {
    auto resource=FindResourceW(nullptr,MAKEINTRESOURCEW(id),RT_RCDATA);
    if(!resource)throw HRESULT_FROM_WIN32(GetLastError());
    auto size=SizeofResource(nullptr,resource);auto loaded=LoadResource(nullptr,resource);
    if(!loaded||!size)throw E_FAIL;
    auto data=static_cast<const char*>(LockResource(loaded));if(!data)throw E_FAIL;
    auto path=directory/name;
    HANDLE file=CreateFileW(path.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE){
        if(GetLastError()!=ERROR_FILE_EXISTS)throw HRESULT_FROM_WIN32(GetLastError());
        std::ifstream existing(path,std::ios::binary);
        if(!existing||fs::file_size(path)!=size||!std::equal(data,data+size,std::istreambuf_iterator<char>(existing)))
            throw HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
        return;
    }
    DWORD written=0;bool success=WriteFile(file,data,size,&written,nullptr)&&written==size;
    auto error=success?ERROR_SUCCESS:GetLastError();CloseHandle(file);
    if(!success){DeleteFileW(path.c_str());throw HRESULT_FROM_WIN32(error==ERROR_SUCCESS?ERROR_WRITE_FAULT:error);}
}
static fs::path InstallCurrentUser() {
    auto directory=KnownFolder(FOLDERID_LocalAppData)/L"Programs"/L"Lost Eye"/L"0.3.3";
    for(auto current=directory;!current.empty();){
        auto attributes=GetFileAttributesW(current.c_str());
        if(attributes!=INVALID_FILE_ATTRIBUTES&&(attributes&FILE_ATTRIBUTE_REPARSE_POINT))throw HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE);
        auto parent=current.parent_path();if(parent==current)break;current=parent;
    }
    fs::create_directories(directory);auto destination=directory/L"Lost Eye.exe";auto source=fs::path(ExePath());
    if(fs::exists(destination)) {
        if(!fs::equivalent(source,destination)) {
            std::ifstream a(source,std::ios::binary),b(destination,std::ios::binary);
            if(fs::file_size(source)!=fs::file_size(destination)||!std::equal(std::istreambuf_iterator<char>(a),std::istreambuf_iterator<char>(),std::istreambuf_iterator<char>(b))) throw HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
        }
    } else if(!CopyFileW(source.c_str(),destination.c_str(),TRUE)) throw HRESULT_FROM_WIN32(GetLastError());
    WriteInstallResource(directory,L"Uninstaller.cmd",101);
    WriteInstallResource(directory,L"Uninstall-LostEye.ps1",102);
    RegisterOpenWith(HKEY_CURRENT_USER,destination.wstring());
    auto shortcut=KnownFolder(FOLDERID_Programs)/L"Lost Eye.lnk";
    if(fs::exists(shortcut)) {
        auto backup=directory/L"Previous Lost Eye shortcut.lnk";
        if(!fs::exists(backup)&&!CopyFileW(shortcut.c_str(),backup.c_str(),TRUE))throw HRESULT_FROM_WIN32(GetLastError());
    }
    ComPtr<IShellLinkW> link;Check(CoCreateInstance(CLSID_ShellLink,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&link)));
    Check(link->SetPath(destination.c_str()));Check(link->SetWorkingDirectory(directory.c_str()));Check(link->SetIconLocation(destination.c_str(),0));Check(link->SetDescription(L"Lost Eye video player"));
    ComPtr<IPersistFile> persist;Check(link.As(&persist));Check(persist->Save(shortcut.c_str(),TRUE));
    SHChangeNotify(SHCNE_ASSOCCHANGED,SHCNF_IDLIST,nullptr,nullptr);
    return destination;
}
struct App {
    HWND window{},video{},open{},backButton{},rewindButton{},play{},forwardButton{},skipButton{},loopBox{},muteButton{},fullButton{},seek{},volume{},time{},name{},percent{};
    HFONT font{}; HBRUSH background=CreateSolidBrush(RGB(28,29,33)),canvas=CreateSolidBrush(RGB(19,20,23));
    UINT dpi=96; bool full=false,loop=false,muted=false,failed=false,loading=false,dragging=false,forceFrame=true,atEnd=false;
    WINDOWPLACEMENT placement{sizeof(placement)}; std::wstring message=L"Open a video to start watching\n\nUse Open file or drop a video here.",pending,activeFile,navigationFile;
    int wheelRemainder=0; bool videoPressed=false;
    FILETIME activeWrite{}; ULONGLONG activeSize=0;
    std::unique_ptr<Engine> engine; std::shared_ptr<Ready> ready=std::make_shared<Ready>();
    ULONGLONG shownAt=0,openedAt=0,lastUi=0; unsigned long long startFrame=0; bool initialized=false;
#ifdef LOSTEYE_TEST
    std::wstring fixtures,report; int testStep=0,testFailures=0,loopCount=0; ULONGLONG testAt=0;
    double previousTime=0,pausedTime=0;std::ofstream tests;int keyTarget=0;bool pausedBeforeInput=false;
    void TestTick();
    void Test(const char* title,bool pass) {tests<<(pass?"PASS ":"FAIL ")<<title<<"\n";tests.flush();if(!pass)++testFailures;}
#endif
    ~App() { {std::lock_guard guard(ready->lock);ready->alive=false;} engine.reset();if(font)DeleteObject(font);DeleteObject(background);DeleteObject(canvas); }
    int S(int n) const {return MulDiv(n,static_cast<int>(dpi),96);}
    HWND Control(const wchar_t* cls,const wchar_t* text,DWORD style,int id) {
        HWND h=CreateWindowExW(0,cls,text,WS_CHILD|WS_VISIBLE|style,0,0,1,1,window,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);
        if(!h) throw HRESULT_FROM_WIN32(GetLastError());SendMessageW(h,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE); return h;
    }
    void EnableTransport(bool enabled) {
        for(HWND control:{rewindButton,play,forwardButton,seek})if(control)EnableWindow(control,enabled);
        for(HWND control:{backButton,skipButton})if(control)EnableWindow(control,engine&&!loading&&!navigationFile.empty());
    }
    void DarkFrame(){BOOL dark=TRUE;DwmSetWindowAttribute(window,DWMWA_USE_IMMERSIVE_DARK_MODE,&dark,sizeof(dark));}
    void CreateControls() {
        dpi=GetDpiForWindow(window);font=CreateFontW(-S(14),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
        video=CreateWindowExW(0,L"LostEye.VideoSurface",L"",WS_CHILD|WS_VISIBLE,0,0,1,1,window,nullptr,GetModuleHandleW(nullptr),this);
        open=Control(L"BUTTON",L"Open file",BS_OWNERDRAW|WS_TABSTOP,ID_OPEN);
        name=Control(L"STATIC",L"Lost Eye",SS_LEFT|SS_PATHELLIPSIS,0);
        backButton=Control(L"BUTTON",L"Back video",BS_OWNERDRAW|WS_TABSTOP,ID_BACK);
        rewindButton=Control(L"BUTTON",L"Rewind 10s",BS_OWNERDRAW|WS_TABSTOP,ID_REWIND);
        play=Control(L"BUTTON",L"Play",BS_OWNERDRAW|WS_TABSTOP,ID_PLAY);
        forwardButton=Control(L"BUTTON",L"Forward 10s",BS_OWNERDRAW|WS_TABSTOP,ID_FORWARD);
        skipButton=Control(L"BUTTON",L"Skip video",BS_OWNERDRAW|WS_TABSTOP,ID_SKIP);
        loopBox=Control(L"BUTTON",L"Loop",BS_AUTOCHECKBOX|WS_TABSTOP,ID_LOOP);
        SetWindowSubclass(loopBox,LoopProc,1,reinterpret_cast<DWORD_PTR>(this));
        muteButton=Control(L"BUTTON",L"Mute",BS_OWNERDRAW|WS_TABSTOP,ID_MUTE);
        fullButton=Control(L"BUTTON",L"Full screen",BS_OWNERDRAW|WS_TABSTOP,ID_FULL);
        seek=Control(TRACKBAR_CLASSW,L"Seek",TBS_HORZ|TBS_NOTICKS|WS_TABSTOP,ID_SEEK);SendMessageW(seek,TBM_SETRANGE,TRUE,MAKELPARAM(0,10000));SendMessageW(seek,TBM_SETPAGESIZE,0,500);
        volume=Control(TRACKBAR_CLASSW,L"Volume",TBS_HORZ|TBS_NOTICKS|WS_TABSTOP,ID_VOLUME);SendMessageW(volume,TBM_SETRANGE,TRUE,MAKELPARAM(0,100));SendMessageW(volume,TBM_SETPOS,TRUE,80);
        time=Control(L"STATIC",L"0:00 / 0:00",SS_RIGHT,0);percent=Control(L"STATIC",L"80%",SS_LEFT,0);
        DarkFrame();EnableTransport(false);DragAcceptFiles(window,TRUE);DragAcceptFiles(video,TRUE); Layout();
    }
    void Layout() {
        if(!video) return;RECT r{};GetClientRect(window,&r);int w=r.right,h=r.bottom;
        MoveWindow(open,S(12),S(8),S(100),S(34),TRUE);MoveWindow(name,S(126),S(15),std::max(1,w-S(142)),S(24),TRUE);
        int seekRow=h-S(142);MoveWindow(video,0,S(50),w,std::max(1,seekRow-S(50)),TRUE);
        MoveWindow(seek,S(10),seekRow+S(4),std::max(1,w-S(174)),S(30),TRUE);MoveWindow(time,w-S(155),seekRow+S(11),S(142),S(24),TRUE);
        const int widths[]={S(102),S(106),S(78),S(112),S(92)},gap=S(8);
        int transportWidth=widths[0]+widths[1]+widths[2]+widths[3]+widths[4]+gap*4;
        int x=std::max(S(10),(w-transportWidth)/2),transportRow=h-S(96);
        MoveWindow(backButton,x,transportRow,widths[0],S(34),TRUE);x+=widths[0]+gap;
        MoveWindow(rewindButton,x,transportRow,widths[1],S(34),TRUE);x+=widths[1]+gap;
        MoveWindow(play,x,transportRow,widths[2],S(34),TRUE);x+=widths[2]+gap;
        MoveWindow(forwardButton,x,transportRow,widths[3],S(34),TRUE);x+=widths[3]+gap;
        MoveWindow(skipButton,x,transportRow,widths[4],S(34),TRUE);
        int row=h-S(46);MoveWindow(loopBox,S(14),row,S(74),S(34),TRUE);MoveWindow(muteButton,S(96),row,S(78),S(34),TRUE);
        MoveWindow(volume,S(182),row+S(2),S(110),S(30),TRUE);MoveWindow(percent,S(300),row+S(7),S(46),S(24),TRUE);MoveWindow(fullButton,w-S(120),row,S(106),S(34),TRUE);
        if(engine){RECT v{};GetClientRect(video,&v);engine->Resize(v.right,v.bottom);forceFrame=true;}
        InvalidateRect(window,nullptr,TRUE);
    }
    void BeginInitialize() {
        HWND parent=window,target=video;auto box=ready;
        std::thread([parent,target,box]{
            HRESULT com=CoInitializeEx(nullptr,COINIT_MULTITHREADED);auto created=std::make_unique<Engine>();HRESULT result=S_OK;
#ifdef LOSTEYE_TEST
            Sleep(2000); // Prove the window stays visible/responding during a slow engine start.
#endif
            try{Check(com);created->Initialize(parent,target);}catch(HRESULT hr){result=hr;}catch(...){result=E_FAIL;}
            {std::lock_guard guard(box->lock);if(box->alive){box->hr=result;if(SUCCEEDED(result))box->engine=std::move(created);PostMessageW(parent,WM_READY,0,0);}}
            created.reset();if(SUCCEEDED(com))CoUninitialize();
        }).detach();
    }
    void ReadyEngine() {
        HRESULT hr;{std::lock_guard guard(ready->lock);hr=ready->hr;engine=std::move(ready->engine);}
        initialized=true;
        if(FAILED(hr)){Fail(L"Windows couldn't start video playback ("+ErrorCode(hr)+L").\nCheck Windows media components and your graphics driver.");return;}
        Layout();ApplyVolume();engine->media->SetLoop(loop);
        if(!pending.empty()){auto file=pending;pending.clear();Open(file);} else Status(L"Open a video to start watching\n\nUse Open file or drop a video here.");
    }
    void Status(const std::wstring& value){message=value;InvalidateRect(video,nullptr,TRUE);}
    void Fail(const std::wstring& value){failed=true;loading=false;atEnd=false;activeFile.clear();if(engine)engine->media->Pause();EnableTransport(false);SetWindowTextW(play,L"Play");Status(value+L"\n\nUse Open file to choose another video.");}
    void Open(const std::wstring& path) {
        if(path.empty())return;
        std::wstring file;try{file=fs::absolute(path).lexically_normal().wstring();}catch(...){Fail(L"This file path is invalid.");return;}
        DWORD attributes=GetFileAttributesW(file.c_str());
        if(attributes!=INVALID_FILE_ATTRIBUTES&&(attributes&FILE_ATTRIBUTE_DIRECTORY)){Fail(L"Choose a video file, not a folder.");return;}
        navigationFile=file;
        SetWindowTextW(name,fs::path(file).filename().c_str());SetWindowTextW(window,(fs::path(file).filename().wstring()+L" - Lost Eye").c_str());
        if(!engine){pending=file;Status(initialized?L"The Windows playback engine is unavailable.":L"Starting video playback...");return;}
        engine->media->Pause();failed=false;loading=true;dragging=false;forceFrame=true;atEnd=false;startFrame=engine->frames;openedAt=GetTickCount64();
        EnableTransport(false);SendMessageW(seek,TBM_SETPOS,TRUE,0);SetWindowTextW(time,L"0:00 / 0:00");
        WIN32_FILE_ATTRIBUTE_DATA info{};
        if(!GetFileAttributesExW(file.c_str(),GetFileExInfoStandard,&info)||(info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)){Fail(L"This file is missing or unavailable.");return;}
        if(info.nFileSizeHigh==0&&info.nFileSizeLow==0){Fail(L"This file is empty.");return;}
        ULONGLONG fileSize=(static_cast<ULONGLONG>(info.nFileSizeHigh)<<32)|info.nFileSizeLow;
        if(!activeFile.empty()&&_wcsicmp(activeFile.c_str(),file.c_str())==0&&activeSize==fileSize&&CompareFileTime(&activeWrite,&info.ftLastWriteTime)==0){
            loading=false;engine->media->SetCurrentTime(0);engine->media->SetLoop(loop);ApplyVolume();auto hr=engine->media->Play();
            if(FAILED(hr)){Fail(L"Windows couldn't restart this video ("+ErrorCode(hr)+L").");return;}
            message.clear();EnableTransport(true);InvalidateRect(video,nullptr,TRUE);return;
        }
        // Convert paths (including spaces, Unicode and #) to escaped file URLs for MF.
        DWORD size=32768;std::vector<wchar_t> url(size);auto hr=UrlCreateFromPathW(file.c_str(),url.data(),&size,0);
        if(FAILED(hr)){Fail(L"Windows couldn't read this file path.");return;}
        Status(L"Opening video...");BSTR source=SysAllocString(url.data());hr=engine->media->SetSource(source);SysFreeString(source);
        if(FAILED(hr)){Fail(L"Windows couldn't open this video ("+ErrorCode(hr)+L").");return;}
        activeFile=file;activeSize=fileSize;activeWrite=info.ftLastWriteTime;
        engine->media->SetLoop(loop);ApplyVolume();
    }
    void OnMedia(DWORD event) {
        if(!engine||failed)return;
        if(event==MF_MEDIA_ENGINE_EVENT_CANPLAY){
            if(!engine->media->HasVideo()){Fail(L"No video track was found in this file.");return;}
            if(loading){auto hr=engine->media->Play();if(FAILED(hr))Fail(L"Windows couldn't play this video ("+ErrorCode(hr)+L").");}
        } else if(event==MF_MEDIA_ENGINE_EVENT_ERROR){
            ComPtr<IMFMediaError> err;engine->media->GetError(&err);HRESULT hr=err?err->GetExtendedErrorCode():E_FAIL;
            Fail(L"Windows couldn't play this file. It may be damaged,\nor its video/audio format isn't available on this PC.\n"+ErrorCode(hr));
        } else if(event==MF_MEDIA_ENGINE_EVENT_ENDED){atEnd=!loop;if(!loop){engine->media->Pause();SetWindowTextW(play,L"Replay");}}
        else if(event==MF_MEDIA_ENGINE_EVENT_SEEKED){forceFrame=true;}
    }
    void TogglePlay(){if(!engine||failed||loading)return;auto m=engine->media.Get();if(atEnd||m->IsEnded()){atEnd=false;m->SetCurrentTime(0);m->Play();}else if(m->IsPaused())m->Play();else m->Pause();UpdateUi();}
    void SeekTime(double seconds,bool end=false){
        if(!engine||failed||loading)return;double duration=engine->media->GetDuration();if(!std::isfinite(duration)||duration<=0)return;
        if(end||seconds>=duration){engine->media->SetCurrentTime(duration);engine->media->Pause();atEnd=true;}else{engine->media->SetCurrentTime(std::clamp(seconds,0.0,duration));atEnd=false;}
        forceFrame=true;UpdateUi();
    }
    void Seek(double ratio){if(!engine||failed||loading)return;double duration=engine->media->GetDuration();if(std::isfinite(duration)&&duration>0)SeekTime(std::clamp(ratio,0.0,0.9999)*duration);}
    void BackToStart(){SeekTime(0);}
    void JumpBy(double seconds){if(!engine||failed||loading)return;double current=atEnd?engine->media->GetDuration():engine->media->GetCurrentTime();SeekTime(current+seconds);}
    void SkipToEnd(){if(engine)SeekTime(engine->media->GetDuration(),true);}
    void AdjacentVideo(int direction){
        if(!engine||loading||navigationFile.empty())return;
        auto next=LostEyeFiles::AdjacentVideo(navigationFile,direction);
        if(!next.error.empty()){MessageBoxW(window,next.error.c_str(),L"Lost Eye",MB_OK|MB_ICONINFORMATION);return;}
        if(!next.path.empty())Open(next.path);
    }
    void WheelVolume(int delta){
        wheelRemainder+=delta;int steps=wheelRemainder/WHEEL_DELTA;wheelRemainder%=WHEEL_DELTA;
        if(!steps)return;int before=static_cast<int>(SendMessageW(volume,TBM_GETPOS,0,0));int after=std::clamp(before+steps*5,0,100);
        if(after==before)return;SendMessageW(volume,TBM_SETPOS,TRUE,after);muted=false;ApplyVolume();
    }
    void ApplyVolume(){int v=static_cast<int>(SendMessageW(volume,TBM_GETPOS,0,0));if(engine){engine->media->SetVolume(v/100.0);engine->media->SetMuted(muted||v==0);}SetWindowTextW(muteButton,muted||v==0?L"Unmute":L"Mute");SetWindowTextW(percent,(std::to_wstring(v)+L"%").c_str());}
    void SetLoop(bool value){loop=value;SendMessageW(loopBox,BM_SETCHECK,loop?BST_CHECKED:BST_UNCHECKED,0);InvalidateRect(loopBox,nullptr,FALSE);if(engine)engine->media->SetLoop(loop);}
    bool HandleInput(const MSG& msg){
        if(msg.hwnd!=window&&!IsChild(window,msg.hwnd))return false;
        if(msg.message==WM_MOUSEWHEEL){POINT point{GET_X_LPARAM(msg.lParam),GET_Y_LPARAM(msg.lParam)};RECT bounds{};GetWindowRect(window,&bounds);if(PtInRect(&bounds,point)){WheelVolume(GET_WHEEL_DELTA_WPARAM(msg.wParam));return true;}return false;}
        const bool transportKey=msg.wParam==VK_SPACE||msg.wParam==VK_LEFT||msg.wParam==VK_RIGHT;
        if(msg.message==WM_KEYUP&&transportKey)return true;
        if(msg.message==WM_CHAR&&msg.wParam==L' ')return true;
        if(msg.message!=WM_KEYDOWN)return false;
        if(transportKey){
            // Swallow both halves of these keys before buttons/sliders see them.
            // A held key must not repeatedly toggle or race through the folder.
            if(!(msg.lParam&(1LL<<30))){if(msg.wParam==VK_SPACE)TogglePlay();else AdjacentVideo(msg.wParam==VK_LEFT?-1:1);}
            return true;
        }
        if(msg.lParam&(1LL<<30))return false;
        const bool ctrl=(GetKeyState(VK_CONTROL)&0x8000)!=0;
        switch(msg.wParam){
        case 'O':if(ctrl){Choose();return true;}break;
        case VK_F11:case 'F':Fullscreen();return true;
        case VK_ESCAPE:if(full){Fullscreen();return true;}break;
        case 'M':muted=!muted;ApplyVolume();return true;
        case 'L':SetLoop(!loop);return true;
        case VK_HOME:BackToStart();return true;
        case VK_END:SkipToEnd();return true;
        }
        return false;
    }
    void UpdateUi(){
        if(!engine||failed)return;auto m=engine->media.Get();double d=m->GetDuration(),t=m->GetCurrentTime();
        if(std::isfinite(d)&&d>0){if(atEnd)t=d;if(!dragging)SendMessageW(seek,TBM_SETPOS,TRUE,static_cast<LPARAM>(std::clamp(t/d,0.0,1.0)*10000));else t=SendMessageW(seek,TBM_GETPOS,0,0)*d/10000.;SetWindowTextW(time,(TimeText(t)+L" / "+TimeText(d)).c_str());}
        if(!loading&&std::isfinite(d)&&d>0){EnableTransport(true);SetWindowTextW(play,atEnd||m->IsEnded()?L"Replay":m->IsPaused()?L"Play":L"Pause");}
    }
    void Tick(){
        if(engine&&!failed){
            bool rendered=engine->Render(forceFrame);if(rendered){forceFrame=false;if(loading){loading=false;message.clear();}}
            if(FAILED(engine->lastRender)&&(engine->lastRender==DXGI_ERROR_DEVICE_REMOVED||engine->lastRender==DXGI_ERROR_DEVICE_RESET))Fail(L"The graphics device was reset. Close Lost Eye and reopen this video.");
            if(loading&&GetTickCount64()-openedAt>30000)Fail(L"Windows is taking too long to open this video.\nTry a local file with a supported format.");
        }
        if(GetTickCount64()-lastUi>200){UpdateUi();lastUi=GetTickCount64();}
#ifdef LOSTEYE_TEST
        TestTick();
#endif
    }
    void Fullscreen(){
        if(!full){GetWindowPlacement(window,&placement);MONITORINFO mi{sizeof(mi)};GetMonitorInfoW(MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST),&mi);SetWindowLongPtrW(window,GWL_STYLE,GetWindowLongPtrW(window,GWL_STYLE)&~WS_OVERLAPPEDWINDOW);SetWindowPos(window,nullptr,mi.rcMonitor.left,mi.rcMonitor.top,mi.rcMonitor.right-mi.rcMonitor.left,mi.rcMonitor.bottom-mi.rcMonitor.top,SWP_NOZORDER|SWP_FRAMECHANGED);full=true;SetWindowTextW(fullButton,L"Exit full");}
        else{SetWindowLongPtrW(window,GWL_STYLE,GetWindowLongPtrW(window,GWL_STYLE)|WS_OVERLAPPEDWINDOW);SetWindowPlacement(window,&placement);SetWindowPos(window,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);full=false;SetWindowTextW(fullButton,L"Full screen");}
        forceFrame=true;
    }
    void Choose(){
        ComPtr<IFileOpenDialog> dialog; if(FAILED(CoCreateInstance(CLSID_FileOpenDialog,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&dialog))))return;
        COMDLG_FILTERSPEC types[]={{L"Video files",L"*.mp4;*.m4v;*.mov;*.mkv;*.webm;*.avi;*.wmv;*.mpg;*.mpeg;*.ts;*.m2ts;*.3gp"},{L"All files",L"*.*"}};
        dialog->SetFileTypes(2,types);dialog->SetTitle(L"Open a video");dialog->SetOptions(FOS_FILEMUSTEXIST|FOS_PATHMUSTEXIST|FOS_FORCEFILESYSTEM);
        if(SUCCEEDED(dialog->Show(window))){ComPtr<IShellItem> item;if(SUCCEEDED(dialog->GetResult(&item))){PWSTR path=nullptr;if(SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH,&path))){Open(path);CoTaskMemFree(path);}}}
    }
    void Drop(HDROP drop){UINT count=DragQueryFileW(drop,0xFFFFFFFF,nullptr,0);if(count==1){UINT len=DragQueryFileW(drop,0,nullptr,0);std::vector<wchar_t> p(len+1);DragQueryFileW(drop,0,p.data(),len+1);Open(p.data());}DragFinish(drop);}
    void DrawButton(DRAWITEMSTRUCT* d){
        auto brush=CreateSolidBrush((d->itemState&ODS_SELECTED)?RGB(65,67,73):RGB(47,49,55));FillRect(d->hDC,&d->rcItem,brush);DeleteObject(brush);
        SetBkMode(d->hDC,TRANSPARENT);SetTextColor(d->hDC,(d->itemState&ODS_DISABLED)?RGB(150,150,155):RGB(245,245,248));SelectObject(d->hDC,font);
        wchar_t text[64]{};GetWindowTextW(d->hwndItem,text,64);DrawTextW(d->hDC,text,-1,&d->rcItem,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        if(d->itemState&ODS_FOCUS){RECT focus=d->rcItem;InflateRect(&focus,-3,-3);DrawFocusRect(d->hDC,&focus);}
    }
    void DrawLoop(HWND h,HDC dc){
        RECT r{};GetClientRect(h,&r);FillRect(dc,&r,background);int side=S(16);RECT box{S(2),(r.bottom-side)/2,S(2)+side,(r.bottom+side)/2};
        HBRUSH inside=CreateSolidBrush(RGB(47,49,55));FillRect(dc,&box,inside);DeleteObject(inside);
        HBRUSH edge=CreateSolidBrush(RGB(170,173,183));FrameRect(dc,&box,edge);DeleteObject(edge);
        if(SendMessageW(h,BM_GETCHECK,0,0)==BST_CHECKED){HPEN pen=CreatePen(PS_SOLID,std::max(1,S(2)),RGB(235,235,240));auto previous=SelectObject(dc,pen);MoveToEx(dc,box.left+S(3),box.top+S(8),nullptr);LineTo(dc,box.left+S(7),box.top+S(12));LineTo(dc,box.left+S(13),box.top+S(4));SelectObject(dc,previous);DeleteObject(pen);}
        RECT label=r;label.left=box.right+S(7);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(235,235,240));auto oldFont=SelectObject(dc,font);DrawTextW(dc,L"Loop",-1,&label,DT_LEFT|DT_VCENTER|DT_SINGLELINE);SelectObject(dc,oldFont);
        if(GetFocus()==h){InflateRect(&label,-1,-S(6));DrawFocusRect(dc,&label);}
    }
    static LRESULT CALLBACK LoopProc(HWND h,UINT msg,WPARAM w,LPARAM l,UINT_PTR id,DWORD_PTR data){
        auto a=reinterpret_cast<App*>(data);
        if(msg==WM_PAINT){PAINTSTRUCT ps{};HDC dc=BeginPaint(h,&ps);a->DrawLoop(h,dc);EndPaint(h,&ps);return 0;}
        if(msg==WM_PRINTCLIENT){a->DrawLoop(h,reinterpret_cast<HDC>(w));return 0;}
        if(msg==WM_ERASEBKGND)return 1;
        auto result=DefSubclassProc(h,msg,w,l);
        if(msg==BM_SETCHECK||msg==WM_SETFOCUS||msg==WM_KILLFOCUS||msg==WM_ENABLE||msg==WM_THEMECHANGED||msg==WM_UPDATEUISTATE)InvalidateRect(h,nullptr,FALSE);
        if(msg==WM_NCDESTROY)RemoveWindowSubclass(h,LoopProc,id);
        return result;
    }
    static LRESULT CALLBACK VideoProc(HWND h,UINT msg,WPARAM w,LPARAM l){
        App* a=reinterpret_cast<App*>(GetWindowLongPtrW(h,GWLP_USERDATA));if(msg==WM_NCCREATE){a=static_cast<App*>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams);SetWindowLongPtrW(h,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(a));}
        if(!a)return DefWindowProcW(h,msg,w,l);
        if(msg==WM_PAINT){PAINTSTRUCT ps{};HDC dc=BeginPaint(h,&ps);if(!a->message.empty()){RECT r{};GetClientRect(h,&r);FillRect(dc,&r,a->canvas);SelectObject(dc,a->font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(216,218,224));RECT text=r;InflateRect(&text,-a->S(30),0);DrawTextW(dc,a->message.c_str(),-1,&text,DT_CENTER|DT_WORDBREAK|DT_CALCRECT);int height=text.bottom-text.top;text.top=(r.bottom-height)/2;text.bottom=text.top+height;text.left=a->S(30);text.right=r.right-a->S(30);DrawTextW(dc,a->message.c_str(),-1,&text,DT_CENTER|DT_WORDBREAK);}EndPaint(h,&ps);a->forceFrame=true;return 0;}
        if(msg==WM_ERASEBKGND)return 1;
        if(msg==WM_LBUTTONDOWN){a->videoPressed=true;SetFocus(a->window);SetCapture(h);return 0;}
        if(msg==WM_LBUTTONUP){bool pressed=a->videoPressed;a->videoPressed=false;if(GetCapture()==h)ReleaseCapture();RECT r{};GetClientRect(h,&r);POINT point{GET_X_LPARAM(l),GET_Y_LPARAM(l)};if(pressed&&PtInRect(&r,point))a->TogglePlay();return 0;}
        if(msg==WM_CAPTURECHANGED){a->videoPressed=false;return 0;}
        if(msg==WM_DROPFILES){a->Drop(reinterpret_cast<HDROP>(w));return 0;}return DefWindowProcW(h,msg,w,l);
    }
    static LRESULT CALLBACK Proc(HWND h,UINT msg,WPARAM w,LPARAM l){
        App* a=reinterpret_cast<App*>(GetWindowLongPtrW(h,GWLP_USERDATA));if(msg==WM_NCCREATE){a=static_cast<App*>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams);a->window=h;SetWindowLongPtrW(h,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(a));}
        if(!a)return DefWindowProcW(h,msg,w,l);
        switch(msg){
        case WM_CREATE:a->CreateControls();SetTimer(h,1,16,nullptr);return 0;
        case WM_START:a->BeginInitialize();return 0;
        case WM_READY:a->ReadyEngine();return 0;
        case WM_MEDIA:a->OnMedia(static_cast<DWORD>(w));return 0;
        case WM_TIMER:a->Tick();return 0;
        case WM_SIZE:a->Layout();return 0;
        case WM_SETTINGCHANGE:case WM_THEMECHANGED:a->DarkFrame();if(a->loopBox)InvalidateRect(a->loopBox,nullptr,FALSE);return 0;
        case WM_DPICHANGED:{a->dpi=HIWORD(w);if(a->font)DeleteObject(a->font);a->font=CreateFontW(-a->S(14),0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");for(HWND child=GetWindow(h,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT))SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(a->font),TRUE);auto r=reinterpret_cast<RECT*>(l);SetWindowPos(h,nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER);a->Layout();return 0;}
        case WM_GETMINMAXINFO:reinterpret_cast<MINMAXINFO*>(l)->ptMinTrackSize={a->S(700),a->S(420)};return 0;
        case WM_COMMAND:switch(LOWORD(w)){case ID_OPEN:a->Choose();break;case ID_BACK:a->AdjacentVideo(-1);break;case ID_REWIND:a->JumpBy(-10);break;case ID_PLAY:a->TogglePlay();break;case ID_FORWARD:a->JumpBy(10);break;case ID_SKIP:a->AdjacentVideo(1);break;case ID_LOOP:a->SetLoop(SendMessageW(a->loopBox,BM_GETCHECK,0,0)==BST_CHECKED);break;case ID_MUTE:a->muted=!a->muted;a->ApplyVolume();break;case ID_FULL:a->Fullscreen();break;}return 0;
        case WM_HSCROLL:if(reinterpret_cast<HWND>(l)==a->volume){a->muted=false;a->ApplyVolume();}else if(reinterpret_cast<HWND>(l)==a->seek){if(LOWORD(w)==TB_THUMBTRACK)a->dragging=true;else{a->dragging=false;a->Seek(SendMessageW(a->seek,TBM_GETPOS,0,0)/10000.);}a->UpdateUi();}return 0;
        case WM_DRAWITEM:a->DrawButton(reinterpret_cast<DRAWITEMSTRUCT*>(l));return TRUE;
        case WM_CTLCOLORSTATIC:case WM_CTLCOLORBTN:SetBkColor(reinterpret_cast<HDC>(w),RGB(28,29,33));SetTextColor(reinterpret_cast<HDC>(w),RGB(235,235,240));return reinterpret_cast<LRESULT>(a->background);
        case WM_ERASEBKGND:{RECT r{};GetClientRect(h,&r);FillRect(reinterpret_cast<HDC>(w),&r,a->background);return 1;}
        case WM_DROPFILES:a->Drop(reinterpret_cast<HDROP>(w));return 0;
        case WM_COPYDATA:{auto data=reinterpret_cast<COPYDATASTRUCT*>(l);if(data&&data->dwData==1&&data->lpData&&data->cbData>=sizeof(wchar_t)&&data->cbData<=65536&&data->cbData%sizeof(wchar_t)==0){auto p=static_cast<const wchar_t*>(data->lpData);if(p[data->cbData/sizeof(wchar_t)-1]==0)a->Open(p);}ShowWindow(h,IsIconic(h)?SW_RESTORE:SW_SHOW);SetForegroundWindow(h);return TRUE;}
        case WM_DESTROY:{KillTimer(h,1);{std::lock_guard guard(a->ready->lock);a->ready->alive=false;}PostQuitMessage(0);return 0;}
        }
        return DefWindowProcW(h,msg,w,l);
    }
};
#ifdef LOSTEYE_TEST
void App::TestTick(){
    if(fixtures.empty())return;auto now=GetTickCount64();auto elapsed=now-testAt;
    auto next=[&](int step){testStep=step;testAt=now;};
    auto file=[&](const wchar_t* name){return (fs::path(fixtures)/name).wstring();};
    auto click=[&](int id,HWND control){SendMessageW(window,WM_COMMAND,MAKEWPARAM(id,BN_CLICKED),reinterpret_cast<LPARAM>(control));};
    auto key=[&](HWND target,UINT value,bool repeat=false){SetFocus(target);PostMessageW(target,WM_KEYDOWN,value,1);if(repeat)PostMessageW(target,WM_KEYDOWN,value,1|(1LL<<30));PostMessageW(target,WM_KEYUP,value,1|(3LL<<30));};
    auto wheel=[&](HWND target,short delta){POINT point{S(25),S(25)};ClientToScreen(video,&point);PostMessageW(target,WM_MOUSEWHEEL,MAKEWPARAM(0,static_cast<WORD>(delta)),MAKELPARAM(point.x,point.y));};
    const HWND keyTargets[]={open,backButton,rewindButton,play,forwardButton,skipButton,loopBox,muteButton,fullButton,seek,volume,video,window};
    if(fixtures==L"--close-during-init"){if(now-shownAt>200){Test("Close during engine startup",!engine&&IsWindowVisible(window));DestroyWindow(window);}return;}
    if(testStep>0&&elapsed>45000){Test("Step timed out",false);DestroyWindow(window);return;}
    switch(testStep){
    case 0:if(now-shownAt>300){Test("Window visible before slow engine startup",IsWindowVisible(window)&&!engine);Test("Time formatting",TimeText(3661)==L"1:01:01"&&TimeText(65)==L"1:05");next(1);}break;
    case 1:if(initialized){Test("Windows engine initialized",engine!=nullptr);if(!engine){DestroyWindow(window);break;}Test("Hardware Direct3D device and DXGI manager",engine->hardware&&engine->manager);tests<<"ADAPTER ";for(auto c:engine->adapter)tests<<static_cast<char>(c<128?c:'?');tests<<"\n";
        HKEY sandbox{};std::wstring key=L"Software\\LostEye.Native.Tests."+std::to_wstring(GetCurrentProcessId());Check(HRESULT_FROM_WIN32(RegCreateKeyExW(HKEY_CURRENT_USER,key.c_str(),0,nullptr,0,KEY_ALL_ACCESS,nullptr,&sandbox,nullptr)));
        SetRegistry(sandbox,L"Software\\Classes\\.mp4",nullptr,L"Existing.Player");RegisterOpenWith(sandbox,L"C:\\Example folder\\Lost Eye.exe");
        wchar_t result[256]{};DWORD size=sizeof(result);RegGetValueW(sandbox,L"Software\\Classes\\.mp4",nullptr,RRF_RT_REG_SZ,nullptr,result,&size);Test("Open with preserves existing default",std::wstring(result)==L"Existing.Player");
        size=sizeof(result);RegGetValueW(sandbox,L"Software\\Classes\\Applications\\Lost Eye.exe\\shell\\open\\command",nullptr,RRF_RT_REG_SZ,nullptr,result,&size);Test("Open with quotes paths",std::wstring(result)==L"\"C:\\Example folder\\Lost Eye.exe\" \"%1\"");RegCloseKey(sandbox);RegDeleteTreeW(HKEY_CURRENT_USER,key.c_str());
        Open(file(L"missing.mp4"));Test("Missing file is handled",failed);Open(file(L"empty.mp4"));Test("Empty file is handled",failed);Open(file(L"damaged.mp4"));next(2);}break;
    case 2:if(failed){Test("Damaged file is handled",true);Test("Playback controls disabled after an error",!IsWindowEnabled(rewindButton)&&!IsWindowEnabled(play)&&!IsWindowEnabled(forwardButton));Test("Folder navigation available after an error",IsWindowEnabled(backButton)&&IsWindowEnabled(skipButton));Open(file(L"sample video.mp4"));next(3);}break;
    case 3:if(engine->frames>startFrame+4&&!loading){Test("MP4 opens after errors",!failed&&engine->media->GetCurrentTime()>0);Test("Decoded pixels reached Direct3D texture",engine->FrameHasPixels());Test("Frames presented to window",engine->frames>startFrame+4);muted=true;ApplyVolume();Test("Mute",engine->media->GetMuted());muted=false;SendMessageW(volume,TBM_SETPOS,TRUE,35);ApplyVolume();Test("Volume and unmute",std::abs(engine->media->GetVolume()-.35)<.01&&!engine->media->GetMuted());TogglePlay();next(4);}break;
    case 4:if(elapsed>400){pausedTime=engine->media->GetCurrentTime();next(5);}break;
    case 5:if(elapsed>600){Test("Pause holds position",engine->media->IsPaused()&&std::abs(pausedTime-engine->media->GetCurrentTime())<.15);Seek(.5);TogglePlay();next(6);}break;
    case 6:if(engine->media->GetCurrentTime()>3.5&&!engine->media->IsPaused()){
        Test("Seek and resume",true);TogglePlay();SetWindowPos(window,nullptr,0,0,S(700),S(450),SWP_NOMOVE|SWP_NOZORDER);
        RECT back{},rewind{},playRect{},forward{},skip{};GetWindowRect(backButton,&back);GetWindowRect(rewindButton,&rewind);GetWindowRect(play,&playRect);GetWindowRect(forwardButton,&forward);GetWindowRect(skipButton,&skip);
        Test("Transport buttons visible",IsWindowVisible(backButton)&&IsWindowVisible(rewindButton)&&IsWindowVisible(play)&&IsWindowVisible(forwardButton)&&IsWindowVisible(skipButton));
        Test("Transport buttons fit without overlap",back.right<=rewind.left&&rewind.right<=playRect.left&&playRect.right<=forward.left&&forward.right<=skip.left);
        key(play,VK_HOME);next(16);
    }break;
    case 7:{double t=engine->media->GetCurrentTime();if(previousTime>5&&t<2)++loopCount;previousTime=t;if(loopCount>=2){Test("Whole video loops twice",true);Test("Loop stays on the current video",activeFile==file(L"sample video.mp4"));SetLoop(false);Seek(.85);next(8);}break;}
    case 8:if(engine->media->IsEnded()){Test("Loop off stops at end",true);next(9);}break;
    case 9:if(elapsed>600){Test("Remains stopped",engine->media->IsEnded()&&engine->media->IsPaused());TogglePlay();next(10);}break;
    case 10:if(engine->media->GetCurrentTime()>.2&&!engine->media->IsPaused()){Test("Replay",true);Open(file(L"sample.mkv"));next(11);}break;
    case 11:if(failed||(!loading&&engine->frames>startFrame+3)){Test("MKV renders or clearly reports unavailable OS codec",failed||engine->frames>startFrame+3);tests<<"MKV "<<(failed?"UNSUPPORTED":"RENDERED")<<"\n";Open(file(L"sample.webm"));next(12);}break;
    case 12:if(failed||(!loading&&engine->frames>startFrame+3)){Test("WebM renders or clearly reports unavailable OS codec",failed||engine->frames>startFrame+3);tests<<"WEBM "<<(failed?"UNSUPPORTED":"RENDERED")<<"\n";Open(file(L"sample video.mp4"));next(13);}break;
    case 13:if(!loading&&engine->frames>startFrame+3){Test("Reopen MP4 1",!failed);Open(file(L"sample video.mp4"));next(14);}break;
    case 14:if(!loading&&engine->frames>startFrame+3){Test("Reopen MP4 2",!failed);Open(file(L"sample video.mp4"));next(15);}break;
    case 15:if(!loading&&engine->frames>startFrame+3){Test("Reopen MP4 3",!failed);Test("No renderer error",SUCCEEDED(engine->lastRender));TogglePlay();Fullscreen();keyTarget=0;next(24);}break;
    case 16:if(elapsed>350){Test("Home seeks to start",engine->media->IsPaused()&&engine->media->GetCurrentTime()<.2&&!atEnd);engine->media->SetCurrentTime(std::min(4.0,engine->media->GetDuration()/2));next(17);}break;
    case 17:if(elapsed>350){click(ID_REWIND,rewindButton);next(18);}break;
    case 18:if(elapsed>350){Test("Rewind button moves back 10 seconds",engine->media->IsPaused()&&engine->media->GetCurrentTime()<.2&&!atEnd);click(ID_FORWARD,forwardButton);next(19);}break;
    case 19:if(elapsed>350){double duration=engine->media->GetDuration();Test("Fast forward button moves ahead 10 seconds",engine->media->IsPaused()&&atEnd&&engine->media->GetCurrentTime()>=duration-.2);key(play,VK_HOME);next(20);}break;
    case 20:if(elapsed>350){Test("Home works after reaching the end",engine->media->IsPaused()&&engine->media->GetCurrentTime()<.2&&!atEnd);key(play,VK_END);next(21);}break;
    case 21:if(elapsed>350){double duration=engine->media->GetDuration();Test("End seeks to the end",engine->media->IsPaused()&&atEnd&&engine->media->GetCurrentTime()>=duration-.2);click(ID_PLAY,play);next(22);}break;
    case 22:if(engine->media->GetCurrentTime()>.2&&!engine->media->IsPaused()){Test("Play restarts after End",!atEnd);Seek(.5);next(23);}break;
    case 23:if(engine->media->GetCurrentTime()>3.5&&!engine->media->IsPaused()){RECT before{};GetWindowRect(window,&before);Fullscreen();Test("Fullscreen entered",full&&!(GetWindowLongPtrW(window,GWL_STYLE)&WS_OVERLAPPEDWINDOW));Fullscreen();RECT after{};GetWindowRect(window,&after);Test("Fullscreen restores bounds",!full&&EqualRect(&before,&after));SetWindowPos(window,nullptr,0,0,S(720),S(450),SWP_NOMOVE|SWP_NOZORDER);Test("Resize keeps frame rendering",engine->Render(true));SetLoop(true);Seek(.85);previousTime=engine->media->GetCurrentTime();loopCount=0;next(7);}break;
    case 24:pausedBeforeInput=engine->media->IsPaused();key(keyTargets[keyTarget],VK_SPACE,true);next(25);break;
    case 25:if(elapsed>180){std::string label="Space toggles once with focus target "+std::to_string(keyTarget);Test(label.c_str(),engine->media->IsPaused()!=pausedBeforeInput&&full&&!loop&&activeFile==file(L"sample video.mp4")&&SendMessageW(volume,TBM_GETPOS,0,0)==35);key(keyTargets[keyTarget],VK_SPACE);next(26);}break;
    case 26:if(elapsed>180){Test("Space key-up does not activate focused control",engine->media->IsPaused()==pausedBeforeInput&&full&&!loop);if(++keyTarget<static_cast<int>(std::size(keyTargets))){next(24);break;}Fullscreen();
        SendMessageW(video,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(20,20));SendMessageW(video,WM_LBUTTONUP,0,MAKELPARAM(20,20));Test("Video click resumes",!engine->media->IsPaused());
        SendMessageW(video,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(20,20));SendMessageW(video,WM_LBUTTONUP,0,MAKELPARAM(20,20));Test("Second video click pauses without fullscreen",engine->media->IsPaused()&&!full);
        HDC screen=GetDC(loopBox),memory=CreateCompatibleDC(screen);HBITMAP bitmap=CreateCompatibleBitmap(screen,S(74),S(34));auto old=SelectObject(memory,bitmap);SendMessageW(loopBox,WM_PRINTCLIENT,reinterpret_cast<WPARAM>(memory),PRF_CLIENT);bool light=false;for(int y=S(5);y<S(29);++y)for(int x=S(25);x<S(70);++x){COLORREF p=GetPixel(memory,x,y);if(p!=CLR_INVALID&&GetRValue(p)>200&&GetGValue(p)>200&&GetBValue(p)>200)light=true;}Test("Loop label paints light text on dark background",light&&GetPixel(memory,0,0)==RGB(28,29,33));SelectObject(memory,old);DeleteObject(bitmap);DeleteDC(memory);ReleaseDC(loopBox,screen);
        BOOL dark=FALSE;Test("Dark title bar stays enabled",SUCCEEDED(DwmGetWindowAttribute(window,DWMWA_USE_IMMERSIVE_DARK_MODE,&dark,sizeof(dark)))&&dark);pausedTime=engine->media->GetCurrentTime();wheel(seek,120);next(27);
    }break;
    case 27:if(elapsed>180){Test("Wheel over seek raises volume without seeking",SendMessageW(volume,TBM_GETPOS,0,0)==40&&std::abs(engine->media->GetCurrentTime()-pausedTime)<.15);wheel(volume,-120);next(28);}break;
    case 28:if(elapsed>180){Test("Wheel over volume lowers by five without double handling",SendMessageW(volume,TBM_GETPOS,0,0)==35);wheel(video,40);wheel(video,40);next(29);}break;
    case 29:if(elapsed>180){Test("Partial wheel deltas accumulate",SendMessageW(volume,TBM_GETPOS,0,0)==35);wheel(video,40);next(30);}break;
    case 30:if(elapsed>180){Test("Accumulated wheel notch changes volume",SendMessageW(volume,TBM_GETPOS,0,0)==40);SendMessageW(volume,TBM_SETPOS,TRUE,95);muted=true;ApplyVolume();wheel(fullButton,240);next(31);}break;
    case 31:if(elapsed>180){Test("Wheel clamps at 100 and unmutes",SendMessageW(volume,TBM_GETPOS,0,0)==100&&!muted&&!engine->media->GetMuted());wheel(window,-2400);next(32);}break;
    case 32:if(elapsed>180){Test("Wheel clamps at zero and mutes",SendMessageW(volume,TBM_GETPOS,0,0)==0&&engine->media->GetMuted());SendMessageW(volume,TBM_SETPOS,TRUE,35);ApplyVolume();Open(file(L"navigation\\clip 2.mp4"));next(33);}break;
    case 33:if(!loading&&!failed&&engine->frames>startFrame+3){click(ID_BACK,backButton);next(34);}break;
    case 34:if(!loading&&!failed&&engine->frames>startFrame+3){Test("Back opens previous video in same folder",activeFile==file(L"navigation\\clip 1.mp4"));key(fullButton,VK_LEFT);next(35);}break;
    case 35:if(elapsed>180){Test("Left at first video stays in same folder",activeFile==file(L"navigation\\clip 1.mp4")&&!full);key(seek,VK_RIGHT);next(36);}break;
    case 36:if(!loading&&!failed&&engine->frames>startFrame+3){Test("Right with seek focus opens next video",activeFile==file(L"navigation\\clip 2.mp4"));click(ID_SKIP,skipButton);next(37);}break;
    case 37:if(!loading&&!failed&&engine->frames>startFrame+3){Test("Skip opens natural-order next video",activeFile==file(L"navigation\\clip 10.mp4"));click(ID_SKIP,skipButton);key(volume,VK_RIGHT);next(38);}break;
    case 38:if(elapsed>180){Test("Skip and Right at last video never enter other folders",activeFile==file(L"navigation\\clip 10.mp4"));key(volume,VK_LEFT,true);next(39);}break;
    case 39:if(!loading&&!failed&&engine->frames>startFrame+3){Test("Left with volume focus opens exactly one previous video",activeFile==file(L"navigation\\clip 2.mp4"));Open(file(L"recovery\\clip 1.mp4"));next(40);}break;
    case 40:if(failed){Test("Bad neighboring file gives clear error with navigation available",!message.empty()&&IsWindowEnabled(skipButton));key(skipButton,VK_RIGHT);next(41);}break;
    case 41:if(!loading&&!failed&&engine->frames>startFrame+3){Test("Right recovers from bad video within its folder",activeFile==file(L"recovery\\clip 2.mp4"));Test("No renderer error after folder navigation",SUCCEEDED(engine->lastRender));tests<<"FRAMES "<<engine->frames<<"\nFAILURES "<<testFailures<<"\n";tests.flush();DestroyWindow(window);}break;
    }
}
#endif
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,LPWSTR,int show){
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    HRESULT com=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);if(FAILED(com))return 1;
    int argc=0;LPWSTR* argv=CommandLineToArgvW(GetCommandLineW(),&argc);std::wstring path=argc>1?argv[1]:L"";
#ifdef LOSTEYE_TEST
    if(path==L"--verify-shell"&&argc>=4){
        std::wofstream report{fs::path(argv[3])};report.setf(std::ios::unitbuf);report<<L"START shell enumeration\n";int result=1;
        ComPtr<IEnumAssocHandlers> handlers;HRESULT hr=SHAssocEnumHandlers(L".mp4",ASSOC_FILTER_NONE,&handlers);
        report<<L"ENUM "<<ErrorCode(hr)<<L"\n";if(SUCCEEDED(hr)){
            ComPtr<IAssocHandler> handler;ULONG fetched=0;
            while(handlers->Next(1,&handler,&fetched)==S_OK){
                PWSTR rawName=nullptr,rawUi=nullptr;handler->GetName(&rawName);
                std::wstring handlerName=rawName?rawName:L"",ui=rawUi?rawUi:L"";CoTaskMemFree(rawName);CoTaskMemFree(rawUi);
                report<<L"CANDIDATE "<<handlerName<<L"\n";
                if(handlerName.find(L"Lost Eye.exe")!=std::wstring::npos||handlerName.find(L"LostEye.Video")!=std::wstring::npos){
                    handler->GetUIName(&rawUi);ui=rawUi?rawUi:L"";CoTaskMemFree(rawUi);
                    report<<L"HANDLER "<<handlerName<<L"\nLABEL "<<ui<<L"\n";
                    ComPtr<IShellItem> item;hr=SHCreateItemFromParsingName(argv[2],nullptr,IID_PPV_ARGS(&item));
                    if(SUCCEEDED(hr)){ComPtr<IDataObject> data;hr=item->BindToHandler(nullptr,BHID_DataObject,IID_PPV_ARGS(&data));if(SUCCEEDED(hr))hr=handler->Invoke(data.Get());}
                    report<<L"INVOKE "<<ErrorCode(hr)<<L"\n";result=SUCCEEDED(hr)?0:1;
                    // Shell activation can finish asynchronously on this STA; keep pumping before releasing COM.
                    if(SUCCEEDED(hr)){auto until=GetTickCount64()+5000;MSG msg{};while(GetTickCount64()<until){while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);}MsgWaitForMultipleObjects(0,nullptr,FALSE,50,QS_ALLINPUT);}}break;
                }
                handler.Reset();
            }
        }
        report<<L"RESULT "<<result<<L"\n";report.close();LocalFree(argv);CoUninitialize();return result;
    }
#endif
    if(path==L"--install") {
        int result=0;try{InstallCurrentUser();if(argc<3||std::wstring(argv[2])!=L"--quiet")MessageBoxW(nullptr,L"Lost Eye is ready.\n\nRight-click a video > Open with > Lost Eye.\nYou can also launch Lost Eye from Start.\n\nYour default player has not changed.",L"Lost Eye",MB_OK|MB_ICONINFORMATION);}
        catch(...){result=1;if(argc<3||std::wstring(argv[2])!=L"--quiet")MessageBoxW(nullptr,L"Lost Eye couldn't finish setup. You can still run this EXE directly. An existing different copy is never overwritten.",L"Lost Eye",MB_OK|MB_ICONERROR);}
        LocalFree(argv);CoUninitialize();return result;
    }
    if(path==L"--register-open-with"){
        int result=0;try{RegisterOpenWith(HKEY_CURRENT_USER,ExePath());SHChangeNotify(SHCNE_ASSOCCHANGED,SHCNF_IDLIST,nullptr,nullptr);if(argc<3||std::wstring(argv[2])!=L"--quiet")MessageBoxW(nullptr,L"Lost Eye is now in Windows Open with. Your default player has not changed.",L"Lost Eye",MB_OK|MB_ICONINFORMATION);}catch(HRESULT){result=1;MessageBoxW(nullptr,L"Windows couldn't register Lost Eye. Try Open with > Choose another app and select Lost Eye.exe.",L"Lost Eye",MB_OK|MB_ICONERROR);}LocalFree(argv);CoUninitialize();return result;
    }
    HANDLE single=nullptr;
#ifndef LOSTEYE_TEST
    single=CreateMutexW(nullptr,FALSE,L"Local\\LostEye.Native.Player");
    if(single&&GetLastError()==ERROR_ALREADY_EXISTS){
        HWND existing=nullptr;for(int i=0;i<20&&!existing;++i){existing=FindWindowW(WindowClass,nullptr);if(!existing)Sleep(100);}
        if(existing){if(!path.empty()){try{path=fs::absolute(path).wstring();}catch(...){}COPYDATASTRUCT data{1,static_cast<DWORD>((path.size()+1)*sizeof(wchar_t)),path.data()};DWORD_PTR result=0;SendMessageTimeoutW(existing,WM_COPYDATA,0,reinterpret_cast<LPARAM>(&data),SMTO_ABORTIFHUNG,3000,&result);}ShowWindow(existing,IsIconic(existing)?SW_RESTORE:SW_SHOW);SetForegroundWindow(existing);CloseHandle(single);LocalFree(argv);CoUninitialize();return 0;}
    }
#endif
    int result=0;
    try {
        INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_BAR_CLASSES|ICC_STANDARD_CLASSES};InitCommonControlsEx(&controls);
        WNDCLASSEXW cls{sizeof(cls)};cls.lpfnWndProc=App::Proc;cls.hInstance=instance;cls.hIcon=LoadIconW(instance,MAKEINTRESOURCEW(1));cls.hIconSm=cls.hIcon;cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);cls.lpszClassName=WindowClass;RegisterClassExW(&cls);
        WNDCLASSEXW vc{sizeof(vc)};vc.lpfnWndProc=App::VideoProc;vc.hInstance=instance;vc.hCursor=cls.hCursor;vc.lpszClassName=L"LostEye.VideoSurface";RegisterClassExW(&vc);
        App app;
#ifdef LOSTEYE_TEST
        app.fixtures=path;app.report=argc>2?argv[2]:L"native-tests.txt";app.tests.open(fs::path(app.report));
#else
        app.pending=path;
#endif
        UINT dpi=GetDpiForSystem();RECT r{0,0,MulDiv(900,dpi,96),MulDiv(560,dpi,96)};AdjustWindowRectExForDpi(&r,WS_OVERLAPPEDWINDOW,FALSE,0,dpi);
        HWND window=CreateWindowExW(WS_EX_ACCEPTFILES,WindowClass,L"Lost Eye",WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,CW_USEDEFAULT,CW_USEDEFAULT,r.right-r.left,r.bottom-r.top,nullptr,nullptr,instance,&app);
        if(!window)throw HRESULT_FROM_WIN32(GetLastError());ShowWindow(window,show);if(!app.pending.empty())app.Open(app.pending);UpdateWindow(window);app.shownAt=GetTickCount64();PostMessageW(window,WM_START,0,0);
        MSG msg{};while(GetMessageW(&msg,nullptr,0,0)>0){
            bool consumed=app.HandleInput(msg);
            if(!consumed&&!IsDialogMessageW(window,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}
        }
#ifdef LOSTEYE_TEST
        result=app.testFailures?1:0;
#endif
    }catch(HRESULT hr){MessageBoxW(nullptr,(L"Lost Eye couldn't start ("+ErrorCode(hr)+L").").c_str(),L"Lost Eye",MB_OK|MB_ICONERROR);result=1;}catch(...){MessageBoxW(nullptr,L"Lost Eye couldn't start.",L"Lost Eye",MB_OK|MB_ICONERROR);result=1;}
    if(single)CloseHandle(single);LocalFree(argv);CoUninitialize();return result;
}





