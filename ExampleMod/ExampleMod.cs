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
    public static RegisteredBlock? Orichalcum;
    public static RegisteredItem? Ruby;
    public static RegisteredItem? RubyPickaxeItem;
    public static RegisteredItem? RubyShovelItem;
    public static RegisteredItem? RubyHoeItem;
    public static RegisteredItem? RubyAxeItem;
    public static RegisteredItem? RubySwordItem;
    public static RegisteredItem? RubyWandItem;

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

    private sealed class RubyShovel : ShovelItem
    {
    }

    private sealed class RubyPickaxe : PickaxeItem
    {
        public override MineBlockResult OnMineBlock(MineBlockContext context)
        {
            Logger.Info($"RubyPickaxe mined tile={context.TileId} at ({context.X}, {context.Y}, {context.Z})");
            return base.OnMineBlock(context);
        }
    }

    private sealed class RubyAxe : AxeItem
    {
    }

    private sealed class RubyHoe : HoeItem
    {
    }

    private sealed class RubySword : SwordItem
    {
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
                .RequiredHarvestLevel(2)
                .RequiredTool(ToolType.Pickaxe)
                .InCreativeTab(CreativeTab.BuildingBlocks));

        Orichalcum = Registry.Block.Register("examplemod:orichalcum_ore",
            new BlockProperties()
                .Material(MaterialType.Metal)
                .Hardness(5.0f)
                .Resistance(30f)
                .Sound(SoundType.Metal)
                .Icon("examplemod:orichalcum_ore")  // From assets/blocks/orichalcum.png
                .Name("Orichalcum Ore")
                .RequiredHarvestLevel(4)
                .RequiredTool(ToolType.Pickaxe)
                .InCreativeTab(CreativeTab.BuildingBlocks));

        Ruby = Registry.Item.Register("examplemod:ruby",
            new ItemProperties()
                .MaxStackSize(64)
                .Icon("examplemod:ruby")  // From assets/items/ruby.png
                .Name("Ruby")
                .InCreativeTab(CreativeTab.Materials));

        Registry.Item.RegisterToolMaterial("examplemod:ruby_material",
            new ToolMaterialDefinition()
                .BaseTier(ToolTier.Diamond)
                .HarvestLevel(4)
                .DestroySpeed(10.0f));

        RubySwordItem = Registry.Item.Register("examplemod:ruby_sword", new RubySword { CustomMaterialId = "examplemod:ruby_material" },
            new ItemProperties()
                .MaxStackSize(1)
                .MaxDamage(512)
                .AttackDamage(8.0f)
                .Icon("examplemod:ruby_sword")
                .Name("Ruby Sword")
                .InCreativeTab(CreativeTab.ToolsAndWeapons));

        RubyShovelItem = Registry.Item.Register("examplemod:ruby_shovel", new RubyShovel { CustomMaterialId = "examplemod:ruby_material" },
            new ItemProperties()
                .MaxStackSize(1)
                .MaxDamage(512)
                .AttackDamage(4.5f)
                .Icon("examplemod:ruby_shovel")
                .Name("Ruby Shovel")
                .InCreativeTab(CreativeTab.ToolsAndWeapons));

        RubyPickaxeItem = Registry.Item.Register("examplemod:ruby_pickaxe", new RubyPickaxe { CustomMaterialId = "examplemod:ruby_material" },
            new ItemProperties()
                .MaxStackSize(1)
                .MaxDamage(512)
                .AttackDamage(5.0f)
                .Icon("examplemod:ruby_pickaxe")  // From assets/items/ruby_pickaxe.png
                .Name("Ruby Pickaxe")
                .InCreativeTab(CreativeTab.ToolsAndWeapons));

        RubyAxeItem = Registry.Item.Register("examplemod:ruby_axe", new RubyAxe { CustomMaterialId = "examplemod:ruby_material" },
            new ItemProperties()
                .MaxStackSize(1)
                .MaxDamage(512)
                .AttackDamage(7.0f)
                .Icon("examplemod:ruby_axe")
                .Name("Ruby Axe")
                .InCreativeTab(CreativeTab.ToolsAndWeapons));

        RubyHoeItem = Registry.Item.Register("examplemod:ruby_hoe", new RubyHoe { CustomMaterialId = "examplemod:ruby_material" },
            new ItemProperties()
                .MaxStackSize(1)
                .MaxDamage(512)
                .AttackDamage(1.0f)
                .Icon("examplemod:ruby_hoe")
                .Name("Ruby Hoe")
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
