#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <sys/stat.h>

#include "doctest.h"
#include "oc/util/filesystem.h"
#include "test_support.h"

namespace {

std::string ReadText(const std::filesystem::path &path) {
    std::ifstream file(path);
    REQUIRE(file.is_open());
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

ino_t InodeOf(const std::filesystem::path &path) {
    struct stat info;
    REQUIRE(stat(path.c_str(), &info) == 0);
    return info.st_ino;
}

} // namespace

TEST_SUITE("atomic_write") {
    TEST_CASE("rename replacement is all-old or all-new through the S31 symlink") {
        const ScopedTempDir tmp;
        const auto data_dir = tmp.path() / "data";
        const auto etc_dir = tmp.path() / "etc";
        std::filesystem::create_directories(data_dir);
        std::filesystem::create_directories(etc_dir);
        const std::string old_bytes = "schema_version=4\nmqtt_host=old-broker\n";
        const std::string new_bytes = "schema_version=5\nmqtt_host=new-broker\nextra=1\n";
        const auto backing = tmp.WriteFile("data/chime.conf", old_bytes);
        const auto link = etc_dir / "chime.conf";
        std::filesystem::create_symlink(backing, link);

        std::string error;
        REQUIRE(oc::util::AtomicWriteFile(link.string(), new_bytes, 0600, &error));
        CHECK(error.empty());
        CHECK(std::filesystem::is_symlink(link));
        CHECK(ReadText(backing) == new_bytes);
        CHECK(ReadText(link) == new_bytes);
        CHECK(ReadText(backing).find("old-broker") == std::string::npos);

        const ino_t new_inode = InodeOf(backing);
        REQUIRE(chmod(data_dir.c_str(), 0555) == 0);
        const bool failed = !oc::util::AtomicWriteFile(link.string(), "partial-new\n", 0600, &error);
        REQUIRE(chmod(data_dir.c_str(), 0755) == 0);
        REQUIRE(failed);
        CHECK(ReadText(backing) == new_bytes);
        CHECK(ReadText(link) == new_bytes);
        CHECK(InodeOf(backing) == new_inode);
        CHECK(std::filesystem::is_symlink(link));
    }

    TEST_CASE("failed create leaves the original inode and bytes") {
        const ScopedTempDir tmp;
        const std::string original = "keep-me\n";
        const auto path = tmp.WriteFile("chime.conf", original);
        const ino_t inode = InodeOf(path);
        REQUIRE(chmod(tmp.path().c_str(), 0555) == 0);
        std::string error;
        const bool ok = oc::util::AtomicWriteFile(path.string(), "new-content-that-is-longer\n", 0600, &error);
        REQUIRE(chmod(tmp.path().c_str(), 0755) == 0);
        CHECK_FALSE(ok);
        CHECK(ReadText(path) == original);
        CHECK(InodeOf(path) == inode);
    }
}
