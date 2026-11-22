// Source: Absorbed from /data/dev/bun/src/uws_sys/quic/Header.cp | Synced: 2026-11-22T10:39:50Z

export {
    QUIC_VERSION_1,
    QUIC_VERSION_2,
    QUIC_VERSION_NEGOTIATION,
    QUIC_MAX_CID_LEN,
    QUIC_VARINT_MAX,
    QuicHeaderError,
    QuicPacketKind,
    DecodedVarInt,
    varIntEncodedLen,
    encodeVarInt,
    decodeVarInt,
    QuicHeader,
    decodeQuicHeader,
    encodeQuicHeader,
    QpackHeaderClass,
    QpackIndex,
    QpackLookup,
    classifyQpackHeader,
    SocketKind,
    socketKindFromByte,
} from "./quic.cp";
