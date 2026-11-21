// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc OpenSSH PROTOCOL.sshsig v1 Parser)
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
