// Source: Absorbed from /data/dev/bun/src/cli/cli.cp | Synced: 2026-11-21T16:38:51Z
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
