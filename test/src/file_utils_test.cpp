#include <filesystem>
#include <fstream>
#include <stdexcept>

#include <gtest/gtest.h>

#include "file_utils.hpp"

using oocdda::ensure_save_directory_exists;

class SaveDirectoryTest : public ::testing::Test {
protected:
    void SetUp() override { test_directory_path = "save"; }

    void TearDown() override { std::filesystem::remove_all(test_directory_path); }

    std::filesystem::path test_directory_path;
};

TEST_F(SaveDirectoryTest, DirectoryDoesNotExist)
{
    ensure_save_directory_exists();

    EXPECT_TRUE(std::filesystem::exists(test_directory_path));
    EXPECT_TRUE(std::filesystem::is_directory(test_directory_path));
}

TEST_F(SaveDirectoryTest, DirectoryExistsAndIsDirectory)
{
    std::filesystem::create_directories(test_directory_path);

    ASSERT_TRUE(std::filesystem::exists(test_directory_path));
    ASSERT_TRUE(std::filesystem::is_directory(test_directory_path));

    ensure_save_directory_exists();

    EXPECT_TRUE(std::filesystem::exists(test_directory_path));
    EXPECT_TRUE(std::filesystem::is_directory(test_directory_path));
}

TEST_F(SaveDirectoryTest, DirectoryExistsButNotDirectory)
{
    std::ofstream file { test_directory_path };
    file.close();

    ASSERT_TRUE(std::filesystem::exists(test_directory_path));
    ASSERT_FALSE(std::filesystem::is_directory(test_directory_path));

    EXPECT_THROW(ensure_save_directory_exists(), std::runtime_error);
}
