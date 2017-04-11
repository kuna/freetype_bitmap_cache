#include "Font.h"

#ifdef USE_LIBPNG
#include <png.h>
#endif

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

// @description generate bitmap for SINGLE char with fully rendered font bitmap. (fallback follows FontOption value)
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
	/* as it's vertical, ay is meaningless */
	m_renderedChar->width = slot->advance.x >> 6;
	m_renderedChar->height = text_height;
	int chrbitmapsize =
		m_renderedChar->width * m_renderedChar->height * sizeof(int);
	m_renderedChar->p = (unsigned int*)malloc(chrbitmapsize);
	memset(m_renderedChar->p, 0, chrbitmapsize);	// clear buffer

	/* bitmap size */
	int px = slot->bitmap_left;
	int py = slot->bitmap_top;
	/* real bitmap size by freetype */
	int bwid = slot->bitmap.width;
	int bhei = slot->bitmap.rows;
	/*
	* we'll copy bitmap to "big Bbox (global bbox height + vertical glyph bbox width)"
	* so we cannot copy memory at once. we need to copy each pixel.
	*/
	for (int y = 0; y < bhei; y++) {
		for (int x = 0; x < bwid; x++) {
			int dx = x + px;
			if (dx < 0) continue;				// dx offset could be minus value; ignore in that case (we won't make such sensitive font, sorry ...)
			int dy = y - py + text_ascender;	// as its axis is upside-down, we have to subtract
			if (dx >= m_renderedChar->width || dy >= m_renderedChar->height || dy < 0)
				break;

			/* get bitmap pixel & software alpha-blending */
			int pixel = m_ftOption.color;
			if (m_ftOption.foreground_surface.p) {
				int surf_x = dx;
				int surf_y = dy;
				if (surf_x < 0) surf_x = 0;
				if (surf_y < 0) surf_y = 0;
				if (surf_x > m_ftOption.foreground_surface.width) surf_x = m_ftOption.foreground_surface.width;
				if (surf_y > m_ftOption.foreground_surface.height) surf_y = m_ftOption.foreground_surface.height;
				pixel = static_cast<int*>(m_ftOption.foreground_surface.p)[surf_x + surf_y * m_ftOption.foreground_surface.width];
			}
			pixel = (((pixel >> 24) * slot->bitmap.buffer[x + y * bwid]) << 24) | (pixel & 0x00FFFFFF);
			static_cast<int*>(m_renderedChar->p)[dx + dy * m_renderedChar->width] = pixel;
		}
	}

	//
	// Outline part
	// - is the same with previous one.
	// (TODO)
	//

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

namespace {
	void CopyBit(FontSurface *dst, const FontSurface *src, const FontRect &dst_rect, int src_x, int src_y)
	{
		if (src_x < 0) src_x = 0;
		if (src_y < 0) src_y = 0;

		// check is dst is bigger than src
		int src_w = dst_rect.w;
		int src_h = dst_rect.h;
		if (src_w > src->width || src_w < 0) src_w = src->width;
		if (src_h > src->height || src_h < 0) src_h = src->height;

		// check real copying size by checking src/dst width
		if (src_w > src->width - src_x) src_w = src->width - src_x;
		if (src_w > dst->width - dst_rect.x) src_w = dst->width - dst_rect.x;
		if (src_h > src->height - src_y) src_h = src->height - src_y;
		if (src_h > dst->height - dst_rect.y) src_h = dst->height - dst_rect.y;
		if (src_w <= 0 || src_h <= 0) return;

		// now copy pixel one by one
		for (int y = 0; y < src_h; y++) {
			for (int x = 0; x < src_w; x++) {
				int dx = x + dst_rect.x;
				int dy = y + dst_rect.y;
				static_cast<int*>(dst->p)[dx + dy * dst->width] = static_cast<int*>(src->p)[x + y * src->width];
			}
		}
	}
}

void FontRenderer::CacheGlyphs(const uint32_t *chrs)
{
	CHECK_CACHE_AVAILABLE;

	// COMMENT: it's kind of double-buffering, is it better to remove double buffering?
	FontRect glyphmetrics;
	std::map<uint32_t, FontRect> vGlyphmetrics;
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
		vGlyphmetrics[*chrs] = glyphmetrics;
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
		m_ftGraphic->UploadGlyph(bitmap, *chrs, glyphmetrics);

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

void FontBaseGraphic::ClearFontSurface()
{
	// WARNING: this won't clear font glyphs, so be careful to use this method.
	for (auto a = m_cache_page.begin(); a != m_cache_page.end(); ++a) {
		free(a->p);
	}
	m_cache_page.clear();
}

#ifdef USE_SAVEANDLOAD
#define ABORT(expr)\
		if (expr) { fclose(fp); return -1; }
namespace {
	int loadpngfile(FontSurface* surf, const std::string& destpath)
	{
		FILE *fp = fopen(folder.c_str(), "rb");
		fread(header, 1, 8, fp);
		ABORT(png_sig_cmp(header, 0, 8));
		ABORT(!(png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)));
		ABORT(!(info_ptr = png_create_info_struct(png_ptr)));
		ABORT(setjmp(png_jmpbuf(png_ptr)))

			png_init_io(png_ptr, fp);
		png_set_sig_bytes(png_ptr, 8);

		png_read_info(png_ptr, info_ptr);

		width = png_get_image_width(png_ptr, info_ptr);
		height = png_get_image_height(png_ptr, info_ptr);
		color_type = png_get_color_type(png_ptr, info_ptr);
		bit_depth = png_get_bit_depth(png_ptr, info_ptr);

		number_of_passes = png_set_interlace_handling(png_ptr);
		png_read_update_info(png_ptr, info_ptr);

		/* read file */
		ABORT(setjmp(png_jmpbuf(png_ptr)));

		row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
		for (y = 0; y<height; y++)
			row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png_ptr, info_ptr));

		png_read_image(png_ptr, row_pointers);

		fclose(fp);
		return -1;
	}

	int savepngfile(const FontSurface* surf, const std::string& destpath)
	{
		/* create file */
		FILE *fp = fopen(file_name, "wb");
		if (!fp)
			abort_("[write_png_file] File %s could not be opened for writing", file_name);


		/* initialize stuff */
		png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

		if (!png_ptr)
			abort_("[write_png_file] png_create_write_struct failed");

		info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr)
			abort_("[write_png_file] png_create_info_struct failed");

		if (setjmp(png_jmpbuf(png_ptr)))
			abort_("[write_png_file] Error during init_io");

		png_init_io(png_ptr, fp);


		/* write header */
		if (setjmp(png_jmpbuf(png_ptr)))
			abort_("[write_png_file] Error during writing header");

		png_set_IHDR(png_ptr, info_ptr, width, height,
			bit_depth, color_type, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

		png_write_info(png_ptr, info_ptr);


		/* write bytes */
		if (setjmp(png_jmpbuf(png_ptr)))
			abort_("[write_png_file] Error during writing bytes");

		png_write_image(png_ptr, row_pointers);


		/* end write */
		if (setjmp(png_jmpbuf(png_ptr)))
			abort_("[write_png_file] Error during end of write");

		png_write_end(png_ptr, NULL);

		/* cleanup heap allocation */
		for (y = 0; y<height; y++)
			free(row_pointers[y]);
		free(row_pointers);

		fclose(fp);
	}
}
#undef ABORT

virtual int FontBaseGraphic::SaveCache(const std::string& folder, const std::string& name)
{
	// make surface if not exists
	if (!m_cache_page.size()) PrepareFontSurface();
	// save glyphs
	// TODO
	// save image to png
	for (auto a = m_cache_page.begin(); a != m_cache_page.end(); ++a) {
		if (!savepngfile(*a, folder+"/dummy.png"))
		{
			throw "CANNOT save font cache ...";
		}
	}
	return 0;
}

virtual int FontBaseGraphic::LoadCache(const std::string& folder, const std::string& name)
{
	// clear surface and other etcs first
	ClearFontSurface();
	// load glyphs
	// TODO

	// load png images
	// ...
}
#endif




/*
* class FontBitmapGraphic
*/

void FontBitmapGraphic::GenerateNewPage(int w, int h)
{
	if (w == 0) w = cache_width;
	if (h == 0) h = cache_height;
	FontSurface p;
	p.p = (unsigned int*)malloc(w * h * sizeof(int));
	p.width = w;
	p.height = h;
	ResetGlyphCachePosition();
	m_cache_page.push_back(p);
}

void FontBitmapGraphic::RenderSingleMetrics(FontRenderMetrics& metrics)
{
	_ASSERT(m_rendering_target);
	_ASSERT(metrics.p);

	// COMMENT: we don't check FontRenderMetrics object here; this MUST be valid object.
	FontSurface *src = (FontSurface*)metrics.p;
	FontRect dst_rect;
	dst_rect.x = metrics.dx + offset_x;
	dst_rect.y = metrics.dy + offset_y;
	dst_rect.w = metrics.dw;
	dst_rect.h = metrics.dh;
	CopyBit(m_rendering_target, src, dst_rect, 0, 0);
}

void FontBitmapGraphic::SetRenderTarget(FontSurface* bitmap)
{
	m_rendering_target = bitmap;
}

void FontBitmapGraphic::AddCache(const FontSurface* bitmap, const std::map<uint32_t, FontRect>& glyphmetrics)
{
	// copy bitmap first
	_ASSERT(bitmap->p);
	GenerateNewPage(bitmap->width, bitmap->height);
	FontSurface *p = &m_cache_page.back();
	memcpy(p->p, bitmap->p, bitmap->width * bitmap->height * sizeof(int));

	// and convert into FontRenderRect
	FontRenderRect r;
	r.p = p;	// COMMENT: void* : FontSurface*
	for (auto a = glyphmetrics.begin(); a != glyphmetrics.end(); ++a)
	{
		r.r = a->second;
		m_cache_glyphs[a->first] = r;
	}
}

bool FontBitmapGraphic::UploadGlyph(const FontSurface *bitmap, uint32_t charcode, FontRect &r)
{
	// if not available, then make new page
	if (!GetNewGlyphCachePosition(r)) {
		GenerateNewPage();
		GetNewGlyphCachePosition(r);
	}
	// render at new page
	FontSurface* p = &m_cache_page.back();
	CopyBit(p, bitmap, r, 0, 0);
	// cache FontRenderRect
	// COMMENT: void* : FontSurface*
	FontRenderRect ft_r;
	ft_r.p = p;
	ft_r.r = r;
	m_cache_glyphs[charcode] = ft_r;
}

void FontBitmapGraphic::ClearCache()
{
	ClearFontSurface();
	m_cache_glyphs.clear();
}

void FontBitmapGraphic::RenderText(const FontText& t)
{
	if (t.m_cachetype != m_ftCachetype) {
		printf("Cache type mismatch; unusable text");
		return;
	}
	offset_x = t.pos_x;
	offset_y = t.pos_y;
	FontBaseGraphic::RenderText(t);
}

FontBitmapGraphic::FontBitmapGraphic(int w, int h)
	: FontBaseGraphic(w, h, FontCacheType_Bitmap), m_rendering_target(0)
{}

const FontSurface* FontBitmapGraphic::GetFontPageBitmap(int pagenum)
{
	return &m_cache_page[pagenum];
}
/*
 * TODOs

 *. support UTF8 text
 *. (not glyph, but bitmap caching enabled)
 *. texture-rendering available
 4. outline available
 5. fallback font settable
 
 */