// Source: Absorbed from /data/dev/bun/src/dns.zig (RFC 2782 DNS SRV Record Parser & Weighted Selector)

export {
    SrvWireHeader,
    SrvRecord,
    SrvServiceName,
    compareSrvOrder,
    selectSrvByRoll,
} from "./srv.cp";
