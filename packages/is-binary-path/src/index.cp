// is-binary-path — Zero-allocation binary file extension checker in pure CP
// Clean-room MIT implementation of npm `is-binary-path` (#534, 107M/wk) & `binary-extensions`.

import { eqIgnoreCase } from "cap:ascii";

/// High-level binary file category classification.
pub enum BinaryCategory {
    None,
    Image,
    AudioVideo,
    Archive,
    Executable,
    Font,
    DocumentData,
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

/// Classifies `ext` (without leading dot, case-insensitive) into a `BinaryCategory`.
pub fn classifyBinaryExtension(ext: string): BinaryCategory {
    if (ext.len == 0 || ext.len > 12) {
        return .None;
    }
    // Images & graphics
    if (eqIgnoreCase(ext, "png") || eqIgnoreCase(ext, "jpg") || eqIgnoreCase(ext, "jpeg") ||
        eqIgnoreCase(ext, "gif") || eqIgnoreCase(ext, "webp") || eqIgnoreCase(ext, "avif") ||
        eqIgnoreCase(ext, "bmp") || eqIgnoreCase(ext, "ico") || eqIgnoreCase(ext, "icns") ||
        eqIgnoreCase(ext, "tif") || eqIgnoreCase(ext, "tiff") || eqIgnoreCase(ext, "psd") ||
        eqIgnoreCase(ext, "heic") || eqIgnoreCase(ext, "heif") || eqIgnoreCase(ext, "cur") ||
        eqIgnoreCase(ext, "dds") || eqIgnoreCase(ext, "tga") || eqIgnoreCase(ext, "jp2") ||
        eqIgnoreCase(ext, "jxl") || eqIgnoreCase(ext, "exr") || eqIgnoreCase(ext, "hdr") ||
        eqIgnoreCase(ext, "raw") || eqIgnoreCase(ext, "cr2") || eqIgnoreCase(ext, "nef") ||
        eqIgnoreCase(ext, "arw") || eqIgnoreCase(ext, "dng") || eqIgnoreCase(ext, "eps")) {
        return .Image;
    }
    // Audio & video
    if (eqIgnoreCase(ext, "mp3") || eqIgnoreCase(ext, "mp4") || eqIgnoreCase(ext, "m4a") ||
        eqIgnoreCase(ext, "m4v") || eqIgnoreCase(ext, "wav") || eqIgnoreCase(ext, "flac") ||
        eqIgnoreCase(ext, "ogg") || eqIgnoreCase(ext, "oga") || eqIgnoreCase(ext, "ogv") ||
        eqIgnoreCase(ext, "opus") || eqIgnoreCase(ext, "aac") || eqIgnoreCase(ext, "wma") ||
        eqIgnoreCase(ext, "wmv") || eqIgnoreCase(ext, "avi") || eqIgnoreCase(ext, "mov") ||
        eqIgnoreCase(ext, "mkv") || eqIgnoreCase(ext, "webm") || eqIgnoreCase(ext, "flv") ||
        eqIgnoreCase(ext, "3gp") || eqIgnoreCase(ext, "3g2") || eqIgnoreCase(ext, "mid") ||
        eqIgnoreCase(ext, "midi") || eqIgnoreCase(ext, "aif") || eqIgnoreCase(ext, "aiff") ||
        eqIgnoreCase(ext, "mpeg") || eqIgnoreCase(ext, "mpg") || eqIgnoreCase(ext, "m2v") ||
        eqIgnoreCase(ext, "mxf") || eqIgnoreCase(ext, "vob")) {
        return .AudioVideo;
    }
    // Archives & compression
    if (eqIgnoreCase(ext, "zip") || eqIgnoreCase(ext, "tar") || eqIgnoreCase(ext, "gz") ||
        eqIgnoreCase(ext, "tgz") || eqIgnoreCase(ext, "bz2") || eqIgnoreCase(ext, "xz") ||
        eqIgnoreCase(ext, "zst") || eqIgnoreCase(ext, "zstd") || eqIgnoreCase(ext, "lz4") ||
        eqIgnoreCase(ext, "lz") || eqIgnoreCase(ext, "lzma") || eqIgnoreCase(ext, "br") ||
        eqIgnoreCase(ext, "7z") || eqIgnoreCase(ext, "rar") || eqIgnoreCase(ext, "cab") ||
        eqIgnoreCase(ext, "deb") || eqIgnoreCase(ext, "rpm") || eqIgnoreCase(ext, "dmg") ||
        eqIgnoreCase(ext, "iso") || eqIgnoreCase(ext, "img") || eqIgnoreCase(ext, "apk") ||
        eqIgnoreCase(ext, "ipa") || eqIgnoreCase(ext, "jar") || eqIgnoreCase(ext, "war") ||
        eqIgnoreCase(ext, "ear") || eqIgnoreCase(ext, "whl") || eqIgnoreCase(ext, "egg") ||
        eqIgnoreCase(ext, "asar") || eqIgnoreCase(ext, "crx") || eqIgnoreCase(ext, "xpi") ||
        eqIgnoreCase(ext, "cpio") || eqIgnoreCase(ext, "lha") || eqIgnoreCase(ext, "z")) {
        return .Archive;
    }
    // Executables, bytecode, native libraries & objects
    if (eqIgnoreCase(ext, "exe") || eqIgnoreCase(ext, "dll") || eqIgnoreCase(ext, "so") ||
        eqIgnoreCase(ext, "dylib") || eqIgnoreCase(ext, "node") || eqIgnoreCase(ext, "wasm") ||
        eqIgnoreCase(ext, "o") || eqIgnoreCase(ext, "obj") || eqIgnoreCase(ext, "a") ||
        eqIgnoreCase(ext, "lib") || eqIgnoreCase(ext, "pdb") || eqIgnoreCase(ext, "bin") ||
        eqIgnoreCase(ext, "dat") || eqIgnoreCase(ext, "class") || eqIgnoreCase(ext, "pyc") ||
        eqIgnoreCase(ext, "pyo") || eqIgnoreCase(ext, "pyd") || eqIgnoreCase(ext, "beam") ||
        eqIgnoreCase(ext, "msi") || eqIgnoreCase(ext, "app") || eqIgnoreCase(ext, "elf") ||
        eqIgnoreCase(ext, "dex") || eqIgnoreCase(ext, "bc") || eqIgnoreCase(ext, "rlib") ||
        eqIgnoreCase(ext, "ko") || eqIgnoreCase(ext, "sys")) {
        return .Executable;
    }
    // Fonts
    if (eqIgnoreCase(ext, "woff") || eqIgnoreCase(ext, "woff2") || eqIgnoreCase(ext, "ttf") ||
        eqIgnoreCase(ext, "otf") || eqIgnoreCase(ext, "eot")) {
        return .Font;
    }
    // Documents, databases & 3D/media assets
    if (eqIgnoreCase(ext, "pdf") || eqIgnoreCase(ext, "doc") || eqIgnoreCase(ext, "docx") ||
        eqIgnoreCase(ext, "xls") || eqIgnoreCase(ext, "xlsx") || eqIgnoreCase(ext, "ppt") ||
        eqIgnoreCase(ext, "pptx") || eqIgnoreCase(ext, "odt") || eqIgnoreCase(ext, "ods") ||
        eqIgnoreCase(ext, "odp") || eqIgnoreCase(ext, "epub") || eqIgnoreCase(ext, "mobi") ||
        eqIgnoreCase(ext, "sqlite") || eqIgnoreCase(ext, "sqlite3") || eqIgnoreCase(ext, "db") ||
        eqIgnoreCase(ext, "mdb") || eqIgnoreCase(ext, "parquet") || eqIgnoreCase(ext, "avro") ||
        eqIgnoreCase(ext, "pb") || eqIgnoreCase(ext, "onnx") || eqIgnoreCase(ext, "safetensors") ||
        eqIgnoreCase(ext, "gguf") || eqIgnoreCase(ext, "pt") || eqIgnoreCase(ext, "pth") ||
        eqIgnoreCase(ext, "glb") || eqIgnoreCase(ext, "fbx") || eqIgnoreCase(ext, "blend") ||
        eqIgnoreCase(ext, "swf") || eqIgnoreCase(ext, "ds_store")) {
        return .DocumentData;
    }
    return .None;
}

/// Returns true if `ext` (without leading dot, case-insensitive) is a known binary extension.
pub fn isBinaryExtension(ext: string): bool {
    return classifyBinaryExtension(ext) != .None;
}

/// Classifies `file_path` into a `BinaryCategory` based on its file extension.
pub fn classifyBinaryPath(file_path: string): BinaryCategory {
    return classifyBinaryExtension(extractExtension(file_path));
}

/// Returns true if `file_path` ends with a known binary file extension.
pub fn isBinaryPath(file_path: string): bool {
    return classifyBinaryPath(file_path) != .None;
}

/// Returns true if `file_path` is non-empty and does not have a known binary file extension.
pub fn isTextPath(file_path: string): bool {
    if (file_path.len == 0) {
        return false;
    }
    return !isBinaryPath(file_path);
}

/// Convenience predicate: returns true if `file_path` has a known binary image extension.
pub fn isImagePath(file_path: string): bool {
    return classifyBinaryPath(file_path) == .Image;
}

/// Convenience predicate: returns true if `file_path` has a known audio/video extension.
pub fn isMediaPath(file_path: string): bool {
    return classifyBinaryPath(file_path) == .AudioVideo;
}

/// Convenience predicate: returns true if `file_path` has a known archive/compression extension.
pub fn isArchivePath(file_path: string): bool {
    return classifyBinaryPath(file_path) == .Archive;
}

/// Convenience predicate: returns true if `file_path` has a known executable/bytecode extension.
pub fn isExecutablePath(file_path: string): bool {
    return classifyBinaryPath(file_path) == .Executable;
}

/// Convenience predicate: returns true if `file_path` has a known font extension.
pub fn isFontPath(file_path: string): bool {
    return classifyBinaryPath(file_path) == .Font;
}

export {
    BinaryCategory,
    extractExtension,
    classifyBinaryExtension,
    classifyBinaryPath,
    isBinaryExtension,
    isBinaryPath,
    isTextPath,
    isImagePath,
    isMediaPath,
    isArchivePath,
    isExecutablePath,
    isFontPath,
};
