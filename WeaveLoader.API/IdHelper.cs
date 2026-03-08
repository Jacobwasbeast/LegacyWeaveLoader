using System.Collections.Generic;
using WeaveLoader.API.Block;
using WeaveLoader.API.Entity;
using WeaveLoader.API.Item;

namespace WeaveLoader.API;

public static class IdHelper
{
    private static readonly Dictionary<string, int> s_vanillaBlockToId = new(StringComparer.Ordinal)
    {
        ["minecraft:air"] = 0,
        ["minecraft:stone"] = 1,
        ["minecraft:grass"] = 2,
        ["minecraft:dirt"] = 3,
        ["minecraft:cobblestone"] = 4,
        ["minecraft:planks"] = 5,
        ["minecraft:sapling"] = 6,
        ["minecraft:bedrock"] = 7,
        ["minecraft:flowing_water"] = 8,
        ["minecraft:water"] = 9,
        ["minecraft:flowing_lava"] = 10,
        ["minecraft:lava"] = 11,
        ["minecraft:sand"] = 12,
        ["minecraft:gravel"] = 13,
        ["minecraft:gold_ore"] = 14,
        ["minecraft:iron_ore"] = 15,
        ["minecraft:coal_ore"] = 16,
        ["minecraft:log"] = 17,
        ["minecraft:leaves"] = 18,
        ["minecraft:sponge"] = 19,
        ["minecraft:glass"] = 20,
        ["minecraft:lapis_ore"] = 21,
        ["minecraft:lapis_block"] = 22,
        ["minecraft:dispenser"] = 23,
        ["minecraft:sandstone"] = 24,
        ["minecraft:noteblock"] = 25,
        ["minecraft:bed"] = 26,
        ["minecraft:golden_rail"] = 27,
        ["minecraft:detector_rail"] = 28,
        ["minecraft:sticky_piston"] = 29,
        ["minecraft:web"] = 30,
        ["minecraft:tallgrass"] = 31,
        ["minecraft:deadbush"] = 32,
        ["minecraft:piston"] = 33,
        ["minecraft:piston_head"] = 34,
        ["minecraft:wool"] = 35,
        ["minecraft:dandelion"] = 37,
        ["minecraft:poppy"] = 38,
        ["minecraft:brown_mushroom"] = 39,
        ["minecraft:red_mushroom"] = 40,
        ["minecraft:gold_block"] = 41,
        ["minecraft:iron_block"] = 42,
        ["minecraft:double_stone_slab"] = 43,
        ["minecraft:stone_slab"] = 44,
        ["minecraft:brick_block"] = 45,
        ["minecraft:tnt"] = 46,
        ["minecraft:bookshelf"] = 47,
        ["minecraft:mossy_cobblestone"] = 48,
        ["minecraft:obsidian"] = 49,
        ["minecraft:torch"] = 50,
        ["minecraft:fire"] = 51,
        ["minecraft:mob_spawner"] = 52,
        ["minecraft:oak_stairs"] = 53,
        ["minecraft:chest"] = 54,
        ["minecraft:redstone_wire"] = 55,
        ["minecraft:diamond_ore"] = 56,
        ["minecraft:diamond_block"] = 57,
        ["minecraft:crafting_table"] = 58,
        ["minecraft:wheat"] = 59,
        ["minecraft:farmland"] = 60,
        ["minecraft:furnace"] = 61,
        ["minecraft:lit_furnace"] = 62,
        ["minecraft:standing_sign"] = 63,
        ["minecraft:wooden_door"] = 64,
        ["minecraft:ladder"] = 65,
        ["minecraft:rail"] = 66,
        ["minecraft:stone_stairs"] = 67,
        ["minecraft:wall_sign"] = 68,
        ["minecraft:lever"] = 69,
        ["minecraft:stone_pressure_plate"] = 70,
        ["minecraft:iron_door"] = 71,
        ["minecraft:wooden_pressure_plate"] = 72,
        ["minecraft:redstone_ore"] = 73,
        ["minecraft:lit_redstone_ore"] = 74,
        ["minecraft:unlit_redstone_torch"] = 75,
        ["minecraft:redstone_torch"] = 76,
        ["minecraft:stone_button"] = 77,
        ["minecraft:snow_layer"] = 78,
        ["minecraft:ice"] = 79,
        ["minecraft:snow"] = 80,
        ["minecraft:cactus"] = 81,
        ["minecraft:clay"] = 82,
        ["minecraft:reeds"] = 83,
        ["minecraft:jukebox"] = 84,
        ["minecraft:fence"] = 85,
        ["minecraft:pumpkin"] = 86,
        ["minecraft:netherrack"] = 87,
        ["minecraft:soul_sand"] = 88,
        ["minecraft:glowstone"] = 89,
        ["minecraft:portal"] = 90,
        ["minecraft:lit_pumpkin"] = 91,
        ["minecraft:cake"] = 92,
        ["minecraft:unpowered_repeater"] = 93,
        ["minecraft:powered_repeater"] = 94,
        ["minecraft:stained_glass"] = 95,
        ["minecraft:trapdoor"] = 96,
        ["minecraft:monster_egg"] = 97,
        ["minecraft:stonebrick"] = 98,
        ["minecraft:brown_mushroom_block"] = 99,
        ["minecraft:red_mushroom_block"] = 100,
        ["minecraft:iron_bars"] = 101,
        ["minecraft:glass_pane"] = 102,
        ["minecraft:melon_block"] = 103,
        ["minecraft:pumpkin_stem"] = 104,
        ["minecraft:melon_stem"] = 105,
        ["minecraft:vine"] = 106,
        ["minecraft:fence_gate"] = 107,
        ["minecraft:brick_stairs"] = 108,
        ["minecraft:stone_brick_stairs"] = 109,
        ["minecraft:mycelium"] = 110,
        ["minecraft:waterlily"] = 111,
        ["minecraft:nether_brick"] = 112,
        ["minecraft:nether_brick_fence"] = 113,
        ["minecraft:nether_brick_stairs"] = 114,
        ["minecraft:nether_wart"] = 115,
        ["minecraft:enchanting_table"] = 116,
        ["minecraft:brewing_stand"] = 117,
        ["minecraft:cauldron"] = 118,
        ["minecraft:end_portal"] = 119,
        ["minecraft:end_portal_frame"] = 120,
        ["minecraft:end_stone"] = 121,
        ["minecraft:dragon_egg"] = 122,
        ["minecraft:redstone_lamp"] = 123,
        ["minecraft:lit_redstone_lamp"] = 124,
        ["minecraft:double_wooden_slab"] = 125,
        ["minecraft:wooden_slab"] = 126,
        ["minecraft:cocoa"] = 127,
        ["minecraft:sandstone_stairs"] = 128,
        ["minecraft:emerald_ore"] = 129,
        ["minecraft:ender_chest"] = 130,
        ["minecraft:tripwire_hook"] = 131,
        ["minecraft:tripwire"] = 132,
        ["minecraft:emerald_block"] = 133,
        ["minecraft:spruce_stairs"] = 134,
        ["minecraft:birch_stairs"] = 135,
        ["minecraft:jungle_stairs"] = 136,
        ["minecraft:command_block"] = 137,
        ["minecraft:beacon"] = 138,
        ["minecraft:cobblestone_wall"] = 139,
        ["minecraft:flower_pot"] = 140,
        ["minecraft:carrots"] = 141,
        ["minecraft:potatoes"] = 142,
        ["minecraft:wooden_button"] = 143,
        ["minecraft:skull"] = 144,
        ["minecraft:anvil"] = 145,
        ["minecraft:trapped_chest"] = 146,
        ["minecraft:light_weighted_pressure_plate"] = 147,
        ["minecraft:heavy_weighted_pressure_plate"] = 148,
        ["minecraft:unpowered_comparator"] = 149,
        ["minecraft:powered_comparator"] = 150,
        ["minecraft:daylight_detector"] = 151,
        ["minecraft:redstone_block"] = 152,
        ["minecraft:quartz_ore"] = 153,
        ["minecraft:hopper"] = 154,
        ["minecraft:quartz_block"] = 155,
        ["minecraft:quartz_stairs"] = 156,
        ["minecraft:activator_rail"] = 157,
        ["minecraft:dropper"] = 158,
        ["minecraft:hardened_clay"] = 159,
        ["minecraft:stained_glass_pane"] = 160,
        ["minecraft:leaves2"] = 161,
        ["minecraft:log2"] = 162,
        ["minecraft:acacia_stairs"] = 163,
        ["minecraft:dark_oak_stairs"] = 164,
        ["minecraft:slime"] = 165,
        ["minecraft:barrier"] = 166,
        ["minecraft:iron_trapdoor"] = 167,
        ["minecraft:prismarine"] = 168,
        ["minecraft:sea_lantern"] = 169,
        ["minecraft:hay_block"] = 170,
        ["minecraft:carpet"] = 171,
        ["minecraft:stained_hardened_clay"] = 172,
        ["minecraft:coal_block"] = 173
    };

    private static readonly Dictionary<string, int> s_vanillaItemToId = new(StringComparer.Ordinal)
    {
        ["minecraft:iron_shovel"] = 256,
        ["minecraft:iron_pickaxe"] = 257,
        ["minecraft:iron_axe"] = 258,
        ["minecraft:flint_and_steel"] = 259,
        ["minecraft:apple"] = 260,
        ["minecraft:bow"] = 261,
        ["minecraft:arrow"] = 262,
        ["minecraft:coal"] = 263,
        ["minecraft:diamond"] = 264,
        ["minecraft:iron_ingot"] = 265,
        ["minecraft:gold_ingot"] = 266,
        ["minecraft:iron_sword"] = 267,
        ["minecraft:wooden_sword"] = 268,
        ["minecraft:wooden_shovel"] = 269,
        ["minecraft:wooden_pickaxe"] = 270,
        ["minecraft:wooden_axe"] = 271,
        ["minecraft:stone_sword"] = 272,
        ["minecraft:stone_shovel"] = 273,
        ["minecraft:stone_pickaxe"] = 274,
        ["minecraft:stone_axe"] = 275,
        ["minecraft:diamond_sword"] = 276,
        ["minecraft:diamond_shovel"] = 277,
        ["minecraft:diamond_pickaxe"] = 278,
        ["minecraft:diamond_axe"] = 279,
        ["minecraft:stick"] = 280,
        ["minecraft:bowl"] = 281,
        ["minecraft:mushroom_stew"] = 282,
        ["minecraft:golden_sword"] = 283,
        ["minecraft:golden_shovel"] = 284,
        ["minecraft:golden_pickaxe"] = 285,
        ["minecraft:golden_axe"] = 286,
        ["minecraft:string"] = 287,
        ["minecraft:feather"] = 288,
        ["minecraft:gunpowder"] = 289,
        ["minecraft:wooden_hoe"] = 290,
        ["minecraft:stone_hoe"] = 291,
        ["minecraft:iron_hoe"] = 292,
        ["minecraft:diamond_hoe"] = 293,
        ["minecraft:golden_hoe"] = 294,
        ["minecraft:wheat_seeds"] = 295,
        ["minecraft:wheat"] = 296,
        ["minecraft:bread"] = 297,
        ["minecraft:leather_helmet"] = 298,
        ["minecraft:leather_chestplate"] = 299,
        ["minecraft:leather_leggings"] = 300,
        ["minecraft:leather_boots"] = 301,
        ["minecraft:chainmail_helmet"] = 302,
        ["minecraft:chainmail_chestplate"] = 303,
        ["minecraft:chainmail_leggings"] = 304,
        ["minecraft:chainmail_boots"] = 305,
        ["minecraft:iron_helmet"] = 306,
        ["minecraft:iron_chestplate"] = 307,
        ["minecraft:iron_leggings"] = 308,
        ["minecraft:iron_boots"] = 309,
        ["minecraft:diamond_helmet"] = 310,
        ["minecraft:diamond_chestplate"] = 311,
        ["minecraft:diamond_leggings"] = 312,
        ["minecraft:diamond_boots"] = 313,
        ["minecraft:golden_helmet"] = 314,
        ["minecraft:golden_chestplate"] = 315,
        ["minecraft:golden_leggings"] = 316,
        ["minecraft:golden_boots"] = 317,
        ["minecraft:flint"] = 318,
        ["minecraft:porkchop"] = 319,
        ["minecraft:cooked_porkchop"] = 320,
        ["minecraft:painting"] = 321,
        ["minecraft:golden_apple"] = 322,
        ["minecraft:sign"] = 323,
        ["minecraft:wooden_door"] = 324,
        ["minecraft:bucket"] = 325,
        ["minecraft:water_bucket"] = 326,
        ["minecraft:lava_bucket"] = 327,
        ["minecraft:minecart"] = 328,
        ["minecraft:saddle"] = 329,
        ["minecraft:iron_door"] = 330,
        ["minecraft:redstone"] = 331,
        ["minecraft:snowball"] = 332,
        ["minecraft:boat"] = 333,
        ["minecraft:leather"] = 334,
        ["minecraft:milk_bucket"] = 335,
        ["minecraft:brick"] = 336,
        ["minecraft:clay_ball"] = 337,
        ["minecraft:reeds"] = 338,
        ["minecraft:paper"] = 339,
        ["minecraft:book"] = 340,
        ["minecraft:slime_ball"] = 341,
        ["minecraft:chest_minecart"] = 342,
        ["minecraft:furnace_minecart"] = 343,
        ["minecraft:egg"] = 344,
        ["minecraft:compass"] = 345,
        ["minecraft:fishing_rod"] = 346,
        ["minecraft:clock"] = 347,
        ["minecraft:glowstone_dust"] = 348,
        ["minecraft:fish"] = 349,
        ["minecraft:cooked_fished"] = 350,
        ["minecraft:dye"] = 351,
        ["minecraft:bone"] = 352,
        ["minecraft:sugar"] = 353,
        ["minecraft:cake"] = 354,
        ["minecraft:bed"] = 355,
        ["minecraft:repeater"] = 356,
        ["minecraft:cookie"] = 357,
        ["minecraft:filled_map"] = 358,
        ["minecraft:shears"] = 359,
        ["minecraft:melon"] = 360,
        ["minecraft:pumpkin_seeds"] = 361,
        ["minecraft:melon_seeds"] = 362,
        ["minecraft:beef"] = 363,
        ["minecraft:cooked_beef"] = 364,
        ["minecraft:chicken"] = 365,
        ["minecraft:cooked_chicken"] = 366,
        ["minecraft:rotten_flesh"] = 367,
        ["minecraft:ender_pearl"] = 368,
        ["minecraft:blaze_rod"] = 369,
        ["minecraft:ghast_tear"] = 370,
        ["minecraft:gold_nugget"] = 371,
        ["minecraft:nether_wart"] = 372,
        ["minecraft:potion"] = 373,
        ["minecraft:glass_bottle"] = 374,
        ["minecraft:spider_eye"] = 375,
        ["minecraft:fermented_spider_eye"] = 376,
        ["minecraft:blaze_powder"] = 377,
        ["minecraft:magma_cream"] = 378,
        ["minecraft:brewing_stand"] = 379,
        ["minecraft:cauldron"] = 380,
        ["minecraft:ender_eye"] = 381,
        ["minecraft:speckled_melon"] = 382,
        ["minecraft:spawn_egg"] = 383,
        ["minecraft:experience_bottle"] = 384,
        ["minecraft:fire_charge"] = 385,
        ["minecraft:emerald"] = 388,
        ["minecraft:item_frame"] = 389,
        ["minecraft:flower_pot"] = 390,
        ["minecraft:carrot"] = 391,
        ["minecraft:potato"] = 392,
        ["minecraft:baked_potato"] = 393,
        ["minecraft:poisonous_potato"] = 394,
        ["minecraft:map"] = 395,
        ["minecraft:golden_carrot"] = 396,
        ["minecraft:skull"] = 397,
        ["minecraft:carrot_on_a_stick"] = 398,
        ["minecraft:nether_star"] = 399,
        ["minecraft:pumpkin_pie"] = 400,
        ["minecraft:fireworks"] = 401,
        ["minecraft:firework_charge"] = 402,
        ["minecraft:enchanted_book"] = 403,
        ["minecraft:comparator"] = 404,
        ["minecraft:netherbrick"] = 405,
        ["minecraft:quartz"] = 406,
        ["minecraft:tnt_minecart"] = 407,
        ["minecraft:hopper_minecart"] = 408,
        ["minecraft:iron_horse_armor"] = 417,
        ["minecraft:golden_horse_armor"] = 418,
        ["minecraft:diamond_horse_armor"] = 419,
        ["minecraft:lead"] = 420,
        ["minecraft:name_tag"] = 421,
        ["minecraft:record_13"] = 2256,
        ["minecraft:record_cat"] = 2257,
        ["minecraft:record_blocks"] = 2258,
        ["minecraft:record_chirp"] = 2259,
        ["minecraft:record_far"] = 2260,
        ["minecraft:record_mall"] = 2261,
        ["minecraft:record_mellohi"] = 2262,
        ["minecraft:record_stal"] = 2263,
        ["minecraft:record_strad"] = 2264,
        ["minecraft:record_ward"] = 2265,
        ["minecraft:record_11"] = 2266,
        ["minecraft:record_wait"] = 2267
    };

    // Entity IDs must match the game's EntityIO::staticCtor() registrations.
    private static readonly Dictionary<string, int> s_vanillaEntityToId = new(StringComparer.Ordinal)
    {
        ["minecraft:item"] = 1,
        ["minecraft:xp_orb"] = 2,
        ["minecraft:leash_knot"] = 8,
        ["minecraft:painting"] = 9,
        ["minecraft:arrow"] = 10,
        ["minecraft:snowball"] = 11,
        ["minecraft:fireball"] = 12,
        ["minecraft:small_fireball"] = 13,
        ["minecraft:ender_pearl"] = 14,
        ["minecraft:eye_of_ender_signal"] = 15,
        ["minecraft:thrown_potion"] = 16,
        ["minecraft:xp_bottle"] = 17,
        ["minecraft:item_frame"] = 18,
        ["minecraft:wither_skull"] = 19,
        ["minecraft:primed_tnt"] = 20,
        ["minecraft:falling_block"] = 21,
        ["minecraft:fireworks_rocket"] = 22,
        ["minecraft:boat"] = 41,
        ["minecraft:minecart"] = 42,
        ["minecraft:chest_minecart"] = 43,
        ["minecraft:furnace_minecart"] = 44,
        ["minecraft:tnt_minecart"] = 45,
        ["minecraft:hopper_minecart"] = 46,
        ["minecraft:spawner_minecart"] = 47,
        ["minecraft:mob"] = 48,
        ["minecraft:monster"] = 49,
        ["minecraft:creeper"] = 50,
        ["minecraft:skeleton"] = 51,
        ["minecraft:spider"] = 52,
        ["minecraft:giant"] = 53,
        ["minecraft:zombie"] = 54,
        ["minecraft:slime"] = 55,
        ["minecraft:ghast"] = 56,
        ["minecraft:zombie_pigman"] = 57,
        ["minecraft:enderman"] = 58,
        ["minecraft:cave_spider"] = 59,
        ["minecraft:silverfish"] = 60,
        ["minecraft:blaze"] = 61,
        ["minecraft:magma_cube"] = 62,
        ["minecraft:ender_dragon"] = 63,
        ["minecraft:wither"] = 64,
        ["minecraft:bat"] = 65,
        ["minecraft:witch"] = 66,
        ["minecraft:pig"] = 90,
        ["minecraft:sheep"] = 91,
        ["minecraft:cow"] = 92,
        ["minecraft:chicken"] = 93,
        ["minecraft:squid"] = 94,
        ["minecraft:wolf"] = 95,
        ["minecraft:mooshroom"] = 96,
        ["minecraft:snow_golem"] = 97,
        ["minecraft:ocelot"] = 98,
        ["minecraft:iron_golem"] = 99,
        ["minecraft:horse"] = 100,
        ["minecraft:villager"] = 120,
        ["minecraft:ender_crystal"] = 200,
        ["minecraft:dragon_fireball"] = 1000
    };

    private static readonly Dictionary<int, string> s_vanillaIdToBlock = CreateReverse(s_vanillaBlockToId);
    private static readonly Dictionary<int, string> s_vanillaIdToItem = CreateReverse(s_vanillaItemToId);
    private static readonly Dictionary<int, string> s_vanillaIdToEntity = CreateReverse(s_vanillaEntityToId);

    public static int GetBlockNumericId(Identifier id)
    {
        int numeric = NativeInterop.native_get_block_id(id.ToString());
        if (numeric >= 0)
            return numeric;
        return s_vanillaBlockToId.TryGetValue(id.ToString(), out int vanillaId) ? vanillaId : -1;
    }

    public static int GetItemNumericId(Identifier id)
    {
        int numeric = NativeInterop.native_get_item_id(id.ToString());
        if (numeric >= 0)
            return numeric;
        return s_vanillaItemToId.TryGetValue(id.ToString(), out int vanillaId) ? vanillaId : -1;
    }

    public static int GetEntityNumericId(Identifier id)
    {
        int numeric = NativeInterop.native_get_entity_id(id.ToString());
        if (numeric >= 0)
            return numeric;
        return s_vanillaEntityToId.TryGetValue(id.ToString(), out int vanillaId) ? vanillaId : -1;
    }

    public static bool TryGetBlockIdentifier(int numericId, out Identifier id)
    {
        if (BlockRegistry.TryGetIdentifier(numericId, out id))
            return true;

        if (s_vanillaIdToBlock.TryGetValue(numericId, out string? vanilla))
        {
            id = new Identifier(vanilla);
            return true;
        }
        id = default;
        return false;
    }

    public static bool TryGetItemIdentifier(int numericId, out Identifier id)
    {
        if (ItemRegistry.TryGetIdentifier(numericId, out id))
            return true;
        if (s_vanillaIdToItem.TryGetValue(numericId, out string? vanilla))
        {
            id = new Identifier(vanilla);
            return true;
        }
        id = default;
        return false;
    }

    public static bool TryGetEntityIdentifier(int numericId, out Identifier id)
    {
        if (EntityRegistry.TryGetIdentifier(numericId, out id))
            return true;

        if (s_vanillaIdToEntity.TryGetValue(numericId, out string? vanilla))
        {
            id = new Identifier(vanilla);
            return true;
        }
        id = default;
        return false;
    }

    private static Dictionary<int, string> CreateReverse(Dictionary<string, int> source)
    {
        var reverse = new Dictionary<int, string>();
        foreach (var kv in source)
            reverse[kv.Value] = kv.Key;
        return reverse;
    }
}
