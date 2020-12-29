#include "TextureManager.h"

#include <stdexcept>
#include <filesystem>

#include "../reader/frm/FrmReader.h"
#include "../reader/pal/PalReader.h"

#include "../format/frm/Frm.h"
#include "../format/frm/Frame.h"
#include "../format/frm/Direction.h"
#include "../format/pal/Pal.h"

namespace geck {

bool TextureManager::exists(const std::string& filename)
{
    return _resources.find(filename) != _resources.end();
}

void TextureManager::insert(const std::string &filename)
{
    if (exists(_dataPath + filename)) {
        return;
    }

    // lowercase file extension
    std::string extension = [&]() -> std::string {
        std::string s = std::filesystem::path(filename).extension();
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c){ return std::tolower(c); }
        );
        return s;
    }();

    auto texture = std::make_unique<sf::Texture>();
    if (extension.rfind(".frm", 0) == 0) { // frm, frm0, frmX..
        texture = loadTextureFRM(_dataPath + filename);
    } else {
        if (!texture->loadFromFile(_dataPath + filename)) { // default to SFML's implementation
            throw std::runtime_error{"Failed to load texture " + _dataPath + filename + ", extension: " + extension};
        }
    }

    _resources.insert(std::make_pair(_dataPath + filename, std::move(texture)));
}

const sf::Texture &TextureManager::get(const std::string& filename) const
{
    auto found = _resources.find(_dataPath + filename);

    if (found == _resources.end()) {
        throw std::runtime_error{"Texture " + _dataPath + filename + " does not exist"};
    }

    return *found->second;
}

void TextureManager::setDataPath(const std::string &path)
{
    _dataPath = path;
}

std::unique_ptr<sf::Texture> TextureManager::loadTextureFRM(const std::string& filename)
{
    FrmReader frm_reader;
    auto frm = frm_reader.read(filename);

    // FIXME: using just the first frame for now
    geck::Frame frame = frm->directions().front().frames().front();

    PalReader pal_reader;
    auto pal = pal_reader.read(_dataPath + "color.pal"); // TODO: custom .pal

    const auto& colors = pal->palette();
    const auto& colorIndexes = frame.data();

    size_t pixelCount = frame.width() * frame.height() * 4U; // 4 = RGBA of SFML texture
    std::vector<sf::Uint8> pixels(pixelCount);

    for (size_t i = 0; i < pixelCount; i += 4) {

        uint8_t paletteIndex = colorIndexes[i / 4];

        geck::Rgb color = colors[paletteIndex];

        if (color.r == 255 && color.g == 255 && color.b == 255) {
            // transparent
            pixels[i]   = 0;
            pixels[i+1] = 0;
            pixels[i+2] = 0;
            pixels[i+3] = 0;
        } else {
            constexpr int brightness = 4; // brightness modifier
            pixels[i]   = color.r * brightness;
            pixels[i+1] = color.g * brightness;
            pixels[i+2] = color.b * brightness;
            pixels[i+3] = 255;
        }
    }

    auto texture = std::make_unique<sf::Texture>();
    texture->create(frame.width(), frame.height());
    texture->update(&pixels[0]);

    return texture;
}

}