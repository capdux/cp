// Source: cp/lib/bpe/types.cp (Pure CP BPE Tokenizer Core Types & Errors)

pub enum BpeError {
    InvalidFormat,
    InvalidBase64,
    UnknownTokenId,
    DuplicateToken,
    InvalidTargetVocabSize,
    OutOfMemory,
}

pub struct SpecialToken {
    pub text: []const u8,
    pub id: u32,
}

pub struct BpeMerge {
    pub left_id: u32,
    pub right_id: u32,
    pub merged_id: u32,
    pub rank: u32,
}

pub struct TokenBytes {
    pub data: []const u8,
}
