using WeaveLoader.API;
using WeaveLoader.API.Block;
using WeaveLoader.API.Item;
using WeaveLoader.API.Events;
using WeaveLoader.API.Loot;

namespace ExampleMod;

[Mod("examplemod", Name = "Example Mod", Version = "1.0.0", Author = "WeaveLoader",
     Description = "A sample mod demonstrating the WeaveLoader API")]
public class ExampleMod : IMod
{
    private static nint s_currentLevel;
    private static bool s_hasLevel;
    private static bool s_loggedCreeperLootInjection;
    public static RegisteredBlock? RubyOre;
    public static RegisteredBlock? RubyStone;
    public static RegisteredBlock? RubyWoodPlanks;
    public static RegisteredBlock? RubyChair;
    public static RegisteredBlock? RubySand;
    public static RegisteredBlock? DebugHooksBlock;
    public static RegisteredSlabBlock? RubyStoneSlab;
    public static RegisteredSlabBlock? RubyWoodSlab;
    public static RegisteredBlock? RubyLamp;
    public static RegisteredBlock? RubyLampLit;
    public static RegisteredBlock? Orichalcum;
    public static RegisteredItem? Ruby;
    public static RegisteredItem? DebugHooksItemEntry;
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

    private sealed class RubyLampBlock : WeaveLoader.API.Block.Block
    {
        private readonly bool _isLit;

        public RubyLampBlock(bool isLit)
        {
            _isLit = isLit;
        }

        public override void OnPlace(BlockUpdateContext context)
        {
            if (context.IsClientSide)
                return;

            if (_isLit)
            {
                if (!context.HasNeighborSignal())
                    context.ScheduleTick(4);
            }
            else if (context.HasNeighborSignal())
            {
                context.SetBlock(new Identifier("examplemod:ruby_lamp_lit"));
            }
        }

        public override void OnNeighborChanged(BlockNeighborChangedContext context)
        {
            if (context.Block.IsClientSide)
                return;

            if (_isLit)
            {
                if (!context.Block.HasNeighborSignal())
                    context.Block.ScheduleTick(4);
            }
            else if (context.Block.HasNeighborSignal())
            {
                context.Block.SetBlock(new Identifier("examplemod:ruby_lamp_lit"));
            }
        }

        public override void OnScheduledTick(BlockTickContext context)
        {
            if (!_isLit || context.Block.IsClientSide)
                return;

            if (!context.Block.HasNeighborSignal())
                context.Block.SetBlock(new Identifier("examplemod:ruby_lamp"));
        }
    }

    private sealed class RubySandBlock : FallingBlock
    {
        private static readonly int FlowingLavaId = IdHelper.GetBlockNumericId("minecraft:flowing_lava");
        private static readonly int LavaId = IdHelper.GetBlockNumericId("minecraft:lava");

        public override void OnPlace(BlockUpdateContext context)
        {
            TryHarden(context);
        }

        public override void OnNeighborChanged(BlockNeighborChangedContext context)
        {
            TryHarden(context.Block);
        }

        private static void TryHarden(BlockUpdateContext context)
        {
            if (context.IsClientSide || RubyStone == null)
                return;

            if (HasLavaAtOrAdjacent(context))
                context.SetBlock(RubyStone.NumericId);
        }

        private static bool HasLavaAtOrAdjacent(BlockUpdateContext context)
        {
            static bool IsLava(int blockId) => blockId == FlowingLavaId || blockId == LavaId;

            return IsLava(context.GetBlockId()) ||
                   IsLava(context.GetBlockId(-1, 0, 0)) ||
                   IsLava(context.GetBlockId(1, 0, 0)) ||
                   IsLava(context.GetBlockId(0, -1, 0)) ||
                   IsLava(context.GetBlockId(0, 1, 0)) ||
                   IsLava(context.GetBlockId(0, 0, -1)) ||
                   IsLava(context.GetBlockId(0, 0, 1));
        }
    }

    private sealed class DebugHooks : WeaveLoader.API.Block.Block
    {
        private static void Log(string hook, BlockUpdateContext block, string? extra = null)
        {
            if (string.IsNullOrWhiteSpace(extra))
                Logger.Info($"DebugHooks::{hook} at ({block.X}, {block.Y}, {block.Z}) blockId={block.BlockId} client={block.IsClientSide}");
            else
                Logger.Info($"DebugHooks::{hook} at ({block.X}, {block.Y}, {block.Z}) blockId={block.BlockId} client={block.IsClientSide} {extra}");
        }

        public override void OnPlace(BlockUpdateContext context)
        {
            Log("OnPlace", context);
        }

        public override void OnNeighborChanged(BlockNeighborChangedContext context)
        {
            Log("OnNeighborChanged", context.Block, $"neighborId={context.NeighborBlockId}");
        }

        public override void OnScheduledTick(BlockTickContext context)
        {
            Log("OnScheduledTick", context.Block);
        }

        public override BlockActionResult OnUse(BlockUseContext context)
        {
            Log("OnUse", context.Block,
                $"face={context.Face} click=({context.ClickX:0.00},{context.ClickY:0.00},{context.ClickZ:0.00}) soundOnly={context.SoundOnly}");
            return BlockActionResult.ContinueVanilla;
        }

        public override void OnStepOn(BlockEntityContext context)
        {
            Log("OnStepOn", context.Block, $"entityPtr=0x{context.NativeEntityPtr.ToString("X")}");
        }

        public override void OnEntityInsideTile(BlockEntityContext context)
        {
            Log("OnEntityInsideTile", context.Block, $"entityPtr=0x{context.NativeEntityPtr.ToString("X")}");
        }

        public override void OnFallOn(BlockFallContext context)
        {
            Log("OnFallOn", context.Block, $"entityPtr=0x{context.NativeEntityPtr.ToString("X")} fall={context.FallDistance:0.00}");
        }

        public override void OnRemoving(BlockRemovingContext context)
        {
            Log("OnRemoving", context.Block, $"blockData={context.BlockData}");
        }

        public override void OnRemoved(BlockRemoveContext context)
        {
            Log("OnRemoved", context.Block, $"removedId={context.RemovedBlockId} removedData={context.RemovedBlockData}");
        }

        public override void OnDestroyed(BlockDestroyContext context)
        {
            Log("OnDestroyed", context.Block, $"blockData={context.BlockData}");
        }

        public override void OnPlayerDestroy(BlockPlayerDestroyContext context)
        {
            Log("OnPlayerDestroy", context.Block,
                $"playerPtr=0x{context.NativePlayerPtr.ToString("X")} blockData={context.BlockData}");
        }

        public override void OnPlayerWillDestroy(BlockPlayerWillDestroyContext context)
        {
            Log("OnPlayerWillDestroy", context.Block,
                $"playerPtr=0x{context.NativePlayerPtr.ToString("X")} blockData={context.BlockData}");
        }

        public override void OnPlacedBy(BlockPlacedByContext context)
        {
            Log("OnPlacedBy", context.Block,
                $"placerPtr=0x{context.NativePlacerPtr.ToString("X")} itemPtr=0x{context.NativeItemInstancePtr.ToString("X")}");
        }

    }

    private sealed class DebugHooksItem : Item
    {
        private static void Log(string hook, string? extra = null)
        {
            if (string.IsNullOrWhiteSpace(extra))
                Logger.Info($"DebugItem::{hook}");
            else
                Logger.Info($"DebugItem::{hook} {extra}");
        }

        public override MineBlockResult OnMineBlock(MineBlockContext context)
        {
            Log("OnMineBlock", $"itemId={context.ItemId} tileId={context.TileId} pos=({context.X},{context.Y},{context.Z})");
            return MineBlockResult.ContinueVanilla;
        }

        public override UseItemResult OnUseItem(UseItemContext context)
        {
            Log("OnUseItem",
                $"itemId={context.ItemId} client={context.IsClientSide} " +
                $"itemPtr=0x{context.NativeItemInstancePtr.ToString("X")} playerPtr=0x{context.NativePlayerPtr.ToString("X")}");
            return UseItemResult.ContinueVanilla;
        }

        public override ItemActionResult OnUseOn(UseOnItemContext context)
        {
            Log("OnUseOn",
                $"itemId={context.ItemId} client={context.IsClientSide} " +
                $"pos=({context.X},{context.Y},{context.Z}) face={context.Face} " +
                $"click=({context.ClickX:0.00},{context.ClickY:0.00},{context.ClickZ:0.00}) " +
                $"playerPtr=0x{context.NativePlayerPtr.ToString("X")} itemPtr=0x{context.NativeItemInstancePtr.ToString("X")}");
            return ItemActionResult.ContinueVanilla;
        }

        public override ItemActionResult OnInteractEntity(ItemEntityInteractionContext context)
        {
            Log("OnInteractEntity",
                $"itemId={context.ItemId} playerPtr=0x{context.NativePlayerPtr.ToString("X")} " +
                $"targetPtr=0x{context.NativeTargetEntityPtr.ToString("X")} itemPtr=0x{context.NativeItemInstancePtr.ToString("X")}");
            return ItemActionResult.ContinueVanilla;
        }

        public override ItemActionResult OnHurtEntity(ItemEntityInteractionContext context)
        {
            Log("OnHurtEntity",
                $"itemId={context.ItemId} playerPtr=0x{context.NativePlayerPtr.ToString("X")} " +
                $"targetPtr=0x{context.NativeTargetEntityPtr.ToString("X")} itemPtr=0x{context.NativeItemInstancePtr.ToString("X")}");
            return ItemActionResult.ContinueVanilla;
        }

        public override void OnInventoryTick(ItemInventoryTickContext context)
        {
            Log("OnInventoryTick",
                $"itemId={context.ItemId} slot={context.Slot} selected={context.IsSelected} client={context.IsClientSide} " +
                $"ownerPtr=0x{context.NativeOwnerEntityPtr.ToString("X")} itemPtr=0x{context.NativeItemInstancePtr.ToString("X")}");
        }

        public override void OnCraftedBy(ItemCraftedByContext context)
        {
            Log("OnCraftedBy",
                $"itemId={context.ItemId} amount={context.Amount} client={context.IsClientSide} " +
                $"playerPtr=0x{context.NativePlayerPtr.ToString("X")} itemPtr=0x{context.NativeItemInstancePtr.ToString("X")}");
        }
    }

    public void OnInitialize()
    {
        GameEvents.OnWorldLoaded += (_, e) =>
        {
            s_currentLevel = e.NativeLevelPointer;
            s_hasLevel = s_currentLevel != 0;
        };
        GameEvents.OnWorldUnloaded += (_, __) =>
        {
            s_currentLevel = 0;
            s_hasLevel = false;
        };

        RubyOre = Registry.Block.Register("examplemod:ruby_ore",
            new BlockProperties()
                .Material(MaterialType.Stone)
                .Hardness(3.0f)
                .Resistance(15f)
                .Sound(SoundType.Stone)
                .Icon("examplemod:block/ruby_ore")
                .Name(Text.Translatable("block.examplemod.ruby_ore"))
                .RequiredHarvestLevel(2)
                .RequiredTool(ToolType.Pickaxe)
                .InCreativeTab(CreativeTab.BuildingBlocks));

        RubyStone = Registry.Block.Register("examplemod:ruby_stone",
            new BlockProperties()
                .Material(MaterialType.Stone)
                .Hardness(1.5f)
                .Resistance(10f)
                .Sound(SoundType.Stone)
                .Icon("examplemod:block/ruby_stone")
                .Name(Text.Translatable("block.examplemod.ruby_stone"))
                .RequiredHarvestLevel(1)
                .RequiredTool(ToolType.Pickaxe)
                .InCreativeTab(CreativeTab.BuildingBlocks));

        RubyWoodPlanks = Registry.Block.Register("examplemod:ruby_wood_planks",
            new BlockProperties()
                .Material(MaterialType.Wood)
                .Hardness(2.0f)
                .Resistance(5f)
                .Sound(SoundType.Wood)
                .Icon("examplemod:block/ruby_wood_planks")
                .Name(Text.Translatable("block.examplemod.ruby_wood_planks"))
                .InCreativeTab(CreativeTab.BuildingBlocks));

        RubyChair = Registry.Block.Register("examplemod:ruby_chair",
            new BlockProperties()
                .Material(MaterialType.Wood)
                .Hardness(1.5f)
                .Resistance(5f)
                .Sound(SoundType.Wood)
                .Model("examplemod:block/ruby_chair")
                .BlockState("examplemod:ruby_chair")
                .RotationProfile(BlockRotationProfile.Facing)
                .Name(Text.Translatable("block.examplemod.ruby_chair"))
                .InCreativeTab(CreativeTab.Decoration));

        RubySand = Registry.Block.Register("examplemod:ruby_sand",
            new RubySandBlock(),
            new BlockProperties()
                .Material(MaterialType.Sand)
                .Hardness(0.5f)
                .Resistance(2.5f)
                .Sound(SoundType.Sand)
                .Icon("examplemod:block/ruby_sand")
                .Name(Text.Translatable("block.examplemod.ruby_sand"))
                .RequiredTool(ToolType.Shovel)
                .InCreativeTab(CreativeTab.BuildingBlocks)
                .Prepend());

        DebugHooksBlock = Registry.Block.Register("examplemod:debug_hooks",
            new DebugHooks(),
            new BlockProperties()
                .Material(MaterialType.Stone)
                .Hardness(1.0f)
                .Resistance(5.0f)
                .Sound(SoundType.Stone)
                .Icon("examplemod:block/ruby_stone")
                .Name(Text.Translatable("block.examplemod.debug_hooks"))
                .InCreativeTab(CreativeTab.BuildingBlocks));

        RubyStoneSlab = (RegisteredSlabBlock)Registry.Block.Register("examplemod:ruby_stone_slab",
            new SlabBlock(),
            new BlockProperties()
                .Material(MaterialType.Stone)
                .Hardness(1.5f)
                .Resistance(10f)
                .Sound(SoundType.Stone)
                .Icon("examplemod:block/ruby_stone")
                .Name(Text.Translatable("block.examplemod.ruby_stone_slab"))
                .RequiredHarvestLevel(1)
                .RequiredTool(ToolType.Pickaxe)
                .InCreativeTab(CreativeTab.BuildingBlocks));

        RubyWoodSlab = (RegisteredSlabBlock)Registry.Block.Register("examplemod:ruby_wood_slab",
            new SlabBlock(),
            new BlockProperties()
                .Material(MaterialType.Wood)
                .Hardness(2.0f)
                .Resistance(5f)
                .Sound(SoundType.Wood)
                .Icon("examplemod:block/ruby_wood_planks")
                .Name(Text.Translatable("block.examplemod.ruby_wood_slab"))
                .InCreativeTab(CreativeTab.BuildingBlocks));

        RubyLamp = Registry.Block.Register("examplemod:ruby_lamp", new RubyLampBlock(false),
            new BlockProperties()
                .Material(MaterialType.Glass)
                .Hardness(0.3f)
                .Resistance(1.5f)
                .Sound(SoundType.Glass)
                .Icon("examplemod:block/ruby_lamp")
                .Name(Text.Translatable("block.examplemod.ruby_lamp"))
                .RequiredHarvestLevel(0)
                .RequiredTool(ToolType.Pickaxe)
                .AcceptsRedstonePower()
                .InCreativeTab(CreativeTab.BuildingBlocks));

        RubyLampLit = Registry.Block.Register("examplemod:ruby_lamp_lit",
            new RubyLampBlock(true)
            {
                DropAsBlockId = new Identifier("examplemod:ruby_lamp"),
                CloneAsBlockId = new Identifier("examplemod:ruby_lamp")
            },
            new BlockProperties()
                .Material(MaterialType.Glass)
                .Hardness(0.3f)
                .Resistance(1.5f)
                .Sound(SoundType.Glass)
                .Icon("examplemod:block/ruby_lamp_on")
                .LightLevel(1.0f)
                .Name(Text.Translatable("block.examplemod.ruby_lamp_lit"))
                .RequiredHarvestLevel(0)
                .RequiredTool(ToolType.Pickaxe)
                .AcceptsRedstonePower());

        Orichalcum = Registry.Block.Register("examplemod:orichalcum_ore",
            new BlockProperties()
                .Material(MaterialType.Metal)
                .Hardness(5.0f)
                .Resistance(30f)
                .Sound(SoundType.Metal)
                .Icon("examplemod:block/orichalcum_ore")
                .Name(Text.Translatable("block.examplemod.orichalcum_ore"))
                .RequiredHarvestLevel(4)
                .RequiredTool(ToolType.Pickaxe)
                .InCreativeTab(CreativeTab.BuildingBlocks));

        Ruby = Registry.Item.Register("examplemod:ruby",
            new ItemProperties()
                .MaxStackSize(64)
                .Icon("examplemod:item/ruby")
                .Name(Text.Translatable("item.examplemod.ruby"))
                .InCreativeTab(CreativeTab.Materials));

        DebugHooksItemEntry = Registry.Item.Register("examplemod:debug_item",
            new DebugHooksItem(),
            new ItemProperties()
                .MaxStackSize(1)
                .Icon("examplemod:item/ruby")
                .Name(Text.Translatable("item.examplemod.debug_item"))
                .InCreativeTab(CreativeTab.ToolsAndWeapons));

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
                .Icon("examplemod:item/ruby_sword")
                .Name(Text.Translatable("item.examplemod.ruby_sword"))
                .InCreativeTab(CreativeTab.ToolsAndWeapons));

        RubyShovelItem = Registry.Item.Register("examplemod:ruby_shovel", new RubyShovel { CustomMaterialId = "examplemod:ruby_material" },
            new ItemProperties()
                .MaxStackSize(1)
                .MaxDamage(512)
                .AttackDamage(4.5f)
                .Icon("examplemod:item/ruby_shovel")
                .Name(Text.Translatable("item.examplemod.ruby_shovel"))
                .InCreativeTab(CreativeTab.ToolsAndWeapons));

        RubyPickaxeItem = Registry.Item.Register("examplemod:ruby_pickaxe", new RubyPickaxe { CustomMaterialId = "examplemod:ruby_material" },
            new ItemProperties()
                .MaxStackSize(1)
                .MaxDamage(512)
                .AttackDamage(5.0f)
                .Icon("examplemod:item/ruby_pickaxe")
                .Name(Text.Translatable("item.examplemod.ruby_pickaxe"))
                .InCreativeTab(CreativeTab.ToolsAndWeapons));

        RubyAxeItem = Registry.Item.Register("examplemod:ruby_axe", new RubyAxe { CustomMaterialId = "examplemod:ruby_material" },
            new ItemProperties()
                .MaxStackSize(1)
                .MaxDamage(512)
                .AttackDamage(7.0f)
                .Icon("examplemod:item/ruby_axe")
                .Name(Text.Translatable("item.examplemod.ruby_axe"))
                .InCreativeTab(CreativeTab.ToolsAndWeapons));

        RubyHoeItem = Registry.Item.Register("examplemod:ruby_hoe", new RubyHoe { CustomMaterialId = "examplemod:ruby_material" },
            new ItemProperties()
                .MaxStackSize(1)
                .MaxDamage(512)
                .AttackDamage(1.0f)
                .Icon("examplemod:item/ruby_hoe")
                .Name(Text.Translatable("item.examplemod.ruby_hoe"))
                .InCreativeTab(CreativeTab.ToolsAndWeapons)
                .Prepend());

        RubyWandItem = Registry.Item.Register("examplemod:ruby_wand", new RubyWand(),
            new ItemProperties()
                .MaxStackSize(1)
                .Icon("examplemod:item/ruby_wand")
                .Model("examplemod:item/ruby_wand")
                .Name(Text.Translatable("item.examplemod.ruby_wand"))
                .InCreativeTab(CreativeTab.ToolsAndWeapons));

        LootTableEvents.MODIFY.Register((_, _, tableId, tableBuilder, _) =>
        {
            if (!tableId.Namespace.Equals("minecraft", StringComparison.OrdinalIgnoreCase) ||
                !tableId.Path.Equals("entities/creeper", StringComparison.OrdinalIgnoreCase))
            {
                return;
            }

            var pool = LootPool.builder()
                .rolls(ConstantLootNumberProvider.create(1))
                .conditionally(RandomChanceLootCondition.builder(0.05f))
                .with(ItemEntry.builder(new Identifier("examplemod:ruby_sand"))
                    .apply(new SetCountLootFunction(1)));

            tableBuilder.pool(pool);

            if (!s_loggedCreeperLootInjection)
            {
                s_loggedCreeperLootInjection = true;
                Logger.Info("ExampleMod: injected creeper loot pool (ruby_sand)");
            }
        });

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

    internal static bool TryGetCurrentLevel(out nint levelPtr)
    {
        levelPtr = s_currentLevel;
        return s_hasLevel && levelPtr != 0;
    }
}
