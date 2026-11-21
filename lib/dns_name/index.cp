// Source: Absorbed from /data/dev/bun/src/dns.zig (RFC 1035 DNS Name Validator & Wire Label Codec)

export {
    isValidDnsLabel,
    isValidHostname,
    encodeWireName,
    decodeWireName,
    isSubdomainOf,
    normalizeHostname,
} from "./dns_name.cp";
