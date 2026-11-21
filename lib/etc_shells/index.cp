// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /etc/shells Parser)
export {
    parseShellLine,
    EtcShellsIter,
    isAllowedShell,
    findShellByBasename,
    countShells,
    isNologinShell,
} from "./shells.cp";
