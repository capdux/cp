// Source: cp/lib/dns_wire/index.cp (Pure CP RFC 1035/6891 DNS Wire Standard Library)
export {
    DNS_HEADER_LEN,
    DNS_MAX_LABEL_LEN,
    DNS_MAX_NAME_LEN,
    DNS_MAX_POINTER_JUMPS,
    DnsWireError,
    DnsOpcode,
    DnsRcode,
    DnsRecordType,
    DnsClass,
    DnsHeader,
    DnsQuestion,
    MxData,
    SrvData,
    SoaData,
    DnsRdata,
    DnsResourceRecord,
    DnsPacket,
} from "./types.cp";

export {
    DecodedName,
    decodeDnsName,
    encodeDnsName,
    encodeDnsPacket,
    decodeDnsPacket,
} from "./codec.cp";
