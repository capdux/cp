// Source: Absorbed from /data/dev/bun/src/dns.zig (DNS TXT Key-Value, SPF & DMARC Record Parser)

export {
    TxtTag,
    TxtTagIter,
    findTxtTag,
    DmarcPolicy,
    DmarcRecord,
    SpfQualifier,
    SpfTerm,
    SpfIter,
} from "./dns_txt.cp";
