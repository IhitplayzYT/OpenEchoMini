#include "echo/firmware_image.hpp"

#include <exception>
#include <iostream>
#include <string_view>

int main(int argc, char** argv) {
    if (argc != 2 && argc != 4 && argc != 5) {
        std::cerr << "Usage: echo-fw-inspect <HIFIEC40.IMG> [--export-images|--export-modules <directory>]\n"
                     "       echo-fw-inspect <HIFIEC40.IMG> --apply-images <bmp-directory> <output.IMG>\n";
        return 2;
    }
    const auto option = argc == 4 ? std::string_view(argv[2]) : std::string_view{};
    if (argc == 4 && option != "--export-images" && option != "--export-modules") {
        std::cerr << "Unknown option: " << argv[2] << '\n';
        return 2;
    }
    if (argc == 5 && std::string_view(argv[2]) != "--apply-images") {
        std::cerr << "Unknown option: " << argv[2] << '\n';
        return 2;
    }

    try {
        const auto image = echo::firmware::FirmwareImage::open(argv[1]);
        image.print_report(std::cout);
        if (argc == 4 && option == "--export-images") {
            const auto count = image.export_bitmaps(argv[3]);
            std::cout << "Exported " << count << " bitmaps to " << argv[3] << '\n';
        }
        if (argc == 4 && option == "--export-modules") {
            const auto count = image.export_modules(argv[3]);
            std::cout << "Exported " << count << " module code/data slices to " << argv[3] << '\n';
        }
        if (argc == 5) {
            const auto count = image.apply_bitmaps(argv[3], argv[4]);
            std::cout << "Applied " << count << " bitmap replacements to " << argv[4] << '\n';
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "echo-fw-inspect: " << error.what() << '\n';
        return 1;
    }
}
