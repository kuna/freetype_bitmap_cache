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
// (only contains alpha channel - 8bit)
struct FontBitmap {
	unsigned char *p;
	int width;
	int height;
};

// @description used for font foreground texture
// (ARGB data - 32bit)
struct FontSurface {
	unsigned int *p;	// direct pointer to bitmap/texture
	int width;
	int height;
};

// @description used for shadow position setting
struct FontVector {
	int x, y;
};

// @description used for SetText / or something else ...
struct FontRect {
	int x, y, w, h;
};

struct FontRenderMetrics {
	void *p;
	float sx, sy, sw, sh;
	float dx, dy, dw, dh;
};

enum FontCacheType {
	FontCacheType_Bitmap,
	FontCacheType_GLFW,
};



/*
 * @description
 * these options are related about rendering texts
 * TODO: add some blend option, in case of need.
 */
struct FontText {
	FontCacheType m_cachetype;
	std::vector<FontRenderMetrics> m_char_metrics;
	int pos_x, pos_y;
	int text_align;
};

/*
 * @description
 * do graphic related (caching / rendering)
 *
 * CLAIM: only allows SAME-HEIGHT bitmap character for caching!
 */
class FontBaseGraphic {
private:
	/* for internal use */
	struct FontRenderRect {
		void *p;
		FontRect r;
	};

	// @description this does not cache really; these variable/funcs just presets cache position.
	unsigned int cache_x, cache_y, cache_width, cache_height;
	std::vector<void*> m_cache_page;
	std::map<uint32_t, FontRenderRect> m_cache_glyphs;
	// @description reset glyph's sx/sy of cache position. requires width/height.
	// returns false if no more space enough for new glyph.
	// you need to manually generate new page in case of need.
	bool GetNewGlyphCachePosition(FontRect& glyph);
	void ResetGlyphCachePosition();
	virtual void GenerateNewPage() {};
	// @description render single char using pre-calculated metrics.
	virtual void RenderSingleMetrics(const FontRenderMetrics& metrics) {};
	friend class FontRenderer;
public:
	// @description load cache from file.
	// if multiple cache, then call this method for many times you want.
	virtual void AddCache(const FontSurface* bitmap, const std::vector<FontRect>& glyphmetrics) {};
	// @description upload glyphs, ONLY ALLOWS 8bit BITMAP DATA!
	virtual bool UploadGlyph(const FontSurface *bitmap, const FontRect &r) {};
	// @description called when cache is cleared.
	virtual void ClearCache() {};



	// @description generate FontRenderMetrics for fast-font rendering.
	virtual void BuildText(const uint32_t* chrs, int x, int y, FontText &t);
	// @description render string with cached FontRenderMetrics
	virtual void RenderText(const FontText& t);
	// @description render string in specific position, without generating metrics
	virtual void RenderTextInstantly(const uint32_t* chrs, int x, int y);



	FontBaseGraphic(int cache_width, int cache_height)
		: cache_width(cache_width), cache_height(cache_height),
		cache_x(0xFFFFFFFF), cache_y(0xFFFFFFFF) {}
	~FontBaseGraphic() { ClearCache(); };
};



/*
* Raw-bitmap caching
*/
class FontBitmapGraphic : public FontBaseGraphic {
private:
	// @description target of rendering font cache
	FontBitmap *m_rendering_target;

	virtual void GenerateNewPage();
	virtual void RenderSingleMetrics(FontRenderMetrics& metrics);
public:
	void SetRenderTarget(FontBitmap* bitmap);

	virtual void BuildText(const uint32_t* chrs, int x, int y, FontText &t);
	virtual void RenderText(FontText& t);
	virtual void RenderTextInstantly(const uint32_t* chrs, int x, int y);


	FontBitmapGraphic(int w, int h) : FontBaseGraphic(w, h) {};
};



/*
* GLFW library based texture caching TODO
*/
#ifdef GLFW
class FontGLFWGraphic : public FontBaseGraphic {

};
#endif




// @description configurable font options
struct FontOption {
	int size;
	FontColor color;
	FontSurface foreground_surface;
	FontStyle style;
	int antialiased;

	int outline_size;
	FontColor outline_color;

	FontVector shadow;
	FontColor shadow_color;
};

class FontRenderer {
	FontOption m_ftOption;
	// @description used for real heighting. (global height)
	int text_height;	// Bbox height
	int text_ascender;	// baseline position

	FT_Face m_ftFace;
	FontRenderer* m_ftFallback;
	FontBaseGraphic* m_ftGraphic;
	FontSurface* m_renderedChar;
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
	FontBaseGraphic* GetCache();
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
	const FontSurface* RenderBitmap(const char *chrs_utf8);
	const FontSurface* RenderBitmap(const uint32_t chr);



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
	void MakeText(const uint32_t *chrs, FontText& t);
	void MakeText(const char* chrs, FontText& t);
	// @description render text using cached & built metrics.
	void RenderText(const FontText& t);
	// @description render text only using cached texture.
	// metrics is generated instantly, so suggests using text rendering only once.
	void RenderTextInstantly(const uint32_t *chrs, int x, int y);
	void RenderTextInstantly(const char* chrs, int x, int y);


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