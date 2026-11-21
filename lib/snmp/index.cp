// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc RFC 1157/3416 SNMP v1/v2c PDU & VarBind Parser)

export {
    SNMP_PORT,
    SNMP_TRAP_PORT,
    SNMP_V1,
    SNMP_V2C,
    SnmpPduType,
    SnmpValueTag,
    SnmpVarBind,
    SnmpVarBindIter,
    SnmpMessage,
    parseSnmpMessage,
    formatSnmpOid,
} from "./snmp.cp";
