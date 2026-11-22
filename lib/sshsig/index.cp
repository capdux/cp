// Origin: Pure-CP Stdlib (Zero-Alloc OpenSSH PROTOCOL.sshsig v1 Parser) | Created: 2026-11-22
// Unified exports for cap:sshsig.

export {
    SSHSIG_MAGIC,
    SSHSIG_VERSION,
    SSHSIG_ARMOR_BEGIN,
    SSHSIG_ARMOR_END,
    SshsigBlob,
    isSshsigBinary,
    isSshsigArmored,
    decodeSshsigArmor,
    parseSshsigBinary,
    buildSshsigSignedData,
} from "./sshsig.cp";
