// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc IEEE 802.1AB LLDP TLV & Neighbor Codec)
// Unified exports for cap:lldp.

export {
    LLDP_ETHERTYPE,
    TLV_END,
    TLV_CHASSIS_ID,
    TLV_PORT_ID,
    TLV_TTL,
    TLV_PORT_DESC,
    TLV_SYSTEM_NAME,
    TLV_SYSTEM_DESC,
    TLV_SYSTEM_CAPS,
    TLV_MGMT_ADDR,
    TLV_ORG_SPECIFIC,
    CAP_REPEATER,
    CAP_BRIDGE,
    CAP_WLAN_AP,
    CAP_ROUTER,
    CAP_TELEPHONE,
    CAP_STATION_ONLY,
    LldpTlv,
    LldpOrgTlv,
    LldpNeighbor,
    LldpTlvIter,
    LldpWriter,
    parseLldpOrgTlv,
    parseLldpdu,
} from "./lldp.cp";
