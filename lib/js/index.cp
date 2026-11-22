// Source: Absorbed from /data/dev/bun/src/js_lexer/identifier.cp | Synced: 2026-11-21T17:06:43Z (Unified Exports for cap:js)

export {
    KeywordKind,
    classifyKeyword,
    isReservedWord,
    isValidAsciiIdent,
    isValidPrivateIdent,
    needsPropertyQuoting,
    escapeJsString,
} from "./ident.cp";

export {
    AssignTarget,
    Code,
    Level,
    OpInfo,
    opInfo,
} from "./op.cp";

export {
    UseDirectiveBoundering,
    UseDirectiveFlags,
    UseDirective,
    scanUseDirectives,
} from "./useDirective.cp";

export {
    FOLD_MAX_STR_LEN,
    FOLD_MAX_PART_TAIL_LEN,
    FOLD_MAX_TEMPLATE_PARTS,
    FoldStringKind,
    JsPrimitiveType,
    JsFoldExprKind,
    JsTemplatePart,
    JsFoldExpr,
    formatJsInt,
    foldStringAddition,
} from "./foldString.cp";

export {
    JS_SCOPE_MAX_SCOPES,
    JS_SCOPE_MAX_MEMBERS,
    JS_SCOPE_NO_PARENT,
    JsScopeError,
    JsScopeKind,
    JsStrictModeKind,
    JsSymbolKind,
    JsSymbolMergeResult,
    canMergeJsSymbols,
    JsScopeMember,
    JsScopeNode,
    JsScopeTree,
} from "./scope.cp";

export {
    JS_SYMBOL_MAX_ENTRIES,
    JS_SYMBOL_INVALID_INDEX,
    JsSymbolTableError,
    JsSlotNamespace,
    JsImportItemStatus,
    JsSymbolEntryKind,
    JsSymbolRef,
    JsSymbolRecord,
    JsSymbolTable,
} from "./symbolTable.cp";

export {
    TS_NS_MAX_SCOPES,
    TS_NS_MAX_MEMBERS,
    TS_NS_MAX_PROXIES,
    TsNamespaceError,
    TsNamespaceMemberKind,
    TsNamespaceMember,
    TsNamespaceBlock,
    TsPropertyAccessProxy,
    TsNamespaceRegistry,
} from "./tsNamespace.cp";

export {
    TsMetadataError,
    TsMetadataKind,
    TsDecoratorMetadata,
    TsTypeArgLookaheadToken,
    canFollowTsTypeArguments,
} from "./tsMetadata.cp";

export {
    JsToken,
    lookupJsKeyword,
    isStrictModeReserved,
    remapStrictModeReserved,
    JsPropertyModifier,
    isTsAccessibilityModifier,
    lookupJsxEntity,
} from "./lexerTables.cp";

export {
    IMPORT_TRIM_MAX_ITEMS,
    TsImportsNotUsedAsValues,
    ImportTrimPolicy,
    ImportBindingUsage,
    ImportStmtInput,
    ImportStmtDecision,
    ImportTrimResult,
    evaluateImportStmt,
} from "./importTrim.cp";

export {
    REPL_MAX_STMTS,
    REPL_MAX_HOISTED_VARS,
    ReplTransformError,
    ReplStmtKind,
    ReplWrapMode,
    ReplStmtDesc,
    ReplProgramInput,
    ReplTransformPlan,
    planReplTransform,
} from "./replTransform.cp";

export {
    JS_BINDING_NONE,
    JS_BINDING_MAX_DEPTH,
    JS_BINDING_PROP_NONE,
    JS_BINDING_PROP_IS_SPREAD,
    JS_BINDING_PROP_IS_COMPUTED,
    JS_BINDING_PROP_IS_SHORTHAND,
    JsBindingError,
    JsBindingTag,
    JsArrayBindingItem,
    JsObjectBindingProp,
    JsBindingNode,
    JsBindingArena,
} from "./bindingPattern.cp";

export {
    JS_MEMBER_FLAG_NONE,
    JS_MEMBER_FLAG_STATIC,
    JS_MEMBER_FLAG_COMPUTED,
    JS_MEMBER_FLAG_SPREAD,
    JS_MEMBER_FLAG_METHOD,
    JS_MEMBER_FLAG_PRIVATE,
    JS_MEMBER_FLAG_DECORATED,
    JS_MEMBER_FLAG_READONLY,
    JS_MEMBER_FLAG_OVERRIDE,
    JS_FN_FLAG_NONE,
    JS_FN_FLAG_ASYNC,
    JS_FN_FLAG_GENERATOR,
    JS_FN_FLAG_ARROW,
    JS_FN_FLAG_USES_ARGUMENTS,
    JS_FN_FLAG_STRICT,
    JsClassAstError,
    JsPropertyKind,
    JsExprPurity,
    JsClassMoveBlocker,
    JsClassMember,
    JsClassDesc,
    JsFnParamDesc,
    JsFnSignature,
} from "./classAst.cp";

export {
    JS_RESOLVE_NONE,
    JS_RESOLVE_MAX_DEPTH,
    JsSymbolResolveError,
    JsResolveScopeKind,
    JsResolveSymKind,
    hashJsSymbolName,
    JsResolveSymbol,
    JsResolveScope,
    JsResolveMember,
    JsTsNsExport,
    JsFindSymbolResult,
    JsSymbolResolver,
} from "./symbolResolve.cp";

export {
    JS_SOURCE_INDEX_INVALID,
    JS_SOURCE_INDEX_RUNTIME,
    JS_SOURCE_INDEX_BAKE_SERVER_DATA,
    JS_SOURCE_INDEX_BAKE_CLIENT_DATA,
    JS_AST_REF_MAX_INDEX,
    JsAstRefError,
    JsSourceIndex,
    JsAstRefTag,
    JsAstRef,
    JsSourceTableSpan,
    JsMergedSymbolMap,
} from "./astRef.cp";

export {
    BAST_FLAG_NONE,
    BAST_FLAG_USES_EXPORTS_REF,
    BAST_FLAG_USES_MODULE_REF,
    BAST_FLAG_USES_EXPORT_KEYWORD,
    BAST_FLAG_HAS_CHAR_FREQ,
    BAST_FLAG_FORCE_CJS_TO_ESM,
    BAST_FLAG_HAS_LAZY_EXPORT,
    BAST_FLAG_CJS_EXPORTS_DEOPTIMIZED,
    BAST_FLAG_EXPLICIT_USE_STRICT,
    BAST_FLAG_HAS_IMPORT_META,
    BAST_NO_REDIRECT,
    JsBundledAstError,
    JsExportsKind,
    JsTlaStatus,
    JsBundledTarget,
    JsBundleWrapMode,
    JsBundledAstMeta,
    JsBundledImportEdge,
    JsBundledGraphStats,
    propagateBundledTla,
    summarizeBundledGraph,
} from "./bundledAst.cp";
