using WeaveLoader.API;
using WeaveLoader.API.Block;
using WeaveLoader.API.Item;
using WeaveLoader.API.Events;

namespace ExampleMod;

[Mod("examplemod", Name = "Example Mod", Version = "1.0.0", Author = "WeaveLoader",
     Description = "A sample mod demonstrating the WeaveLoader API")]
public class ExampleMod : IMod
{
    public static RegisteredBlock? RubyOre;
    public static RegisteredItem? Ruby;
    public static RegisteredItem? RubyPickaxeItem;

    private sealed class RubyPickaxe : PickaxeItem
    {
        public override MineBlockResult OnMineBlock(MineBlockContext context)
        {
            Logger.Info($"RubyPickaxe mined tile={context.TileId} at ({context.X}, {context.Y}, {context.Z})");
            return base.OnMineBlock(context);
        }
    }

    public void OnInitialize()
    {
        RubyOre = Registry.Block.Register("examplemod:ruby_ore",
            new BlockProperties()
                .Material(MaterialType.Stone)
                .Hardness(3.0f)
                .Resistance(15f)
                .Sound(SoundType.Stone)
                .Icon("examplemod:ruby_ore")  // From assets/blocks/ruby_ore.png
                .Name("Ruby Ore")
                .InCreativeTab(CreativeTab.BuildingBlocks));

        Ruby = Registry.Item.Register("examplemod:ruby",
            new ItemProperties()
                .MaxStackSize(64)
                .Icon("examplemod:ruby")  // From assets/items/ruby.png
                .Name("Ruby")
                .InCreativeTab(CreativeTab.Materials));

        RubyPickaxeItem = Registry.Item.Register("examplemod:ruby_pickaxe", new RubyPickaxe(),
            new ItemProperties()
                .MaxStackSize(1)
                .MaxDamage(512)
                .Icon("examplemod:ruby_pickaxe")  // From assets/items/ruby_pickaxe.png
                .Name("Ruby Pickaxe")
                .InCreativeTab(CreativeTab.ToolsAndWeapons));

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
