// Source: cp/lib/jsonpatch/index.cp (Unified Exports for cap:jsonpatch)
// Pure CP RFC 6901 JSON Pointer & RFC 6902 JSON Patch Atomic Engine.

export {
    JsonPatchError,
    PatchOpKind,
    escapeSegment,
    unescapeSegment,
    resolvePointer,
    jsonValuesEqual,
    applyPatch,
    createDiffPatch,
} from "./jsonpatch.cp";
