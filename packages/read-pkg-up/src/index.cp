// packages/@cap/read-pkg-up/src/index.cp
// Zero-allocation upward package.json / cap.json finder, parser, and normalizer for CP.
// Clean-room MIT pure CP implementation reusing `@cap/find-up` and `@cap/read-pkg`.

import { strEq, isSpace } from "cap:ascii";
import {
    FindUpOptions,
    FindUpWalker,
    joinDirAndName,
} from "../../find-up/src/index.cp";
import {
    ReadPkgResult,
    parsePackageJson,
} from "../../read-pkg/src/index.cp";

/// In-memory virtual filesystem entry pairing a full path with its manifest content.
pub struct PkgFileEntry {
    pub path: string,
    pub content: string,
}

/// Options for `readPackageUpWithOptions` matching npm `read-pkg-up` + CP `cap.json` support.
pub struct ReadPkgUpOptions {
    pub cwd: string,
    pub stop_at: string,
    pub normalize: bool,
    pub prefer_cap_json: bool,

    pub static fn defaults(cwd: string): ReadPkgUpOptions {
        return ReadPkgUpOptions {
            cwd: cwd,
            stop_at: "",
            normalize: true,
            prefer_cap_json: false,
        };
    }
}

/// Lightweight manifest summary extracting `name`, `version`, and `description` without heap allocation.
pub struct PkgQuickMeta {
    pub name: string,
    pub version: string,
    pub description: string,
    pub has_name: bool,
    pub has_version: bool,
    pub has_description: bool,
}

/// Result of searching upward for the closest manifest and parsing its metadata.
pub struct ReadPkgUpResult {
    pub found: bool,
    pub pkg_path_len: usize,
    pub pkg: ReadPkgResult,
}

fn rpuCopyBytes(out: []u8, src: string): usize {
    let i: usize = 0;
    while (i < src.len && i < out.len) : (i += 1) {
        out[i] = src[i];
    }
    return i;
}

/// Step one directory level up from `path` (POSIX `/` or Windows `\`).
/// Returns `null` when already at root (`/`) or when `path` is empty / has no parent.
pub fn parentDir(path: string): string? {
    if (path.len == 0) {
        return null;
    }
    let end: usize = path.len;
    while (end > 1 && (path[end - 1] == '/' || path[end - 1] == '\\')) : (end -= 1) {}
    if (end == 1 && (path[0] == '/' || path[0] == '\\')) {
        return null;
    }
    let i: usize = end;
    while (i > 0) {
        i -= 1;
        if (path[i] == '/' || path[i] == '\\') {
            if (i == 0) {
                return path[0..1];
            }
            return path[0..i];
        }
    }
    return null;
}

/// Join directory `dir` and filename `name` into `out`, normalizing trailing slashes.
pub fn joinPath(dir: string, name: string, out: []u8): usize {
    return joinDirAndName(dir, name, out);
}

/// Zero-allocation top-level JSON string field extractor for `"key": "value"` pairs.
/// Skips nested objects/arrays so inner `"name"` / `"description"` keys do not shadow top-level fields.
pub fn extractPkgStringField(json: string, key: string): string? {
    let i: usize = 0;
    let depth: i32 = 0;
    while (i < json.len) {
        const c = json[i];
        if (c == '{' || c == '[') {
            depth += 1;
            i += 1;
            continue;
        }
        if (c == '}' || c == ']') {
            depth -= 1;
            i += 1;
            continue;
        }
        if (c == '"') {
            const k_start: usize = i + 1;
            i += 1;
            while (i < json.len) {
                if (json[i] == '\\' && i + 1 < json.len) {
                    i += 2;
                    continue;
                }
                if (json[i] == '"') {
                    break;
                }
                i += 1;
            }
            if (i >= json.len) {
                return null;
            }
            const k_end: usize = i;
            i += 1; // skip closing quote

            if (depth == 1) {
                let j: usize = i;
                while (j < json.len && isSpace(json[j])) : (j += 1) {}
                if (j < json.len && json[j] == ':') {
                    j += 1;
                    while (j < json.len && isSpace(json[j])) : (j += 1) {}
                    if (j < json.len && json[j] == '"') {
                        const v_start: usize = j + 1;
                        j += 1;
                        while (j < json.len) {
                            if (json[j] == '\\' && j + 1 < json.len) {
                                j += 2;
                                continue;
                            }
                            if (json[j] == '"') {
                                break;
                            }
                            j += 1;
                        }
                        if (j >= json.len) {
                            return null;
                        }
                        const v_end: usize = j;
                        if (strEq(json[k_start..k_end], key)) {
                            return json[v_start..v_end];
                        }
                        i = j + 1;
                        continue;
                    }
                }
            }
            continue;
        }
        i += 1;
    }
    return null;
}

/// Extract `name`, `version` (or CP `ver`), and `description` from a `package.json` or `cap.json` payload.
pub fn parseQuickPkgMeta(json: string): PkgQuickMeta {
    const name_opt = extractPkgStringField(json, "name");
    let ver_opt = extractPkgStringField(json, "version");
    if (ver_opt == null) {
        ver_opt = extractPkgStringField(json, "ver");
    }
    const desc_opt = extractPkgStringField(json, "description");
    return PkgQuickMeta {
        name: name_opt ?? "",
        version: ver_opt ?? "",
        description: desc_opt ?? "",
        has_name: name_opt != null,
        has_version: ver_opt != null,
        has_description: desc_opt != null,
    };
}

/// Search upward from `cwd` (stopping inclusively at `stop_at` if non-empty) for the closest
/// manifest named `filename` (e.g. `"package.json"` or `"cap.json"`) in `entries`.
pub fn readPackageUpNamed(
    cwd: string,
    stop_at: string,
    filename: string,
    normalize: bool,
    entries: []PkgFileEntry,
    path_out: []u8,
): ReadPkgUpResult {
    const opts: FindUpOptions = FindUpOptions {
        cwd: cwd,
        stop_at: stop_at,
        allow_symlinks: true,
    };
    let walker: FindUpWalker = FindUpWalker.new(opts);
    let cand: [512]u8 = undefined;

    while (true) {
        const n_opt: usize? = walker.nextCandidate(filename, cand[0..512]);
        if (n_opt == null) {
            break;
        }
        const n: usize = n_opt ?? 0;
        const cand_str: string = cand[0..n] as string;

        let i: usize = 0;
        while (i < entries.len) : (i += 1) {
            if (strEq(entries[i].path, cand_str)) {
                const written: usize = rpuCopyBytes(path_out, cand_str);
                const parsed_pkg: ReadPkgResult = parsePackageJson(entries[i].content, normalize);
                return ReadPkgUpResult {
                    found: true,
                    pkg_path_len: written,
                    pkg: parsed_pkg,
                };
            }
        }
    }

    return ReadPkgUpResult {
        found: false,
        pkg_path_len: 0,
        pkg: parsePackageJson("", false),
    };
}

/// Search upward from `cwd` (stopping inclusively at `stop_at` if non-empty) for the closest
/// `package.json` in `entries`. When found, copies the matched path into `path_out` and returns
/// the parsed (and optionally normalized) result via `@cap/read-pkg`.
pub fn readPackageUpInEntries(
    cwd: string,
    stop_at: string,
    normalize: bool,
    entries: []PkgFileEntry,
    path_out: []u8,
): ReadPkgUpResult {
    return readPackageUpNamed(cwd, stop_at, "package.json", normalize, entries, path_out);
}

/// Search upward using `ReadPkgUpOptions`. When `opts.prefer_cap_json` is `true`, checks
/// `cap.json` before `package.json` at each directory level while walking upward.
pub fn readPackageUpWithOptions(
    opts: ReadPkgUpOptions,
    entries: []PkgFileEntry,
    path_out: []u8,
): ReadPkgUpResult {
    if (!opts.prefer_cap_json) {
        return readPackageUpNamed(opts.cwd, opts.stop_at, "package.json", opts.normalize, entries, path_out);
    }

    const fopts: FindUpOptions = FindUpOptions {
        cwd: opts.cwd,
        stop_at: opts.stop_at,
        allow_symlinks: true,
    };
    let walker: FindUpWalker = FindUpWalker.new(fopts);
    let cand_cap: [512]u8 = undefined;
    let cand_pkg: [512]u8 = undefined;

    while (!walker.done) {
        const cur_dir = walker.current;
        if (cur_dir.len == 0) {
            break;
        }
        const n_cap = joinDirAndName(cur_dir, "cap.json", cand_cap[0..512]);
        const cap_str: string = cand_cap[0..n_cap] as string;
        let i: usize = 0;
        while (i < entries.len) : (i += 1) {
            if (strEq(entries[i].path, cap_str)) {
                const written = rpuCopyBytes(path_out, cap_str);
                return ReadPkgUpResult {
                    found: true,
                    pkg_path_len: written,
                    pkg: parsePackageJson(entries[i].content, opts.normalize),
                };
            }
        }

        const n_opt = walker.nextCandidate("package.json", cand_pkg[0..512]);
        if (n_opt == null) {
            break;
        }
        const n_pkg = n_opt ?? 0;
        const pkg_str: string = cand_pkg[0..n_pkg] as string;
        i = 0;
        while (i < entries.len) : (i += 1) {
            if (strEq(entries[i].path, pkg_str)) {
                const written = rpuCopyBytes(path_out, pkg_str);
                return ReadPkgUpResult {
                    found: true,
                    pkg_path_len: written,
                    pkg: parsePackageJson(entries[i].content, opts.normalize),
                };
            }
        }
    }

    return ReadPkgUpResult {
        found: false,
        pkg_path_len: 0,
        pkg: parsePackageJson("", false),
    };
}

export {
    PkgFileEntry,
    ReadPkgUpOptions,
    PkgQuickMeta,
    ReadPkgUpResult,
    parentDir,
    joinPath,
    extractPkgStringField,
    parseQuickPkgMeta,
    readPackageUpNamed,
    readPackageUpInEntries,
    readPackageUpWithOptions,
};
