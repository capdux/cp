// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /proc/net/route Routing Table Parser)

export {
    RTF_UP,
    RTF_GATEWAY,
    RTF_HOST,
    parseHexIpv4Le,
    RouteEntry,
    ProcRouteIter,
    findDefaultGateway,
} from "./route.cp";
