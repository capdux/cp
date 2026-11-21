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

import linediffMod from "./linediff.cp";
import unifiedMod from "./unified.cp";

export {
    MergeResult,
    UnifiedDiff,
} from "./patch.cp";

export {
    DiffOp,
    DiffResult,
    diffLines,
    reconstructNew,
    formatUnifiedDiff,
} from "./linediff.cp";
export const MyersOpKind = linediffMod.DiffOpKind;

export {
    DiffLine,
    DiffSummary,
    summarize,
    formatUnified,
} from "./unified.cp";
export const UnifiedOpKind = unifiedMod.DiffOpKind;
export const unifiedDiffLines = unifiedMod.diffLines;
