#include <chrono>
#include <fstream>
#include <iomanip>
#include <string>

#include "bmp.hpp"
#include "filters.hpp"

int main(int argc, char const* argv[]) {
    auto start_clk = std::chrono::high_resolution_clock::now();
    std::string file_name(argv[1]);
    std::ifstream input_file(file_name);
    BMP bmp;
    input_file >> bmp;
    input_file.close();
    bmp = emboss_filter(bmp);
    bmp = horizontal_mirror_filter(bmp);
    bmp = draw_rhombus(bmp);
    std::ofstream output_file("out.bmp");
    output_file << bmp;
    output_file.close();
    auto end_clk = std::chrono::high_resolution_clock::now();
    auto time_taken = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(
        end_clk - start_clk);
    std::cout << "Execution Time: " << time_taken.count() << '\n';
}
