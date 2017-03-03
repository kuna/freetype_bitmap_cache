#pragma once

/*
 * by @lazykuna, MIT License
 */

#include "ft2build.h"
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
	unsigned int *p;	// direct pointer to bitmap/texture
	int width;
	int height;
};

// @description used for shadow position setting
struct FontVector {
	int x, y, z;
};

// @description used for SetText / or something else ...
struct FontRect {
	int x, y, w, h;
};

struct FontGlyphMetrics {
	void* p;	// directly points to bitmap/texture
	int sx, sy;
	int width, height;
};

struct FontRenderMetrics {
	void *p;
	float sx, sy, sw, sh;
	float dx, dy, dw, dh;
};



/*
 * @description
 * as caching style quite different by using graphic engine,
 * this class actually do not only caching but caching.
 * be aware of that.
 *
 * CLAIM: only allows SAME-HEIGHT bitmap character for caching!
 */
class FontBaseCache {
private:
	// @description this does not cache really; these variable/funcs just presets cache position.
	unsigned int cache_x, cache_y, cache_width, cache_height;
	std::vector<void*> m_cache_page;
	std::vector<FontGlyphMetrics> m_cache_glyphs;
	// @description reset glyph's sx/sy of cache position. requires width/height.
	// if new page required, then automatically generate new page.
	void GetNewGlyphCachePosition(FontGlyphMetrics& glyph);
	virtual void GenerateNewPage() = 0;

	// @description these option is setted for fast rendering.
	std::vector<FontRenderMetrics> m_char_rendering_info;
	// @description render single char using pre-calculated metrics.
	virtual void RenderSingleMetrics(FontRenderMetrics& metrics) = 0;


	// @description these options are related about rendering texts
	int text_align;

public:
	// @description load cache from file.
	// if multiple cache, then call this method for many times you want.
	virtual void AddCache(FontBitmap* bitmap, std::vector<FontGlyphMetrics>& glyphmetrics) = 0;
	// @description upload glyphs, ONLY ALLOWS 8bit BITMAP DATA!
	virtual bool UploadGlyph(std::vector<FontGlyphMetrics>& glyphmetrics) = 0;
	// @description called when cache is cleared.
	virtual void ClearCache() = 0;



	// @description generate FontRenderMetrics for fast-font rendering.
	virtual void BuildText(const uint32_t* chrs, int x, int y);
	// @description render string with cached FontRenderMetrics
	virtual void RenderText();
	// @description render string in specific position, without generating metrics
	virtual void RenderTextInstantly(const uint32_t* chrs, int x, int y);



	FontBaseCache(int cache_width, int cache_height)
		: cache_width(cache_width), cache_height(cache_height),
		cache_x(0xFFFFFFFF), cache_y(0xFFFFFFFF) {}
	~FontBaseCache() { ClearCache(); };
};



/*
* Raw-bitmap caching
*/
class FontBitmapCache : public FontBaseCache {

};



/*
* GLFW library based texture caching TODO
*/
#ifdef GLFW
class FontGLFWCache : public FontBaseCache {

};
#endif



enum FontCacheType {
	FontCacheType_Bitmap,
	FontCacheType_GLFW,
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
	FontBaseCache* ftCache;
	FontTexture* m_rendered_char;
public:
	// @description very basic initalizer, useful for logging text
	bool LoadFont(const char *ttffp, int size, FontColor color=0x000000);
	// @description more complicated initalizer, texture/outline/shadow support
	bool LoadFont(const char *ttffp, const struct FontOption* option);
	// @description more complicated initalizer, texture/outline/shadow support
	bool LoadFont(const FT_Byte *buf, size_t size, const struct FontOption* option);
	// @description automatically called when initalizefont / destruction
	void ClearFont();

	// @description initalize font cache with bitmap
	bool CreateCache(FontCacheType type);
	// @description get internal variable, FontCache.
	FontBaseCache* GetCache();
	// @description clear font cache
	void ClearCache();

	// @description get glyph index, returns 0 if not exists, no fallback.
	FT_UInt GetGlyphIndex(char **chrs_utf8);
	FT_UInt GetGlyphIndex(FT_ULong charcode);



	/*
	 * Rendering each character without cache
	 */

	// @description calls (fallback is optional)
	const FT_GlyphSlot RenderGlyph(const uint32_t charcode, bool usefallback = true);
	// @description generate bitmap with fully rendered font bitmap. (fallback follows FontOption value)
	const FontBitmap* RenderBitmap(const char *chrs_utf8);
	const FontBitmap* RenderBitmap(const uint32_t chr);



	/*
	* Caching / Rendering string with cache
	* CLAIM: if you want to directly cache bitmap/glyphs,
	* call (this)->GetCache()->AddCache( ~ )
	*/

	// @description cache multiple glyphs "at once" (memory consuming, fast)
	void CacheGlyphs(const char *chrs);
	void CacheGlyphs(const uint32_t *chrs);
	// @description upload multiple glyphs (performance consuming)
	void UploadGlyphs(const char *chrs);
	void UploadGlyphs(const uint32_t *chrs);
	// @description render text using cache
	void SetText(const uint32_t *chrs);
	void SetText(const char* chrs);
	// @description render text using cached & built metrics.
	void RenderText();
	// @description render text only using cached texture.
	// metrics is generated instantly, so suggests using text rendering only once.
	void RenderTextInstantly(const uint32_t *chrs);
	void RenderTextInstantly(const char* chrs);


	FontRenderer();
	~FontRenderer();
private:
	// @description this MUST be set right after font is loaded.
	bool SetFontSize(int size);
};



namespace FontUtil {
	FT_Library GetFontLib();
	wchar_t utf8_to_utf16(char **chr_utf8);
	bool utf8_to_utf16(const char *s, size_t iLength, unsigned &start, wchar_t &ch);
	uint32_t utf8_to_utf32(char **chr_utf8);
	bool utf8_to_utf32(const char *s, size_t iLength, unsigned &start, uint32_t &ch);
	bool utf8_to_utf32_string(const char *chr_utf8, std::basic_string<uint32_t>& chr_utf32);
}


// final definition
#define Font FontRenderer

/*
 * I won't provide rendering option here,
 * as we can do many things with rendered images/textures,
 * without modifying bitmap in here.
 */