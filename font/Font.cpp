#include "Font.h"

//
// little refered from: https://www.freetype.org/freetype2/docs/tutorial/step1.html
//

#define FONT_ARGB(a,r,g,b) ( ((a<<24)&0xFF000000)|((r<<16)&0x00FF0000)|((g<<8)&0x0000FF00)|(b&0x000000FF) )
#define FONT_A(v) ((v>>24)&0x000000FF)
#define FONT_R(v) ((v>>16)&0x000000FF)
#define FONT_G(v) ((v>>8)&0x000000FF)
#define FONT_B(v) (v&0x000000FF)

/* internal global function for using freetype font */
FT_Library ftLib;
int ftLibCnt = 0;
void RefFTLib()
{
	if (!ftLibCnt) {
		FT_Error error = FT_Init_FreeType(&ftLib);
		if (error)
		{
			printf("Failed Freetype initalization\n");
			throw("Failed Freetype initalization");
		}
	}
	ftLibCnt++;
}
void DerefFTLib() {
	ftLibCnt--;
	if (ftLibCnt < 0) {
		printf("WARNING - Freetype was more dereferenced than declared\n");
		return;
	}
	if (ftLibCnt == 0) {
		FT_Done_FreeType(ftLib);
	}
}

namespace FontUtil {
	FT_Library GetFontLib() { return ftLib; }

	void ClearFontOption(FontOption *option) {
		memset(option, 0, sizeof(FontOption));
		option->size = 12;
		option->antialiased = 1;
	}

	/* from: stepmania/rage/RageUnicode.cpp */
	int utf8_get_char_len(char p)
	{
		if (!(p & 0x80)) return 1; /* 0xxxxxxx - 1 */
		if (!(p & 0x40)) return 1; /* 10xxxxxx - continuation */
		if (!(p & 0x20)) return 2; /* 110xxxxx */
		if (!(p & 0x10)) return 3; /* 1110xxxx */
		if (!(p & 0x08)) return 4; /* 11110xxx */
		if (!(p & 0x04)) return 5; /* 111110xx */
		if (!(p & 0x02)) return 6; /* 1111110x */
		return 1; /* 1111111x */
	}

	wchar_t utf8_to_utf16(const char *s, int len) {
		// save as UTF32, but some will be truncated over len 3
		wchar_t ch = 0;
		switch (len)
		{
		case 1:
			ch = (s[0] & 0x7F);
			break;
		case 2:
			ch = ((s[0] & 0x1F) << 6) |
				(s[1] & 0x3F);
			break;
		case 3:
			ch = ((s[0] & 0x0F) << 12) |
				((s[1] & 0x3F) << 6) |
				(s[2] & 0x3F);
			break;
		case 4:
			ch = ((s[0] & 0x07) << 18) |
				((s[1] & 0x3F) << 12) |
				((s[2] & 0x3F) << 6) |
				(s[3] & 0x3F);
			break;
		case 5:
			ch = ((s[0] & 0x03) << 24) |
				((s[1] & 0x3F) << 18) |
				((s[2] & 0x3F) << 12) |
				((s[3] & 0x3F) << 6) |
				(s[4] & 0x3F);
			break;
		case 6:
			ch = ((s[0] & 0x01) << 30) |
				((s[1] & 0x3F) << 24) |
				((s[2] & 0x3F) << 18) |
				((s[3] & 0x3F) << 12) |
				((s[4] & 0x3F) << 6) |
				(s[5] & 0x3F);
			break;
		}
		return ch;
	}

	wchar_t utf8_to_utf16(char **s_) {
		char *s = *s_;
		if (*s == 0) return 0;
		int len = utf8_get_char_len(*s);
		wchar_t ch = utf8_to_utf16(s, len);
		*s_ += len;
		return ch;
	}

	bool utf8_to_utf16(const char *s, size_t iLength, unsigned &start, wchar_t &ch)
	{
		if (start >= iLength)
			return false;

		int len = utf8_get_char_len(s[start]);

		if (start + len > iLength)
		{
			// We don't have room for enough continuation bytes. Return error.
			start += len;
			ch = L'?';
			return false;
		}

		ch = utf8_to_utf16(s, len);
		start += len;
		return true;
	}

	uint32_t utf8_to_utf32(const char *s, int len) {
		/*
		U+0000  U+007F    0xxxxxxx
		U+0080  U+07FF    110xxxxx  10xxxxxx
		U+0800  U+FFFF    1110xxxx  10xxxxxx    10xxxxxx
		U+10000 U+10FFFF  11110xxx  10xxxxxx    10xxxxxx    10xxxxxx
		*/
		uint32_t ch = 0;
		switch (len)
		{
		case 1:
			ch = (s[0] & 0x7F);
			break;
		case 2:
			ch = ((s[0] & 0x1F) << 6) |
				(s[1] & 0x3F);
			break;
		case 3:
			ch = ((s[0] & 0x0F) << 12) |
				((s[1] & 0x3F) << 6) |
				(s[2] & 0x3F);
			break;
		case 4:
			ch = ((s[0] & 0x07) << 18) |
				((s[1] & 0x3F) << 12) |
				((s[2] & 0x3F) << 6) |
				(s[3] & 0x3F);
			break;
		case 5:
			ch = ((s[0] & 0x03) << 24) |
				((s[1] & 0x3F) << 18) |
				((s[2] & 0x3F) << 12) |
				((s[3] & 0x3F) << 6) |
				(s[4] & 0x3F);
			break;
		case 6:
			ch = ((s[0] & 0x01) << 30) |
				((s[1] & 0x3F) << 24) |
				((s[2] & 0x3F) << 18) |
				((s[3] & 0x3F) << 12) |
				((s[4] & 0x3F) << 6) |
				(s[5] & 0x3F);
			break;
		}
		return ch;
	}

	uint32_t utf8_to_utf32(char **s_) 
	{
		char *s = *s_;
		if (*s == 0) return 0;
		int len = utf8_get_char_len(*s);
		uint32_t ch = utf8_to_utf32(s, len);
		*s_ += len;
		return ch;
	}

	bool utf8_to_utf32(const char *s, size_t iLength, unsigned &start, uint32_t &ch)
	{
		if (start >= iLength)
			return false;

		int len = utf8_get_char_len(s[start]);

		if (start + len > iLength)
		{
			// We don't have room for enough continuation bytes. Return error.
			start += len;
			ch = '?';	// 0000 003f, anyway it's same byte.
			return false;
		}

		ch = utf8_to_utf32(s, len);
		start += len;
		return true;
	}

	bool utf8_to_utf32_string(const char *chrs, std::basic_string<uint32_t>& wchrs) {
		wchrs.clear();
		char *_c = const_cast<char*>(chrs);
		uint32_t charcode;
		while (charcode = FontUtil::utf8_to_utf32(&_c)) wchrs.push_back(charcode);
		wchrs.push_back(0);
		return true;	// TODO: return false for invalid character
	}
}




/*
 * class FontRenderer
 */

bool FontRenderer::LoadFont(const char * ttffp, int size, FontColor color)
{
	FontOption opt;
	FontUtil::ClearFontOption(&opt);
	opt.size = size;
	opt.color = color;
	return LoadFont(ttffp, &opt);
}

bool FontRenderer::LoadFont(const char * ttffp, const FontOption * option)
{
	FT_Error error = FT_New_Face(ftLib, ttffp, 0, &m_ftFace);
	if (error == FT_Err_Unknown_File_Format) {
		printf("Font loading failed (Unsupported file format)\n");
		return false;
	} else if (error) {
		printf("Font loading failed for unknown reason: %s / error %d\n", ttffp, error);
		return false;
	}
	this->m_ftOption = *option;
	return SetFontSize(option->size);
}

bool FontRenderer::LoadFont(const FT_Byte *buf, size_t size, const struct FontOption* option)
{
	FT_Error error = FT_New_Memory_Face(ftLib, buf, size, 0, &m_ftFace);
	if (error == FT_Err_Unknown_File_Format) {
		printf("Font memory loading failed (Unsupported file format)\n");
		return false;
	}
	else {
		printf("Font memory loading failed for unknown reason: error %d\n", error);
		return false;
	}
	this->m_ftOption = *option;
	return SetFontSize(option->size);
}

void FontRenderer::ClearFont()
{
	if (m_ftFace) {
		FT_Done_Face(m_ftFace);
		m_ftFace = 0;
	}
}

// @description initalize font cache with bitmap
bool FontRenderer::CreateCache(FontCacheType type)
{
	ClearCache();
	switch (type)
	{
	case FontCacheType_Bitmap:
		m_ftGraphic = new FontBitmapGraphic(2048, 2048);
		break;
#ifdef GLFW
	case FontCacheType_GLFW:
		m_ftGraphic = new FontGLFWGraphic(2048, 2048);
		break;
#endif
	default:
		printf("Unknown type of font cache creation failed.\n");
		return false;
	}
	return true;
}

// @description get internal variable, FontCache.
FontBaseGraphic* FontRenderer::GetCache()
{
	return m_ftGraphic;
}

// @description clear font cache
void FontRenderer::ClearCache()
{
}

FT_UInt FontRenderer::GetGlyphIndex(FT_ULong charcode) {
	if (charcode == 0) return 0;
	return FT_Get_Char_Index(m_ftFace, charcode);
}

FT_UInt FontRenderer::GetGlyphIndex(char **chrs) {
	if (*chrs == 0) return 0;
	FT_ULong charcode = FontUtil::utf8_to_utf32(chrs);
	return GetGlyphIndex(charcode);
}

// @description calls (fallback is optional)
const FT_GlyphSlot FontRenderer::RenderGlyph(const uint32_t charcode, bool usefallback = true)
{
	FT_UInt gidx = FT_Get_Char_Index(m_ftFace, charcode);
	FT_GlyphSlot slot_fallback = 0;
	if (!gidx) {
		/* in that case, we can call for *help* to fallback Font object. */
		if (usefallback && m_ftFallback) {
			slot_fallback = m_ftFallback->RenderGlyph(charcode, usefallback);
		}
		printf("Failed getting char index %C", charcode);
	}
	FT_GlyphSlot slot = slot_fallback;
	if (!slot) {
		FT_Error error = FT_Load_Glyph(m_ftFace, gidx, 0);
		if (error) {
			printf("Failed during loading Glyph: %u, error %d", gidx, error);
			return false;
		}
		error = FT_Render_Glyph(m_ftFace->glyph, m_ftOption.antialiased ? FT_RENDER_MODE_NORMAL : FT_RENDER_MODE_MONO);
		if (error) {
			printf("Failed during rendering Glyph: %u, error %d", gidx, error);
			return false;
		}
		slot = m_ftFace->glyph;
	}
	return slot;
}

// @description generate bitmap with fully rendered font bitmap. (fallback follows FontOption value)
const FontSurface* FontRenderer::RenderBitmap(const char *chrs_utf8)
{
	uint32_t chr = FontUtil::utf8_to_utf32(chrs_utf8, FontUtil::utf8_get_char_len(chrs_utf8[0]));
	if (!chr) return 0;
	else return RenderBitmap(chr);
}

const FontSurface* FontRenderer::RenderBitmap(const uint32_t chr)
{
	// internally load glyph first
	FT_GlyphSlot slot = RenderGlyph(chr);
	if (!slot) return 0;

	// prepare to render to surface ...
	_ASSERT(m_renderedChar);
	if (m_renderedChar->p) free(m_renderedChar->p);
	m_renderedChar->p = 0;

	/* bitmap size */
	int px = slot->bitmap_left;
	int py = slot->bitmap_top;
	/* increment size (as it's vertical, ay is meaningless) */
	m_renderedChar->width = slot->advance.x >> 6;
	m_renderedChar->height = text_height;
	/* real bitmap size */
	int bwid = slot->bitmap.width;
	int bhei = slot->bitmap.rows;
	/*
	* we'll copy bitmap to "big Bbox (global bbox height + vertical glyph bbox width)"
	* so we cannot copy memory at once. we need to copy each pixel.
	*/
	int size = m_renderedChar->width * m_renderedChar->height;
	m_renderedChar->p = (unsigned int*)malloc(size);
	memset(m_renderedChar->p, 0, size);	// clear buffer
	for (int y = 0; y < bhei; y++) {
		for (int x = 0; x < bwid; x++) {
			int dx = x + px;
			if (dx < 0) continue;				// dx offset could be minus value; ignore in that case (we won't make such sensitive font, sorry ...)
			int dy = y - py + text_ascender;	// as its axis is upside-down, we have to subtract
			if (dx >= m_renderedChar->width || dy >= m_renderedChar->height || dy < 0)
				break;
			m_renderedChar->p[dx + dy * m_renderedChar->width]
				= slot->bitmap.buffer[x + y * bwid];
		}
	}

	//
	// Outline part
	// - is the same with previous one.
	// (TODO)
	//


	// TODO
	// get glyphs and cache them
	std::vector<FontBitmap*> gs;
	int bitmap_width = 0;
	int bitmap_height = text_height;
	while (*chrs) {
		FontBitmap *b = GetGlyphFromCache(*chrs++);
		if (!b) continue;
		bitmap_width += b->width;
		gs.push_back(b);
	}
	// make big bitmap
	unsigned int *bdata = (unsigned int*)malloc(bitmap_width * bitmap_height * sizeof(int));
	memset(bdata, 0, bitmap_width * bitmap_height * sizeof(int));
	bitmap->p = bdata;
	bitmap->width = bitmap_width;
	bitmap->height = bitmap_height;
	// copy all of them
	int ax = 0;
	for (auto a = gs.begin(); a != gs.end(); ++a) {
		for (int y = 0; y < (*a)->height; y++) {
			for (int x = 0; x < (*a)->width; x++) {
				bdata[ax + x + y * bitmap_width] = 0x00FFFFFF / 255 * (*a)->p[x + y * (*a)->width];
			}
		}
		ax += (*a)->width;
	}


	return m_renderedChar;
}

void FontRenderer::CacheGlyphs(const char *chrs)
{
	std::basic_string<uint32_t> wchrs;
	FontUtil::utf8_to_utf32_string(chrs, wchrs);
	CacheGlyphs(wchrs.c_str());
}

// return if no cache available
#define CHECK_CACHE_AVAILABLE\
	if (!m_ftGraphic) { printf("No font graphic/cache initialized; cannot cache font.\n"); return; }

void FontRenderer::CacheGlyphs(const uint32_t *chrs)
{
	CHECK_CACHE_AVAILABLE;

	// COMMENT: it's kind of double-buffering, is it better to remove double buffering?
	FontRect glyphmetrics;
	std::vector<FontRect> vGlyphmetrics;
	FontSurface cachesurface;

	for (FT_ULong charcode = *chrs; charcode = *chrs; ++chrs) {
		// render new character 
		const FontSurface* bitmap = RenderBitmap(charcode);
		if (!bitmap) continue;
		glyphmetrics.w = bitmap->width;
		glyphmetrics.h = bitmap->height;

		// check new character is available for new caching
		// if not, upload current surface, before write new bitmap character.
		if (!m_ftGraphic->GetNewGlyphCachePosition(glyphmetrics)) {
			m_ftGraphic->AddCache(&cachesurface, vGlyphmetrics);
			vGlyphmetrics.clear();
			m_ftGraphic->ResetGlyphCachePosition();
			m_ftGraphic->GetNewGlyphCachePosition(glyphmetrics);
			memset(cachesurface.p, 0, sizeof(int)*cachesurface.width*cachesurface.height);
		}

		// write new bitmap character.
		// (dst, src, dst_rect, src_x, src_y)
		CopyBit(&cachesurface, bitmap, glyphmetrics, 0, 0);
	}

	// upload remaining surface, if available.
	if (vGlyphmetrics.size()) {
		m_ftGraphic->AddCache(&cachesurface, vGlyphmetrics);
	}
}

void FontRenderer::UploadGlyphs(const char *chrs)
{
	CHECK_CACHE_AVAILABLE;
	std::basic_string<uint32_t> ustr;
	FontUtil::utf8_to_utf32_string(chrs, ustr);
	UploadGlyphs(ustr.c_str());
}

void FontRenderer::UploadGlyphs(const uint32_t *chrs)
{
	CHECK_CACHE_AVAILABLE;

	// TODO: change fontglyphmetrics into fontrect?
	FontRect glyphmetrics;
	while (*chrs) {
		// render character ...
		const FontSurface* bitmap = RenderBitmap(*chrs);

		// ... and upload each character
		if (!m_ftGraphic->GetNewGlyphCachePosition(glyphmetrics)) {
			m_ftGraphic->GenerateNewPage();	// automatically position resetted
			m_ftGraphic->GetNewGlyphCachePosition(glyphmetrics);
		}
		m_ftGraphic->UploadGlyph(bitmap, glyphmetrics);

		// next
		++chrs;
	}
}

// @description render text using cache
void FontRenderer::MakeText(const uint32_t *chrs, FontText& t)
{
	CHECK_CACHE_AVAILABLE;
	m_ftGraphic->BuildText(chrs, 0, 0, t);
}

void FontRenderer::MakeText(const char* chrs, FontText& t)
{
	CHECK_CACHE_AVAILABLE;
	std::basic_string<uint32_t> ustr;
	FontUtil::utf8_to_utf32_string(chrs, ustr);
	MakeText(ustr.c_str(), t);
}

// @description render text using cached & built metrics.
void FontRenderer::RenderText(const FontText& t)
{
	CHECK_CACHE_AVAILABLE;
	m_ftGraphic->RenderText(t);
}

// @description render text only using cached texture.
// metrics is generated instantly, so suggests using text rendering only once.
void FontRenderer::RenderTextInstantly(const uint32_t *chrs, int x, int y)
{
	CHECK_CACHE_AVAILABLE;
	m_ftGraphic->RenderTextInstantly(chrs, x, y);
}

void FontRenderer::RenderTextInstantly(const char* chrs, int x, int y)
{
	CHECK_CACHE_AVAILABLE;
	std::basic_string<uint32_t> ustr;
	FontUtil::utf8_to_utf32_string(chrs, ustr);
	RenderTextInstantly(ustr.c_str(), x, y);
}
#undef CHECK_CACHE_AVAILABLE

FontRenderer::FontRenderer()
	: m_ftFace(0), m_ftFallback(0), m_ftGraphic(0), m_renderedChar(0)
{
	RefFTLib();
	m_renderedChar = new FontSurface();
	m_renderedChar->p = 0;
	m_renderedChar->height = 0;
	m_renderedChar->width = 0;
}

FontRenderer::~FontRenderer()
{
	if (m_renderedChar)
		delete m_renderedChar;

#if USE_TEXTURE
	ClearTexture();
#endif
	ClearCache();
	ClearFont();
	DerefFTLib();
}

bool FontRenderer::SetFontSize(int size)
{
	/* base: 96dpi (none means 72dpi) */
	FT_Error error = FT_Set_Char_Size(m_ftFace, 0, size * 64, 0, 96);
	if (error) {
		printf("Font size error : error %d\n", error);
		return false;
	}
	/* store font height */
	int texhei = m_ftFace->size->metrics.ascender - m_ftFace->size->metrics.descender;
	text_height = texhei >> 6;
	text_ascender = m_ftFace->size->metrics.ascender >> 6;
	return true;
}











/*
 * class FontBaseGraphic
 */

// @description reset glyph's sx/sy of cache position. requiers width/height.
bool FontBaseGraphic::GetNewGlyphCachePosition(FontRect& glyph)
{
	_ASSERT(glyph.w < cache_width && glyph.h < cache_height);
	if (cache_y == 0xFFFFFFFF) {
		// to prevent overflow. new page required
		return false;
	}
	if (cache_x + glyph.w > cache_width) {
		cache_x = 0;
		cache_y += glyph.h;
	}
	if (cache_y + glyph.h > cache_height) {
		// new page required
		return false;
	}
	glyph.x = cache_x;
	glyph.y = cache_y;
	cache_x += glyph.w;
	return true;
}

void FontBaseGraphic::ResetGlyphCachePosition()
{
	cache_x = cache_y = 0;
}

void FontBaseGraphic::BuildText(const uint32_t* chrs, int x, int y, FontText &t)
{
	t.pos_x = x;
	t.pos_y = y;
	t.m_char_metrics.clear();

	// if no glyph found, then use question mark as default.
	int cx = 0;
	while (*chrs) {
		uint32_t chr = *chrs;
		if (m_cache_glyphs.find(*chrs) == m_cache_glyphs.end()) {
			chr = '?';
		}
		FontRenderRect r = m_cache_glyphs[chr];
		FontRenderMetrics m;
		m.p = r.p;
		m.sx = r.r.x;
		m.sy = r.r.y;
		m.sw = r.r.w;
		m.sh = r.r.h;
		m.dx = cx;
		m.dy = 0;
		m.dw = r.r.w;
		m.dh = r.r.h;
		cx += r.r.w;
		t.m_char_metrics.push_back(m);

		++chrs;
	}
}

// @description render string with cached FontRenderMetrics
void FontBaseGraphic::RenderText(const FontText& t)
{
	// COMMENT: x/y position operation should be done in parent class
	for (auto a = t.m_char_metrics.begin(); a != t.m_char_metrics.end(); ++a)
	{
		RenderSingleMetrics(*a);
	}
}

// @description render string in specific position, without generating metrics
void FontBaseGraphic::RenderTextInstantly(const uint32_t* chrs, int x, int y)
{
	// internally build text, and render.
	FontText t;
	BuildText(chrs, x, y, t);
	RenderText(t);
}




/*
* class FontBitmapCache
*/

void FontRenderer::CacheGlyphs(const uint32_t *chrs)
{
	// use RenderGlyph() to make this function work
	for (FT_ULong charcode = *chrs; charcode = *chrs; ++chrs) {
		FontBitmap fbit;
		if (RenderGlyph(charcode, &fbit))
			glyphs[charcode] = fbit;
	}
}

void FontRenderer::CacheGlyphs(const char *chrs)
{
	std::basic_string<uint32_t> wchrs;
	FontUtil::utf8_to_utf32_string(chrs, wchrs);
	CacheGlyphs(wchrs.c_str());
}

// @description clear glyph cache
void FontRenderer::ClearCache()
{
	for (auto a = glyphs.begin(); a != glyphs.end(); ++a) {
		free(a->second.p);
	}
	glyphs.clear();
}


/*
 * TODOs

 *. support UTF8 text
 *. (not glyph, but bitmap caching enabled)
 3. texture-rendering available
 4. outline available
 5. fallback font settable
 
 */