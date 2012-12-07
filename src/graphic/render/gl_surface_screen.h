/*
 * Copyright 2010 by the Widelands Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef GL_SURFACE_SCREEN_H
#define GL_SURFACE_SCREEN_H

#include <boost/scoped_array.hpp>

#include "graphic/pixelaccess.h"
#include "graphic/screen.h"

/**
 * This surface represents the screen in OpenGL mode.
 */
class GLSurfaceScreen : virtual public Screen, virtual public IPixelAccess {
public:
	GLSurfaceScreen(uint32_t w, uint32_t h);

	/// Interface implementations
	//@{
	virtual uint32_t get_w() const;
	virtual uint32_t get_h() const;

	virtual void update();

	virtual const SDL_PixelFormat & format() const;
	virtual void lock(LockMode);
	virtual void unlock(UnlockMode);
	virtual uint16_t get_pitch() const;
	virtual uint8_t * get_pixels() const;
	virtual void set_pixel(uint32_t x, uint32_t y, Uint32 clr);
	virtual uint32_t get_pixel(uint32_t x, uint32_t y);
	virtual IPixelAccess & pixelaccess() {return *this;}

	virtual void clear();
	virtual void draw_rect(const Rect&, RGBColor);
	virtual void fill_rect(const Rect&, RGBAColor);
	virtual void brighten_rect(const Rect&, int32_t factor);

	virtual void draw_line (int32_t x1, int32_t y1, int32_t x2, int32_t y2,
			const RGBColor&, uint8_t width);

	virtual void blit(const Point&, const IPicture*, const Rect& srcrc, Composite cm);
	//@}

private:
	void swap_rows();

	/// Size of the screen
	uint32_t m_w, m_h;

	/// Pixel data while locked
	boost::scoped_array<uint8_t> m_pixels;
};

#endif // GL_SURFACE_SCREEN_H
