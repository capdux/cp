// Source: cp/lib/mvcc/types.cp (MVCC core types, errors, version chain nodes, and write-set entries)

pub const MvccError = error{
    WriteConflict,
    TxnNotActive,
    TxnAborted,
    ReadOnlyTxn,
    KeyNotFound,
    OutOfMemory,
};

pub const TxnState = enum {
    Active,
    Committed,
    Aborted,
};

pub struct VersionValue {
    pub commit_ts: u64,
    pub txn_id: u64,
    pub value: ([]const u8)?,
}

pub struct WriteEntry {
    pub key: []const u8,
    pub value: ([]const u8)?,
}

pub struct ActiveTxnInfo {
    pub txn_id: u64,
    pub read_ts: u64,
}
