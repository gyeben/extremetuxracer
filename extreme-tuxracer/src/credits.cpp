/* 
 * PPRacer 
 * Copyright (C) 2004-2005 Volker Stroebel <volker@planetpenguin.de>
 *
 * Copyright (C) 1999-2001 Jasmin F. Patry
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */


#include "credits.h"

#include "ppgltk/audio/audio.h"
#include "ppgltk/font.h"


#define CREDITS_MAX_Y -140
#define CREDITS_MIN_Y 64

typedef struct {
    pp::Font *font;
	char *binding;
    char *text;
} credit_line_t;

static credit_line_t credit_lines[] = 
{
	{ NULL, "credits_h1", "Extreme TuxRacer" },
    { NULL, "credits_text", "http://www.extremetuxracer.com" },
	{ NULL, "credits_text", "Extreme TuxRacer is based on PlanetPenguin Racer" },
    { NULL, "credits_text", "http://racer.planetpenguin.de" },
	{ NULL, "credits_text", "PlanetPenguin Racer is based on Tux Racer" },
    { NULL, "credits_text", "Tux Racer is a Sunspire Studios Production" },
    { NULL, "credits_text", "www.sunspirestudios.com" },
    { NULL, "credits_text", "" },
    { NULL, "credits_h2", "PPRacer Development Team" },
    { NULL, "credits_text", "Volker Ströbel" },
    { NULL, "credits_text", "" },
	{ NULL, "credits_h2", "Translators" },
	{ NULL, "credits_text", "Alexander Aksenov" },	
	{ NULL, "credits_text", "Eric Alvarez Llimos" },	
	{ NULL, "credits_text", "Helder Correia" },	
	{ NULL, "credits_text", "Karl Ove Hufthammer" },	
	{ NULL, "credits_text", "Marco Antonio Blanco" },	
	{ NULL, "credits_text", "Massimo Spazian" },	
	{ NULL, "credits_text", "Mikel Olasagasti" },	
	{ NULL, "credits_text", "Spiral" },	
	{ NULL, "credits_text", "Theo Snelleman" },	
	{ NULL, "credits_text", "Trygve B. Wiig" },
    { NULL, "credits_text", "" },
	{ NULL, "credits_h2", "Contributors" },
	{ NULL, "credits_text", "Peter Reichel" },
	{ NULL, "credits_text", "Teemu Vesala" },
	{ NULL, "credits_text", "Theo Snelleman" },
    { NULL, "credits_text", "" },
    { NULL, "credits_h2", "Tux Racer Development Team" },
    { NULL, "credits_text_small", "(Alphabetical Order)" },
    { NULL, "credits_text", "Patrick \"Pog\" Gilhuly" },
    { NULL, "credits_text", "Eric \"Monster\" Hall" },
    { NULL, "credits_text", "Rick Knowles" },
    { NULL, "credits_text", "Vincent Ma" },
    { NULL, "credits_text", "Jasmin Patry" },
    { NULL, "credits_text", "Mark Riddell" },
    { NULL, "credits_text", "" },
    { NULL, "credits_h2", "Music" },
    { NULL, "credits_text", "Joseph Toscano" },
    { NULL, "credits_text", "" },
    { NULL, "credits_h2", "Thanks To:" },
    { NULL, "credits_text_small", "(In No Particular Order)" },
    { NULL, "credits_text", "Larry Ewing" },
    { NULL, "credits_text", "Thatcher Ulrich" },
    { NULL, "credits_text", "Steve Baker" },
    { NULL, "credits_text", "Ingo Ruhnke" },
    { NULL, "credits_text", "James Barnard" },
    { NULL, "credits_text", "Alan Levy" },
    { NULL, "credits_text", "University of Waterloo Computer Graphics Lab" },
    { NULL, "credits_text", "" },
  { NULL, "credits_text_small", "Tux Racer is a trademark of Jasmin F. Patry" },
	{ NULL, "credits_text_small", "Extreme TuxRacer is copyright 2007 The PPRacer team" },
	{ NULL, "credits_text_small", "PlanetPenguin Racer is copyright 2004, 2005 The PPRacer team" },
  { NULL, "credits_text_small", "Tux Racer is copyright 1999, 2000, 2001 Jasmin F. Patry" },
};

Credits::Credits()
 : m_yOffset(0.0)
{
	for (unsigned int i=0; i<sizeof( credit_lines ) / sizeof( credit_lines[0] ); i++) {
		credit_lines[i].font = pp::Font::get(credit_lines[i].binding);		
	}
	
  play_music( "credits_screen" );
}


Credits::~Credits()
{	
}

void
Credits::loop(float timeStep)
{
	int width, height;

  width = getparam_x_resolution();
  height = getparam_y_resolution();

  update_audio();

  clear_rendering_context();

  set_gl_options( GUI );

  UIMgr.setupDisplay();

  drawText( timeStep );

	drawSnow(timeStep);

	theme.drawMenuDecorations();

  UIMgr.draw();

  reshape( width, height );

  winsys_swap_buffers();
}


void
Credits::drawText( float timeStep )
{
    int w = getparam_x_resolution();
    int h = getparam_y_resolution();
    float y;

    m_yOffset += timeStep * 30;
    y = CREDITS_MIN_Y + m_yOffset;

	//loop through all credit lines
	for (unsigned int i=0; i<sizeof( credit_lines ) / sizeof( credit_lines[0] ); i++) {
	    credit_line_t line = credit_lines[i];

		//get the font and sizes for the binding
		//pp::Font *font = pp::Font::get(line.binding);
		float width = line.font->advance(line.text);
		float desc = line.font->descender();
		float asc = line.font->ascender();
		
		//draw the line on the screen
		line.font->draw(line.text, w/2 - width/2, y);

		//calculate the y value for the next line
		y-=asc-desc;
	}

	//if the last string reaches the top, reset the y offset
    if ( y > h+CREDITS_MAX_Y ) {
		m_yOffset = 0;
    }

    // Draw strips at the top and bottom to clip out text 
    glDisable( GL_TEXTURE_2D );

    glColor4dv( (double*)&theme.background );

    glRectf( 0, 0, w, CREDITS_MIN_Y );

    glBegin( GL_QUADS );
    {
	glVertex2f( 0, CREDITS_MIN_Y );
	glVertex2f( w, CREDITS_MIN_Y );
	glColor4f( theme.background.r, 
		   theme.background.g,
		   theme.background.b,
		   0 );
	glVertex2f( w, CREDITS_MIN_Y + 30 );
	glVertex2f( 0, CREDITS_MIN_Y + 30 );
    }
    glEnd();

    glColor4dv( (double*)&theme.background );

    glRectf( 0, h+CREDITS_MAX_Y, w, h );

    glBegin( GL_QUADS );
    {
	glVertex2f( w, h+CREDITS_MAX_Y );
	glVertex2f( 0, h+CREDITS_MAX_Y );
	glColor4f( theme.background.r, 
		   theme.background.g,
		   theme.background.b,
		   0 );
	glVertex2f( 0, h+CREDITS_MAX_Y - 30 );
	glVertex2f( w, h+CREDITS_MAX_Y - 30 );
    }
    glEnd();

    glColor4f( 1, 1, 1, 1 );

    glEnable( GL_TEXTURE_2D );
}

bool
Credits::mouseButtonReleaseEvent(int button, int x, int y)
{
	set_game_mode( GAME_TYPE_SELECT );
    winsys_post_redisplay();
	return true;
}

bool
Credits::keyReleaseEvent(SDLKey key)
{
	set_game_mode( GAME_TYPE_SELECT );
    winsys_post_redisplay();
	return true;
}
