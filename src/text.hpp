#pragma once

#include <map>
#include <memory>
#include <string>

#include <render_components.hpp>

#include <glm/glm.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

// Forward declaration, see class definition below
class Font;

/**
 * `Text` is a basic class used for rendering text to the screen.
 * Any `Text` object added to the ECS system via `ECS::registry<Text>`
 * will be drawn automatically on top of other visual elements.
 */
struct Text {
    /**
     * Construct a Text object from a string, shared_ptr to a font, and a position.
     * Text objects that are placed in `ECS::registry<Text>` will automatically
     * be rendered to the screen.
     *
     * `content` must be an ASCII or UTF-8 encoded Unicode string.
     * ASCII-encoded `std::string`s can constructing using ordinary
     * string literals without any special characters, such as:
     *     auto s = std::string("abcABC123!@#");
     * UTF-8 encoded `std::string`s can be constructed using string
     * literals with the `u8` previx, such as:
     *     auto s = std::string(u8"abcABC123!@# αβγΑΒΓ①②③☝☻‽");
     * 
     * See `Font::load` for how to load and cache fonts. Note that
     * fonts typically provided a fairly limited set of glyphs for
     * specific use cases, such as scripts in European scripts, Asian
     * scripts, Indigenous scripts, mathematical symbols, Emoji, etc.
     * 
     * `position` defines left edge of the baseline of the first glyph,
     * and is relative to the bottom-left corner. For example, a `Text`
     * object at {0.0f, 0.0f} will have place most letters on top of the
     * bottom edge of the screen, but some character with overhanging
     * features (such a 'g', 'j', 'q', etc) may appear cut off.
     */
    Text(std::string content, std::shared_ptr<Font> font, glm::vec2 position, float scale = 1.0f, glm::vec3 colour = {0.0f, 0.0f, 0.0f}) noexcept;

    /**
     * Construct a Text object from a string, path to a TTF file, and a position.
     * 
     * Shorthand for `Text(content, Font::load(pathToTTF), position, scale, colour);`
     * 
     * See documentation for the constructor taking shared_ptr<Font>.
     */
    Text(std::string content, const std::string& pathToTTF, glm::vec2 position, float scale = 1.0f, glm::vec3 colour = {0.0f, 0.0f, 0.0f}) noexcept;

    // The contents of the Text object, as a ASCII- or UTF-8-encoded string
    std::string content;

    // A shared_ptr to the Text's font.
    // The font may be changed at any time, but must not be null.
    std::shared_ptr<Font> font;

    // The on-screen position of the left edge of the first glyph's baseline,
    // relative to the bottom left corner.
    glm::vec2 position;

    // The text's scale. Default value is 1.0f
    float scale;

    // The text's colour. Default value of {0.0f, 0.0f, 0.0f} (black)
    glm::vec3 colour;
};

// Forward declaration, only for internal use.
class FreeTypeContext;

/**
 * `Font` is used to load a TTF font from a file and is used to draw text.
 * 
 * Instances of `Font` are intended to be used via `shared_ptr` and created
 * using `Font::load()`. See `Font::load()` below.
 * 
 * Note that fonts typically provided a fairly limited set of glyphs for
 * specific use cases, such as scripts in European scripts, Asian
 * scripts, Indigenous scripts, mathematical symbols, Emoji, etc.
 */
class Font {
public:
    // A font must be constructed from a path to a valid TTF file.
    // An exception is thrown if any errors occur, such as the
    // file not existing.
    // NOTE: Font::load should be used for simplicity and to allow
    // fonts to be cached.
    Font(const std::string& pathToTTF);

    ~Font() noexcept;

    // Fonts cannot be copied. Use `shared_ptr` font to reuse the same
    // font between `Text` objects, and see `Font::load` below.
    Font(const Font&) = delete;

    // Load a `Font` instance from a path to a TTF file and return
    // a shared_ptr to that font.
    // The font will be cached, so that multiple calls with the same
    // font path will not necessarily re-load the TTF file.
    static std::shared_ptr<Font> load(const std::string& pathToTTF);

private:

    // Character informtion used for rendering a single glyph
    struct Character {
        // the OpenGL texture of the character
        GLResource<TEXTURE> Texture = 0;

        // The size of the texture, in pixels
        glm::ivec2 Size;

        // The baseline origin of the glyph within the texture
        glm::ivec2 Bearing;

        // The horizontal displacement at which to place the
        // next glyph
        unsigned int Advance = 0;
    };

    // Load and render a character from the font.
    // Characters are cached and loaded at most once
    // per font instance.
    const Character& getCharacter(std::uint32_t codePoint);

    // The FreeType font
    FT_Face m_face;

    // The shared FreeType library
    std::shared_ptr<FreeTypeContext> m_context;

    // The cache of loaded characters for rendering
    std::map<std::uint32_t, Character> m_characters;

    // Allow the `drawText` function to access the private
    // `getCharacter` function. See `drawText` below.
    friend void drawText(const Text&, glm::ivec2);
};

/**
 * Draw a Text object to the screen, given the screen buffer size.
 * NOTE: this function is called automatically by `RenderSystem::draw`
 * for all text objects in `ECS::registry<Text>` and this function is
 * not to be used otherwise.
 */
void drawText(const Text& text, glm::ivec2 frameBufferSize);
