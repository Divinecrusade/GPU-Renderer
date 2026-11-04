// Полноэкранный прямоугольник: вершины уже в пространстве отсечения.
float4 main(float3 pos : Position) : SV_Position
{
    return float4(pos, 1.0f);
}
