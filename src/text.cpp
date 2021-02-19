#include <text.hpp>

#include <common.hpp>
#include <render.hpp>

#include <codecvt>
#include <iomanip>
#include <iostream>
#include <locale>
#include <map>
#include <sstream>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

/**
 * Helper function for checking FreeType errors
 */
void FT_Check(FT_Error e) {
    if (e == 0) {
        // Not a problem
        return;
    }

    // Immediately-Invoked Function Expression for getting error string
    const char* errStr = [&]{
        // Codes and descriptions taken from
        // https://www.freetype.org/freetype2/docs/reference/ft2-error_code_values.html
        // because these strings are missing in release builds of FreeType
        switch (e) {
            default: return "unknown error";

            /* generic errors */
            case 0x00: return "no error";

            case 0x01: return "cannot open resource";
            case 0x02: return "unknown file format";
            case 0x03: return "broken file";
            case 0x04: return "invalid FreeType version";
            case 0x05: return "module version is too low";
            case 0x06: return "invalid argument";
            case 0x07: return "unimplemented feature";
            case 0x08: return "broken table";
            case 0x09: return "broken offset within table";
            case 0x0A: return "array allocation size too large";
            case 0x0B: return "missing module";
            case 0x0C: return "missing property";

            /* glyph/character errors */

            case 0x10: return "invalid glyph index";
            case 0x11: return "invalid character code";
            case 0x12: return "unsupported glyph image format";
            case 0x13: return "cannot render this glyph format";
            case 0x14: return "invalid outline";
            case 0x15: return "invalid composite glyph";
            case 0x16: return "too many hints";
            case 0x17: return "invalid pixel size";

            /* handle errors */

            case 0x20: return "invalid object handle";
            case 0x21: return "invalid library handle";
            case 0x22: return "invalid module handle";
            case 0x23: return "invalid face handle";
            case 0x24: return "invalid size handle";
            case 0x25: return "invalid glyph slot handle";
            case 0x26: return "invalid charmap handle";
            case 0x27: return "invalid cache manager handle";
            case 0x28: return "invalid stream handle";

            /* driver errors */

            case 0x30: return "too many modules";
            case 0x31: return "too many extensions";

            /* memory errors */

            case 0x40: return "out of memory";
            case 0x41: return "unlisted object";

            /* stream errors */

            case 0x51: return "cannot open stream";
            case 0x52: return "invalid stream seek";
            case 0x53: return "invalid stream skip";
            case 0x54: return "invalid stream read";
            case 0x55: return "invalid stream operation";
            case 0x56: return "invalid frame operation";
            case 0x57: return "nested frame access";
            case 0x58: return "invalid frame read";

            /* raster errors */

            case 0x60: return "raster uninitialized";
            case 0x61: return "raster corrupted";
            case 0x62: return "raster overflow";
            case 0x63: return "negative height while rastering";

            /* cache errors */

            case 0x70: return "too many registered caches";

            /* TrueType and SFNT errors */

            case 0x80: return "invalid opcode";
            case 0x81: return "too few arguments";
            case 0x82: return "stack overflow";
            case 0x83: return "code overflow";
            case 0x84: return "bad argument";
            case 0x85: return "division by zero";
            case 0x86: return "invalid reference";
            case 0x87: return "found debug opcode";
            case 0x88: return "found ENDF opcode in execution stream";
            case 0x89: return "nested DEFS";
            case 0x8A: return "invalid code range";
            case 0x8B: return "execution context too long";
            case 0x8C: return "too many function definitions";
            case 0x8D: return "too many instruction definitions";
            case 0x8E: return "SFNT font table missing";
            case 0x8F: return "horizontal header (hhea) table missing";
            case 0x90: return "locations (loca) table missing";
            case 0x91: return "name table missing";
            case 0x92: return "character map (cmap) table missing";
            case 0x93: return "horizontal metrics (hmtx) table missing";
            case 0x94: return "PostScript (post) table missing";
            case 0x95: return "invalid horizontal metrics";
            case 0x96: return "invalid character map (cmap) format";
            case 0x97: return "invalid ppem value";
            case 0x98: return "invalid vertical metrics";
            case 0x99: return "could not find context";
            case 0x9A: return "invalid PostScript (post) table format";
            case 0x9B: return "invalid PostScript (post) table";
            case 0x9C: return "found FDEF or IDEF opcode in glyf bytecode";
            case 0x9D: return "missing bitmap in strike";

            /* CFF, CID, and Type 1 errors */

            case 0xA0: return "opcode syntax error";
            case 0xA1: return "argument stack underflow";
            case 0xA2: return "ignore";
            case 0xA3: return "no Unicode glyph name found";
            case 0xA4: return "glyph too big for hinting";

            /* BDF errors */

            case 0xB0: return "`STARTFONT' field missing";
            case 0xB1: return "`FONT' field missing";
            case 0xB2: return "`SIZE' field missing";
            case 0xB3: return "`FONTBOUNDINGBOX' field missing";
            case 0xB4: return "`CHARS' field missing";
            case 0xB5: return "`STARTCHAR' field missing";
            case 0xB6: return "`ENCODING' field missing";
            case 0xB7: return "`BBX' field missing";
            case 0xB8: return "`BBX' too big";
            case 0xB9: return "Font header corrupted or missing fields";
            case 0xBA: return "Font glyphs corrupted or missing fields";
        }
    }();

    auto msg = "FreeType error: FT_Error " + std::to_string(e) + ": \"" + errStr + '\"';
    std::cerr << msg << std::endl;
    throw std::runtime_error(msg);
}

/**
 * Helper class for loading and unloading FreeType.
 * Intended to be used as a singleton, so that a single
 * FT_Library can be used across the entire program.
 * Use FreeTypeContext::get() to access the singleton.
 */
class FreeTypeContext {
public:
    FreeTypeContext()
        : m_ftl{}
        , m_vao{0}
        , m_vbo{0}
        , m_textShader{0} {

        // Initialize FreeType library
        FT_Check(FT_Init_FreeType(&m_ftl));

        // disable byte-alignment restriction in OpenGL
        // so that unaligned 1-byte-per-colour textures
        // can be used
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        gl_has_errors();

        // Enable alpha blending
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        gl_has_errors();

        // Generate vertex array and vertex buffer objects for rendering
        // one glyph at a time. GL_DYNAMIC_DRAW is used to allow rapidly
        // changing vertex data while re-computing glyph coordinates.
        glGenVertexArrays(1, m_vao.data());
        glGenBuffers(1, m_vbo.data());
        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
        gl_has_errors();

        // Load text-rendering shaders
        m_textShader.load_from_file("data/shaders/text.vs.glsl", "data/shaders/text.fs.glsl");
    }

    ~FreeTypeContext() {
        // Clean up the FreeType library
        FT_Check(FT_Done_FreeType(m_ftl));
    }

    // Copy construction is disabled
    FreeTypeContext(const FreeTypeContext&) = delete;

    // Get the static FreeTypeContext instance
    // NOTE: shared_ptr is used (instead of ordinary reference) so that
    // any fonts destroyed after main() exists still point to a valid
    // font library. Otherwise, the static font library might be destroyed
    // before other static objects that still need it (such as Font objects
    // being used by Text objects in ECS::registry<Text>)
    // See Static Initialization Order Fiasco for details:
    // https://en.cppreference.com/w/cpp/language/siof

    static std::shared_ptr<FreeTypeContext> get() noexcept {
        static std::shared_ptr<FreeTypeContext> theInstance = std::make_shared<FreeTypeContext>();
        return theInstance;
    }

    FT_Library library() noexcept {
        return m_ftl;
    }

    const GLResource<VERTEX_ARRAY>& vao() const noexcept {
        return m_vao;
    }

    const GLResource<BUFFER>& vbo() const noexcept {
        return m_vbo;
    }

    Effect& textShader() noexcept {
        return m_textShader;
    }

private:
    FT_Library m_ftl;
    GLResource<VERTEX_ARRAY> m_vao;
    GLResource<BUFFER> m_vbo;
    Effect m_textShader;
};



Text::Text(std::string content, std::shared_ptr<Font> font, glm::vec2 position, float scale, glm::vec3 colour) noexcept
    : content(std::move(content))
    , font(std::move(font))
    , position(position)
    , scale(scale)
    , colour(colour) {

}

Text::Text(std::string content, const std::string& pathToTTF, glm::vec2 position, float scale, glm::vec3 colour) noexcept
    : content(std::move(content))
    , font(Font::load(pathToTTF))
    , position(position)
    , scale(scale)
    , colour(colour) {

}

Font::Font(const std::string& pathToTTF)
    : m_face{}
    , m_context(FreeTypeContext::get()) {
    
    assert(m_context);
    auto ftl = m_context->library();

    // Construct a new FreeType font face from the TTF file
    FT_Check(FT_New_Face(ftl, pathToTTF.c_str(), 0, &m_face));

    // Request a vertical size of 48 pixels. The horizontal
    // size is inferred if 0 is passed.
    FT_Check(FT_Set_Pixel_Sizes(m_face, 0, 48));

    // Use the Unicode character encoding
    FT_Check(FT_Select_Charmap(m_face, FT_ENCODING_UNICODE));

    // pre-load all printable ASCII characters
    for (std::uint32_t c = 0x20; c < 0x7F; ++c) {
		(void)getCharacter(c);
	}
}

Font::~Font() noexcept {
    // Clean up the font face
    FT_Check(FT_Done_Face(m_face));
}

std::shared_ptr<Font> Font::load(const std::string& pathToTTF) {
    // Static cache for fonts that have already been loaded.
    // fonts are used elsewhere using shared_ptr, but are stored
    // in the cache using weak_ptr. This allows fonts to be destroyed
    // when they are no longer used elsewhere, in which case the
    // weak_ptr will no longer be convertible to a valid shared_ptr.
    static std::map<std::string, std::weak_ptr<Font>> s_fontCache;

    // Look for a matching font in the cache
    auto it = s_fontCache.find(pathToTTF);
    if (it != end(s_fontCache)) {
        // If there is a weak_ptr to the font
        if (auto sp = it->second.lock()) {
            // If that weak_ptr still points to a living font, return it
            return sp;
        }
    }
    // otherwise, if there is no match, construct a new shared_ptr for the font
    auto sp = std::make_shared<Font>(pathToTTF);

    // cache the new font
    s_fontCache[pathToTTF] = sp;

    return sp;
}

const Font::Character& Font::getCharacter(std::uint32_t codePoint) {
    // Search for the code point in the cached character map
    auto it = m_characters.find(codePoint);
    if (it != end(m_characters)) {
        // The code point has been cached
        return it->second;
    }

    // The code point is being loaded for the first time

    // Load and render the character from the font face
    // NOTE: errors are reported but not thrown as exceptions here.
    // This allows missing glyphs (which happens often) to be
    // acknowledged without stopping execution.
    if (FT_Load_Char(m_face, codePoint, FT_LOAD_RENDER) != 0) {
        // dummy iostream for restoring formatting flags
        auto prevFmtState = std::stringstream{};
        prevFmtState.copyfmt(std::cerr);

		std::cerr << "Failed to load glyph: ";
        // Unicode code points are of the form U+NNNN where NNNN is a hexadecimal number with leading zeroes
        std::cerr << "U+" << std::setfill('0') << std::setw(4) << std::right << std::hex << codePoint;
        if (codePoint >= 0x20 && codePoint <= 0x7E) {
            std::cerr << '\'' << static_cast<char>(codePoint) << '\'';
        }
        std::cerr << std::endl;

        // restore previous formatting options
        std::cerr.copyfmt(prevFmtState);
	}

    // Generate an OpenGL texture from the newly-rendered bitmap
    // NOTE: the bitmap may be empty (buffer is null and width &
    // rows are 0) if the glyph could not be loaded by FT_Load_Char
    // but this is fine and OpenGL can handle it.
	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RED, // One monochromatic byte per texel
		m_face->glyph->bitmap.width,
		m_face->glyph->bitmap.rows,
		0,
		GL_RED,
		GL_UNSIGNED_BYTE,
		m_face->glyph->bitmap.buffer
	);
    
    gl_has_errors();

	// set texture options
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
    gl_has_errors();

	// Cache the character configuration
	auto character = Character{
        // OpenGL texture
		texture,

        // size of the texture, in pixels
		glm::ivec2{
            m_face->glyph->bitmap.width,
            m_face->glyph->bitmap.rows
        },

        // the glyph's origin within the texture
		glm::ivec2{
            m_face->glyph->bitmap_left,
            m_face->glyph->bitmap_top
        },

        // the horizontal displacement for the next glyph
		static_cast<unsigned int>(m_face->glyph->advance.x)
	};

    // Insert the newly-created character into the cache.
    // If the character is already in the cache, it would
    // have been returned above, so this should succeed.
	auto it_and_success = m_characters.emplace(codePoint, std::move(character));
    assert(it_and_success.second);
    return it_and_success.first->second;
}


/**
 * Helper function to convert a UTF-8 encoded std::string to a
 * UTF-32 string containing complete code points.
 * 
 * NOTE: ASCII strings are valid UTF-8 strings because UTF-8
 * is backwards-compatible with ASCII.
 * 
 * NOTE: UTF-8-encode std::strings can be constructed using
 * the `u8` string literal prefix, as in `u8"some international text"`.
 * See https://en.cppreference.com/w/cpp/language/string_literal
 */
std::u32string utf8ToUtf32(const std::string& str) {
    // NOTE: std::wstring_convert was introduced in C++11 and then
    // spontaneously deprecated in C++17 without any replacement.
    // Non-ASCII handling in C++ sucks in general. This will hopefully
    // improve in C++23 but who can say.
    using convert_t = std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t>;
    return convert_t{}.from_bytes(str);
}

void drawText(const Text& text, glm::ivec2 frameBufferSize) {
    assert(text.font);

    // The on-screen baseline origin of the current glyph being drawn
    auto cursor = text.position;

    // Use the text shader
    assert(text.font->m_context);
    auto& ctx = *text.font->m_context;
    auto& shader = ctx.textShader();
    glUseProgram(shader.program);
    
    gl_has_errors();

    // Orthographic projection matrix for placing text on-screen
    glm::mat4 projection = glm::ortho(
        0.0f,
        static_cast<float>(frameBufferSize.x),
        0.0f,
        static_cast<float>(frameBufferSize.y)
    );

    // Pass the projection matrix uniform, see data/shaders/text.vs.glsl
    glUniformMatrix4fv(
        glGetUniformLocation(shader.program, "projection"),
        1,
        GL_FALSE,
        glm::value_ptr(projection)
    );

    // Pass the text color uniform, see data/shaders/text.fs.glsl
	glUniform3f(
        glGetUniformLocation(shader.program, "textColor"),
        text.colour.x,
        text.colour.y,
        text.colour.z
    );
        
    gl_has_errors();

	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(ctx.vao());
        
    gl_has_errors();


    // Convert ASCII/UTF-8 text to Unicode code points
    const auto u32str = utf8ToUtf32(text.content);

    // For each Unicode code point
	for (const auto& c : u32str) {
        // get (or create) the character from the font
		const auto& ch = text.font->getCharacter(c);
		
        // compute the on-screen texture coordinates from the cursor's
        // baseline origin
		const auto xpos = cursor.x + ch.Bearing.x * text.scale;
		const auto ypos = cursor.y + (ch.Bearing.y - ch.Size.y) * text.scale;

		const auto w = ch.Size.x * text.scale;
		const auto h = ch.Size.y * text.scale;

        // Two triangles for the top and bottom halves of a quad
		float vertices[6][4] = {
			{ xpos,     ypos + h, 0.0f, 0.0f },
			{ xpos,     ypos,     0.0f, 1.0f },
			{ xpos + w, ypos,     1.0f, 1.0f },
			{ xpos,     ypos + h, 0.0f, 0.0f },
			{ xpos + w, ypos,     1.0f, 1.0f },
			{ xpos + w, ypos + h, 1.0f, 0.0f }
		};

        // draw the texture in the quad
		glBindTexture(GL_TEXTURE_2D, ch.Texture);
		glBindBuffer(GL_ARRAY_BUFFER, ctx.vbo());
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glDrawArrays(GL_TRIANGLES, 0, 6);
        
        gl_has_errors();

        // Move the cursor to the next glyph position.
        // NOTE: advance is in units of 1/64 pixels
		cursor.x += ch.Advance / 64.0f * text.scale;
	}
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
        
    gl_has_errors();
}
