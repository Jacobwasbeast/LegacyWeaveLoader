using System;
using System.Collections.Generic;
using WeaveLoader.API;
using WeaveLoader.API.Mixins;
using WeaveLoader.API.Native;

namespace ExampleMod.Mixins;

[Mixin("Creeper")]
public static class CreeperExplosionMixin
{
    private const int Radius = 6;

    private static readonly HashSet<nint> s_processed = new();
    private static int s_triggerLogCount;
    private static bool s_offsetsReady;
    private static bool s_offsetsWarned;
    private static int s_swellOffset;
    private static int s_maxSwellOffset;
    private static int s_isClientSideOffset;

    [Inject("tick", At.Tail)]
    private static void OnTick(MixinContext ctx)
    {
        nint thisPtr = ctx.ThisPtr;
        if (thisPtr == 0 || ExampleMod.RubySand == null)
            return;

        nint levelPtr = 0;
        if (NativeOffsets.TryGet("Entity", "level", out var levelOffset))
            levelPtr = NativeMemory.ReadPtr(thisPtr, levelOffset);

        if (levelPtr == 0 && !ExampleMod.TryGetCurrentLevel(out levelPtr))
            return;

        if (!EnsureOffsets())
            return;

        if (s_isClientSideOffset != 0 && NativeMemory.ReadBool(levelPtr, s_isClientSideOffset))
            return;

        int swell = NativeMemory.ReadInt32(thisPtr, s_swellOffset);
        int maxSwell = NativeMemory.ReadInt32(thisPtr, s_maxSwellOffset);
        if (swell < maxSwell)
            return;

        if (!NativeMemory.ReadBool(thisPtr, NativeOffsets.Entity.Removed))
            return;

        if (!s_processed.Add(thisPtr))
            return;
        if (s_processed.Count > 256)
            s_processed.Clear();

        if (s_triggerLogCount < 1)
        {
            Logger.Info($"CreeperExplosionMixin triggered at ({NativeMemory.ReadDouble(thisPtr, NativeOffsets.Entity.X):F2}, {NativeMemory.ReadDouble(thisPtr, NativeOffsets.Entity.Y):F2}, {NativeMemory.ReadDouble(thisPtr, NativeOffsets.Entity.Z):F2})");
            s_triggerLogCount++;
        }

        int x = (int)Math.Floor(NativeMemory.ReadDouble(thisPtr, NativeOffsets.Entity.X));
        int y = (int)Math.Floor(NativeMemory.ReadDouble(thisPtr, NativeOffsets.Entity.Y));
        int z = (int)Math.Floor(NativeMemory.ReadDouble(thisPtr, NativeOffsets.Entity.Z));
        PaintSphereTop(levelPtr, x, y, z);
    }

    private static bool EnsureOffsets()
    {
        if (s_offsetsReady)
            return true;

        bool ok = true;
        ok &= NativeOffsets.TryGet("Creeper", "swell", out s_swellOffset);
        ok &= NativeOffsets.TryGet("Creeper", "maxSwell", out s_maxSwellOffset);
        NativeOffsets.TryGet("Level", "isClientSide", out s_isClientSideOffset);

        s_offsetsReady = ok;
        if (!s_offsetsReady && !s_offsetsWarned)
        {
            Logger.Warning("CreeperExplosionMixin: missing offsets for Creeper swell/maxSwell.");
            s_offsetsWarned = true;
        }
        return s_offsetsReady;
    }

    private static void PaintSphereTop(nint levelPtr, int centerX, int centerY, int centerZ)
    {
        int minY = Math.Max(0, centerY - Radius);
        int maxY = Math.Min(255, centerY + Radius);
        int radiusSq = Radius * Radius;
        var targets = new List<(int X, int Y, int Z)>();

        for (int dx = -Radius; dx <= Radius; dx++)
        {
            int dxSq = dx * dx;
            for (int dz = -Radius; dz <= Radius; dz++)
            {
                int distSq = dxSq + dz * dz;
                if (distSq > radiusSq)
                    continue;

                int worldX = centerX + dx;
                int worldZ = centerZ + dz;
                int verticalRadius = (int)Math.Floor(Math.Sqrt(radiusSq - distSq));
                int startY = Math.Min(maxY, centerY + verticalRadius);
                int endY = Math.Max(minY, centerY - verticalRadius);

                int topY = -1;
                for (int y = startY; y >= endY; --y)
                {
                    int id = NativeLevel.GetTile(levelPtr, worldX, y, worldZ);
                    if (id != 0)
                    {
                        topY = y;
                        break;
                    }
                }

                if (topY >= 0)
                    targets.Add((worldX, topY, worldZ));
            }
        }

        foreach (var (tx, ty, tz) in targets)
            NativeLevel.SetTile(levelPtr, tx, ty, tz, ExampleMod.RubySand!.NumericId, 0, 2);
    }
}
