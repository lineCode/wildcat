#ifndef _S2X_VIDEO_H
#define _S2X_VIDEO_H

#include "s2x_types.h"
#include <SDL.h>
#include <SDL_image.h>
#include <string>
#include <ostream>
#include <algorithm>

namespace s2x
{

class Window
{
public:
    Window(const std::string& title, int x, int y, int width, int height)
        : title_(title)
    {
        this->win = SDL_CreateWindow(title.c_str(), x, y, width, height, SDL_WINDOW_ALLOW_HIGHDPI);    
    }

    ~Window() { SDL_DestroyWindow(this->win); }
    Window(const Window&) = delete;
    Window(Window&&) = delete;

    Window& operator=(const Window&) = delete;
    Window& operator=(Window&&) = delete;

public:
    const std::string& title() const { return this->title_; }
    inline Point position() const;
    inline Size size() const;

    void setPosition(int x, int y) { SDL_SetWindowPosition(this->win, x, y); }
    void setSize(int width, int height) { SDL_SetWindowSize(this->win, width, height); }
    void setFullscreen(bool enable) { SDL_SetWindowFullscreen(this->win, enable ? SDL_WINDOW_FULLSCREEN : 0); }

    operator SDL_Window*() const { return this->win; }

private:
    SDL_Window* win = nullptr;
    std::string title_;
};

inline Point Window::position() const
{
    int x, y;
    SDL_GetWindowPosition(this->win, &x, &y);
    return { x, y };
}

inline Size Window::size() const
{
    int w, h;
    SDL_GetWindowSize(this->win, &w, &h);
    return { w, h };
}

class Pixel
{
public:
    explicit Pixel(SDL_PixelFormat* format) : format(format) {}
    Pixel(const Pixel&) = default;
    Pixel(Pixel&&) = default;
    Pixel& operator=(Pixel&&) = default;

public:
    Pixel& operator=(Uint32* ptr)
    {
        this->ptr = ptr;
        return *this;
    }

    operator Uint32*() const { return this->ptr; }

    void set(Uint8 r, Uint8 g, Uint8 b, Uint8 a)
    {
        Uint32 pxl = SDL_MapRGBA(this->format, r, g, b, a);
        *this->ptr = pxl;
    }

    void set(const Color& c)
    {
        set(c.r, c.g, c.b, c.a);
    }

    Color asRGBA() const
    {
        Uint32 pxl = *this->ptr;
        Uint8 r, g, b, a;
        SDL_GetRGBA(pxl, this->format, &r, &g, &b, &a);
        return { r, g, b, a };
    }

    Uint32& operator*()
    {
        return *ptr;
    }

private:
    Uint32* ptr;
    SDL_PixelFormat* format;
};

inline std::ostream& operator<<(std::ostream& os, const Pixel& p)
{
    os << *static_cast<Uint32*>(p);
    return os;
}

class Surface
{
public:
    Surface(int width, int height)
    {
        this->surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
    }

    Surface(SDL_Surface* srf) : surface(srf) { }

    inline Surface(const std::string& filename);
    inline Surface(const Surface&, const SDL_Rect&);
    ~Surface() { if(this->surface) SDL_FreeSurface(this->surface); }

    inline Surface(const Surface&);
    inline Surface(Surface&&);

    inline Surface& operator=(const Surface&);
    inline Surface& operator=(Surface&&);

public:
    inline Uint32* operator()(int x, int y);
    inline const Uint32* operator()(int x, int y) const;

    Size size() const { return { this->surface->w, this->surface->h }; }

    void fill(const Color& c) { SDL_FillRect(*this, nullptr, SDL_MapRGBA(this->surface->format, c.r, c.g, c.b, c.a)); }
    void fill(const Color& c, const Rect& r) { SDL_FillRect(*this, &r, SDL_MapRGBA(this->surface->format, c.r, c.g, c.b, c.a)); }
    inline void blit(const Rect& r, const Surface& srf);
    inline void blit(const Rect& r, const Surface& srf, float scaleX, float scaleY);

    inline void setClip(int x, int y, int width, int height);
    inline SDL_Rect getClip() const;
    void resetClip() { SDL_SetClipRect(this->surface, nullptr); }
    SDL_PixelFormat* pixelFormat() const { return this->surface->format; }

    operator SDL_Surface*() const { return this->surface; }

private:
    SDL_Surface* surface = nullptr;
};

inline Surface::Surface(const std::string& filename)
{
    std::string ufn = filename;
    std::transform(ufn.begin(), ufn.end(), ufn.begin(), ::tolower);
    const char* cstr = filename.c_str();

    this->surface = ufn.substr(ufn.size() - 3) == "bmp" ? SDL_LoadBMP(cstr) : IMG_Load(cstr);
}

inline Surface::Surface(const Surface& srf, const SDL_Rect& rect)
{
    this->surface = SDL_CreateRGBSurfaceWithFormat(0, rect.w, rect.h, 32, srf.pixelFormat()->format);
    SDL_BlitSurface(srf, &rect, this->surface, nullptr);
}

inline Surface::Surface(const Surface& srf)
{
    this->surface = SDL_ConvertSurface(srf.surface, srf.pixelFormat(), 0);
}

inline Surface::Surface(Surface&& srf)
{
    std::swap(this->surface, srf.surface);
}

inline Surface& Surface::operator=(const Surface& srf)
{
    SDL_Surface* s = srf.surface;
    auto srfSize = srf.size();
    auto selfSize = size();
    SDL_Rect srfRect = {0, 0, srfSize.width, srfSize.height};
    SDL_Rect selfRect = {0, 0, selfSize.width, selfSize.height};
    SDL_BlitSurface(s, static_cast<const SDL_Rect*>(&srfRect), this->surface, &selfRect);
    return *this;
}

inline Surface& Surface::operator=(Surface&& srf)
{
    std::swap(this->surface, srf.surface);

    if(srf.surface)
        SDL_FreeSurface(srf.surface);

    return *this;
}

inline Uint32* Surface::operator()(int x, int y)
{
    Uint8* target = reinterpret_cast<Uint8*>(this->surface->pixels) + y * this->surface->pitch + x * sizeof(Uint32);
    return reinterpret_cast<Uint32*>(target);
}

inline const Uint32* Surface::operator()(int x, int y) const
{
    return const_cast<Surface*>(this)->operator()(x, y);
}

inline void Surface::setClip(int x, int y, int width, int height)
{
    SDL_Rect r = { x, y, width, height };
    SDL_SetClipRect(this->surface, &r);
}

inline SDL_Rect Surface::getClip() const
{
    SDL_Rect r;
    SDL_GetClipRect(this->surface, &r);
    return r;
}

inline void Surface::blit(const Rect& r, const Surface& srf)
{
    SDL_BlitSurface(srf, nullptr, this->surface, const_cast<SDL_Rect*>(&r));
}

inline void Surface::blit(const Rect& r, const Surface& srf, float scaleX, float scaleY)
{
    SDL_Rect stretchRect = {
        (int)(r.w * scaleX),
        (int)(r.h * scaleY)
    };

    SDL_BlitSurface(srf, nullptr, this->surface, &stretchRect);
}

class Renderer
{
public:
    Renderer(const Window& window, int flags) : window(window)
    { 
        this->resource = SDL_CreateRenderer(window, -1, flags); 
        SDL_RenderSetLogicalSize(this->resource, window.size().width, window.size().height); 
    }

    Renderer(const Renderer&) = delete;
    Renderer(Renderer&& ren) : window(ren.window) { std::swap(this->resource, ren.resource); }
    ~Renderer() { if(this->resource) SDL_DestroyRenderer(this->resource); }

    Renderer& operator=(const Renderer&) = delete;
    inline Renderer& operator=(Renderer&&);

public:
    inline Size size() const;
    void clear() { SDL_RenderClear(*this); }
    void present() { SDL_RenderPresent(*this); }
    void copy(SDL_Texture* tex) { SDL_RenderCopy(*this, tex, nullptr, nullptr); }
    
    void copy(SDL_Texture* tex, const SDL_Rect& texSlice, const SDL_Rect& drawPos) 
    { SDL_RenderCopy(*this, tex, &texSlice, &drawPos); }

    void copy(SDL_Texture* tex, int width, int height)
    {
        SDL_Rect r = { 0, 0, width, height };
        SDL_RenderCopy(*this, tex, nullptr, &r);
    }

    void copy(SDL_Texture* tex, 
        const SDL_Rect& texSlice, 
        const SDL_Rect& drawPos, 
        const double angle, 
        const SDL_Point& rotationAnchor,
        const SDL_RendererFlip flip = SDL_FLIP_NONE
    )
    {
        SDL_RenderCopyEx(*this, tex, &texSlice, &drawPos, angle, &rotationAnchor, flip);
    }

    void copy(SDL_Texture* tex, 
        const SDL_Rect& drawPos, 
        const double angle, 
        const SDL_Point& rotationAnchor,
        const SDL_RendererFlip flip = SDL_FLIP_NONE
    )
    {
        SDL_RenderCopyEx(*this, tex, NULL, &drawPos, angle, &rotationAnchor, flip);
    }

    void copy(SDL_Texture* tex,
        const SDL_Rect& drawPos,
        const double angle,
        const SDL_Point& rotationAnchor,
        float scaleX,
        float scaleY,
        const SDL_Rect& texRect = {},
        SDL_RendererFlip flip = SDL_FLIP_NONE
    )
    {
        SDL_RenderSetScale(*this, scaleX * devicePixelRatio(), scaleY * devicePixelRatio());
        SDL_RenderCopyEx(*this, tex, texRect.w > 0 && texRect.h > 0 ? &texRect : NULL, &drawPos, angle, &rotationAnchor, flip);
        SDL_RenderSetScale(*this, devicePixelRatio(), devicePixelRatio());
    }

    operator SDL_Renderer*() const { return this->resource; }

    const Window& getWindow() const { return this->window; }
    float devicePixelRatio() const { return size().width / getWindow().size().width; }

private:
    SDL_Renderer* resource = nullptr;
    const Window& window;
};

inline Size Renderer::size() const
{
    int w, h;
    SDL_GetRendererOutputSize(this->resource, &w, &h);
    return { w, h };
}

inline Renderer& Renderer::operator=(Renderer&& ren)
{
    std::swap(this->resource, ren.resource);
    return *this;
}

class Texture
{
public:
    inline Texture(const Renderer&, const Surface&);
    Texture(const Texture&) = delete;
    inline Texture(Texture&&);
    ~Texture() { if(this->resource) SDL_DestroyTexture(this->resource); }

    Texture& operator=(const Texture&) = delete;
    inline Texture& operator=(Texture&&);

    inline Texture& operator=(const Surface& surface);

public:
    operator SDL_Texture*() const { return this->resource; }

    void setOpacity(Uint8 alpha) { SDL_SetTextureAlphaMod(*this, alpha); }
    inline Uint8 getOpacity() const;
    void setColor(SDL_Color c) { SDL_SetTextureColorMod(*this, c.r, c.g, c.b); }
    inline SDL_Color getColor() const;
    void setBlendMode(SDL_BlendMode bm) { SDL_SetTextureBlendMode(*this, bm); }
    inline SDL_BlendMode getBlendMode() const;

private:
    SDL_Texture* resource = nullptr;
    int height;
    const Renderer& renderer;
};

inline Texture::Texture(const Renderer& renderer, const Surface& surface) : renderer(renderer)
{
    auto size = surface.size();
    this->resource = SDL_CreateTexture(renderer, surface.pixelFormat()->format, SDL_TEXTUREACCESS_STREAMING, size.width, size.height);
    SDL_SetTextureBlendMode(this->resource, SDL_BLENDMODE_BLEND);

    void* pixels;
    int pitch;
    SDL_LockTexture(this->resource, nullptr, &pixels, &pitch);
    memcpy(pixels, static_cast<SDL_Surface*>(surface)->pixels, pitch * size.height);
    SDL_UnlockTexture(this->resource);
    this->height = size.height;
}

inline Texture::Texture(Texture&& tex) : renderer(tex.renderer)
{
    std::swap(this->resource, tex.resource);
    SDL_SetTextureBlendMode(this->resource, SDL_BLENDMODE_BLEND);
}

inline Texture& Texture::operator=(Texture&& tex)
{
    std::swap(this->resource, tex.resource);
    SDL_SetTextureBlendMode(this->resource, SDL_BLENDMODE_BLEND);
    return *this;
}

inline Uint8 Texture::getOpacity() const
{
    Uint8 alpha;
    SDL_GetTextureAlphaMod(*this, &alpha);
    return alpha;
}

inline SDL_Color Texture::getColor() const
{
    SDL_Color c;
    SDL_GetTextureColorMod(*this, &c.r, &c.g, &c.b);
    return c;
}

inline SDL_BlendMode Texture::getBlendMode() const
{
    SDL_BlendMode bm;
    SDL_GetTextureBlendMode(*this, &bm);
    return bm;
}

inline Texture& Texture::operator=(const Surface& surface)
{
    void* pixels;
    int pitch;
    SDL_LockTexture(this->resource, nullptr, &pixels, &pitch);
    memcpy(pixels, static_cast<SDL_Surface*>(surface)->pixels, pitch * this->height);
    SDL_UnlockTexture(this->resource);

    return *this;
}

}

#endif