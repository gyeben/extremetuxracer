/* --------------------------------------------------------------------
EXTREME TUXRACER

Copyright (C) 1999-2001 Jasmin F. Patry (Tuxracer)
Copyright (C) 2010 Extreme Tuxracer Team

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
---------------------------------------------------------------------*/

#include "bh.h"
#include "textures.h"
#include "ogl.h"
#include "splash_screen.h"
#include "intro.h"
#include "racing.h"
#include "game_over.h"
#include "paused.h"
#include "reset.h"
#include "audio.h"
#include "game_type_select.h"
#include "race_select.h"
#include "event_select.h"
#include "credits.h"
#include "loading.h"
#include "course.h"
#include "event.h"
#include "font.h"
#include "translation.h"
#include "help.h"
#include "tools.h"
#include "tux.h"
#include "regist.h"
#include "keyframe.h"
#include "newplayer.h"
#include "score.h"
#include "ogl_test.h"
#include "bcm_host.h"
#include <assert.h>

TGameData g_game;

void InitGame (int argc, char **argv) {
	g_game.toolmode = NONE;
	g_game.argument = 0;
	if (argc == 4) {
		g_game.group_arg = argv[1];
		if (g_game.group_arg == "--char") g_game.argument = 4;
		g_game.dir_arg = argv[2];
		g_game.file_arg = argv[3];
	} else if (argc == 2) {
		g_game.group_arg = argv[1];
		if (g_game.group_arg == "9") g_game.argument = 9;
	}

	g_game.secs_since_start = 0;
	g_game.player_id = 0;
	g_game.start_player = 0;
	g_game.course_id = 0;
	g_game.mirror_id = 0;
	g_game.char_id = 0;
	g_game.location_id = 0;
	g_game.light_id = 0;
	g_game.snow_id = 0;
	g_game.cup_id = 0;
	g_game.race_id = 0;
	g_game.theme_id = 0;
	g_game.force_treemap = 0;
	g_game.treesize = 3;
	g_game.treevar = 3;
	g_game.loopdelay = 1;
}

typedef struct
{
   uint32_t screen_width;
   uint32_t screen_height;
// OpenGL|ES objects
   EGLDisplay display;
   EGLSurface surface;
   EGLContext context;
   GLuint tex[6];
// model rotation vector and direction
   GLfloat rot_angle_x_inc;
   GLfloat rot_angle_y_inc;
   GLfloat rot_angle_z_inc;
// current model rotation angles
   GLfloat rot_angle_x;
   GLfloat rot_angle_y;
   GLfloat rot_angle_z;
// current distance from camera
   GLfloat distance;
   GLfloat distance_inc;
// pointers to texture buffers
   char *tex_buf1;
   char *tex_buf2;
   char *tex_buf3;
} CUBE_STATE_T;

static void init_ogl(CUBE_STATE_T *state);
static volatile int terminate;
static CUBE_STATE_T _state, *state=&_state;


/***********************************************************
 * Name: init_ogl
 *
 * Arguments:
 *       CUBE_STATE_T *state - holds OGLES model info
 *
 * Description: Sets the display, OpenGL|ES context and screen stuff
 *
 * Returns: void
 *
 ***********************************************************/
static void init_ogl(CUBE_STATE_T *state)
{
   int32_t success = 0;
   EGLBoolean result;
   EGLint num_config;

   static EGL_DISPMANX_WINDOW_T nativewindow;

   DISPMANX_ELEMENT_HANDLE_T dispman_element;
   DISPMANX_DISPLAY_HANDLE_T dispman_display;
   DISPMANX_UPDATE_HANDLE_T dispman_update;
   VC_RECT_T dst_rect;
   VC_RECT_T src_rect;

   static const EGLint attribute_list[] =
   {
      EGL_RED_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_BLUE_SIZE, 8,
      EGL_ALPHA_SIZE, 8,
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
      EGL_NONE
   };
   
   EGLConfig config;

   // get an EGL display connection
   state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
   assert(state->display!=EGL_NO_DISPLAY);

   // initialize the EGL display connection
   result = eglInitialize(state->display, NULL, NULL);
   assert(EGL_FALSE != result);

   // get an appropriate EGL frame buffer configuration
   result = eglChooseConfig(state->display, attribute_list, &config, 1, &num_config);
   assert(EGL_FALSE != result);

   // create an EGL rendering context
   state->context = eglCreateContext(state->display, config, EGL_NO_CONTEXT, NULL);
   assert(state->context!=EGL_NO_CONTEXT);

   // create an EGL window surface
   success = graphics_get_display_size(0 /* LCD */, &state->screen_width, &state->screen_height);
   assert( success >= 0 );

   dst_rect.x = 0;
   dst_rect.y = 0;
   dst_rect.width = state->screen_width;
   dst_rect.height = state->screen_height;
      
   src_rect.x = 0;
   src_rect.y = 0;
   src_rect.width = state->screen_width << 16;
   src_rect.height = state->screen_height << 16;        

   dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
   dispman_update = vc_dispmanx_update_start( 0 );
         
   dispman_element = vc_dispmanx_element_add ( dispman_update, dispman_display,
      0/*layer*/, &dst_rect, 0/*src*/,
      &src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0/*clamp*/, (DISPMANX_TRANSFORM_T)0/*transform*/);
      
   nativewindow.element = dispman_element;
   nativewindow.width = state->screen_width;
   nativewindow.height = state->screen_height;
   vc_dispmanx_update_submit_sync( dispman_update );
      
   state->surface = eglCreateWindowSurface( state->display, config, &nativewindow, NULL );
   assert(state->surface != EGL_NO_SURFACE);

   // connect the context to the surface
   result = eglMakeCurrent(state->display, state->surface, state->surface, state->context);
   assert(EGL_FALSE != result);

   // Set background color and clear buffers
   glClearColor(0.15f, 0.25f, 0.35f, 1.0f);

   // Enable back face culling.
   glEnable(GL_CULL_FACE);

   glMatrixMode(GL_MODELVIEW);
}

// ====================================================================
// 					main
// ====================================================================

#if defined ( OS_WIN32_MINGW ) || defined ( OS_WIN32_NATIVE )
	#undef main
#endif

int main( int argc, char **argv ) {
	// ****************************************************************
	printf ("\n----------- Extreme Tux Racer " VERSION " ----------------");
    printf ("\n----------- (C) 2010 Extreme Tuxracer Team  --------\n\n ");

        bcm_host_init();
	srand (time (NULL));
	InitConfig (argv[0]);
	InitGame (argc, argv);
	Winsys.Init ();
	init_ogl(state);
    InitOpenglExtensions ();
	// for checking the joystick and the OpgenGL version (the info is
	// written on the console):
	//	Winsys.PrintJoystickInfo ();
	//	PrintGLInfo ();

	// register loop functions
    splash_screen_register();
	regist_register ();
    intro_register();
    racing_register();
    game_over_register();
    paused_register();
    reset_register();
    game_type_select_register();
	RegisterGameConfig ();
    event_select_register();
	event_register ();
    RaceSelectRegister();
    credits_register();
    loading_register();
	RegisterKeyInfo ();
	RegisterToolFuncs ();
	NewPlayerRegister ();
	RegisterScoreFunctions ();
	RegisterTestFuncs ();
	
	// theses resources must or should be loaded before splashscreen starts
 	Course.MakeStandardPolyhedrons ();
	Tex.LoadTextureList ();
	FT.LoadFontlist ();
	Winsys.SetFonttype ();
	Audio.Open ();
	Sound.LoadSoundList ();
	Music.LoadMusicList ();
	Music.SetVolume (param.music_volume);

	g_game.mode = NO_MODE;

	switch (g_game.argument) {
		case 0: Winsys.SetMode (SPLASH); break;
		case 4: 
			g_game.toolmode = TUXSHAPE; 
			Winsys.SetMode (TOOLS); 
			break;
		case 9: Winsys.SetMode (OGLTEST); break;
	}

 	Winsys.EventLoop ();
    return 0;
} 

