// @cap/binary-extensions — Zero-heap O(log n) binary file extension & category lookup in pure CP
// MIT License — Clean-room implementation inspired by npm `binary-extensions` & `is-binary-path`

pub const CAT_NONE: u8 = 0;
pub const CAT_IMAGE: u8 = 1;
pub const CAT_AUDIO: u8 = 2;
pub const CAT_VIDEO: u8 = 3;
pub const CAT_ARCHIVE: u8 = 4;
pub const CAT_FONT: u8 = 5;
pub const CAT_EXECUTABLE: u8 = 6;
pub const CAT_DOCUMENT: u8 = 7;
pub const CAT_DATA_ML: u8 = 8;

const EXT_COUNT: usize = 127;

// Strictly sorted lowercase ASCII extension table for O(log n) binary search
const EXT_NAMES: [127]string = [
    "3gp", "7z", "a", "aac", "aiff", "apk", "ar", "arrow", "avi", "avif",
    "bin", "bmp", "br", "bz2", "cab", "class", "cpio", "cr2", "cur", "dat",
    "db", "dds", "deb", "dex", "dll", "dmg", "dng", "doc", "docx", "dylib",
    "ear", "elf", "eot", "epub", "exe", "exr", "flac", "flv", "gguf", "gif",
    "glb", "gz", "h5", "hdf5", "heic", "heif", "icns", "ico", "img", "iso",
    "jar", "jp2", "jpeg", "jpg", "jxl", "ktx", "lib", "lz", "lz4", "lzma",
    "m4a", "m4v", "mid", "midi", "mkv", "mov", "mp3", "mp4", "mpeg", "mpg",
    "msi", "npy", "npz", "o", "obj", "odp", "ods", "odt", "ogg", "ogv",
    "onnx", "opus", "otf", "parquet", "pb", "pbm", "pdf", "pgm", "pkg", "png",
    "ppm", "ppt", "pptx", "psd", "pt", "pth", "pyc", "pyo", "rar", "rlib",
    "rpm", "safetensors", "so", "sqlite", "sqlite3", "swf", "tar", "tga", "tgz", "tif",
    "tiff", "ttf", "war", "wasm", "wav", "webm", "webp", "whl", "wma", "wmv",
    "woff", "woff2", "xls", "xlsx", "xz", "zip", "zst",
];

const EXT_CATS: [127]u8 = [
    3, 4, 6, 2, 2, 6, 4, 8, 3, 1,
    8, 1, 4, 4, 4, 6, 4, 1, 1, 8,
    8, 1, 6, 6, 6, 6, 1, 7, 7, 6,
    4, 6, 5, 7, 6, 1, 2, 3, 8, 1,
    8, 4, 8, 8, 1, 1, 1, 1, 6, 6,
    6, 1, 1, 1, 1, 1, 6, 4, 4, 4,
    2, 3, 2, 2, 3, 3, 2, 3, 3, 3,
    6, 8, 8, 6, 6, 7, 7, 7, 2, 3,
    8, 2, 5, 8, 8, 1, 7, 1, 6, 1,
    1, 7, 7, 1, 8, 8, 6, 6, 4, 6,
    6, 8, 6, 8, 8, 3, 4, 1, 4, 1,
    1, 5, 4, 6, 2, 3, 1, 4, 2, 3,
    5, 5, 7, 7, 4, 4, 4,
];

fn toLower(ch: u8): u8 {
    if (ch >= 65 && ch <= 90) {
        return ch + 32;
    }
    return ch;
}

/// Compare lowercase `a` (case-folded on the fly) against already-lowercase `b`.
fn cmpLowerToAscii(a: string, b: string): i32 {
    const min_len: usize = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        const ca: u8 = toLower(a[i]);
        const cb: u8 = b[i];
        if (ca < cb) {
            return -1;
        }
        if (ca > cb) {
            return 1;
        }
        i += 1;
    }
    if (a.len < b.len) {
        return -1;
    }
    if (a.len > b.len) {
        return 1;
    }
    return 0;
}

/// Return the binary category constant (`CAT_NONE`..`CAT_DATA_ML`) for `ext`.
/// Case-insensitive; accepts both leading-dot `".png"` and bare `"png"`.
pub fn extensionCategory(ext: string): u8 {
    if (ext.len == 0) {
        return CAT_NONE;
    }
    const clean: string = if (ext[0] == 46) ext[1..ext.len] else ext;
    if (clean.len == 0) {
        return CAT_NONE;
    }

    let lo: usize = 0;
    let hi: usize = EXT_COUNT;
    while (lo < hi) {
        const mid: usize = lo + (hi - lo) / 2;
        const c: i32 = cmpLowerToAscii(clean, EXT_NAMES[mid]);
        if (c == 0) {
            return EXT_CATS[mid];
        }
        if (c < 0) {
            hi = mid;
        } else {
            lo = mid + 1;
        }
    }
    return CAT_NONE;
}

/// Return true if `ext` (with or without leading `.`, case-insensitive) is a known binary extension.
pub fn isBinaryExtension(ext: string): bool {
    return extensionCategory(ext) != CAT_NONE;
}

/// Return true if `path` ends with a filename having a known binary extension.
/// Handles POSIX `/` and Windows `\` separators, ignores dots in parent directories
/// (e.g. `dir.zip/readme.txt` -> false), and treats dotfiles like `.gitignore` as non-binary.
pub fn isBinaryPath(path: string): bool {
    if (path.len == 0) {
        return false;
    }
    // Find start of basename after last '/' or '\\'
    let base_start: usize = 0;
    let i: usize = path.len;
    while (i > 0) {
        i -= 1;
        if (path[i] == 47 || path[i] == 92) {
            base_start = i + 1;
            break;
        }
    }
    if (base_start >= path.len) {
        return false;
    }
    const base: string = path[base_start..path.len];

    // Find last '.' inside basename; index 0 is a dotfile (e.g. ".gitignore"), not an extension
    let dot_idx: usize = 0;
    let found_dot: bool = false;
    let j: usize = base.len;
    while (j > 0) {
        j -= 1;
        if (base[j] == 46) {
            dot_idx = j;
            found_dot = true;
            break;
        }
    }
    if (!found_dot || dot_idx == 0 || dot_idx + 1 >= base.len) {
        return false;
    }
    return isBinaryExtension(base[dot_idx + 1 .. base.len]);
}
