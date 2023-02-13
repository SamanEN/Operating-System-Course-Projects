#ifndef FILTERS_HPP
#define FILTERS_HPP

#include "bmp.hpp"

struct DoPartialArgs {
    DoPartialArgs(BMP& bmp_, const BMP& original_bmp_, int row_begin_, int row_end_)
        : bmp(bmp_),
          original_bmp(original_bmp_),
          row_begin(row_begin_),
          row_end(row_end_) {}
    BMP& bmp;
    const BMP& original_bmp;
    int row_begin;
    int row_end;
};

BMP emboss_filter(const BMP& input_pic);

BMP horizontal_mirror_filter(BMP input_pic);

BMP draw_rhombus(BMP input_pic);

#endif
