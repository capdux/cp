// Source: Absorbed from /data/dev/bun/src/dns/ | Synced: 2026-11-22T12:49:17Z
// lib/dns/index.cp
// Unified Cap DNS Resolution, Wire Codec, HostsTable & c-ares Subsystem (cap:dns)

export {
    ares_channeldata,
    ares_addrinfo,
    ares_options,
    ARES_SUCCESS,
    ARES_ENODATA,
    ARES_EFORMERR,
    ARES_ESERVFAIL,
    ARES_ENOTFOUND,
    ARES_ENOTIMP,
    ARES_EREFUSED,
    ARES_EBADQUERY,
    ARES_EBADNAME,
    ARES_EBADFAMILY,
    ARES_EBADRESP,
    ARES_ECONNREFUSED,
    ARES_ETIMEOUT,
    ARES_EOF,
    ARES_EFILE,
    ARES_ENOMEM,
    ARES_EDESTRUCTION,
    ARES_EBADSTR,
    ARES_EBADFLAGS,
    ARES_ENONAME,
    ARES_EBADHINTS,
    ARES_ENOTINITIALIZED,
    ARES_ECANCELLED,
    ARES_ESERVICE,
    ARES_ENOSERVER,
    NSClass,
    NSType,
    CaresStatus,
    ares_library_init,
    ares_library_cleanup,
    ares_init,
    ares_destroy,
    ares_cancel,
    ares_strerror,
    ares_free_data,
    ares_freeaddrinfo,
} from "./_cares.cp";

export {
    DnsRCode,
    DnsHeader,
    DecodedDomain,
    DnsQuestion,
    DnsResourceRecord,
    DnsMxRecord,
    DnsSrvRecord,
    DnsWireCodec,
} from "./packet.cp";

export {
    DnsFamily,
    DnsBackend,
    ResolveOptions,
    DnsCacheEntry,
    DnsCache,
    HostsEntry,
    HostsTable,
} from "./cache.cp";

export {
    DnsRecordError,
    DnsNameCodec,
    SrvRecord,
    SrvSelector,
    SoaRecord,
    CaaRecord,
} from "./records.cp";

export {
    DohError,
    DohHeader,
    DohPostHeaders,
    DohQueryBuilder,
    DohJsonQuestion,
    DohJsonAnswer,
    DohJsonResponse,
} from "./doh.cp";

export {
    MAX_NAMESERVERS,
    MAX_SEARCH_DOMAINS,
    DEFAULT_NDOTS,
    DEFAULT_TIMEOUT,
    DEFAULT_ATTEMPTS,
    ResolvConf,
} from "./resolvConf.cp";

export {
    CaaTag,
    CaaRecord as CaaPolicyRecord,
    isCaAuthorized,
} from "./caa.cp";

export {
    isValidDnsLabel,
    isValidHostname,
    encodeWireName,
    decodeWireName,
    isSubdomainOf,
    normalizeHostname,
} from "./name.cp";

export {
    DnsRrType,
    DnsRcode,
    DnsClass,
    SrvRecord as RrSrvRecord,
    MxRecord,
} from "./rtype.cp";

export {
    SrvWireHeader,
    SrvRecord as SrvEntry,
    SrvServiceName,
    compareSrvOrder,
    selectSrvByRoll,
} from "./srv.cp";

export {
    TxtTag,
    TxtTagIter,
    findTxtTag,
    DmarcPolicy,
    DmarcRecord,
    SpfQualifier,
    SpfTerm,
    SpfIter,
} from "./txt.cp";

export {
    DNS_HEADER_LEN,
    DNS_MAX_LABEL_LEN,
    DNS_MAX_NAME_LEN,
    DNS_MAX_POINTER_JUMPS,
    DnsWireError,
    DnsOpcode,
    DnsRcode as WireDnsRcode,
    DnsRecordType,
    DnsClass as WireDnsClass,
    DnsHeader as WireDnsHeader,
    DnsQuestion as WireDnsQuestion,
    MxData,
    SrvData,
    SoaData,
    CaaData,
    Edns0Opt,
    DnsRdata,
    DnsResourceRecord as WireResourceRecord,
    DnsPacket,
    DecodedName,
    dnsNameEqlIgnoreCaseScalar,
    dnsNameEqlIgnoreCaseSimd,
    copyDnsBytesScalar,
    copyDnsBytesSimd,
    decodeDnsName,
    encodeDnsName,
    encodeDnsPacket,
    decodeDnsPacket,
    parseEdns0Opt,
    validateDnsName,
    validateDnsPacket,
} from "./wire.cp";

export {
    MAX_DNS_HOSTNAME_LEN,
    AI_FLAG_PASSIVE,
    AI_FLAG_CANONNAME,
    AI_FLAG_NUMERICHOST,
    AI_FLAG_V4MAPPED,
    AI_FLAG_ALL,
    AI_FLAG_ADDRCONFIG,
    AI_FLAG_NUMERICSERV,
    AI_FLAGS_KNOWN_MASK,
    GetAddrInfoError,
    AddrInfoFamily,
    AddrInfoSocketType,
    AddrInfoProtocol,
    AddrInfoBackend,
    AddrInfoFlags,
    GetAddrInfoOptions,
    GetAddrInfoQuery,
} from "./getaddrinfo.cp";

export {
    DNS_PREFETCH_DEFAULT_TTL_MS,
    DNS_PREFETCH_MAX_TTL_MS,
    DNS_PREFETCH_MAX_ENTRIES,
    DnsPrefetchError,
    parseDnsTtlEnvSeconds,
    DnsCacheStats,
    DnsLookupState,
    DnsLookupOutcomeKind,
    DnsLookupOutcome,
    DnsPrefetchEntry,
    DnsPrefetchCache,
} from "./prefetch.cp";
