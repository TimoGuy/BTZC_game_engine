#include "texture.h"

// @NOTE: This should be abstract but is actually a specialization of OpenGL code.
#include "glad/glad.h"
#include "btlogger.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <cassert>


uint32_t BT::Texture_bank::load_texture_2d_from_file(string const& fname, uint32_t channels)
{
    int32_t width;
    int32_t height;
    int32_t num_channels;
    stbi_set_flip_vertically_on_load_thread(true);
    uint8_t* data{ stbi_load(fname.c_str(), &width, &height, &num_channels, STBI_default) };
    if (!data)
    {
        logger::printef(logger::ERROR, "Image loading failed: \"%s\"", fname.c_str());
        assert(false);
        return -1;
    }
    assert(num_channels == channels);  // I think this should pass unless there's an error in the channel num?

    // Create RGB texture.
    assert(channels == 3);
    uint32_t texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    return texture_id;
}

void BT::Texture_bank::emplace_texture_2d(string const& name, uint32_t texture_id, bool replace)
{
    bool found{ false };
    for (auto& texture : s_textures_2d)
        if (texture.first == name)
        {
            if (replace)
            {
                // Replace texture with new texture id.
                texture.second = texture_id;
                found = true;
            }
            else
            {
                // Report texture already exists.
                logger::printef(logger::ERROR, "Texture \"%s\" already exists.", name.c_str());
                assert(false);
                return;
            }
            break;
        }

    if (!found)
    {
        // Emplace new texture.
        s_textures_2d.emplace_back(name, texture_id);
    }
}

uint32_t BT::Texture_bank::get_texture_2d(string const& name)
{
    uint32_t texture_id{ (uint32_t)-1 };

    for (auto& texture : s_textures_2d)
        if (texture.first == name)
        {
            texture_id = texture.second;
            break;
        }

    return texture_id;
}
