#include <stdint.h>
#include <stddef.h>
#include <ft2build.h>
#include FT_FREETYPE_H

void *cp_ft_init(void) {
    FT_Library lib = NULL;
    if (FT_Init_FreeType(&lib) != 0) return NULL;
    return (void *)lib;
}

void cp_ft_done(void *lib) {
    if (lib) FT_Done_FreeType((FT_Library)lib);
}

void cp_ft_version(void *lib, int32_t *maj, int32_t *min, int32_t *pat) {
    FT_Int amaj = 0, amin = 0, apat = 0;
    if (lib) FT_Library_Version((FT_Library)lib, &amaj, &amin, &apat);
    if (maj) *maj = (int32_t)amaj;
    if (min) *min = (int32_t)amin;
    if (pat) *pat = (int32_t)apat;
}

void *cp_ft_new_memory_face(void *lib, const uint8_t *data, int64_t size, int64_t face_idx) {
    if (!lib || !data || size <= 0) return NULL;
    FT_Face face = NULL;
    if (FT_New_Memory_Face((FT_Library)lib, (const FT_Byte *)data, (FT_Long)size, (FT_Long)face_idx, &face) != 0) {
        return NULL;
    }
    return (void *)face;
}

void cp_ft_done_face(void *face) {
    if (face) FT_Done_Face((FT_Face)face);
}

int64_t cp_ft_face_num_glyphs(void *face) {
    if (!face) return 0;
    return (int64_t)((FT_Face)face)->num_glyphs;
}
