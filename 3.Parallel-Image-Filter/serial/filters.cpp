#include "filters.hpp"

#include <algorithm>
#include <cmath>

#include "bmp.hpp"

static const float emboss_kernel[3][3] = {
    {-2, -1, 0},
    {-1, 1, 1},
    {0, 1, 2}};

static BMP::Pixel apply_3x3_convolution(
    const BMP& pic,
    const float kernel[3][3],
    const int row,
    const int col) {
    float red = 0, blue = 0, green = 0;

    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            int cur_row = row + i;
            int cur_col = col + j;
            const auto& temp = pic(cur_row, cur_col);
            red += (temp.R_) * kernel[i + 1][j + 1];
            blue += (temp.B_) * kernel[i + 1][j + 1];
            green += (temp.G_) * kernel[i + 1][j + 1];
        }
    }

    BMP::Pixel result;
    result.R_ = (unsigned char)std::min(std::max<int>(red, 0), 255);
    result.B_ = (unsigned char)std::min(std::max<int>(blue, 0), 255);
    result.G_ = (unsigned char)std::min(std::max<int>(green, 0), 255);
    return result;
}

BMP emboss_filter(const BMP& input_pic) {
    BMP pic_cpy(input_pic);
    for (int i = 1; i < input_pic.get_height() - 1; ++i) {
        for (int j = 1; j < input_pic.get_width() - 1; ++j) {
            pic_cpy(i, j) =
                apply_3x3_convolution(
                    input_pic,
                    emboss_kernel,
                    i,
                    j);
        }
    }
    return pic_cpy;
}

BMP horizontal_mirror_filter(BMP input_pic) {
    for (int i = 0; i < input_pic.get_height(); ++i) {
        for (int j = 0; j < std::floor(input_pic.get_width() / 2); ++j) {
            BMP::Pixel temp = input_pic(i, j);
            input_pic(i, j) = input_pic(i, input_pic.get_width() - j - 1);
            input_pic(i, input_pic.get_width() - j - 1) = temp;
        }
    }
    return input_pic;
}

BMP draw_rhombus(BMP input_pic) {
    float slope = (float)input_pic.get_height() / (float)input_pic.get_width();
    for (int i = 0; i < input_pic.get_width() / 2; ++i) {
        input_pic(input_pic.get_height() / 2 - i * slope, i) =
            WHITE;
    }

    for (int i = 0; i < input_pic.get_width() / 2; ++i) {
        input_pic(i * slope, input_pic.get_width() / 2 + i) =
            WHITE;
    }

    for (int i = 0; i < input_pic.get_width() / 2; ++i) {
        input_pic(input_pic.get_height() / 2 + i * slope, i) =
            WHITE;
    }

    for (int i = 0; i < input_pic.get_width() / 2; ++i) {
        input_pic(input_pic.get_height() - 1 - i * slope, input_pic.get_width() / 2 + i) =
            WHITE;
    }
    return input_pic;
}
