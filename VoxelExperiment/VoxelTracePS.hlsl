// Трассировка первичных лучей по воксельным представлениям (ps_5_0).
// Формулы совпадают с RayTraversal.cpp (CPU-копия для метрики «шагов на луч»).
//
// Соглашения:
//   * сетка занимает куб [0, N]^3 в своих координатах, камера задаётся в них же;
//   * gVoxels — IVoxelGrid::GpuData() (StructuredBuffer<uint>, регистр t0) для блочной сетки, SVO, SVDAG;
//   * gDenseVoxels — то же для плотных сеток, но элементами uint4 (регистр t1): при N = 2048
//     слов 2^28, а представление буфера D3D11 допускает не больше 2^27 элементов;
//   * gMode: 0 — плотная сетка (3D-DDA), 4 — плотная сетка кирпичами 8^3 (3D-DDA),
//     1 — блочная, 2 — SVO, 3 — SVDAG (иерархический обход);
//   * gTraversal (SVO, SVDAG): 0 — спуск от корня на каждом шаге, 1 — со стеком пути.

#define MODE_DENSE       0
#define MODE_BLOCK       1
#define MODE_SVO         2
#define MODE_SVDAG       3
#define MODE_DENSE_BRICK 4
#define TRAVERSAL_RESTART 0
#define TRAVERSAL_STACK   1
#define BLOCK_SIDE 16
#define BLOCK_WORDS 128   // 16^3 бит
#define MAX_STEPS  65536  // > 6N + D + 1 при N = 8192
#define MAX_DEPTH  13     // N <= 8192

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
    uint   gTraversal;
    uint3  gPadding;
};

StructuredBuffer<uint>  gVoxels      : register(t0);
StructuredBuffer<uint4> gDenseVoxels : register(t1);

// Состояние спуска со стеком: узел уровня l, содержащий предыдущую ячейку (static — своё у каждого пикселя).
static uint  sStack[MAX_DEPTH];
static uint  sValidLevel;
static uint3 sPrevCell;

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

// ───────── Плотная сетка: 1 бит на воксель ─────────
// Линейная раскладка x + N(y + Nz); номер слова считается без переполнения 32 бит
// (при N = 2048 номер бита — 2^33): для N >= 32 строка вдоль x занимает целое число слов.
// Кирпичи 8^3: номер кирпича (x>>3) + (N/8)((y>>3) + (N/8)(z>>3)), внутри x + 8(y + 8z), 16 слов на кирпич.
uint DenseGet(uint3 c)
{
    uint word;
    uint bit;
    if (gMode == MODE_DENSE_BRICK)
    {
        uint bricks = gResolution >> 3;
        uint3 b = c >> 3;
        uint3 l = c & 7;
        uint local = l.x + 8 * (l.y + 8 * l.z);
        word = (b.x + bricks * (b.y + bricks * b.z)) * 16 + (local >> 5);
        bit = local & 31;
    }
    else if (gResolution >= 32)
    {
        word = (c.x >> 5) + (gResolution >> 5) * (c.y + gResolution * c.z);
        bit = c.x & 31;
    }
    else
    {
        uint i = c.x + gResolution * (c.y + gResolution * c.z);
        word = i >> 5;
        bit = i & 31;
    }
    uint4 v = gDenseVoxels[word >> 2];
    uint sel = word & 3;
    uint w = sel == 0 ? v.x : (sel == 1 ? v.y : (sel == 2 ? v.z : v.w));
    return (w >> bit) & 1;
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

// Уровень, с которого начинается спуск. Узел уровня l покрывает куб со стороной 2^(D-l);
// предыдущая и текущая ячейки лежат в одном узле уровня l, если совпадают биты с номерами >= D - l,
// поэтому общий предок — уровень D - 1 - firstbithigh(diff). Совпадает с StartLevel в VoxelGrid.cpp.
uint StartLevel(uint3 c)
{
    uint level = 0;
    if (gTraversal == TRAVERSAL_STACK)
    {
        uint diff = (c.x ^ sPrevCell.x) | (c.y ^ sPrevCell.y) | (c.z ^ sPrevCell.z);
        uint ancestor = diff == 0 ? sValidLevel : gDepth - 1 - firstbithigh(diff);
        level = min(sValidLevel, ancestor);
    }
    sPrevCell = c;
    return level;
}

uint SvoEmptyCell(uint3 c)
{
    uint level = StartLevel(c);
    uint node = gTraversal == TRAVERSAL_STACK ? sStack[level] : 0;
    [loop] for (; level < gDepth; ++level)
    {
        if (gTraversal == TRAVERSAL_STACK)
        {
            sStack[level] = node;
            sValidLevel = level;
        }
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
    uint level = StartLevel(c);
    uint pointer = gTraversal == TRAVERSAL_STACK ? sStack[level] : 0;
    [loop] for (; level < gDepth; ++level)
    {
        if (gTraversal == TRAVERSAL_STACK)
        {
            sStack[level] = pointer;
            sValidLevel = level;
        }
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
    sStack[0] = 0;   // корень: узел 0 в SVO, указатель 0 в SVDAG
    sValidLevel = 0;
    sPrevCell = uint3(c);
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
    if (gMode == MODE_DENSE || gMode == MODE_DENSE_BRICK)
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
