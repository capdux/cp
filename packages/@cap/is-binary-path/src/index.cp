// @cap/is-binary-path — Zero-allocation binary file extension checker in pure CP
// Clean-room MIT implementation of npm `is-binary-path` (#534, 107M/wk) & `binary-extensions`.

fn ibpToLower(b: u8): u8 {
    if (b >= 65 && b <= 90) {
        return b + 32;
    }
    return b;
}

fn ibpEqLower(s: string, target: string): bool {
    if (s.len != target.len) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        if (ibpToLower(s[i]) != target[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Extracts the lowercase-comparable extension slice (without leading '.') from a file path.
pub fn extractExtension(file_path: string): string {
    if (file_path.len == 0) {
        return "";
    }
    let i = file_path.len;
    while (i > 0) {
        i -= 1;
        const c = file_path[i];
        if (c == 47 || c == 92) { // '/' or '\'
            return "";
        }
        if (c == 46) { // '.'
            if (i == 0 || file_path[i - 1] == 47 || file_path[i - 1] == 92) {
                return ""; // dotfile like ".png" has no extension
            }
            return file_path[(i + 1)..file_path.len];
        }
    }
    return "";
}

/// Returns true if `ext` (without leading dot, case-insensitive) is a known binary extension.
pub fn isBinaryExtension(ext: string): bool {
    if (ext.len == 0 || ext.len > 12) {
        return false;
    }
    // Images & graphics
    if (ibpEqLower(ext, "png") || ibpEqLower(ext, "jpg") || ibpEqLower(ext, "jpeg") ||
        ibpEqLower(ext, "gif") || ibpEqLower(ext, "webp") || ibpEqLower(ext, "avif") ||
        ibpEqLower(ext, "bmp") || ibpEqLower(ext, "ico") || ibpEqLower(ext, "icns") ||
        ibpEqLower(ext, "tif") || ibpEqLower(ext, "tiff") || ibpEqLower(ext, "psd") ||
        ibpEqLower(ext, "heic") || ibpEqLower(ext, "heif") || ibpEqLower(ext, "cur") ||
        ibpEqLower(ext, "dds") || ibpEqLower(ext, "tga") || ibpEqLower(ext, "jp2") ||
        ibpEqLower(ext, "jxl") || ibpEqLower(ext, "exr") || ibpEqLower(ext, "hdr") ||
        ibpEqLower(ext, "raw") || ibpEqLower(ext, "cr2") || ibpEqLower(ext, "nef")) {
        return true;
    }
    // Audio & video
    if (ibpEqLower(ext, "mp3") || ibpEqLower(ext, "mp4") || ibpEqLower(ext, "m4a") ||
        ibpEqLower(ext, "m4v") || ibpEqLower(ext, "wav") || ibpEqLower(ext, "flac") ||
        ibpEqLower(ext, "ogg") || ibpEqLower(ext, "oga") || ibpEqLower(ext, "ogv") ||
        ibpEqLower(ext, "opus") || ibpEqLower(ext, "aac") || ibpEqLower(ext, "wma") ||
        ibpEqLower(ext, "wmv") || ibpEqLower(ext, "avi") || ibpEqLower(ext, "mov") ||
        ibpEqLower(ext, "mkv") || ibpEqLower(ext, "webm") || ibpEqLower(ext, "flv") ||
        ibpEqLower(ext, "3gp") || ibpEqLower(ext, "mid") || ibpEqLower(ext, "midi") ||
        ibpEqLower(ext, "aif") || ibpEqLower(ext, "aiff") || ibpEqLower(ext, "mpeg") ||
        ibpEqLower(ext, "mpg")) {
        return true;
    }
    // Archives & compression
    if (ibpEqLower(ext, "zip") || ibpEqLower(ext, "tar") || ibpEqLower(ext, "gz") ||
        ibpEqLower(ext, "tgz") || ibpEqLower(ext, "bz2") || ibpEqLower(ext, "xz") ||
        ibpEqLower(ext, "zst") || ibpEqLower(ext, "zstd") || ibpEqLower(ext, "lz4") ||
        ibpEqLower(ext, "lz") || ibpEqLower(ext, "lzma") || ibpEqLower(ext, "br") ||
        ibpEqLower(ext, "7z") || ibpEqLower(ext, "rar") || ibpEqLower(ext, "cab") ||
        ibpEqLower(ext, "deb") || ibpEqLower(ext, "rpm") || ibpEqLower(ext, "dmg") ||
        ibpEqLower(ext, "iso") || ibpEqLower(ext, "img") || ibpEqLower(ext, "apk") ||
        ibpEqLower(ext, "ipa") || ibpEqLower(ext, "jar") || ibpEqLower(ext, "war") ||
        ibpEqLower(ext, "ear") || ibpEqLower(ext, "whl") || ibpEqLower(ext, "egg") ||
        ibpEqLower(ext, "asar") || ibpEqLower(ext, "crx") || ibpEqLower(ext, "xpi")) {
        return true;
    }
    // Executables, bytecode, native libraries & objects
    if (ibpEqLower(ext, "exe") || ibpEqLower(ext, "dll") || ibpEqLower(ext, "so") ||
        ibpEqLower(ext, "dylib") || ibpEqLower(ext, "node") || ibpEqLower(ext, "wasm") ||
        ibpEqLower(ext, "o") || ibpEqLower(ext, "obj") || ibpEqLower(ext, "a") ||
        ibpEqLower(ext, "lib") || ibpEqLower(ext, "pdb") || ibpEqLower(ext, "bin") ||
        ibpEqLower(ext, "dat") || ibpEqLower(ext, "class") || ibpEqLower(ext, "pyc") ||
        ibpEqLower(ext, "pyo") || ibpEqLower(ext, "pyd") || ibpEqLower(ext, "beam") ||
        ibpEqLower(ext, "msi") || ibpEqLower(ext, "app") || ibpEqLower(ext, "elf") ||
        ibpEqLower(ext, "dex") || ibpEqLower(ext, "bc") || ibpEqLower(ext, "rlib")) {
        return true;
    }
    // Fonts
    if (ibpEqLower(ext, "woff") || ibpEqLower(ext, "woff2") || ibpEqLower(ext, "ttf") ||
        ibpEqLower(ext, "otf") || ibpEqLower(ext, "eot")) {
        return true;
    }
    // Documents, databases & 3D/media assets
    if (ibpEqLower(ext, "pdf") || ibpEqLower(ext, "doc") || ibpEqLower(ext, "docx") ||
        ibpEqLower(ext, "xls") || ibpEqLower(ext, "xlsx") || ibpEqLower(ext, "ppt") ||
        ibpEqLower(ext, "pptx") || ibpEqLower(ext, "odt") || ibpEqLower(ext, "ods") ||
        ibpEqLower(ext, "odp") || ibpEqLower(ext, "epub") || ibpEqLower(ext, "mobi") ||
        ibpEqLower(ext, "sqlite") || ibpEqLower(ext, "sqlite3") || ibpEqLower(ext, "db") ||
        ibpEqLower(ext, "mdb") || ibpEqLower(ext, "parquet") || ibpEqLower(ext, "avro") ||
        ibpEqLower(ext, "pb") || ibpEqLower(ext, "onnx") || ibpEqLower(ext, "safetensors") ||
        ibpEqLower(ext, "gguf") || ibpEqLower(ext, "pt") || ibpEqLower(ext, "pth") ||
        ibpEqLower(ext, "glb") || ibpEqLower(ext, "fbx") || ibpEqLower(ext, "blend") ||
        ibpEqLower(ext, "swf") || ibpEqLower(ext, "ds_store")) {
        return true;
    }
    return false;
}

/// Returns true if `file_path` ends with a known binary file extension.
pub fn isBinaryPath(file_path: string): bool {
    const ext = extractExtension(file_path);
    return isBinaryExtension(ext);
}

export {
    extractExtension,
    isBinaryExtension,
    isBinaryPath,
};
