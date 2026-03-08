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
    public static RegisteredItem? RubyWandItem;

    private sealed class RubyPickaxe : PickaxeItem
    {
        public override MineBlockResult OnMineBlock(MineBlockContext context)
        {
            Logger.Info($"RubyPickaxe mined tile={context.TileId} at ({context.X}, {context.Y}, {context.Z})");
            return base.OnMineBlock(context);
        }
    }

    private sealed class RubyWand : Item
    {
        private const long CooldownMs = 1500;
        private long _nextClientUseAtMs;
        private long _nextServerUseAtMs;

        public override UseItemResult OnUseItem(UseItemContext context)
        {
            if (context.IsTestUseOnly)
                return UseItemResult.CancelVanilla;

            long now = Environment.TickCount64;
            ref long nextUseAtMs = ref context.IsClientSide ? ref _nextClientUseAtMs : ref _nextServerUseAtMs;
            if (now < nextUseAtMs)
            {
                long remaining = nextUseAtMs - now;
                Logger.Info($"RubyWand is cooling down ({remaining}ms remaining)");
                return UseItemResult.CancelVanilla;
            }

            if (!context.ConsumeInventoryItem("minecraft:gunpowder", 1))
            {
                Logger.Info("RubyWand needs gunpowder.");
                return UseItemResult.CancelVanilla;
            }

            if (context.IsClientSide)
            {
                context.DamageItem(10);
                nextUseAtMs = now + CooldownMs;
                return UseItemResult.ContinueVanilla;
            }

            bool spawned = context.SpawnEntityFromLook("minecraft:wither_skull", speed: 1.4, spawnForward: 1.0, spawnUp: 1.2);
            if (!spawned)
            {
                Logger.Info("RubyWand failed to spawn fireball.");
                return UseItemResult.CancelVanilla;
            }

            context.DamageItem(10);
            Logger.Info($"RubyWand cast fireball! (item={context.ItemId})");
            nextUseAtMs = now + CooldownMs;
            return UseItemResult.CancelVanilla;
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

        RubyWandItem = Registry.Item.Register("examplemod:ruby_wand", new RubyWand(),
            new ItemProperties()
                .MaxStackSize(1)
                .Icon("examplemod:ruby_wand")  // From assets/items/ruby_wand.png
                .Name("Ruby Wand")
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
