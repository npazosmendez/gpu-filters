//
// Created by Daro on 23/09/2018 (changed by Nico, some time later)
//

#include <gtest/gtest.h>

extern "C" {
#include "c/cutils.h"
}

TEST(InpaintingUtilsTests, contour_orthogonal_1) {
    bool maskA[9] = {
        1, 0, 0,
        0, 1, 0,
        1, 1, 0,
    };
    point orthogonal = get_ortogonal_to_contour(1, 1, maskA, 3, 3);
    ASSERT_FLOAT_EQ(orthogonal.x, 0.92387956f);
    ASSERT_FLOAT_EQ(orthogonal.y, -0.38268337f);
}

TEST(InpaintingUtilsTests, contour_orthogonal_2) {
    bool maskA[9] = {
        0, 1, 0,
        0, 1, 0,
        0, 1, 0
    };
    point orthogonal = get_ortogonal_to_contour(1, 1, maskA, 3, 3);
    ASSERT_FLOAT_EQ(orthogonal.x, 1.0f);
    ASSERT_FLOAT_EQ(orthogonal.y, 0.0f);
}

TEST(InpaintingUtilsTests, contour_orthogonal_3) {
    bool maskA[9] = {
        1, 0, 0,
        0, 1, 0,
        0, 0, 1
    };
    point orthogonal = get_ortogonal_to_contour(1, 1, maskA, 3, 3);
    ASSERT_FLOAT_EQ(orthogonal.x, 0.707107f);
    ASSERT_FLOAT_EQ(orthogonal.y, -0.707107f);
}

TEST(InpaintingUtilsTests, contour_orthogonal_4) {
    bool maskA[9] = {
        1, 0, 0,
        1, 1, 0,
        0, 0, 1
    };
    point orthogonal = get_ortogonal_to_contour(1, 1, maskA, 3, 3);
    ASSERT_FLOAT_EQ(orthogonal.x, 0.707107f);
    ASSERT_FLOAT_EQ(orthogonal.y, -0.707107f);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
