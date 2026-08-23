#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "platform.h"

namespace platform
{
    bool Image::load(const char* path, bool flipVertical)
    {
        if (m_handle)
        {
            free();
        }

        int channelCount{ 0 };

        stbi_set_flip_vertically_on_load(flipVertical);

        m_handle = stbi_load(path, &m_width, &m_height, &channelCount, 0);

        if (!m_handle || m_width <= 0 || m_height <= 0 || channelCount <= 0)
        {
            return false;
        }

        switch (channelCount)
        {
        case 3:
            m_format = Format::RGB;
            break;
        case 4:
            m_format = Format::RGBA;
            break;
        default:
            return false;
        }

        return true;
    }

    void Image::free()
    {
        stbi_image_free(m_handle);

        m_handle = nullptr;
    }

    bool Image::isLoaded() const
    {
        return ( m_handle != nullptr );
    }

    Image::Format Image::getFormat() const
    {
        return m_format;
    }

    int Image::getWidth() const
    {
        return m_width;
    }

    int Image::getHeight() const
    {
        return m_height;
    }

    void* Image::getData() const
    {
        return m_handle;
    }

    Image::~Image()
    {
        free();
    }

    std::unique_ptr<Image> loadImage(const char* path, bool flipVertical)
    {
        auto image = std::make_unique<Image>();

        image->load(path, flipVertical);

        return image;
    }
}