/*   
 * Copyright (C) 2003 Robert Kooima
 *
 * NEVERBALL is  free software; you can redistribute  it and/or modify
 * it under the  terms of the GNU General  Public License as published
 * by the Free  Software Foundation; either version 2  of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 */

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <string.h>
#include <math.h>

#include "glext.h"
#include "image.h"
#include "config.h"

/*---------------------------------------------------------------------------*/

void image_snap(char *filename)
{
    static char path[MAXSTR];

    int w = config_get(CONFIG_WIDTH);
    int h = config_get(CONFIG_HEIGHT);
    int i;

    SDL_Surface *buf = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 24,
                                            RMASK, GMASK, BMASK, 0);
    SDL_Surface *img = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 24,
                                            RMASK, GMASK, BMASK, 0);

    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, buf->pixels);

    for (i = 0; i < h; i++)
        memcpy((GLubyte *) img->pixels + 3 * w * i,
               (GLubyte *) buf->pixels + 3 * w * (h - i), 3 * w);

    if (config_home(path, filename, MAXSTR))
        SDL_SaveBMP(img,  path);

    SDL_FreeSurface(img);
    SDL_FreeSurface(buf);
}

void image_size(int *W, int *H, int w, int h)
{
    *W = 1;
    *H = 1;

    while (*W < w) *W *= 2;
    while (*H < h) *H *= 2;
}

/*---------------------------------------------------------------------------*/

static const GLenum format[5] = {
    0,
    GL_LUMINANCE,
    GL_LUMINANCE_ALPHA,
    GL_RGB,
    GL_RGBA
};

/*
 * Create on  OpenGL texture  object using the  given SDL  surface and
 * format,  scaled  using the  current  scale  factor.  When  scaling,
 * assume dimensions are used only for layout and lie about the size.
 */
GLuint make_image_from_surf(int *w, int *h, SDL_Surface *s)
{
    int t = config_get(CONFIG_TEXTURES);

    GLuint o = 0;

    glGenTextures(1, &o);
    glBindTexture(GL_TEXTURE_2D, o);

    if (t > 1)
    {
        int w = s->w / t;
        int h = s->h / t;

        /* Create a new buffer and copy the scaled image to it. */

        SDL_Surface *d = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h,
                                              s->format->BitsPerPixel,
                                              s->format->Rmask,
                                              s->format->Gmask,
                                              s->format->Bmask,
                                              s->format->Amask);
        if (d)
        {
            SDL_LockSurface(s);
            SDL_LockSurface(d);
            {
                if (s->format->BitsPerPixel == 32)
                    gluScaleImage(GL_RGBA,
                                  s->w, s->h, GL_UNSIGNED_BYTE, s->pixels,
                                  d->w, d->h, GL_UNSIGNED_BYTE, d->pixels);
                else
                    gluScaleImage(GL_RGB,
                                  s->w, s->h, GL_UNSIGNED_BYTE, s->pixels,
                                  d->w, d->h, GL_UNSIGNED_BYTE, d->pixels);
            }
            SDL_UnlockSurface(d);
            SDL_UnlockSurface(s);

            /* Load the scaled image. */

            if (s->format->BitsPerPixel == 32)
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, d->w, d->h, 0,
                             GL_RGBA, GL_UNSIGNED_BYTE, d->pixels);
            else
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,  d->w, d->h, 0,
                             GL_RGB,  GL_UNSIGNED_BYTE, d->pixels);

            SDL_FreeSurface(d);
        }
    }
    else
    {
        /* Load the unscaled image. */

        if (s->format->BitsPerPixel == 32)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, s->w, s->h, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, s->pixels);
        else
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,  s->w, s->h, 0,
                         GL_RGB,  GL_UNSIGNED_BYTE, s->pixels);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (w) *w = s->w;
    if (h) *h = s->h;

    return o;
}

/*---------------------------------------------------------------------------*/

/*
 * Load  an image  from the  named file.   If the  image is  not RGBA,
 * convert it to RGBA.  Return an OpenGL texture object.
 */
GLuint make_image_from_file(int *W, int *H,
                            int *w, int *h, const char *name)
{
    SDL_Surface *src;
    SDL_Surface *dst;
    SDL_Rect rect;

    GLuint o = 0;

    /* Load the file. */

    if ((src = IMG_Load(name)))
    {
        int w2;
        int h2;

        image_size(&w2, &h2, src->w, src->h);

        if (w) *w = src->w;
        if (h) *h = src->h;

        /* Create a new destination surface. */
        
        if ((dst = SDL_CreateRGBSurface(SDL_SWSURFACE, w2, h2, 32,
                                        RMASK, GMASK, BMASK, AMASK)))
        {
            /* Copy source pixels to the center of the destination. */

            rect.x = (Sint16) (w2 - src->w) / 2;
            rect.y = (Sint16) (h2 - src->h) / 2;

            SDL_SetAlpha(src, 0, 0);
            SDL_BlitSurface(src, NULL, dst, &rect);

            o = make_image_from_surf(W, H, dst);

            SDL_FreeSurface(dst);
        }
        SDL_FreeSurface(src);
    }
    return o;
}

/*---------------------------------------------------------------------------*/

/*
 * Render the given  string using the given font.   Transfer the image
 * to a  surface of  power-of-2 size large  enough to fit  the string.
 * Return an OpenGL texture object.
 */
GLuint make_image_from_font(int *W, int *H,
                            int *w, int *h, const char *text, TTF_Font *font)
{
    SDL_Color fg = { 0xFF, 0xFF, 0xFF, 0xFF };

    SDL_Surface *src;
    SDL_Surface *dst;
    SDL_Rect rect;

    GLuint o = 0;

    /* Render the text. */

    if (text && strlen(text) > 0)
    {
        if ((src = TTF_RenderText_Blended(font, text, fg)))
        {
            int w2;
            int h2;

            image_size(&w2, &h2, src->w, src->h);

            if (w) *w = src->w;
            if (h) *h = src->h;

            /* Create a new destination surface. */
            
            if ((dst = SDL_CreateRGBSurface(SDL_SWSURFACE, w2, h2, 32,
                                            RMASK, GMASK, BMASK, AMASK)))
            {
                /* Copy source pixels to the center of the destination. */

                rect.x = (Sint16) (w2 - src->w) / 2;
                rect.y = (Sint16) (h2 - src->h) / 2;

                SDL_SetAlpha(src, 0, 0);
                SDL_BlitSurface(src, NULL, dst, &rect);

                glPushAttrib(GL_PIXEL_MODE_BIT);
                {
                    /* Clamp RGB to white so glyphs are alpha-blended cleanly. */

                    glPixelTransferf(GL_RED_BIAS,   1.0f);
                    glPixelTransferf(GL_GREEN_BIAS, 1.0f);
                    glPixelTransferf(GL_BLUE_BIAS,  1.0f);

                    /* Create the image using the new surface. */

                    o = make_image_from_surf(W, H, dst);
                }
                glPopAttrib();

                SDL_FreeSurface(dst);
            }
            SDL_FreeSurface(src);
        }
    }
    else
    {
        if (W) *W = 0;
        if (H) *H = 0;
        if (w) *w = 0;
        if (h) *h = 0;
    }
    return o;
}

/*---------------------------------------------------------------------------*/
