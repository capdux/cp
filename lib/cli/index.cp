// Source: Absorbed from /data/dev/bun/src/cli/cli.cp | Synced: 2026-11-22T04:32:45Z
// lib/cli/index.cp
// Unified Public Exports for cap:cli

export * from "./args.cp";
export * from "./tty.cp";
export * from "./table.cp";
export * from "./ci_info.cp";
export * from "./subcommand.cp";

export {
    ColorLevel,
    ColorEnv,
    parseForceColor,
    detectColorLevel,
    supportsColor,
    supportsTruecolor,
} from "./color.cp";

export {
    ProgressSpinnerStyle,
    spinnerFrame,
    ProgressMeter,
} from "./progress.cp";

export {
    SpinnerPreset,
    SpinnerSpec,
    formatSpinnerFrame,
    formatSpinnerDone,
} from "./spinner.cp";

export {
    TableBorderStyle,
    CellAlign,
    computeColumnWidths,
    renderBorderLine,
    renderTableRow,
    renderTable,
} from "./table_render.cp";

export {
    TreeStyle,
    TreePrefix,
    TreeNode,
    renderFlatTree,
} from "./tree.cp";

export {
    ColonListError,
    ColonPair,
    parseColonPair,
    parseColonPairNonEmptyKey,
    parseLoaderMapping,
    parseColonList,
    parseLoaderMappingList,
    findColonValue,
} from "./colon_list.cp";

export {
    CompletionError,
    CompletionShell,
    CompletionItem,
    formatCompletions,
    formatCompletionItems,
} from "./completions.cp";

export {
    PmCommandsError,
    YarnCommandEra,
    PmStage,
    yarnCommandEra,
    isYarnCommand,
    mapYarnToCapSubcommand,
    pmStagePlainName,
    pmStageLabel,
    formatPmStageBanner,
} from "./pm_commands.cp";

export {
    PM_VIEW_MAX_PATH_SEGMENTS,
    PM_VIEW_RECENT_VERSIONS_LIMIT,
    PmViewError,
    PmViewSpec,
    PmPropertyPathSegment,
    PmPropertyPath,
    PmViewDistInfo,
    PmViewKv,
    PmViewMaintainer,
    PmViewSummary,
    formatRecentVersionsHint,
    formatPmViewJsonError,
} from "./pm_view.cp";

export {
    EDITOR_MAX_ARGS,
    EDITOR_DEFAULT_PREFERENCE_COUNT,
    EditorOpenError,
    OpenerPlatform,
    platformOpener,
    UrlOpenCommand,
    buildOpenUrlCommand,
    EditorKind,
    defaultEditorPreferenceAt,
    EditorResolution,
    resolveEditorFromPreference,
    EditorOpenCommand,
    buildOpenEditorCommand,
    normalizeEditorTempFilename,
} from "./open.cp";

export {
    COMPLETION_MAX_CANDIDATES,
    COMPLETION_PATH_BUF_CAP,
    CompletionInstallError,
    CompletionInstallEnv,
    CompletionCandidateList,
    completionFilename,
    collectCompletionDirs,
    resolveZshrcPath,
    zshrcNeedsCompletionHint,
    formatZshrcSourceLine,
    collectAliasSymlinkTargets,
    isWindowsInstallerBinDir,
} from "./install_completions.cp";

export {
    PmVersionError,
    PmVersionType,
    PmSemver,
    PmParsedVersionArg,
    PmVersionOptions,
    parsePmVersionArg,
    normalizeGitTagVersion,
    computeNextPmVersion,
    formatPmCommitMessage,
    formatPmGitTag,
} from "./pm_version.cp";

export {
    PM_PKG_MAX_SEGMENTS,
    PmPkgError,
    PmPkgSubCommand,
    PmPkgSegmentKind,
    PmPkgPathSegment,
    PmPkgKeyPath,
    PmPkgAssignment,
    PmPkgValueKind,
    classifyPmPkgValue,
} from "./pm_pkg.cp";

export {
    FILTER_MAX_PATTERNS,
    FILTER_PATH_BUF_CAP,
    FilterArgError,
    FilterPatternKind,
    FilterRule,
    FilterSet,
    isFilterSkippedDir,
    formatWorkspacePackagePattern,
    normalizeFilterPath,
    matchFilterGlob,
    formatFilterScriptPrefix,
} from "./filter_arg.cp";

export {
    TEST_SCANNER_MAX_FILTERS,
    TEST_SCANNER_MAX_EXCLUSIONS,
    TEST_SCANNER_MAX_IGNORE_PATTERNS,
    TestScannerError,
    isTestSupportedExtension,
    hasTestNameSuffix,
    couldBeTestFile,
    isExactTestPathArg,
    formatScopedTestName,
    TestFileScanner,
} from "./test_scanner.cp";

export {
    PM_AUDIT_MAX_IGNORE,
    PM_AUDIT_MAX_PACKAGES,
    PM_AUDIT_MAX_VERSIONS_PER_PKG,
    PM_AUDIT_MAX_ADVISORIES,
    PM_AUDIT_MAX_DEP_EDGES,
    PM_AUDIT_MAX_PATH_DEPTH,
    PmAuditError,
    PmAuditSeverity,
    PmAuditFilter,
    PmAuditPackageEntry,
    PmAuditRequestBuilder,
    PmAuditAdvisory,
    PmAuditCounts,
    PmAuditReport,
    PmAuditDepEdge,
    PmAuditDepGraph,
} from "./pm_audit.cp";

export {
    PM_OUTDATED_MAX_ENTRIES,
    PM_OUTDATED_MAX_PATTERNS,
    PM_OUTDATED_MAX_GROUPED_WS,
    PmOutdatedError,
    PmOutdatedDepKind,
    PmOutdatedUpdateKind,
    PmOutdatedFilterKind,
    PmOutdatedSemver,
    matchOutdatedGlob,
    PmOutdatedFilterRule,
    PmOutdatedFilter,
    PmOutdatedEntry,
    PmOutdatedTable,
} from "./pm_outdated.cp";
