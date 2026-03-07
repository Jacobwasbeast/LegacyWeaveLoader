#include "MainMenuOverlay.h"
#include "SymbolResolver.h"
#include "LogUtil.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <string>

#define LEGACYFORGE_VERSION L"1.0.0"

namespace MainMenuOverlay
{

static int  s_modCount = 0;
static bool s_onMainMenu = false;
static bool s_loggedOnce = false;
static bool s_symbolsOk = false;

// ── Resolved PDB addresses ──────────────────────────────────────────────
static void** pMinecraftInstance = nullptr;
static void*  pRenderManager    = nullptr;

// C4JRender member functions
typedef void (__fastcall *VoidMethod_fn)(void*);
typedef void (__fastcall *MatrixMode_fn)(void*, int);
typedef void (__fastcall *MatrixOrthogonal_fn)(void*, float, float, float, float, float, float);
typedef void (__fastcall *MatrixTranslate_fn)(void*, float, float, float);
typedef void (__fastcall *BoolMethod_fn)(void*, bool);

static VoidMethod_fn       fnStartFrame        = nullptr;
static VoidMethod_fn       fnMatrixPush        = nullptr;
static VoidMethod_fn       fnMatrixPop         = nullptr;
static MatrixMode_fn       fnMatrixMode        = nullptr;
static VoidMethod_fn       fnMatrixSetIdentity = nullptr;
static MatrixOrthogonal_fn fnMatrixOrthogonal  = nullptr;
static MatrixTranslate_fn  fnMatrixTranslate   = nullptr;
static BoolMethod_fn       fnDepthTest         = nullptr;
static BoolMethod_fn       fnBlendEnable       = nullptr;
static VoidMethod_fn       fnSetMatrixDirty    = nullptr;

// Font::drawShadow(const wstring&, int x, int y, int color)
typedef void (__fastcall *FontDrawShadow_fn)(void*, const std::wstring&, int, int, int);
static FontDrawShadow_fn fnDrawShadow = nullptr;

static const int C4J_GL_MODELVIEW  = 0;
static const int C4J_GL_PROJECTION = 1;

// Minecraft instance member offsets (no vtable, no base class)
static const int OFF_WIDTH_PHYS = 32;
static const int OFF_HEIGHT_PHYS = 36;
static const int OFF_FONT = 432;  // Font* at 0x1B0

static int ComputeGuiScale(int pixelW, int pixelH)
{
    int scale = 1;
    while (pixelW / (scale + 1) >= 320 && pixelH / (scale + 1) >= 240)
        scale++;
    return scale;
}

bool ResolveSymbols(SymbolResolver& resolver)
{
    pMinecraftInstance = (void**)resolver.Resolve("?m_instance@Minecraft@@0PEAV1@EA");
    pRenderManager    = resolver.Resolve("?RenderManager@@3VC4JRender@@A");

    fnStartFrame       = (VoidMethod_fn)    resolver.Resolve("?StartFrame@C4JRender@@QEAAXXZ");
    fnMatrixPush       = (VoidMethod_fn)    resolver.Resolve("?MatrixPush@C4JRender@@QEAAXXZ");
    fnMatrixPop        = (VoidMethod_fn)    resolver.Resolve("?MatrixPop@C4JRender@@QEAAXXZ");
    fnMatrixMode       = (MatrixMode_fn)    resolver.Resolve("?MatrixMode@C4JRender@@QEAAXH@Z");
    fnMatrixSetIdentity= (VoidMethod_fn)    resolver.Resolve("?MatrixSetIdentity@C4JRender@@QEAAXXZ");
    fnMatrixOrthogonal = (MatrixOrthogonal_fn)resolver.Resolve("?MatrixOrthogonal@C4JRender@@QEAAXMMMMMM@Z");
    fnMatrixTranslate  = (MatrixTranslate_fn) resolver.Resolve("?MatrixTranslate@C4JRender@@QEAAXMMM@Z");
    fnDepthTest        = (BoolMethod_fn)    resolver.Resolve("?StateSetDepthTestEnable@C4JRender@@QEAAX_N@Z");
    fnBlendEnable      = (BoolMethod_fn)    resolver.Resolve("?StateSetBlendEnable@C4JRender@@QEAAX_N@Z");
    fnSetMatrixDirty   = (VoidMethod_fn)    resolver.Resolve("?Set_matrixDirty@C4JRender@@QEAAXXZ");

    fnDrawShadow = (FontDrawShadow_fn)resolver.Resolve(
        "?drawShadow@Font@@QEAAXAEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@HHH@Z");

    s_symbolsOk = pMinecraftInstance && pRenderManager && fnStartFrame &&
                  fnMatrixPush && fnMatrixPop && fnMatrixMode &&
                  fnMatrixSetIdentity && fnMatrixOrthogonal &&
                  fnMatrixTranslate && fnDepthTest && fnDrawShadow;

    if (s_symbolsOk)
        LogUtil::Log("[WeaveLoader] MainMenuOverlay symbols resolved OK");
    else
        LogUtil::Log("[WeaveLoader] Warning: some MainMenuOverlay symbols missing -- branding text disabled");

    return s_symbolsOk;
}

void SetModCount(int count) { s_modCount = count; }
int  GetModCount()          { return s_modCount; }

void NotifyOnMainMenu()
{
    s_onMainMenu = true;
}

void RenderBranding()
{
    if (!s_onMainMenu || !s_symbolsOk) return;
    s_onMainMenu = false;

    void* mc = *pMinecraftInstance;
    if (!mc) return;

    int pixelW = *(int*)((char*)mc + OFF_WIDTH_PHYS);
    int pixelH = *(int*)((char*)mc + OFF_HEIGHT_PHYS);
    if (pixelW <= 0 || pixelH <= 0 || pixelW > 8192 || pixelH > 8192) return;

    void* font = *(void**)((char*)mc + OFF_FONT);
    if (!font) return;

    if (!s_loggedOnce)
    {
        LogUtil::Log("[WeaveLoader] MainMenuOverlay: first render (screen %dx%d, font=%p, rm=%p)",
                     pixelW, pixelH, font, pRenderManager);
        s_loggedOnce = true;
    }

    int guiScale = ComputeGuiScale(pixelW, pixelH);
    int guiW = (int)ceil((double)pixelW / guiScale);
    int guiH = (int)ceil((double)pixelH / guiScale);

    void* rm = pRenderManager;

    // Re-initialize C4JRender's D3D11 state (shaders, render targets, etc.)
    // After GDraw's NoMoreGDrawThisFrame, C4JRender's shaders are not bound.
    fnStartFrame(rm);
    fnSetMatrixDirty(rm);

    // Set up 2D orthographic projection
    fnMatrixMode(rm, C4J_GL_PROJECTION);
    fnMatrixPush(rm);
    fnMatrixSetIdentity(rm);
    fnMatrixOrthogonal(rm, 0.0f, (float)guiW, (float)guiH, 0.0f, 1000.0f, 3000.0f);

    fnMatrixMode(rm, C4J_GL_MODELVIEW);
    fnMatrixPush(rm);
    fnMatrixSetIdentity(rm);
    fnMatrixTranslate(rm, 0.0f, 0.0f, -2000.0f);

    fnDepthTest(rm, false);
    if (fnBlendEnable) fnBlendEnable(rm, true);

    std::wstring line1 = L"WeaveLoader v" LEGACYFORGE_VERSION;

    wchar_t line2Buf[64];
    swprintf(line2Buf, 64, L"%d mod(s) loaded successfully", s_modCount);
    std::wstring line2(line2Buf);

    int textX  = 2;
    int textY1 = guiH - 20;
    int textY2 = guiH - 10;
    int color  = (int)0xFFFFFFFF;

    fnDrawShadow(font, line1, textX, textY1, color);
    fnDrawShadow(font, line2, textX, textY2, color);

    fnDepthTest(rm, true);

    fnMatrixMode(rm, C4J_GL_MODELVIEW);
    fnMatrixPop(rm);
    fnMatrixMode(rm, C4J_GL_PROJECTION);
    fnMatrixPop(rm);
}

} // namespace MainMenuOverlay
