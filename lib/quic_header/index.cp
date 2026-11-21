// Source: Absorbed from /data/dev/bun/src/uws_sys/quic/Header.cp, /data/dev/bun/src/uws_sys/SocketKind.cp

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
