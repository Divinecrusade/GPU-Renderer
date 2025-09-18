#include "pch.h"
#include "Graphics.hpp"

namespace graphics {
class ColorClassTest : public ::testing::Test {
 protected:
  static constexpr int kIterations = 1000000;
};

using namespace gpu_renderer;

TEST_F(ColorClassTest, ConstructorValidatesRange) {
  // Valid ranges
  EXPECT_NO_THROW(Graphics::Color(0.0f, 0.5f, 1.0f));
  EXPECT_NO_THROW(Graphics::Color(0.0f, 0.0f, 0.0f));
  EXPECT_NO_THROW(Graphics::Color(1.0f, 1.0f, 1.0f));

#ifdef _DEBUG
  // Invalid ranges should trigger assertions in debug mode
  EXPECT_DEATH(Graphics::Color(-0.1f, 0.5f, 0.5f), ".*");
  EXPECT_DEATH(Graphics::Color(0.5f, 1.1f, 0.5f), ".*");
  EXPECT_DEATH(Graphics::Color(0.5f, 0.5f, -0.1f), ".*");
#endif
}

TEST_F(ColorClassTest, GettersReturnCorrectValues) {
  Graphics::Color color(0.2f, 0.4f, 0.8f);

  EXPECT_FLOAT_EQ(color.GetR(), 0.2f);
  EXPECT_FLOAT_EQ(color.GetG(), 0.4f);
  EXPECT_FLOAT_EQ(color.GetB(), 0.8f);
}

TEST_F(ColorClassTest, OperatorAddressReturnsValidPointer) {
  Graphics::Color color(0.1f, 0.2f, 0.3f);
  const FLOAT* ptr = &color;

  ASSERT_NE(ptr, nullptr);
  EXPECT_FLOAT_EQ(ptr[0], 0.1f);  // R
  EXPECT_FLOAT_EQ(ptr[1], 0.2f);  // G
  EXPECT_FLOAT_EQ(ptr[2], 0.3f);  // B
  EXPECT_FLOAT_EQ(ptr[3], 1.0f);  // A (alpha should be 1.0)
}

TEST_F(ColorClassTest, SettersWorkCorrectly) {
  Graphics::Color color(0.1f, 0.2f, 0.3f);

  color.SetR(0.5f);
  color.SetG(0.6f);  // BUG: writes to R component
  color.SetB(0.7f);  // BUG: writes to R component

  EXPECT_FLOAT_EQ(color.GetR(), 0.5f);
  EXPECT_FLOAT_EQ(color.GetG(), 0.6f); 
  EXPECT_FLOAT_EQ(color.GetB(), 0.7f); 
}

TEST_F(ColorClassTest, SettersValidateRange) {
  Graphics::Color color(0.5f, 0.5f, 0.5f);

  EXPECT_NO_THROW(color.SetR(0.0f));
  EXPECT_NO_THROW(color.SetR(1.0f));

#ifdef _DEBUG
  EXPECT_DEATH(color.SetR(-0.1f), ".*");
  EXPECT_DEATH(color.SetR(1.1f), ".*");
#endif
}

TEST_F(ColorClassTest, MemoryLayout) {
  Graphics::Color color(0.25f, 0.5f, 0.75f);

  // Verify memory is tightly packed (16 bytes total)
  EXPECT_EQ(sizeof(color), 16);  // 4 floats * 4 bytes = 16 bytes

  // Verify alignment suitable for DirectX
  const FLOAT* ptr = &color;
  EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % alignof(FLOAT), 0);
}

TEST_F(ColorClassTest, ColorBoundaryValues) {
  // Test exactly at boundaries
  EXPECT_NO_THROW(Graphics::Color(0.0f, 0.0f, 0.0f));
  EXPECT_NO_THROW(Graphics::Color(1.0f, 1.0f, 1.0f));

  // Test very close to boundaries
  EXPECT_NO_THROW(Graphics::Color(0.000001f, 0.999999f, 0.5f));
}

TEST_F(ColorClassTest, ColorSpecialFloatValues) {
  // Test NaN handling (should be caught by assertions)
  const float nan_val = std::numeric_limits<float>::quiet_NaN();
  const float inf_val = std::numeric_limits<float>::infinity();

#ifdef _DEBUG
  EXPECT_DEATH(Graphics::Color(nan_val, 0.5f, 0.5f), ".*");
  EXPECT_DEATH(Graphics::Color(inf_val, 0.5f, 0.5f), ".*");
#endif
}

TEST_F(ColorClassTest, ColorConstructorPerformance) {
  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < kIterations; ++i) {
    volatile Graphics::Color color(0.5f, 0.5f, 0.5f);
    (void)color;  // Prevent optimization
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  // Should be very fast due to simple array initialization
  EXPECT_LT(duration.count(), 100000);  // < 100ms for 1M constructions

#ifdef _DEBUG
  std::cout << "Color construction time: " << duration.count()
            << " microseconds\n";
#endif
}

TEST_F(ColorClassTest, ColorAccessorPerformance) {
  Graphics::Color color(0.1f, 0.2f, 0.3f);
  volatile float result = 0.0f;

  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < kIterations; ++i) {
    result += color.GetR() + color.GetG() + color.GetB();
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  // Should be extremely fast due to inlining
  EXPECT_LT(duration.count(), 50000);  // < 50ms for 3M accessor calls

#ifdef _DEBUG
  std::cout << "Accessor performance: " << duration.count()
            << " microseconds\n";
  std::cout << "Result (prevent optimization): " << result << "\n";
#endif
}
}  // namespace graphics