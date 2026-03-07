#pragma once

class SymbolResolver;

namespace FurnaceRecipeRegistry
{
    bool ResolveSymbols(SymbolResolver& resolver);
    bool AddRecipe(int inputId, int outputId, float xp);
}
