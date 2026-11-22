// Source: Absorbed from /data/dev/bun/src/io/io.cp | Synced: 2026-11-22T11:04:32Z
/**
 * CP Standard I/O (cap:io)
 * 工业级统一高性能 I/O 流体系、非阻塞管道/文件流与标准终端
 */

export {
    IOError,
    SeekOrigin,
    Reader,
    Writer,
    Closer,
    Seeker
} from "./traits";

export {
    IoStatus,
    ReadState,
    WriteStatus,
    PollState,
    FileType,
    PollResult,
    IoSource
} from "./source";

export {
    BufferReader,
    BufferWriter,
    BufferedReader,
    BufferedWriter,
    LimitedReader,
    MultiReader,
    TeeReader,
    DiscardWriter,
    CountingWriter,
    ChunkedBuffer,
    copyStream,
    copyN
} from "./bufferStream";

export {
    Console,
    console,
    Stdio,
    stdio,
    print,
    println,
    eprint,
    eprintln
} from "./stdio";

export {
    MaxBuf,
    MaxBufKind
} from "./maxBuf";

export {
    PipeReadResult,
    PipeWriteResult,
    PipeReader,
    PipeWriter,
    Pipe
} from "./pipe";

export {
    FileReader,
    FileWriter
} from "./fileStream";

export {
    ARRAY_BUFFER_SINK_CAP,
    ARRAY_BUFFER_SINK_DEFAULT_HWM,
    ArrayBufferSinkError,
    ArrayBufferSinkOptions,
    SinkFlushResult,
    ArrayBufferSink,
} from "./arrayBufferSink";

export {
    StaticPipeState,
    StaticPipeWriteStatus,
    StaticPipeWriterError,
    StaticPipeStep,
    StaticPipeWriter,
} from "./staticPipeWriter";

export {
    SUBPROCESS_READABLE_BUF_CAP,
    SUBPROCESS_PIPE_OVERHEAD_BYTES,
    SubprocessReadableKind,
    SubprocessReadableError,
    SubprocessReadable,
} from "./subprocessReadable";

export {
    SUBPROCESS_WRITABLE_BUF_CAP,
    SUBPROCESS_WRITABLE_PIPE_OVERHEAD_BYTES,
    SUBPROCESS_WRITABLE_BUFFER_OVERHEAD_BYTES,
    SubprocessWritableKind,
    SubprocessWritableError,
    SubprocessWritable,
} from "./subprocessWritable";
