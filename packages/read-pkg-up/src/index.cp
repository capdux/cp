// packages/@cap/read-pkg-up/src/index.cp
// Zero-allocation upward package.json finder, parser, and normalizer for CP.
// Clean-room MIT pure CP implementation reusing `@cap/find-up` and `@cap/read-pkg`.

import { strEq } from "cap:ascii";
import {
    FindUpOptions,
    FindUpWalker,
} from "../../find-up/src/index.cp";
import {
    ReadPkgResult,
    parsePackageJson,
} from "../../read-pkg/src/index.cp";

/// In-memory virtual filesystem entry pairing a full path with its package.json content.
pub struct PkgFileEntry {
    pub path: string,
    pub content: string,
}

/// Result of searching upward for the closest package.json and parsing its metadata.
pub struct ReadPkgUpResult {
    pub found: bool,
    pub pkg_path_len: usize,
    pub pkg: ReadPkgResult,
}

fn rpuCopyBytes(out: []u8, src: string): usize {
    let i: usize = 0;
    while (i < src.len && i < out.len) {
        out[i] = src[i];
        i += 1;
    }
    return i;
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
    let opts: FindUpOptions = FindUpOptions {
        cwd: cwd,
        stop_at: stop_at,
        allow_symlinks: true,
    };
    let walker: FindUpWalker = FindUpWalker.new(opts);
    let cand: [512]u8 = undefined;

    while (true) {
        let n_opt: usize? = walker.nextCandidate("package.json", cand[0..512]);
        if (n_opt == null) {
            break;
        }
        let n: usize = n_opt ?? 0;
        let cand_str: string = cand[0..n] as string;

        let i: usize = 0;
        while (i < entries.len) {
            if (strEq(entries[i].path, cand_str)) {
                let written: usize = rpuCopyBytes(path_out, cand_str);
                let parsed_pkg: ReadPkgResult = parsePackageJson(entries[i].content, normalize);
                return ReadPkgUpResult {
                    found: true,
                    pkg_path_len: written,
                    pkg: parsed_pkg,
                };
            }
            i += 1;
        }
    }

    return ReadPkgUpResult {
        found: false,
        pkg_path_len: 0,
        pkg: parsePackageJson("", false),
    };
}
