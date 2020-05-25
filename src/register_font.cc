#include "register_font.h"

#include <pango/pangocairo.h>
#include <pango/pango-fontmap.h>
#include <pango/pango.h>
#include <iconv.h>

#ifdef __APPLE__
#include <CoreText/CoreText.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#include <fontconfig/fontconfig.h>
#endif

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H
#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_IDS_H
#ifndef FT_SFNT_OS2
#define FT_SFNT_OS2 ft_sfnt_os2
#endif

// OSX seems to read the strings in MacRoman encoding and ignore Unicode entries.
// You can verify this by opening a TTF with both Unicode and Macroman on OSX.
// It uses the MacRoman name, while Fontconfig and Windows use Unicode
#ifdef __APPLE__
#define PREFERRED_PLATFORM_ID TT_PLATFORM_MACINTOSH
#define PREFERRED_ENCODING_ID TT_MAC_ID_ROMAN
#else
#define PREFERRED_PLATFORM_ID TT_PLATFORM_MICROSOFT
#define PREFERRED_ENCODING_ID TT_MS_ID_UNICODE_CS
#endif

#define IS_PREFERRED_ENC(X) \
  X.platform_id == PREFERRED_PLATFORM_ID && X.encoding_id == PREFERRED_ENCODING_ID

#if defined(__APPLE) || defined(_WIN32)
#define GET_NAME_RANK(X) \
  ((IS_PREFERRED_ENC(X) ? 1 : 0) << 1) | \
  (X.name_id == TT_NAME_ID_PREFERRED_FAMILY ? 1 : 0)
#else
#define GET_NAME_RANK(X) \
  ((IS_PREFERRED_ENC(X) ? 1 : 0) << 2) | \
  ((X.name_id == TT_NAME_ID_PS_NAME ? 1 : 0) << 1) | \
  (X.name_id == TT_NAME_ID_PREFERRED_FAMILY ? 1 : 0)
#endif

/*
 * Return a UTF-8 encoded string given a TrueType name buf+len
 * and its platform and encoding
 */

char *
to_utf8(FT_Byte* buf, FT_UInt len, FT_UShort pid, FT_UShort eid) {
  size_t ret_len = len * 4; // max chars in a utf8 string
  char *ret = (char*)malloc(ret_len + 1); // utf8 string + null

  if (!ret) return NULL;

  // In my testing of hundreds of fonts from the Google Font repo, the two types
  // of fonts are TT_PLATFORM_MICROSOFT with TT_MS_ID_UNICODE_CS encoding, or
  // TT_PLATFORM_MACINTOSH with TT_MAC_ID_ROMAN encoding. Usually both, never neither

  char const *fromcode;

  if (pid == TT_PLATFORM_MACINTOSH && eid == TT_MAC_ID_ROMAN) {
    fromcode = "Mac";
  } else if (pid == TT_PLATFORM_MICROSOFT && eid == TT_MS_ID_UNICODE_CS) {
    fromcode = "UTF-16BE";
  } else {
    free(ret);
    return NULL;
  }

  iconv_t cd = iconv_open("UTF-8", fromcode);

  if (cd == (iconv_t)-1) {
    free(ret);
    return NULL;
  }

  size_t inbytesleft = len;
  size_t outbytesleft = ret_len;

  size_t n_converted = iconv(cd, (char**)&buf, &inbytesleft, &ret, &outbytesleft);

  ret -= ret_len - outbytesleft; // rewind the pointers to their
  buf -= len - inbytesleft;      // original starting positions

  if (n_converted == (size_t)-1) {
    free(ret);
    return NULL;
  } else {
    ret[ret_len - outbytesleft] = '\0';
    return ret;
  }
}

/*
 * Find a family name in the face's name table, preferring the one the
 * system, fall back to the other
 */

char *
get_family_name(FT_Face face) {
  FT_SfntName name;

  int best_rank = -1;
  char* best_buf = NULL;

  for (unsigned i = 0; i < FT_Get_Sfnt_Name_Count(face); ++i) {
    FT_Get_Sfnt_Name(face, i, &name);

    if (
      name.name_id == TT_NAME_ID_FONT_FAMILY ||
#if !defined(__APPLE) && !defined(_WIN32)
      name.name_id == TT_NAME_ID_PREFERRED_FAMILY ||
#endif
      name.name_id == TT_NAME_ID_PS_NAME
    ) {
      char *buf = to_utf8(name.string, name.string_len, name.platform_id, name.encoding_id);

      if (buf) {
        int rank = GET_NAME_RANK(name);
        if (rank > best_rank) {
          best_rank = rank;
          if (best_buf) free(best_buf);
          best_buf = buf;

#if !defined(__APPLE) && !defined(_WIN32)
          if (name.name_id === TT_NAME_ID_PREFERRED_FAMILY) {
            size_t len = strlen(buf);
            best_buf = malloc(len + 2);
            best_buf[0] = '@';
            strncpy(best_buf + 1, buf, len);
            best_buf[len + 1] = '\0';
          }
#endif
        } else {
          free(buf);
        }
      }
    }
  }

  return best_buf;
}

PangoWeight
get_pango_weight(FT_UShort weight) {
  switch (weight) {
    case 100: return PANGO_WEIGHT_THIN;
    case 200: return PANGO_WEIGHT_ULTRALIGHT;
    case 300: return PANGO_WEIGHT_LIGHT;
    #if PANGO_VERSION >= PANGO_VERSION_ENCODE(1, 36, 7)
    case 350: return PANGO_WEIGHT_SEMILIGHT;
    #endif
    case 380: return PANGO_WEIGHT_BOOK;
    case 400: return PANGO_WEIGHT_NORMAL;
    case 500: return PANGO_WEIGHT_MEDIUM;
    case 600: return PANGO_WEIGHT_SEMIBOLD;
    case 700: return PANGO_WEIGHT_BOLD;
    case 800: return PANGO_WEIGHT_ULTRABOLD;
    case 900: return PANGO_WEIGHT_HEAVY;
    case 1000: return PANGO_WEIGHT_ULTRAHEAVY;
    default: return PANGO_WEIGHT_NORMAL;
  }
}

PangoStretch
get_pango_stretch(FT_UShort width) {
  switch (width) {
    case 1: return PANGO_STRETCH_ULTRA_CONDENSED;
    case 2: return PANGO_STRETCH_EXTRA_CONDENSED;
    case 3: return PANGO_STRETCH_CONDENSED;
    case 4: return PANGO_STRETCH_SEMI_CONDENSED;
    case 5: return PANGO_STRETCH_NORMAL;
    case 6: return PANGO_STRETCH_SEMI_EXPANDED;
    case 7: return PANGO_STRETCH_EXPANDED;
    case 8: return PANGO_STRETCH_EXTRA_EXPANDED;
    case 9: return PANGO_STRETCH_ULTRA_EXPANDED;
    default: return PANGO_STRETCH_NORMAL;
  }
}

PangoStyle
get_pango_style(FT_Long flags) {
  if (flags & FT_STYLE_FLAG_ITALIC) {
    return PANGO_STYLE_ITALIC;
  } else {
    return PANGO_STYLE_NORMAL;
  }
}

/*
 * Return a PangoFontDescription that will resolve to the font file
 */

PangoFontDescription *
get_pango_font_description(unsigned char* filepath) {
  FT_Library library;
  FT_Face face;
  PangoFontDescription *desc = pango_font_description_new();

  if (!FT_Init_FreeType(&library) && !FT_New_Face(library, (const char*)filepath, 0, &face)) {
    TT_OS2 *table = (TT_OS2*)FT_Get_Sfnt_Table(face, FT_SFNT_OS2);
    if (table) {
      char *family = get_family_name(face);

      if (!family) {
        pango_font_description_free(desc);
        FT_Done_Face(face);
        FT_Done_FreeType(library);

        return NULL;
      }

      pango_font_description_set_family_static(desc, family);
      pango_font_description_set_weight(desc, get_pango_weight(table->usWeightClass));
      pango_font_description_set_stretch(desc, get_pango_stretch(table->usWidthClass));
      pango_font_description_set_style(desc, get_pango_style(face->style_flags));

      FT_Done_Face(face);
      FT_Done_FreeType(library);

      return desc;
    }
  }

  pango_font_description_free(desc);

  return NULL;
}

#if !defined(__APPLE) && !defined(_WIN32)
void (FcPattern *pat, gpointer data) {
  FcChar8 *in_family;
  char *out_family = NULL;

  for (int i = 0; FcPatternGetString(pat, FC_FAMILY, i, &in_family) == FcResultMatch; i++) {
	  if (family[0] === '@') {
      FcPatternPrint(pat);
      out_family = strdup((char*)in_family + 1);
      break;
    }
  }

  if (out_family) {
    FcPatternDelete(pat, FC_FAMILY);
    FcPatternAddString(pat, FC_POSTSCRIPT_NAME, out_family);
  }
}
#endif

/*
 * Register font with the OS
 */

bool
register_font(unsigned char *filepath) {
  bool success;
  
  #ifdef __APPLE__
  CFURLRef filepathUrl = CFURLCreateFromFileSystemRepresentation(NULL, filepath, strlen((char*)filepath), false);
  success = CTFontManagerRegisterFontsForURL(filepathUrl, kCTFontManagerScopeProcess, NULL);
  #elif defined(_WIN32)
  success = AddFontResourceEx((LPCSTR)filepath, FR_PRIVATE, 0) != 0;
  #else
  success = FcConfigAppFontAddFile(FcConfigGetCurrent(), (FcChar8 *)(filepath));
  #endif

  if (!success) return false;

  // Tell Pango to throw away the current FontMap and create a new one. This
  // has the effect of registering the new font in Pango by re-looking up all
  // font families.
  pango_cairo_font_map_set_default(NULL);

#if !defined(__APPLE) && !defined(_WIN32)
  PangoFontMap* map = pango_font_map_new();
  PangoCairoFontMap* c_map = PANGO_CAIRO_FONT_MAP(map);
  PangoFt2FontMap* ft2_map = PANGO_FT2_FONT_MAP(map);
  pango_cairo_font_map_set_default(c_map);
  pango_ft2_font_map_set_default_substitute(ft2_map, , NULL, NULL);
#endif

  return true;
}

