/**
 * CP Standard I/O Traits (cap:io)
 * 统一流式 Trait 契约与错误集
 */

export error IOError {
    EndOfStream,
    UnexpectedEof,
    Closed,
    BufferFull,
    ShortWrite,
    InvalidSeek,
}

pub enum SeekOrigin {
    Start,
    Current,
    End,
}

/**
 * 统一读取器契约
 * 传入目标缓冲区切片，返回实际读取字节数，0 表示 EOF
 */
pub trait Reader {
    fn read(*_, buf: *[]u8): usize!;
}

/**
 * 统一写入器契约
 * 传入只读数据切片，返回实际写入字节数
 */
pub trait Writer {
    fn write(*_, buf: []const u8): usize!;
}

/**
 * 统一资源关闭契约
 */
pub trait Closer {
    fn close(*_);
}

/**
 * 统一流寻址契约
 */
pub trait Seeker {
    fn seek(*_, offset: i64, origin: SeekOrigin): u64!;
}
