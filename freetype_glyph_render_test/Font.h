#pragma once

/*
 * by @lazykuna, MIT License
 */

#include "include\ft2build.h"
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include <vector>
#include <map>

typedef int FontColor;
typedef int FontStyle;
typedef uint32_t GlyphIndex;

// @description bitmap information for rendering
// (only contains alpha channel)
struct FontBitmap {
	unsigned char *p;
	int width;
	int height;
};

// @description used for font foreground texture
// (ARGB data)
struct FontTexture {
	unsigned int *p;
	int width;
	int height;
	int sx;
	int sy;
};

// @description used for shadow position setting
struct FontVector {
	int x, y, z;
};

// @description configurable font options
struct FontOption {
	int size;
	FontColor color;
	FontTexture foreground_texture;
	FontStyle style;
	int antialiased;

	int outline_size;
	FontColor outline_color;

	FontVector shadow;
	FontColor shadow_color;
};

class FontRenderer {
	FontOption option;
	// @description used for real heighting. (global height)
	int text_height;	// Bbox height
	int text_ascender;	// baseline position

	FT_Face ftFace;
	FontRenderer* fallback;

public:
	// @description very basic initalizer, useful for logging text
	bool LoadFont(const char *ttffp, int size, FontColor color=0x000000);
	// @description more complicated initalizer, texture/outline/shadow support
	bool LoadFont(const char *ttffp, const struct FontOption* option);
	// @description more complicated initalizer, texture/outline/shadow support
	bool LoadFont(const FT_Byte *buf, size_t size, const struct FontOption* option);
	// @description automatically called when initalizefont / destruction
	void ClearFont();

	// @description generate string with font data, internally calls CacheGlyphs()
	// MUST release object after using.
	void RenderBitmap(const char *chrs, FontTexture* bitmap);
	void RenderBitmap(const uint32_t *chrs, FontTexture* bitmap);
	// @description render each chars to bitmap (but is it necessary?)
	void RenderBitmap(const char *chrs, std::vector<FontTexture*> bitmaps);
	bool RenderGlyph(FT_ULong charcode, FontBitmap* glyph, FontRenderer* fallback = 0);

	FontRenderer();
	~FontRenderer();

	FT_UInt GetGlyphIndex(char **chrs_utf8);
	FT_UInt GetGlyphIndex(FT_ULong charcode);

private:
	bool SetFontSize(int size);
	FT_GlyphSlot Render(const uint32_t charcode);
	FontBitmap* GetGlyphFromCache(FT_ULong charcode);	// only get glyphs from cached one
};

class FontBaseCached : public FontRenderer {
private:
	// @description this does not cache really; these variable/funcs just presets cache position.
	int cache_x, cache_y, cache_pageidx;	// caching x/y/page index
	int cache_width, cache_height;			// width/height of caching page
	void GetNewGlyphCachePosition(void* glyph, void* pos);

	// @description these options are related about rendering texts
	int text_align;

	// @description these option is setted for fast rendering.
	std::vector<void*> m_char_rendering_info;
	virtual void BuildText(const char* chrs, int x, int y) = 0;
public:
	// these functions should be implemented
	virtual bool UploadGlyph(uint32_t charcode) = 0;
	virtual void CacheGlyphs(const char *chrs) = 0;
	virtual void CacheGlyphs(const uint32_t *chrs) = 0;
	virtual void CacheGlyph(uint32_t charcode) = 0;
	virtual void ClearCache() = 0;
	virtual void RenderChar(const uint32_t chr, int x, int y) = 0;
	virtual void RenderText(const char* chrs, int x, int y) = 0;

	// @description these option is setted for fast rendering.
	virtual void SetText(const char* chrs, int x, int y) = 0;
	// @description you should call SetText first before do RenderTextFast();
	virtual void RenderTextFast() = 0;
};

/*
 * Memory Bitmap Caching
 */
class FontGDICached: public FontBaseCached {
	
};


/*
 * OpenGL Bitmap Caching TODO
 */
class FontOpenGLCached : public FontBaseCached {

};

namespace FontUtil {
	FT_Library GetFontLib();
	wchar_t utf8_to_utf16(char **chr_utf8);
	bool utf8_to_utf16(const char *s, size_t iLength, unsigned &start, wchar_t &ch);
	uint32_t utf8_to_utf32(char **chr_utf8);
	bool utf8_to_utf32(const char *s, size_t iLength, unsigned &start, uint32_t &ch);
	bool utf8_to_utf32_string(const char *chr_utf8, std::basic_string<uint32_t>& chr_utf32);
}


// select font that you want
#define Font FontGDICached

/*
 * I won't provide rendering option here,
 * as we can do many things with rendered images/textures,
 * without modifying bitmap in here.
 */