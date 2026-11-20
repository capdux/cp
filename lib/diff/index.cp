// Source: cp/lib/diff/index.cp (Unified Exports for cap:diff)
// Pure CP zero-external-dependency Myers O(ND) diff, Unified Diff generator,
// strict context-verifying patch applier, and 3-way merge conflict detector.

export {
    DiffError,
    DiffOpKind,
    DiffEdit,
    DiffStats,
    DiffEngine,
    splitLinesAlloc,
    tokenizeWordsAlloc,
} from "./myers.cp";

export {
    MergeResult,
    UnifiedDiff,
} from "./patch.cp";
