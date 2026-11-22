// Source: Absorbed from /data/dev/bun/src/interchange/interchange.cp | Synced: 2026-11-22T11:07:59Z
// lib/conf/index.cp
// Unified Configuration & Interchange Subsystem (cap:conf / cap:dotenv / cap:ini)

export * from "./value.cp";
export * from "./dotenv.cp";
export * from "./ini.cp";
export * from "./toml.cp";
export * from "./yaml_parser.cp";
export * from "./yaml.cp";
export * from "./json5.cp";
export * from "./getter.cp";

// ============================================================================
// Bun-Absorbed Extended Config & Structured Text Modules (Zero-Conflict)
// ============================================================================
export {
    stripJsonc,
    stripJsoncPreserveLines,
    hasJsoncComments,
    countJsoncTrailingCommas,
} from "./jsonc.cp";

export {
    matchSectionPattern,
    IndentStyle,
    EndOfLine,
    Charset,
    EditorConfigProps,
    resolveEditorConfig,
} from "./editorconfig.cp";

export * from "./frontmatter.cp";
export * from "./makefile_parse.cp";

export {
    PathMapping,
    ResolvedCandidate,
    expandConfigDir,
    TsConfigPaths,
} from "./tsconfig_paths.cp";

// ============================================================================
// Pure-CP Extended Config, JSON Extensions & DSL Modules (Zero-Conflict)
// ============================================================================
export * from "./hjson.cp";

export {
    JsonlChunkResult,
    JsonlIter,
    scanCompleteJsonlChunk,
    isBalancedJsonRecord,
    appendJsonlRecord,
} from "./jsonl.cp";

export {
    NdjsonError,
    NdjsonRecord,
    NdjsonStreamParser,
    NdjsonWriter,
    parseAll,
    stringifyAll,
} from "./ndjson.cp";

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

export {
    JSONSCHEMA_MAX_DEPTH,
    JsonSchemaError,
    ValidationIssue,
    ValidationResult,
    SchemaValidator,
} from "./jsonschema.cp";

export * from "./kdl.cp";

export {
    HclTokenKind,
    HclToken,
    HclIter,
    HclBlockHeader,
    HclBlockIter,
    validateHcl,
} from "./hcl.cp";

export {
    CueTokenKind,
    CueToken,
    CueIter,
    validateCue,
    extractCuePackage,
} from "./cue.cp";

export {
    RonTokenKind,
    RonToken,
    RonIter,
    validateRon,
    extractRonField,
} from "./ron.cp";

export {
    NIX_MAX_DEPTH,
    NixTokenKind,
    NixToken,
    NixAttrBinding,
    NixIter,
    NixAttrIter,
    validateNixDelimiters,
} from "./nix.cp";

export {
    PlistValueKind,
    PlistDictEntry,
    isXmlPlist,
    PlistDictIter,
    findPlistString,
    findPlistInt,
    findPlistBool,
    formatPlistStringEntry,
} from "./plist.cp";

export * from "./crontab.cp";
