#include <hc/archive.h>

#include <archive.h>
#include <archive_entry.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

namespace hc::archive {

namespace {

std::string path_to_u8string(fs::path const &p)
{
    return p.string();
}

// Very small helper: reject entry names that contain ".." components or are
// absolute. Returns cleaned entry name (no leading '/'). On invalid returns
// empty string and sets err.
std::string sanitize_entry_name(std::string const &name, std::string &err)
{
    if (name.empty()) {
        err = "empty archive entry name";
        return {};
    }
    // strip leading '/'
    std::string n = name;
    if (!n.empty() && n.front() == '/')
        n.erase(0, 1);
    // simple check for ".." components
    fs::path p = fs::path(n);
    for (auto const &comp : p) {
        if (comp == "..") {
            err = "archive entry contains parent-directory reference (..): " +
                  name;
            return {};
        }
    }
    // also avoid entries that would be empty now
    if (n.empty()) {
        err = "archive entry name resolves to empty after stripping";
        return {};
    }
    return n;
}

} // namespace

// create_tar_zst: minimal implementationA
// - supports only regular files (no recursion)
// - stores each input file under its filename (basename) in the archive
// - does not attempt to preserve ownership/complex metadata
bool create_tar_zst(fs::path const &out_path,
                    std::vector<fs::path> const &files, std::string &err)
{
    if (files.empty()) {
        err = "no input files";
        return false;
    }

    struct archive *a = archive_write_new();
    if (a == nullptr) {
        err = "archive_write_new failed";
        return false;
    }

    // Attempt to add zstd filter (requires libarchive built with zstd)
    if (archive_write_add_filter_by_name(a, "zstd") != ARCHIVE_OK) {
        err = std::string("libarchive has no zstd support: ") +
              (archive_error_string(a) ? archive_error_string(a) : "");
        archive_write_free(a);
        return false;
    }

    archive_write_set_format_pax_restricted(a); // portable tar

    std::string out_s = path_to_u8string(out_path);
    if (archive_write_open_filename(a, out_s.c_str()) != ARCHIVE_OK) {
        err = archive_error_string(a);
        archive_write_free(a);
        return false;
    }

    for (auto const &p : files) {
        std::error_code ec;
        fs::path abs = p;
        if (abs.is_relative())
            abs = fs::absolute(abs, ec);
        if (ec)
            abs = p; // fallback to given path

        if (!fs::exists(abs, ec) || !fs::is_regular_file(abs, ec)) {
            err = "path does not exist or is not a regular file: " +
                  path_to_u8string(p);
            archive_write_close(a);
            archive_write_free(a);
            return false;
        }

        // Store basename only
        std::string entry_name = abs.filename().string();
        if (entry_name.empty())
            entry_name = path_to_u8string(abs); // fallback

        struct archive_entry *entry = archive_entry_new();
        archive_entry_set_pathname(entry, entry_name.c_str());

        std::uintmax_t fsize = fs::file_size(abs, ec);
        if (ec)
            fsize = 0;
        archive_entry_set_size(entry, static_cast<la_int64_t>(fsize));

        archive_entry_set_filetype(entry, AE_IFREG);
        archive_entry_set_perm(entry, 0644);

        if (archive_write_header(a, entry) != ARCHIVE_OK) {
            err = archive_error_string(a);
            archive_entry_free(entry);
            archive_write_close(a);
            archive_write_free(a);
            return false;
        }

        // write file contents
        std::ifstream ifs(path_to_u8string(abs), std::ios::binary);
        if (!ifs) {
            err = "failed to open file for reading: " + path_to_u8string(abs);
            archive_entry_free(entry);
            archive_write_close(a);
            archive_write_free(a);
            return false;
        }
        constexpr size_t BUF_SZ = 8192;
        std::vector<char> buf(BUF_SZ);
        while (ifs) {
            ifs.read(buf.data(), static_cast<std::streamsize>(buf.size()));
            std::streamsize r = ifs.gcount();
            if (r > 0) {
                if (archive_write_data(a, buf.data(), static_cast<size_t>(r)) <
                    0) {
                    err = archive_error_string(a);
                    archive_entry_free(entry);
                    archive_write_close(a);
                    archive_write_free(a);
                    return false;
                }
            }
        }

        archive_entry_free(entry);
    }

    archive_write_close(a);
    archive_write_free(a);
    return true;
}

// extract_tar_zst: minimal implementation
// - only extracts regular file entries
// - writes files under dest_dir, preserving entry's pathname but sanitized
// - does not restore ownership or complex metadata
// bool extract_tar_zst(fs::path const &archive_path, fs::path const &dest_dir,
//                      std::string &err)
// {
//     struct archive *a = archive_read_new();
//     if (!a) {
//         err = "archive_read_new failed";
//         return false;
//     }
//
//     // allow zstd filter and tar format
//     archive_read_support_filter_by_name(a, "zstd");
//     archive_read_support_format_tar(a);
//
//     std::string arc_s = path_to_u8string(archive_path);
//     if (archive_read_open_filename(a, arc_s.c_str(), 10240) != ARCHIVE_OK) {
//         err = archive_error_string(a);
//         archive_read_free(a);
//         return false;
//     }
//
//     // ensure dest_dir exists
//     std::error_code ec;
//     if (!fs::exists(dest_dir, ec)) {
//         if (!fs::create_directories(dest_dir, ec)) {
//             err = "failed to create dest dir: " + ec.message();
//             archive_read_close(a);
//             archive_read_free(a);
//             return false;
//         }
//     }
//
//     struct archive_entry *entry = nullptr;
//     int r = ARCHIVE_OK;
//     while ((r = archive_read_next_header(a, &entry)) == ARCHIVE_OK) {
//         char const *pathname = archive_entry_pathname(entry);
//         if (!pathname) {
//             archive_read_data_skip(a);
//             continue;
//         }
//
//         // sanitize entry name
//         std::string sanitized = sanitize_entry_name(pathname, err);
//         if (sanitized.empty()) {
//             // sanitize_entry_name sets err
//             archive_read_close(a);
//             archive_read_free(a);
//             return false;
//         }
//
//         // we only accept regular file entries
//         int filetype = archive_entry_filetype(entry);
//         if (filetype != AE_IFREG) {
//             // skip non-regular entries
//             archive_read_data_skip(a);
//             continue;
//         }
//
//         fs::path outp = dest_dir / fs::path(sanitized);
//         // create parent directories
//         std::error_code ec2;
//         if (outp.has_parent_path())
//             fs::create_directories(outp.parent_path(), ec2);
//
//         std::ofstream ofs(path_to_u8string(outp),
//                           std::ios::binary | std::ios::trunc);
//         if (!ofs) {
//             err = "failed to create output file: " + path_to_u8string(outp);
//             archive_read_close(a);
//             archive_read_free(a);
//             return false;
//         }
//
//         // read data and write to file
//         void const *buff;
//         size_t size;
//         la_int64_t offset;
//         while ((r = archive_read_data_block(a, &buff, &size, &offset)) ==
//                ARCHIVE_OK) {
//             ofs.write(reinterpret_cast<char const *>(buff),
//                       static_cast<std::streamsize>(size));
//             if (!ofs) {
//                 err = "write failed for output file: " +
//                 path_to_u8string(outp); archive_read_close(a);
//                 archive_read_free(a);
//                 return false;
//             }
//         }
//         if (r != ARCHIVE_EOF && r != ARCHIVE_OK) {
//             err = archive_error_string(a);
//             archive_read_close(a);
//             archive_read_free(a);
//             return false;
//         }
//         // finished entry; continue to next
//     }
//
//     if (r != ARCHIVE_EOF) {
//         err = archive_error_string(a);
//         archive_read_close(a);
//         archive_read_free(a);
//         return false;
//     }
//
//     archive_read_close(a);
//     archive_read_free(a);
//     return true;
// }

} // namespace hc::archive
