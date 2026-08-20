#ifndef OC_PLATFORM_TEST_SUPPORT_H
#define OC_PLATFORM_TEST_SUPPORT_H

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <unistd.h>
#include <vector>

#include "doctest.h"
#include "oc/logging/logger.h"

class ScopedTempDir {
  public:
    ScopedTempDir() {
        const auto parent = std::filesystem::temp_directory_path() / "openchime-tests";
        std::filesystem::create_directories(parent);
        std::string tmpl = (parent / "XXXXXX").string();
        std::vector<char> buf(tmpl.begin(), tmpl.end());
        buf.push_back('\0');
        REQUIRE(mkdtemp(buf.data()) != nullptr);
        path_ = buf.data();
    }

    ~ScopedTempDir() {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }

    ScopedTempDir(const ScopedTempDir &) = delete;
    ScopedTempDir &operator=(const ScopedTempDir &) = delete;

    const std::filesystem::path &path() const { return path_; }

    std::filesystem::path WriteFile(const std::string &name, std::string_view contents) const {
        const auto file_path = path_ / name;
        std::filesystem::create_directories(file_path.parent_path());
        std::ofstream out(file_path);
        REQUIRE(out.is_open());
        out << contents;
        REQUIRE(out.good());
        return file_path;
    }

  private:
    std::filesystem::path path_;
};

class NullLogger final : public oc::logging::Logger {
  public:
    void Log(oc::logging::Level, std::string_view, std::string_view) override {}
};

#endif
