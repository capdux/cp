// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Apache Lucene CodecUtil Parser)

export {
    LUCENE_CODEC_MAGIC,
    LUCENE_FOOTER_MAGIC,
    LUCENE_FOOTER_LENGTH,
    LUCENE_ID_LENGTH,
    LuceneVInt,
    LuceneVLong,
    LuceneString,
    LuceneCodecHeader,
    LuceneIndexHeader,
    LuceneFooter,
    readLuceneVInt,
    readLuceneVLong,
    luceneZigZagDecode32,
    luceneZigZagDecode64,
    readLuceneString,
    parseLuceneHeader,
    parseLuceneIndexHeader,
    parseLuceneFooter,
    isLuceneFile,
} from "./lucene.cp";
