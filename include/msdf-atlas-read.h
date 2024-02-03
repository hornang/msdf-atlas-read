#pragma once

#include <cinttypes>
#include <fstream>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace msdf_atlas_read {

using unicode_t = uint32_t;

using namespace nlohmann;

struct Bounds
{
    float getLeft() const
    {
        return left;
    }

    float getRight() const
    {
        return right;
    }

    float getTop() const
    {
        return top;
    }

    float getBottom() const
    {
        return bottom;
    }

    float getHeight() const
    {
        return bottom - top;
    }

    float getWidth() const
    {
        return right - left;
    }

    Bounds getNormalized(float width, float height) const
    {
        return {
            left / width,
            right / width,
            top / height,
            bottom / height,
        };
    }

    float left = 0;
    float right = 0;
    float top = 0;
    float bottom = 0;
};

struct GlyphMapping
{
    void scale(float scale)
    {
        targetBounds.left *= scale;
        targetBounds.right *= scale;
        targetBounds.top *= scale;
        targetBounds.bottom *= scale;
    }

    void moveRight(float right)
    {
        targetBounds.left += right;
        targetBounds.right += right;
    }

    Bounds targetBounds;
    Bounds atlasBounds;
};

struct Glyph
{
    bool hasMapping() const
    {
        return mapping.has_value();
    }

    const GlyphMapping &getMapping() const
    {
        return mapping.value();
    }

    float advance = 0;
    std::optional<GlyphMapping> mapping;
};

using KerningMap = std::unordered_map<unicode_t, std::unordered_map<unicode_t, float>>;

struct FontGeometry
{
    const Glyph *getGlyph(unicode_t codepoint) const
    {
        if (!glyphs.contains(codepoint)) {
            return nullptr;
        }
        return &glyphs.at(codepoint);
    }
    std::string name;
    std::unordered_map<unicode_t, Glyph> glyphs;
    KerningMap kerning;
};

struct AtlasProperties
{
    int width = 0;
    int height = 0;
};

struct AtlasLayout
{
    AtlasProperties properties;
    std::vector<FontGeometry> fontGeometries;
};

inline Bounds parseBounds(const nlohmann::json &bounds)
{
    return { bounds.at("left"),
             bounds.at("right"),
             bounds.at("top"),
             bounds.at("bottom") };
}

inline KerningMap parseKerning(const nlohmann::json &kerningJson)
{
    KerningMap kerning;

    for (const json::object_t &kerningPair : kerningJson) {
        unicode_t previous = kerningPair.at("unicode1");
        unicode_t next = kerningPair.at("unicode2");
        kerning[previous][next] = kerningPair.at("advance");
    }

    return kerning;
}

inline AtlasProperties loadProperties(const nlohmann::json &json)
{
    return { json.at("width"), json.at("height") };
}

inline FontGeometry parseFontGeometry(const nlohmann::json &jsonFontGeometry)
{
    FontGeometry fontGeometry;

    fontGeometry.name = jsonFontGeometry["name"];

    for (const json::object_t &glyphJson : jsonFontGeometry.at("glyphs")) {
        Glyph glyph;

        glyph.advance = glyphJson.at("advance");

        static const char *planeBoundsKey = "planeBounds";
        static const char *atlasBoundsKey = "atlasBounds";

        if (glyphJson.find(planeBoundsKey) != glyphJson.end() && glyphJson.find(atlasBoundsKey) != glyphJson.end()) {
            glyph.mapping = { parseBounds(glyphJson.at(planeBoundsKey)),
                              parseBounds(glyphJson.at(atlasBoundsKey)) };
        }

        fontGeometry.glyphs.emplace(glyphJson.at("unicode"), glyph);
    }

    static const char *kerningKey = "kerning";

    if (jsonFontGeometry.contains(kerningKey)) {
        fontGeometry.kerning = parseKerning(jsonFontGeometry.at(kerningKey));
    }

    return fontGeometry;
}

inline AtlasLayout loadJson(const std::string &fileName)
{
    AtlasLayout atlasLayout;

    std::ifstream file(fileName);
    json data = nlohmann::json::parse(file);

    atlasLayout.properties = loadProperties(data.at("atlas"));

    for (json::object_t jsonFontGeometry : data.at("variants")) {

        atlasLayout.fontGeometries.push_back(parseFontGeometry(jsonFontGeometry));
    }

    return atlasLayout;
}
}
