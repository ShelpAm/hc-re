#pragma once

// Use with caution; those are AI-generated.

#include <filesystem>
#include <string>
#include <vector>

namespace hc::archive {

namespace fs = std::filesystem;

// Create or extract .tar.zst archives using libarchive.
// Functions return true on success; on failure they set 'err'.

bool create_tar_zst(fs::path const &out_path,
                    std::vector<fs::path> const &files, std::string &err);

// This may be not needed since we only output tar.zst by not input them.
// bool extract_tar_zst(fs::path const &archive_path, fs::path const &dest_dir,
//                      std::string &err);

} // namespace hc::archive
