// Source: cp/lib/wireguard/index.cp (cap:wireguard public module exports)

export {
    WgError,
    Tai64n,
    ReplayWindow,
} from "./replay.cp";

export {
    MSG_INITIATION_TYPE,
    MSG_RESPONSE_TYPE,
    MSG_TRANSPORT_TYPE,
    INITIATION_LEN,
    RESPONSE_LEN,
    TRANSPORT_HEADER_LEN,
    AEAD_TAG_LEN,
    WgKeyPair,
    WgSessionKeys,
    HandshakeInitiation,
    HandshakeResponse,
    TransportPacket,
    NoiseInitiatorState,
    InitiationResult,
    ResponderResult,
    wgKdf1,
    wgKdf2,
    wgKdf3,
    blake2sMac16,
    createInitiation,
    consumeInitiationAndCreateResponse,
    consumeResponse,
    encryptTransport,
    decryptTransport,
} from "./noise.cp";
