// Трассировка первичных лучей по воксельным представлениям (ps_5_0).
// Формулы совпадают с RayTraversal.cpp (CPU-копия для метрики «шагов на луч»).
//
// Соглашения:
//   * сетка занимает куб [0, N]^3 в своих координатах, камера задаётся в них же;
//   * gVoxels — IVoxelGrid::GpuData() (StructuredBuffer<uint>, регистр t0);
//   * gMode: 0 — плотная сетка (3D-DDA), 1 — блочная, 2 — SVO, 3 — SVDAG (иерархический обход).

#define MODE_DENSE 0
#define MODE_BLOCK 1
#define MODE_SVO   2
#define MODE_SVDAG 3
#define BLOCK_SIDE 16
#define BLOCK_WORDS 128   // 16^3 бит
#define MAX_STEPS  16384

cbuffer TraceParams : register(b0)
{
    float3 gCameraPosition;
    uint   gResolution;       // N, степень двойки
    float3 gForward;
    uint   gDepth;            // D = log2 N
    float3 gRight;            // уже умножен на tan(fov/2) * aspect
    uint   gMode;
    float3 gUp;               // уже умножен на tan(fov/2)
    uint   gShowSteps;
    uint   gWidth;
    uint   gHeight;
    uint   gBlocksPerSide;    // N / 16
    uint   gDirectorySize;    // (N / 16)^3
};

StructuredBuffer<uint> gVoxels : register(t0);

bool IntersectBox(float3 o, float3 invD, float n, out float t0, out float t1)
{
    float3 ta = (0.0f - o) * invD;
    float3 tb = (n - o) * invD;
    float3 tmin = min(ta, tb);
    float3 tmax = max(ta, tb);
    t0 = max(max(tmin.x, tmin.y), max(tmin.z, 0.0f));
    t1 = min(min(tmax.x, tmax.y), tmax.z);
    return t0 <= t1;
}

// ───────── Плотная сетка: 1 бит на воксель, индекс x + N(y + Nz) ─────────
uint DenseGet(uint3 c)
{
    uint i = c.x + gResolution * (c.y + gResolution * c.z);
    return (gVoxels[i >> 5] >> (i & 31)) & 1;
}

// ───────── EmptyCell: сторона наибольшего пустого куба вокруг вокселя; 0 — воксель занят ─────────
uint BlockEmptyCell(uint3 c)
{
    uint3 bc = c >> 4;
    uint block = gVoxels[bc.x + gBlocksPerSide * (bc.y + gBlocksPerSide * bc.z)];
    if (block == 0)
        return BLOCK_SIDE;
    uint3 lc = c & (BLOCK_SIDE - 1);
    uint local = lc.x + BLOCK_SIDE * (lc.y + BLOCK_SIDE * lc.z);
    uint word = gVoxels[gDirectorySize + (block - 1) * BLOCK_WORDS + (local >> 5)];
    return ((word >> (local & 31)) & 1) ? 0 : 1;
}

uint Octant(uint3 c, uint shift)
{
    return ((c.x >> shift) & 1) | (((c.y >> shift) & 1) << 1) | (((c.z >> shift) & 1) << 2);
}

uint SvoEmptyCell(uint3 c)
{
    uint node = 0;
    [loop] for (uint level = 0; level < gDepth; ++level)
    {
        uint octant = Octant(c, gDepth - 1 - level);
        uint mask = gVoxels[2 * node];
        if (((mask >> octant) & 1) == 0)
            return gResolution >> (level + 1);
        if (level + 1 == gDepth)
            return 0;
        node = gVoxels[2 * node + 1] + countbits(mask & ((1u << octant) - 1));
    }
    return 0;
}

uint DagEmptyCell(uint3 c)
{
    uint pointer = 0;
    [loop] for (uint level = 0; level < gDepth; ++level)
    {
        uint octant = Octant(c, gDepth - 1 - level);
        uint mask = gVoxels[pointer];
        if (((mask >> octant) & 1) == 0)
            return gResolution >> (level + 1);
        if (level + 1 == gDepth)
            return 0;
        pointer = gVoxels[pointer + 1 + countbits(mask & ((1u << octant) - 1))];
    }
    return 0;
}

uint EmptyCell(uint3 c)
{
    // gMode одинаков для всего прохода — ветвление когерентное
    if (gMode == MODE_BLOCK)
        return BlockEmptyCell(c);
    if (gMode == MODE_SVO)
        return SvoEmptyCell(c);
    return DagEmptyCell(c);
}

// ───────── 3D-DDA [Amanatides, Woo 1987] для плотной сетки ─────────
bool TraceDda(float3 o, float3 d, out uint steps, out float tHit)
{
    steps = 0;
    tHit = 0.0f;
    float3 invD = 1.0f / d;
    float t0, t1;
    if (!IntersectBox(o, invD, float(gResolution), t0, t1))
        return false;
    int n = int(gResolution);
    int3 c = clamp(int3(floor(o + d * t0)), 0, n - 1);
    int3 stepDir = int3(sign(d));
    float3 tMax = (float3(c + int3(d > 0.0f)) - o) * invD;
    float3 tDelta = abs(invD);
    float t = t0;
    [loop] for (uint i = 0; i < MAX_STEPS; ++i)
    {
        ++steps;
        if (DenseGet(uint3(c)))
        {
            tHit = t;
            return true;
        }
        if (tMax.x < tMax.y && tMax.x < tMax.z) { t = tMax.x; c.x += stepDir.x; tMax.x += tDelta.x; }
        else if (tMax.y < tMax.z)               { t = tMax.y; c.y += stepDir.y; tMax.y += tDelta.y; }
        else                                    { t = tMax.z; c.z += stepDir.z; tMax.z += tDelta.z; }
        if (any(c < 0) || any(c >= n))
            return false;
    }
    return false;
}

// ───────── Иерархический обход: выход из наибольшего пустого куба за один шаг ─────────
// Ячейка на оси выхода вычисляется целочисленно, поэтому сдвиг по t на эпсилон не нужен.
bool TraceHierarchical(float3 o, float3 d, out uint steps, out float tHit)
{
    steps = 0;
    tHit = 0.0f;
    float3 invD = 1.0f / d;
    float t0, t1;
    if (!IntersectBox(o, invD, float(gResolution), t0, t1))
        return false;
    int n = int(gResolution);
    float t = t0;
    int3 c = clamp(int3(floor(o + d * t)), 0, n - 1);
    [loop] for (uint i = 0; i < MAX_STEPS; ++i)
    {
        uint size = EmptyCell(uint3(c));
        ++steps;
        if (size == 0)
        {
            tHit = t;
            return true;
        }
        int s = int(size);
        int3 bound = (c / s) * s + int3(d > 0.0f) * s;   // c >= 0, деление корректно
        float3 tb = (float3(bound) - o) * invD;
        uint axis = (tb.x < tb.y) ? ((tb.x < tb.z) ? 0 : 2) : ((tb.y < tb.z) ? 1 : 2);
        t = max(t, axis == 0 ? tb.x : (axis == 1 ? tb.y : tb.z));
        if (t >= t1)
            return false;
        // Луч движется монотонно: координата ячейки по каждой оси не может откатиться назад.
        // Без этого на ребре, где две границы пересекаются при одном t, обход зацикливается.
        int3 next = int3(floor(o + d * t));
        int3 forward = int3(d > 0.0f);
        c = forward * max(next, c) + (1 - forward) * min(next, c);
        // явные ветви вместо c[axis] = ...: FXC не всегда допускает запись по динамическому индексу
        if (axis == 0)      c.x = (d.x > 0.0f) ? bound.x : bound.x - 1;
        else if (axis == 1) c.y = (d.y > 0.0f) ? bound.y : bound.y - 1;
        else                c.z = (d.z > 0.0f) ? bound.z : bound.z - 1;
        if (any(c < 0) || any(c >= n))
            return false;
    }
    return false;
}

float4 main(float4 pixel : SV_Position) : SV_Target
{
    float2 ndc = float2(pixel.x / float(gWidth) * 2.0f - 1.0f, 1.0f - pixel.y / float(gHeight) * 2.0f);
    float3 dir = normalize(gForward + ndc.x * gRight + ndc.y * gUp);
    dir += float3(abs(dir) < 1e-7f) * 1e-7f;   // исключить деление на ноль в 1/d

    uint steps;
    float tHit;
    bool hit;
    if (gMode == MODE_DENSE)
        hit = TraceDda(gCameraPosition, dir, steps, tHit);
    else
        hit = TraceHierarchical(gCameraPosition, dir, steps, tHit);

    if (gShowSteps != 0)
    {
        // Тепловая карта числа шагов (иллюстрация к оценке сложности обхода).
        // Шкала логарифмическая и общая для всех представлений: от 1 шага до 3N + 1
        // (худший случай DDA). Линейная шкала с фиксированным пределом насыщается для плотной
        // сетки (сотни шагов) и не различает иерархические структуры (единицы шагов).
        float h = saturate(log2(1.0f + float(steps)) / log2(2.0f + 3.0f * float(gResolution)));
        return float4(h, h * h, 1.0f - h, 1.0f);
    }
    if (!hit)
        return float4(0.1f, 0.1f, 0.15f, 1.0f);
    float shade = 1.0f - saturate(tHit / (3.0f * float(gResolution)));
    return float4(shade * float3(0.9f, 0.85f, 0.7f), 1.0f);
}
