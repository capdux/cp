// Source: cp/lib/diff3/index.cp (Unified Exports for cap:diff3)
// Pure CP zero-external-dependency three-way text merge & conflict resolution engine.

export {
    Diff3Error,
    ConflictStyle,
    ConflictBlock,
    MergeHunk,
    MergeOptions,
    MergeResult,
    mergeLines,
    mergeTexts,
    hasConflicts,
} from "./diff3.cp";
