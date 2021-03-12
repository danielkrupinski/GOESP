#include "imgui/imgui.h"

#include "EventListener.h"
#include "GameData.h"
#include "GUI.h"
#include "Hacks/ESP.h"
#include "Hacks/Misc.h"
#include "Hooks.h"
#include "Interfaces.h"
#include "Memory.h"

#include "SDK/Engine.h"
#include "SDK/GlobalVars.h"
#include "SDK/InputSystem.h"

#ifdef _WIN32
#include <intrin.h>

#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"

#include "Resources/Shaders/blur_x.h"
#include "Resources/Shaders/blur_y.h"

#elif __linux__
#include <SDL2/SDL.h>

#include "imgui/GL/gl3w.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"
#endif

#ifdef _WIN32
LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static LRESULT WINAPI wndProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
    if (hooks->getState() == Hooks::State::NotInstalled)
        hooks->install();

    if (hooks->getState() == Hooks::State::Installed) {
        GameData::update();

        ImGui_ImplWin32_WndProcHandler(window, msg, wParam, lParam);
        interfaces->inputSystem->enableInput(!gui->isOpen());
    }

    return CallWindowProcW(hooks->wndProc, window, msg, wParam, lParam);
}

static void clearBlurTexture() noexcept;

static HRESULT D3DAPI reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params) noexcept
{
    GameData::clearTextures();
    clearBlurTexture();
    ImGui_ImplDX9_InvalidateDeviceObjects();
    return hooks->reset(device, params);
}

class BlurEffect {
public:
#ifdef _WIN32
    static void draw(ImDrawList* drawList, IDirect3DDevice9* device) noexcept
    {
        instance().device = device;
        instance()._draw(drawList);
    }
#endif

    static void clearTextures() noexcept
    {
        instance()._clearTextures();
    }

private:
    IDirect3DDevice9* device = nullptr; // DO NOT RELEASE!
    IDirect3DSurface9* rtBackup = nullptr;
    IDirect3DPixelShader9* blurShaderX = nullptr;
    IDirect3DPixelShader9* blurShaderY = nullptr;
    IDirect3DTexture9* blurTexture1 = nullptr;
    IDirect3DTexture9* blurTexture2 = nullptr;

    int backbufferWidth = 0;
    int backbufferHeight = 0;
    static constexpr auto blurDownsample = 4;

    BlurEffect() = default;
    ~BlurEffect()
    {
        if (rtBackup)
            rtBackup->Release();
        if (blurShaderX)
            blurShaderX->Release();
        if (blurShaderY)
            blurShaderY->Release();
        if (blurTexture1)
            blurTexture1->Release();
        if (blurTexture2)
            blurTexture2->Release();
    }

    static BlurEffect& instance() noexcept
    {
        static BlurEffect blurEffect;
        return blurEffect;
    }

    void _clearTextures() noexcept
    {
        if (blurTexture1) {
            blurTexture1->Release();
            blurTexture1 = nullptr;
        }
        if (blurTexture2) {
            blurTexture2->Release();
            blurTexture2 = nullptr;
        }
    }

    static void begin(const ImDrawList*, const ImDrawCmd*) noexcept
    {
        instance()._begin();
    }

    static void firstPass(const ImDrawList*, const ImDrawCmd*) noexcept
    {
        instance()._firstPass();
    }

    static void secondPass(const ImDrawList*, const ImDrawCmd*) noexcept
    {
        instance()._secondPass();
    }

    static void end(const ImDrawList*, const ImDrawCmd*) noexcept
    {
        instance()._end();
    }

    void _begin() noexcept
    {
        if (!blurShaderX)
            device->CreatePixelShader(reinterpret_cast<const DWORD*>(Resource::blur_x.data()), &blurShaderX);

        if (!blurShaderY)
            device->CreatePixelShader(reinterpret_cast<const DWORD*>(Resource::blur_y.data()), &blurShaderY);

        IDirect3DSurface9* backBuffer;
        device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);
        D3DSURFACE_DESC desc;
        backBuffer->GetDesc(&desc);

        if (backbufferWidth != desc.Width || backbufferHeight != desc.Height) {
            clearBlurTexture();

            backbufferWidth = desc.Width;
            backbufferHeight = desc.Height;
        }

        if (!blurTexture1)
            device->CreateTexture(desc.Width / blurDownsample, desc.Height / blurDownsample, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &blurTexture1, nullptr);

        if (!blurTexture2)
            device->CreateTexture(desc.Width / blurDownsample, desc.Height / blurDownsample, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &blurTexture2, nullptr);

        device->GetRenderTarget(0, &rtBackup);

        {
            IDirect3DSurface9* surface;
            blurTexture1->GetSurfaceLevel(0, &surface);
            device->StretchRect(backBuffer, NULL, surface, NULL, D3DTEXF_LINEAR);
            surface->Release();
        }

        {
            IDirect3DSurface9* surface;
            blurTexture2->GetSurfaceLevel(0, &surface);
            device->StretchRect(backBuffer, NULL, surface, NULL, D3DTEXF_LINEAR);
            surface->Release();
        }

        backBuffer->Release();

        device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
        device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
        device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
        device->SetRenderState(D3DRS_SCISSORTESTENABLE, false);
    }

    void _firstPass() noexcept
    {
        {
            IDirect3DSurface9* surface;
            blurTexture2->GetSurfaceLevel(0, &surface);
            device->SetRenderTarget(0, surface);
            surface->Release();
        }

        device->SetPixelShader(blurShaderX);
        const float params[4] = { 1.0f / (backbufferWidth / blurDownsample) };
        device->SetPixelShaderConstantF(0, params, 1);
    }

    void _secondPass() noexcept
    {
        {
            IDirect3DSurface9* surface;
            blurTexture1->GetSurfaceLevel(0, &surface);
            device->SetRenderTarget(0, surface);
            surface->Release();
        }

        device->SetPixelShader(blurShaderY);
        const float params[4] = { 1.0f / (backbufferHeight / blurDownsample) };
        device->SetPixelShaderConstantF(0, params, 1);
    }

    void _end() noexcept
    {
        device->SetRenderTarget(0, rtBackup);
        rtBackup->Release();

        device->SetPixelShader(nullptr);
        device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        device->SetRenderState(D3DRS_SCISSORTESTENABLE, true);
    }

    void _draw(ImDrawList* drawList) noexcept
    {
        drawList->AddCallback(&BlurEffect::begin, nullptr);

        for (int i = 0; i < 8; ++i) {
            drawList->AddCallback(&BlurEffect::firstPass, nullptr);
            drawList->AddImage(blurTexture1, { 0.0f, 0.0f }, { backbufferWidth * 1.0f, backbufferHeight * 1.0f });
            drawList->AddCallback(&BlurEffect::secondPass, nullptr);
            drawList->AddImage(blurTexture2, { 0.0f, 0.0f }, { backbufferWidth * 1.0f, backbufferHeight * 1.0f });
        }

        drawList->AddCallback(&BlurEffect::end, nullptr);
        drawList->AddImage(blurTexture1, { 0.0f, 0.0f }, { backbufferWidth * 1.0f, backbufferHeight * 1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f }, IM_COL32(255, 255, 255, 255 * gui->getTransparency()));
    }
};

static void clearBlurTexture() noexcept
{
    BlurEffect::clearTextures();
}

static HRESULT D3DAPI present(IDirect3DDevice9* device, const RECT* src, const RECT* dest, HWND windowOverride, const RGNDATA* dirtyRegion) noexcept
{
    [[maybe_unused]] static const auto _ = ImGui_ImplDX9_Init(device);

    if (ESP::loadScheduledFonts())
        ImGui_ImplDX9_DestroyFontsTexture();

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    Misc::drawPreESP(ImGui::GetBackgroundDrawList());
    ESP::render();
    Misc::drawPostESP(ImGui::GetBackgroundDrawList());
    gui->render();
    gui->handleToggle();

    if (!gui->isFullyClosed())
        BlurEffect::draw(ImGui::GetBackgroundDrawList(), device);

    ImGui::Render();

    if (device->BeginScene() == D3D_OK) {
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
        device->EndScene();
    }

    GameData::clearUnusedAvatars();

    return hooks->present(device, src, dest, windowOverride, dirtyRegion);
}

static BOOL WINAPI setCursorPos(int X, int Y) noexcept
{
    if (gui->isOpen()) {
        POINT p;
        GetCursorPos(&p);
        X = p.x;
        Y = p.y;
    }

    return hooks->setCursorPos(X, Y);
}

Hooks::Hooks(HMODULE moduleHandle) noexcept : moduleHandle{ moduleHandle }
{
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

    window = FindWindowW(L"Valve001", nullptr);
}

#elif __linux__

static int pollEvent(SDL_Event* event) noexcept
{
    if (hooks->getState() == Hooks::State::NotInstalled)
        hooks->install();

    const auto result = hooks->pollEvent(event);

    if (hooks->getState() == Hooks::State::Installed) {
        GameData::update();
        if (result && ImGui_ImplSDL2_ProcessEvent(event) && gui->isOpen())
            event->type = 0;
    }

    return result;
}

static void swapWindow(SDL_Window* window) noexcept
{
    static const auto _ = ImGui_ImplSDL2_InitForOpenGL(window, nullptr);

    if (ESP::loadScheduledFonts()) {
        ImGui_ImplOpenGL3_DestroyDeviceObjects();
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);

    ImGui::NewFrame();

    if (const auto& displaySize = ImGui::GetIO().DisplaySize; displaySize.x > 0.0f && displaySize.y > 0.0f) {
        Misc::drawPreESP(ImGui::GetBackgroundDrawList());
        ESP::render();
        Misc::drawPostESP(ImGui::GetBackgroundDrawList());

        gui->render();
        gui->handleToggle();
    }

    ImGui::EndFrame();
    ImGui::Render();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    GameData::clearUnusedAvatars();

    hooks->swapWindow(window);
}

Hooks::Hooks() noexcept
{
    interfaces = std::make_unique<const Interfaces>();
    memory = std::make_unique<const Memory>();
}

static void warpMouseInWindow(SDL_Window* window, int x, int y) noexcept
{
    if (!gui->isOpen())
        hooks->warpMouseInWindow(window, x, y);
}

#elif __APPLE__
Hooks::Hooks() noexcept
{
    interfaces = std::make_unique<const Interfaces>();
    memory = std::make_unique<const Memory>();
}
#endif

void Hooks::setup() noexcept
{
#ifdef _WIN32
    wndProc = WNDPROC(SetWindowLongPtrW(window, GWLP_WNDPROC, LONG_PTR(&::wndProc)));
#elif __linux__
    pollEvent = *reinterpret_cast<decltype(pollEvent)*>(memory->pollEvent);
    *reinterpret_cast<decltype(::pollEvent)**>(memory->pollEvent) = ::pollEvent;
#endif
}

void Hooks::install() noexcept
{
    state = State::Installing;

#ifndef __linux__
    interfaces = std::make_unique<const Interfaces>();
    memory = std::make_unique<const Memory>();
#endif

    eventListener = std::make_unique<EventListener>();

    ImGui::CreateContext();
#ifdef _WIN32
    ImGui_ImplWin32_Init(window);
#elif __linux__
    gl3wInit();
    ImGui_ImplOpenGL3_Init();
#endif
    gui = std::make_unique<GUI>();

#ifdef _WIN32
    reset = *reinterpret_cast<decltype(reset)*>(memory->reset);
    *reinterpret_cast<decltype(::reset)**>(memory->reset) = ::reset;

    present = *reinterpret_cast<decltype(present)*>(memory->present);
    *reinterpret_cast<decltype(::present)**>(memory->present) = ::present;

    setCursorPos = *reinterpret_cast<decltype(setCursorPos)*>(memory->setCursorPos);
    *reinterpret_cast<decltype(::setCursorPos)**>(memory->setCursorPos) = ::setCursorPos;
#elif __linux__
    swapWindow = *reinterpret_cast<decltype(swapWindow)*>(memory->swapWindow);
    *reinterpret_cast<decltype(::swapWindow)**>(memory->swapWindow) = ::swapWindow;

    /*
    warpMouseInWindow = *reinterpret_cast<decltype(warpMouseInWindow)*>(memory->warpMouseInWindow);
    *reinterpret_cast<decltype(::warpMouseInWindow)**>(memory->warpMouseInWindow) = ::warpMouseInWindow;
    */
#endif

    state = State::Installed;
}

#ifdef _WIN32

extern "C" BOOL WINAPI _CRT_INIT(HMODULE moduleHandle, DWORD reason, LPVOID reserved);

static DWORD WINAPI waitOnUnload(HMODULE hModule) noexcept
{
    Sleep(50);

    interfaces->inputSystem->enableInput(true);
    eventListener->remove();

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    _CRT_INIT(hModule, DLL_PROCESS_DETACH, nullptr);
    FreeLibraryAndExitThread(hModule, 0);
}

#endif

void Hooks::uninstall() noexcept
{
#ifdef _WIN32

    *reinterpret_cast<decltype(reset)*>(memory->reset) = reset;
    *reinterpret_cast<decltype(present)*>(memory->present) = present;
    *reinterpret_cast<decltype(setCursorPos)*>(memory->setCursorPos) = setCursorPos;

    SetWindowLongPtrW(window, GWLP_WNDPROC, LONG_PTR(wndProc));

    if (HANDLE thread = CreateThread(nullptr, 0, LPTHREAD_START_ROUTINE(waitOnUnload), moduleHandle, 0, nullptr))
        CloseHandle(thread);

#elif __linux__

    *reinterpret_cast<decltype(pollEvent)*>(memory->pollEvent) = pollEvent;
    *reinterpret_cast<decltype(swapWindow)*>(memory->swapWindow) = swapWindow;
    // *reinterpret_cast<decltype(warpMouseInWindow)*>(memory->warpMouseInWindow) = warpMouseInWindow;

#endif
}
