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

class BlurEffect {
public:
#ifdef _WIN32
    static void draw(ImDrawList* drawList, IDirect3DDevice9* device) noexcept
    {
        instance().device = device;
        instance()._draw(drawList);
    }

    static void clearTextures() noexcept
    {
        if (instance().blurTexture1) {
            instance().blurTexture1->Release();
            instance().blurTexture1 = nullptr;
        }
        if (instance().blurTexture2) {
            instance().blurTexture2->Release();
            instance().blurTexture2 = nullptr;
        }
    }

#else
    static void draw(ImDrawList* drawList) noexcept
    {
        instance()._draw(drawList);
    }

    static void clearTextures() noexcept
    {
        if (instance().blurTexture1) {
            glDeleteTextures(1, &instance().blurTexture1);
            instance().blurTexture1 = 0;
        }
        if (instance().blurTexture2) {
            glDeleteTextures(1, &instance().blurTexture2);
            instance().blurTexture2 = 0;
        }
    }
#endif
private:
#ifdef _WIN32
    IDirect3DDevice9* device = nullptr; // DO NOT RELEASE!
    IDirect3DSurface9* rtBackup = nullptr;
    IDirect3DPixelShader9* blurShaderX = nullptr;
    IDirect3DPixelShader9* blurShaderY = nullptr;
    IDirect3DTexture9* blurTexture1 = nullptr;
    IDirect3DTexture9* blurTexture2 = nullptr;
#else
    GLint textureBackup = 0;
    GLint fboBackup = 0;
    GLint programBackup = 0;

    GLuint blurTexture1 = 0;
    GLuint blurTexture2 = 0;
    GLuint frameBuffer = 0;
    GLuint blurShaderX = 0;
    GLuint blurShaderY = 0;
#endif

    bool shadersInitialized = false;
    int backbufferWidth = 0;
    int backbufferHeight = 0;
    static constexpr auto blurDownsample = 4;

    BlurEffect() = default;

    static BlurEffect& instance() noexcept
    {
        static BlurEffect blurEffect;
        return blurEffect;
    }

    static void begin(const ImDrawList*, const ImDrawCmd*) noexcept { instance()._begin(); }
    static void firstPass(const ImDrawList*, const ImDrawCmd*) noexcept { instance()._firstPass(); }
    static void secondPass(const ImDrawList*, const ImDrawCmd*) noexcept { instance()._secondPass(); }
    static void end(const ImDrawList*, const ImDrawCmd*) noexcept { instance()._end(); }

#ifdef _WIN32
    [[nodiscard]] IDirect3DTexture9* createTexture() const noexcept
    {
        IDirect3DTexture9* texture;
        device->CreateTexture(backbufferWidth / blurDownsample, backbufferHeight / blurDownsample, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &texture, nullptr);
        return texture;
    }
#else
    [[nodiscard]] GLuint createTexture() const noexcept
    {
        GLint lastTexture;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &lastTexture);

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, backbufferWidth / blurDownsample, backbufferHeight / blurDownsample, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

        glBindTexture(GL_TEXTURE_2D, lastTexture);
        return texture;
    }
#endif

    void createTextures() noexcept
    {
        if (const auto [width, height] = ImGui::GetIO().DisplaySize; backbufferWidth != static_cast<int>(width) || backbufferHeight != static_cast<int>(height)) {
            clearTextures();
            backbufferWidth = static_cast<int>(width);
            backbufferHeight = static_cast<int>(height);
        }

        if (!blurTexture1)
            blurTexture1 = createTexture();
        if (!blurTexture2)
            blurTexture2 = createTexture();
    }

    void createShaders() noexcept
    {
        if (shadersInitialized)
            return;
        shadersInitialized = true;

#ifdef _WIN32
        device->CreatePixelShader(reinterpret_cast<const DWORD*>(Resource::blur_x.data()), &blurShaderX);
        device->CreatePixelShader(reinterpret_cast<const DWORD*>(Resource::blur_y.data()), &blurShaderY);
#else
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        constexpr const GLchar* vsSource =
            #include "Resources/Shaders/blur.glsl"
        ;
        glShaderSource(vertexShader, 1, &vsSource, nullptr);
        glCompileShader(vertexShader);

        GLuint fragmentShaderX = glCreateShader(GL_FRAGMENT_SHADER);
        constexpr const GLchar* fsSourceX =
            #include "Resources/Shaders/blur_x.glsl"
        ;
        glShaderSource(fragmentShaderX, 1, &fsSourceX, nullptr);
        glCompileShader(fragmentShaderX);
            
        GLuint fragmentShaderY = glCreateShader(GL_FRAGMENT_SHADER);
        constexpr const GLchar* fsSourceY =
            #include "Resources/Shaders/blur_y.glsl"
        ;
        glShaderSource(fragmentShaderY, 1, &fsSourceY, nullptr);
        glCompileShader(fragmentShaderY);

        blurShaderX = glCreateProgram();
        glAttachShader(blurShaderX, vertexShader);
        glAttachShader(blurShaderX, fragmentShaderX);
        glLinkProgram(blurShaderX);

        blurShaderY = glCreateProgram();
        glAttachShader(blurShaderY, vertexShader);
        glAttachShader(blurShaderY, fragmentShaderY);
        glLinkProgram(blurShaderY);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShaderX);
        glDeleteShader(fragmentShaderY);
#endif
    }

    void _begin() noexcept
    {
#ifdef _WIN32
        device->GetRenderTarget(0, &rtBackup);

        {
            IDirect3DSurface9* backBuffer;
            device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);

            IDirect3DSurface9* surface;
            blurTexture1->GetSurfaceLevel(0, &surface);
            device->StretchRect(backBuffer, NULL, surface, NULL, D3DTEXF_LINEAR);

            surface->Release();
            backBuffer->Release();
        }

        device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
        device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
        device->SetRenderState(D3DRS_SCISSORTESTENABLE, false);

        constexpr D3DMATRIX identity{ { { 1.0f, 0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 0.0f, 1.0f } } };
        device->SetTransform(D3DTS_PROJECTION, &identity);
#else
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &textureBackup);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &fboBackup);
        glGetIntegerv(GL_CURRENT_PROGRAM, &programBackup);

        if (!frameBuffer)
            glGenFramebuffers(1, &frameBuffer);

        glViewport(0,0, backbufferWidth / blurDownsample, backbufferHeight / blurDownsample);
        glDisable(GL_SCISSOR_TEST);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBuffer);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blurTexture1, 0);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, blurTexture2, 0);
        glReadBuffer(GL_BACK);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glBlitFramebuffer(0, 0, backbufferWidth, backbufferHeight, 0, 0, backbufferWidth / blurDownsample, backbufferHeight / blurDownsample, GL_COLOR_BUFFER_BIT, GL_LINEAR);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, pos));
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, uv));
#endif
    }

    void _firstPass() noexcept
    {
#ifdef _WIN32
        {
            IDirect3DSurface9* surface;
            blurTexture2->GetSurfaceLevel(0, &surface);
            device->SetRenderTarget(0, surface);
            surface->Release();
        }

        device->SetPixelShader(blurShaderX);
        const float params[4] = { 1.0f / (backbufferWidth / blurDownsample) };
        device->SetPixelShaderConstantF(0, params, 1);
#else
        glDrawBuffer(GL_COLOR_ATTACHMENT1);

        glUseProgram(blurShaderX);
        glUniform1i(0, 0);
        glUniform1f(1, 1.0f / (backbufferWidth / blurDownsample));
#endif
    }

    void _secondPass() noexcept
    {
#ifdef _WIN32
        {
            IDirect3DSurface9* surface;
            blurTexture1->GetSurfaceLevel(0, &surface);
            device->SetRenderTarget(0, surface);
            surface->Release();
        }

        device->SetPixelShader(blurShaderY);
        const float params[4] = { 1.0f / (backbufferHeight / blurDownsample) };
        device->SetPixelShaderConstantF(0, params, 1);
#else
        glDrawBuffer(GL_COLOR_ATTACHMENT0);

        glUseProgram(blurShaderY);
        glUniform1i(0, 0);
        glUniform1f(1, 1.0f / (backbufferHeight / blurDownsample));
#endif
    }

    void _end() noexcept
    {
#ifdef _WIN32
        device->SetRenderTarget(0, rtBackup);
        rtBackup->Release();

        device->SetPixelShader(nullptr);
        device->SetRenderState(D3DRS_SCISSORTESTENABLE, true);
#else
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboBackup);
        glUseProgram(programBackup);
        glBindTexture(GL_TEXTURE_2D, textureBackup);
#endif
    }

    void _draw(ImDrawList* drawList) noexcept
    {
        createTextures();
        createShaders();

        if (!blurTexture1 || !blurTexture2 || !blurShaderX || !blurShaderY)
            return;

#ifdef _WIN32
        // half-pixel offset for dx9
        const float offsetX = -1.0f / (backbufferWidth / blurDownsample);
        const float offsetY = 1.0f / (backbufferHeight / blurDownsample);
#else
        constexpr auto offsetX = 0.0f;
        constexpr auto offsetY = 0.0f;
#endif

        drawList->AddCallback(&begin, nullptr);
        for (int i = 0; i < 8; ++i) {
            drawList->AddCallback(&firstPass, nullptr);
            drawList->AddImage(reinterpret_cast<ImTextureID>(blurTexture1), { -1.0f + offsetX, -1.0f + offsetY }, { 1.0f + offsetX, 1.0f + offsetY });
            drawList->AddCallback(&secondPass, nullptr);
            drawList->AddImage(reinterpret_cast<ImTextureID>(blurTexture2), { -1.0f + offsetX, -1.0f + offsetY }, { 1.0f + offsetX, 1.0f + offsetY });
        }
        drawList->AddCallback(&end, nullptr);
        drawList->AddCallback(ImDrawCallback_ResetRenderState, nullptr);

#ifdef _WIN32
        drawList->AddImage(reinterpret_cast<ImTextureID>(blurTexture1), { 0.0f, 0.0f }, { backbufferWidth * 1.0f, backbufferHeight * 1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f }, IM_COL32(255, 255, 255, 255 * gui->getTransparency()));
#else
        drawList->AddImage(reinterpret_cast<ImTextureID>(blurTexture1), { 0.0f, 0.0f }, { backbufferWidth * 1.0f, backbufferHeight * 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f }, IM_COL32(255, 255, 255, 255 * gui->getTransparency()));
#endif
    }
};

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

static HRESULT D3DAPI reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params) noexcept
{
    GameData::clearTextures();
    BlurEffect::clearTextures();
    ImGui_ImplDX9_InvalidateDeviceObjects();
    return hooks->reset(device, params);
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

    if (!gui->isFullyClosed())
      BlurEffect::draw(ImGui::GetBackgroundDrawList());

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

#endif
}
