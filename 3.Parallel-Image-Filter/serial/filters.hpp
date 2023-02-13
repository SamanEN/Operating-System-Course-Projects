#ifndef FILTERS_HPP
#define FILTERS_HPP

#include "bmp.hpp"

BMP emboss_filter(const BMP& input_pic);

BMP horizontal_mirror_filter(BMP input_pic);

BMP draw_rhombus(BMP input_pic);

#endif
