#ifdef _WIN32
#include <d3d9.h>
#else
#include "imgui/GL/gl3w.h"
#endif

#include "imgui/imgui.h"

#include "Resources/Shaders/blur_x.h"
#include "Resources/Shaders/blur_y.h"

#include "PostProcessing.h"

class BlurEffect {
public:
#ifdef _WIN32
    static void draw(ImDrawList* drawList, float alpha, IDirect3DDevice9* device) noexcept
    {
        instance().device = device;
        instance()._draw(drawList, alpha);
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
    static void draw(ImDrawList* drawList, float alpha) noexcept
    {
        instance()._draw(drawList, alpha);
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
    ~BlurEffect()
    {
#ifdef _WIN32
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
#endif
    }

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

        const D3DMATRIX projection{{{
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            -1.0f / (backbufferWidth / blurDownsample), 1.0f / (backbufferHeight / blurDownsample), 0.0f, 1.0f
        }}};
        device->SetVertexShaderConstantF(0, &projection.m[0][0], 4);
#else
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &textureBackup);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &fboBackup);
        glGetIntegerv(GL_CURRENT_PROGRAM, &programBackup);

        if (!frameBuffer)
            glGenFramebuffers(1, &frameBuffer);

        glViewport(0, 0, backbufferWidth / blurDownsample, backbufferHeight / blurDownsample);
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

    void _draw(ImDrawList* drawList, float alpha) noexcept
    {
        createTextures();
        createShaders();

        if (!blurTexture1 || !blurTexture2 || !blurShaderX || !blurShaderY)
            return;

        drawList->AddCallback(&begin, nullptr);
        for (int i = 0; i < 8; ++i) {
            drawList->AddCallback(&firstPass, nullptr);
            drawList->AddImage(reinterpret_cast<ImTextureID>(blurTexture1), { -1.0f, -1.0f }, { 1.0f, 1.0f });
            drawList->AddCallback(&secondPass, nullptr);
            drawList->AddImage(reinterpret_cast<ImTextureID>(blurTexture2), { -1.0f, -1.0f }, { 1.0f, 1.0f });
        }
        drawList->AddCallback(&end, nullptr);
        drawList->AddCallback(ImDrawCallback_ResetRenderState, nullptr);

#ifdef _WIN32
        drawList->AddImage(reinterpret_cast<ImTextureID>(blurTexture1), { 0.0f, 0.0f }, { backbufferWidth * 1.0f, backbufferHeight * 1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f }, IM_COL32(255, 255, 255, 255 * alpha));
#else
        drawList->AddImage(reinterpret_cast<ImTextureID>(blurTexture1), { 0.0f, 0.0f }, { backbufferWidth * 1.0f, backbufferHeight * 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f }, IM_COL32(255, 255, 255, 255 * alpha));
#endif
    }
};

#ifdef _WIN32
void PostProcessing::performFullscreenBlur(ImDrawList* drawList, float alpha, IDirect3DDevice9* device) noexcept
{
    BlurEffect::draw(drawList, alpha, device);
}

void PostProcessing::clearBlurTextures() noexcept
{
    BlurEffect::clearTextures();
}

#else
void PostProcessing::performFullscreenBlur(ImDrawList* drawList, float alpha) noexcept
{
    BlurEffect::draw(drawList, alpha);
}
#endif
