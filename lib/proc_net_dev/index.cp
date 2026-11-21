// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /proc/net/dev Network Interface Stats Parser)

export {
    NetDevStats,
    ProcNetDevIter,
    findNetDev,
    sumNonLoopbackBytes,
} from "./net_dev.cp";
