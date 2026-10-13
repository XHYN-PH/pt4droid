#include "engine/platform/user_data.h"

#include <cassert>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <sys/syscall.h>
#include <unistd.h>

namespace fs = std::filesystem;
static bool block_publication = true;
static int publication_calls = 0;
static int rename_error = ENOSYS;
extern "C" long __real_syscall(long, ...);
extern "C" int __real_link(const char*, const char*);
extern "C" long __wrap_syscall(long number, ...) {
    assert(number == SYS_renameat2);
    ++publication_calls;
    if (block_publication) { errno = rename_error; return -1; }
    va_list args; va_start(args, number);
    const int from_fd = va_arg(args, int);
    const char* from = va_arg(args, const char*);
    const int to_fd = va_arg(args, int);
    const char* to = va_arg(args, const char*);
    const int flags = va_arg(args, int);
    va_end(args);
    return __real_syscall(number, from_fd, from, to_fd, to, flags);
}
extern "C" int __wrap_link(const char* from, const char* to) {
    ++publication_calls;
    if (block_publication) { errno = EPERM; return -1; }
    return __real_link(from, to);
}
static void Write(const fs::path& path, const std::string& contents) {
    fs::create_directories(path.parent_path());
    std::ofstream(path, std::ios::binary) << contents;
}
static std::string Read(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

int main(int argc, char** argv) {
    assert(argc == 4);
    const fs::path root = argv[1];
    rename_error = std::stoi(argv[3]);
    const char* old_marker = ".pt-user-data-migration-v1";
    const char* new_marker = ".pt-user-data-migration-v1.done";
    const auto fresh = root / "fresh";
    auto report = pt::platform::PrepareUserDataDirectory(fresh, {});
    if (std::string(argv[2]) == "baseline") {
        assert(!report.success && !report.errors.empty());
        assert(report.errors.back().find("migration marker") != std::string::npos);
        puts("Test 8 migration marker startup failure reproduced");
        return 0;
    }
    assert(report.success && publication_calls == 0 && fs::is_directory(fresh / new_marker));
    assert(fs::is_empty(fresh / new_marker) && !fs::exists(fresh / old_marker));
    // Completion remains atomic on storage that has neither no-replace rename
    // nor hard links. Deleting a save cannot resurrect it on the next launch.
    const auto legacy = root / "legacy";
    const auto existing = root / "existing";
    Write(legacy / "pt.ini", "old settings"); Write(legacy / "PT_Save_Data0", "old save");
    Write(existing / "pt.ini", "current settings"); Write(existing / "PT_Save_Data0", "current save");
    report = pt::platform::PrepareUserDataDirectory(existing, legacy);
    assert(report.success && report.files_copied == 0 && publication_calls == 0);
    assert(Read(existing / "pt.ini") == "current settings" && Read(existing / "PT_Save_Data0") == "current save");
    fs::remove(existing / "PT_Save_Data0");
    report = pt::platform::PrepareUserDataDirectory(existing, legacy);
    assert(report.success && !fs::exists(existing / "PT_Save_Data0"));
    Write(legacy / "new-save", "must not import after fresh installation");
    report = pt::platform::PrepareUserDataDirectory(fresh, legacy);
    assert(report.success && !fs::exists(fresh / "new-save"));
    // Upgrades recognize the old file marker; malformed markers still fail.
    const auto upgrade = root / "upgrade"; Write(upgrade / old_marker, "1\n");
    report = pt::platform::PrepareUserDataDirectory(upgrade, legacy);
    assert(report.success && !fs::exists(upgrade / "PT_Save_Data0"));
    const auto invalid = root / "invalid"; Write(invalid / new_marker, "must remain");
    report = pt::platform::PrepareUserDataDirectory(invalid, legacy);
    assert(!report.success && Read(invalid / new_marker) == "must remain" && !fs::exists(invalid / "PT_Save_Data0"));
    const auto linked = root / "linked"; fs::create_directories(linked);
    fs::create_directory_symlink(fresh / new_marker, linked / new_marker);
    report = pt::platform::PrepareUserDataDirectory(linked, legacy);
    assert(!report.success && !fs::exists(linked / "PT_Save_Data0"));
    const auto corrupt = root / "corrupt"; Write(corrupt / new_marker / "unexpected", "keep");
    report = pt::platform::PrepareUserDataDirectory(corrupt, legacy);
    assert(!report.success && Read(corrupt / new_marker / "unexpected") == "keep");
    // A failed data import never writes completion or changes existing files.
    const auto failed = root / "failed";
    Write(failed / "pt.ini", "keep settings");
    report = pt::platform::PrepareUserDataDirectory(failed, legacy);
    assert(!report.success && !fs::exists(failed / new_marker) && Read(failed / "pt.ini") == "keep settings");
    // Once normal file publication is available, retry imports the full tree.
    block_publication = false;
    fs::create_directories(legacy / new_marker);
    report = pt::platform::PrepareUserDataDirectory(failed, legacy);
    assert(report.success && report.files_copied == 2);
    assert(Read(failed / "PT_Save_Data0") == "old save" && Read(failed / "new-save") == "must not import after fresh installation");
    assert(fs::is_empty(failed / new_marker) && Read(legacy / "PT_Save_Data0") == "old save");
    // The destination write probe still rejects an unusable folder.
    const auto blocked = root / "blocked"; Write(blocked, "keep file");
    report = pt::platform::PrepareUserDataDirectory(blocked, legacy);
    assert(!report.success && Read(blocked) == "keep file");
    puts("Android migration passed: unsupported rename/link, old and new markers, repeat launch, preserved saves, failed import and retry");
}
