#pragma once

class SymbolResolver;

namespace MainMenuOverlay
{
    bool ResolveSymbols(SymbolResolver& resolver);

    // Called from the customDraw hook to signal we're on the main menu this frame
    void NotifyOnMainMenu();

    // Called from the Present hook, draws branding if on the main menu
    void RenderBranding();

    void SetModCount(int count);
    int  GetModCount();
}
