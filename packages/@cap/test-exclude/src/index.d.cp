// Type definitions for CP

export class ExcludeConfig {
    includes: &[]string,
    excludes: &[]string,
    extensions: &[]string,
}

export fn texcHasPrefix(s: string, prefix: string): bool;
export fn texcHasSuffix(s: string, suffix: string): bool;
export fn texcContainsSegment(path: string, seg: string): bool;
export fn texcMatchAt(pat: string, pi: usize, text: string, ti: usize): bool;
export fn texcMatchRaw(pat: string, path: string): bool;
export fn matchSimpleGlob(pattern: string, path: string): bool;
export fn texcMatchesExt(path: string, ext: string): bool;
export fn shouldInstrument(rel_path: string, cfg: ExcludeConfig): bool;
