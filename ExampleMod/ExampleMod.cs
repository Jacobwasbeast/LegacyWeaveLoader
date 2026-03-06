using LegacyForge.API;
using LegacyForge.API.Block;
using LegacyForge.API.Item;
using LegacyForge.API.Events;

namespace ExampleMod;

[Mod("examplemod", Name = "Example Mod", Version = "1.0.0", Author = "LegacyForge",
     Description = "A sample mod demonstrating the LegacyForge API")]
public class ExampleMod : IMod
{
    public static RegisteredBlock? RubyOre;
    public static RegisteredItem? Ruby;

    public void OnInitialize()
    {
        RubyOre = Registry.Block.Register("examplemod:ruby_ore",
            new BlockProperties()
                .Material(MaterialType.Stone)
                .Hardness(3.0f)
                .Resistance(15f)
                .Sound(SoundType.Stone)
                .Icon("ruby_ore"));

        Ruby = Registry.Item.Register("examplemod:ruby",
            new ItemProperties().MaxStackSize(64));

        Registry.Recipe.AddFurnace("examplemod:ruby_ore", "examplemod:ruby", 1.0f);

        GameEvents.OnBlockBreak += OnBlockBroken;

        Logger.Info("Example Mod initialized! Ruby ore and ruby registered.");
    }

    private void OnBlockBroken(object? sender, BlockBreakEventArgs e)
    {
        if (RubyOre != null && e.BlockId == RubyOre.StringId.ToString())
        {
            Logger.Info($"Ruby ore broken at ({e.X}, {e.Y}, {e.Z})!");
        }
    }

    public void OnTick()
    {
        // Per-tick logic goes here
    }

    public void OnShutdown()
    {
        Logger.Info("Example Mod shutting down.");
    }
}
