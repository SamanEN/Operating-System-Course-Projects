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

void* apply_partial_convolution(void* args) {
    DoPartialArgs* args_struct =
        static_cast<DoPartialArgs*>(args);

    auto& row_begin = args_struct->row_begin;
    auto& row_end = args_struct->row_end;
    auto& bmp = args_struct->bmp;
    auto& original_bmp = args_struct->original_bmp;

    for (int i = row_begin; i < row_end; ++i) {
        if (i == original_bmp.get_height() - 1 || i == 0)
            continue;
        for (int j = 1; j < bmp.get_width() - 1; ++j) {
            bmp(i, j) =
                apply_3x3_convolution(
                    original_bmp,
                    emboss_kernel,
                    i,
                    j);
        }
    }
    return NULL;
}

BMP emboss_filter(const BMP& input_pic) {
    BMP pic_cpy(input_pic);

    auto do_partial = [&pic_cpy, &input_pic](int row_begin, int row_end) {
        DoPartialArgs* args = new DoPartialArgs(
            pic_cpy,
            input_pic,
            row_begin,
            row_end);
        pthread_t t_id;
        pthread_create(&t_id, NULL, apply_partial_convolution, (void*)args);
        return t_id;
    };

    std::vector<pthread_t> t_ids;
    int N_PARTS = 8;
    int height = input_pic.get_height() - 1;
    for (int i = 0; i < N_PARTS; ++i) {
        int row_begin = i * (height / N_PARTS);
        int row_end = (i + 1) * (height / N_PARTS);
        t_ids.push_back(do_partial(row_begin, row_end));
    }

    if (height % N_PARTS) {
        int row_begin = (height - (height % N_PARTS));
        int row_end = height;
        t_ids.push_back(do_partial(row_begin, row_end));
    }

    for (auto t_id : t_ids)
        pthread_join(t_id, NULL);

    return pic_cpy;
}

void* apply_partial_horizontal_mirror(void* args) {
    DoPartialArgs* args_struct =
        static_cast<DoPartialArgs*>(args);

    auto& row_begin = args_struct->row_begin;
    auto& row_end = args_struct->row_end;
    auto& bmp = args_struct->bmp;
    auto& original_bmp = args_struct->original_bmp;

    for (int i = row_begin; i < row_end; ++i) {
        for (int j = 0; j < std::floor(bmp.get_width() / 2); ++j) {
            BMP::Pixel temp = bmp(i, j);
            bmp(i, j) = bmp(i, bmp.get_width() - j - 1);
            bmp(i, bmp.get_width() - j - 1) = temp;
        }
    }
    return NULL;
}

BMP horizontal_mirror_filter(BMP input_pic) {
    auto do_partial = [&input_pic](int row_begin, int row_end) {
        DoPartialArgs* args = new DoPartialArgs(
            input_pic,
            input_pic,
            row_begin,
            row_end);
        pthread_t t_id;
        pthread_create(&t_id, NULL, apply_partial_horizontal_mirror, (void*)args);
        return t_id;
    };

    std::vector<pthread_t> t_ids;
    int N_PARTS = 8;
    int height = input_pic.get_height();
    for (int i = 0; i < N_PARTS; ++i) {
        int row_begin = i * (height / N_PARTS);
        int row_end = (i + 1) * (height / N_PARTS);
        t_ids.push_back(do_partial(row_begin, row_end));
    }

    if (height % N_PARTS) {
        int row_begin = (height - (height % N_PARTS));
        int row_end = height;
        t_ids.push_back(do_partial(row_begin, row_end));
    }

    for (auto t_id : t_ids)
        pthread_join(t_id, NULL);

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
