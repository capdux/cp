// Source: cp/lib/cose/index.cp (cap:cose & cap.cose RFC 9052 / RFC 9053 standard library entry)
export {
    COSE_TAG_MAC0,
    COSE_TAG_SIGN1,
    CoseError,
    CoseAlgorithm,
    CoseKeyType,
    CoseCurve,
    CoseHeader,
} from "./types.cp";

export { CborWriter, CborReader } from "./cbor.cp";

export { CoseKey, encodeCoseKey, decodeCoseKey } from "./key.cp";

export {
    CoseSign1,
    encodeHeaderMap,
    encodeProtectedHeader,
    decodeHeaderMap,
    decodeProtectedHeader,
    encodeSigStructure,
    encodeCoseSign1,
    decodeCoseSign1,
    sign1Ed25519,
    verify1Ed25519,
    sign1Es256,
    verify1Es256,
} from "./sign1.cp";

export {
    CoseMac0,
    encodeMacStructure,
    encodeCoseMac0,
    decodeCoseMac0,
    mac0Hs256,
    verifyMac0Hs256,
} from "./mac0.cp";
