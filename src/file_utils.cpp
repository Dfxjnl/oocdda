#include <filesystem>
#include <stdexcept>

#include "file_utils.hpp"

namespace oocdda {
void ensure_save_directory_exists()
{
    const std::filesystem::path save_path {"save"};

    try {
        if (std::filesystem::exists(save_path)) {
            if (std::filesystem::is_directory(save_path)) {
                return;
            }

            throw std::runtime_error("'" + save_path.string() + "' exists but is not a directory");
        }

        if (std::filesystem::create_directory(save_path)) {
            return;
        }

        throw std::runtime_error("Failed to create the '" + save_path.string() + "' directory");
    } catch (const std::filesystem::filesystem_error& e) {
        throw std::runtime_error("Filesystem error while ensuring the '" + save_path.string()
                                 + "' directory exists: " + e.what());
    }
}
} // namespace oocdda
