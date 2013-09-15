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

#include "winsys.h"
#include "ogl.h"
#include "audio.h"
#include "game_ctrl.h"
#include "font.h"
#include "score.h"
#include "textures.h"
#include "bcm_host.h"
#include <assert.h>

#define USE_JOYSTICK true

#define COLOURDEPTH_RED_SIZE   5
#define COLOURDEPTH_GREEN_SIZE 6
#define COLOURDEPTH_BLUE_SIZE  5
#define COLOURDEPTH_DEPTH_SIZE 16

#if defined(HAVE_GL_GLES1)
EGLDisplay g_eglDisplay = 0;
EGLConfig  g_eglConfig  = 0;
EGLContext g_eglContext = 0;
EGLSurface g_eglSurface = 0;
Display*   g_x11Display = NULL;

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

static const EGLint g_configAttribs[] = {
  EGL_RED_SIZE,              COLOURDEPTH_RED_SIZE,
  EGL_GREEN_SIZE,            COLOURDEPTH_GREEN_SIZE,
  EGL_BLUE_SIZE,             COLOURDEPTH_BLUE_SIZE,
  EGL_DEPTH_SIZE,            COLOURDEPTH_DEPTH_SIZE,
  EGL_SURFACE_TYPE,          EGL_WINDOW_BIT,
  EGL_RENDERABLE_TYPE,       EGL_OPENGL_ES_BIT,
  EGL_BIND_TO_TEXTURE_RGBA,  EGL_TRUE,
  EGL_NONE
};
#endif

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
   eglBindAPI(EGL_OPENGL_ES_API);
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

CWinsys Winsys;

CWinsys::CWinsys () {
	screen = NULL;
	for (int i=0; i<NUM_GAME_MODES; i++) {
		modefuncs[i].init   = NULL;
		modefuncs[i].loop   = NULL;
		modefuncs[i].term   = NULL;
		modefuncs[i].keyb   = NULL;
		modefuncs[i].mouse  = NULL;
		modefuncs[i].motion = NULL;
		modefuncs[i].jaxis  = NULL;
		modefuncs[i].jbutt  = NULL;
	}
	new_mode = NO_MODE;
	lasttick = 0;

	joystick = NULL;
	numJoysticks = 0;
	joystick_active = false;

 	resolution[0] = MakeRes (0, 0);
	resolution[1] = MakeRes (800, 600);
	resolution[2] = MakeRes (1024, 768);
	resolution[3] = MakeRes (1152, 864);
	resolution[4] = MakeRes (1280, 960);
	resolution[5] = MakeRes (1280, 1024);
	resolution[6] = MakeRes (1360, 768);
	resolution[7] = MakeRes (1400, 1050);
	resolution[8] = MakeRes (1440, 900);
	resolution[9] = MakeRes (1680, 1050);
	
	auto_x_resolution = 800;
	auto_y_resolution = 600;

	clock_time = 0;
	cur_time = 0;
	lasttick = 0;
	elapsed_time = 0;
	remain_ticks = 0;
}

CWinsys::~CWinsys () {}

TScreenRes CWinsys::MakeRes (int width, int height) {
	TScreenRes res;
	res.width = width;
	res.height = height;
	return res;
}

TScreenRes CWinsys::GetResolution (int idx) {
	if (idx < 0 || idx >= NUM_RESOLUTIONS) return MakeRes (800, 600);
	return resolution[idx];
}

string CWinsys::GetResName (int idx) {
	string line;
	if (idx < 0 || idx >= NUM_RESOLUTIONS) return "800 x 600";
	if (idx == 0) return ("auto");
	line = Int_StrN (resolution[idx].width);
	line += " x " + Int_StrN (resolution[idx].height);
	return line;
}

double CWinsys::CalcScreenScale () {
	double hh = (double)param.y_resolution;
	if (hh < 768) return 0.78; 
	else if (hh == 768) return 1.0;
	else return (hh / 768);
}

/*
typedef struct SDL_Surface {
    Uint32 flags;                           // Read-only 
    SDL_PixelFormat *format;                // Read-only 
    int w, h;                               // Read-only 
    Uint16 pitch;                           // Read-only 
    void *pixels;                           // Read-write 
    SDL_Rect clip_rect;                     // Read-only 
    int refcount;                           // Read-mostly
} SDL_Surface;
*/

void CWinsys::SetupVideoMode (TScreenRes resolution) {
    int bpp = 0;
    switch (param.bpp_mode) {
		case 0:	bpp = 0; break;
		case 1:	bpp = 16; break;
		case 2:	bpp = 32; break;
		default: param.bpp_mode = 0; bpp = 0;
    }

    Uint32 video_flags = ( param.fullscreen ? SDL_FULLSCREEN : 0 );
#if !defined(HAVE_GL_GLES1)
    video_flags |= SDL_OPENGL;
#endif

	if ((screen = SDL_SetVideoMode 
	(resolution.width, resolution.height, bpp, video_flags)) == NULL) {
		Message ("couldn't initialize video",  SDL_GetError()); 
		Message ("set to 800 x 600");
		screen = SDL_SetVideoMode (800, 600, bpp, video_flags);
		param.res_type = 1;
		SaveConfigFile ();
	}
	SDL_Surface *surf = SDL_GetVideoSurface ();
	param.x_resolution = surf->w;
	param.y_resolution = surf->h;
	if (resolution.width == 0 && resolution.height == 0) {
		auto_x_resolution = param.x_resolution;
		auto_y_resolution = param.y_resolution;
	}
 	param.scale = CalcScreenScale ();
	if (param.use_quad_scale) param.scale = sqrt (param.scale);
}

void CWinsys::SetupVideoMode (int idx) {
	if (idx < 0 || idx >= NUM_RESOLUTIONS) SetupVideoMode (MakeRes (800, 600));
	else SetupVideoMode (resolution[idx]);
}

void CWinsys::SetupVideoMode (int width, int height) {
	SetupVideoMode (MakeRes (width, height));
}

void CWinsys::InitJoystick () {
    if (SDL_InitSubSystem (SDL_INIT_JOYSTICK) < 0) {
	Message ("Could not initialize SDL_joystick: %s", SDL_GetError());
	return;
    }

    numJoysticks = SDL_NumJoysticks ();
    if (numJoysticks < 1) {
	joystick = NULL;
	return;		
    }	

    SDL_JoystickEventState (SDL_ENABLE);
    joystick = SDL_JoystickOpen (0);	// first stick with number 0
    if (joystick == NULL) {
	Message ("Cannot open joystick %s", SDL_GetError ());
	return;
    }

    joystick_active = true;
}

void CWinsys::Init () {
    Uint32 sdl_flags = SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER;
    if (SDL_Init (sdl_flags) < 0) Message ("Could not initialize SDL");

#if !defined(HAVE_GL_GLES1)
    SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1);

    #if defined (USE_STENCIL_BUFFER)
	SDL_GL_SetAttribute (SDL_GL_STENCIL_SIZE, 8);
    #endif
#endif
	
    SetupVideoMode (GetResolution (param.res_type));

#if defined(HAVE_GL_GLES1)
    // use EGL to initialise GLES
    bcm_host_init();
    g_x11Display = XOpenDisplay(NULL);
    if (!g_x11Display)
    {
        fprintf(stderr, "ERROR: unable to get display!n");
        exit(-1);
    }

    init_ogl(state);

    // Get the e window handle
    SDL_SysWMinfo sysInfo; //Will hold our Window information
    SDL_VERSION(&sysInfo.version); //Set SDL version
    if(SDL_GetWMInfo(&sysInfo) <= 0)
    {
        printf("Unable to get window handle");
        exit(-1);
    }
#endif
    Reshape (param.x_resolution, param.y_resolution);

    SDL_WM_SetCaption (WINDOW_TITLE, WINDOW_TITLE);
    KeyRepeat (false);
    if (USE_JOYSTICK) InitJoystick ();
//  SDL_EnableUNICODE (1);
}

void CWinsys::KeyRepeat (bool repeat) {
    int delay = ( repeat ? SDL_DEFAULT_REPEAT_DELAY : 0 );
    int interval = ( repeat ? SDL_DEFAULT_REPEAT_INTERVAL : 0 );
    SDL_EnableKeyRepeat (delay, interval);
}

void CWinsys::SetFonttype () {
    const char* font = (param.use_papercut_font > 0 ? "pc20" : "bold");
    FT.SetFont (font);
}

void CWinsys::CloseJoystick () {
    if (joystick_active) SDL_JoystickClose (joystick);	
}

void CWinsys::Quit () {
    CloseJoystick ();
    Tex.FreeTextureList ();
    Course.FreeCourseList ();
    Course.ResetCourse ();
    SaveMessages ();
    Audio.Close ();		// frees music and sound as well
    FT.Clear ();
    if (g_game.argument < 1) Players.SavePlayers ();
    Score.SaveHighScore ();

#if defined(HAVE_GL_GLES1)
    eglMakeCurrent(g_eglDisplay, NULL, NULL, EGL_NO_CONTEXT);
    eglDestroySurface(g_eglDisplay, g_eglSurface);
    eglDestroyContext(g_eglDisplay, g_eglContext);
    g_eglSurface = 0;
    g_eglContext = 0;
    g_eglConfig = 0;
    eglTerminate(g_eglDisplay);
    g_eglDisplay = 0;
    XCloseDisplay(g_x11Display);
    g_x11Display = NULL;
#endif

    SDL_Quit ();
    exit (0);
}

void CWinsys::PrintJoystickInfo () {
	if (joystick_active == false) {
		Message ("No joystick found");
		return;
	}
	PrintStr ("");
	PrintStr (SDL_JoystickName (0));
	int num_buttons = SDL_JoystickNumButtons (joystick);
	printf ("Joystick has %d button%s\n", num_buttons, num_buttons == 1 ? "" : "s");
	int num_axes = SDL_JoystickNumAxes (joystick);
	printf ("Joystick has %d ax%ss\n\n", num_axes, num_axes == 1 ? "i" : "e");
}

void CWinsys::SwapBuffers() {
#if defined(HAVE_GL_GLES1)
    eglSwapBuffers(g_eglDisplay, g_eglSurface);
#else
    SDL_GL_SwapBuffers();
#endif
}

void CWinsys::Delay(unsigned int ms) {
    SDL_Delay (ms);
}

// ------------ modes -------------------------------------------------

void CWinsys::SetModeFuncs (
		TGameMode mode, TInitFuncN init, TLoopFuncN loop, TTermFuncN term,
		TKeybFuncN keyb, TMouseFuncN mouse, TMotionFuncN motion,
		TJAxisFuncN jaxis, TJButtFuncN jbutt, TKeybFuncS keyb_spec) {
    modefuncs[mode].init      = init;
    modefuncs[mode].loop      = loop;
    modefuncs[mode].term      = term;
    modefuncs[mode].keyb      = keyb;
    modefuncs[mode].mouse     = mouse;
    modefuncs[mode].motion    = motion;
    modefuncs[mode].jaxis     = jaxis;
    modefuncs[mode].jbutt     = jbutt;
    modefuncs[mode].keyb_spec = keyb_spec;
}

void CWinsys::IdleFunc () {}

bool CWinsys::ModePending () {
	return g_game.mode != new_mode;
}

/*
typedef struct{
  Uint8 scancode;
  SDLKey sym;
  SDLMod mod;
  Uint16 unicode;
} SDL_keysym;*/

void CWinsys::PollEvent () {
    SDL_Event event; 
	SDL_keysym sym;
    unsigned int key, axis;
    int x, y;
	float val;

	while (SDL_PollEvent (&event)) {
		if (ModePending()) {
			IdleFunc ();
    	} else {
			switch (event.type) {
				case SDL_KEYDOWN:
				if (modefuncs[g_game.mode].keyb) {
					SDL_GetMouseState (&x, &y);
					key = event.key.keysym.sym; 
					(modefuncs[g_game.mode].keyb) (key, key >= 256, false, x, y);
				} else if (modefuncs[g_game.mode].keyb_spec) {
					sym = event.key.keysym;
					(modefuncs[g_game.mode].keyb_spec) (sym, false);
				}
				break;
	
				case SDL_KEYUP:
				if (modefuncs[g_game.mode].keyb) {
					SDL_GetMouseState (&x, &y);
					key = event.key.keysym.sym; 
					(modefuncs[g_game.mode].keyb)  (key, key >= 256, true, x, y);
				} else if (modefuncs[g_game.mode].keyb_spec) {
					sym = event.key.keysym;
					(modefuncs[g_game.mode].keyb_spec) (sym, true);
				}
				break;
	
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
				if (modefuncs[g_game.mode].mouse) {
					(modefuncs[g_game.mode].mouse) 
						(event.button.button, event.button.state,
						event.button.x, event.button.y);
				}
				break;
	
				case SDL_MOUSEMOTION:
					if (modefuncs[g_game.mode].motion) 
					(modefuncs[g_game.mode].motion) (event.motion.x, event.motion.y);
				break;

				case SDL_JOYAXISMOTION:  
				if (joystick_active) {
					axis = event.jaxis.axis;
					if (modefuncs[g_game.mode].jaxis && axis < 2) {
						val = (float)event.jaxis.value / 32768;
							(modefuncs[g_game.mode].jaxis) (axis, val);
					}
				}
				break; 

				case SDL_JOYBUTTONDOWN:  
				case SDL_JOYBUTTONUP:  
				if (joystick_active) {
					if (modefuncs[g_game.mode].jbutt) {
						(modefuncs[g_game.mode].jbutt) 
							(event.jbutton.button, event.jbutton.state);
					}
				}
				break;

				case SDL_VIDEORESIZE:
					param.x_resolution = event.resize.w;
					param.y_resolution = event.resize.h;
					SetupVideoMode (param.res_type);
					Reshape (event.resize.w, event.resize.h);
				break;
			
				case SDL_QUIT: 
					Quit ();
				break;
			}
    	}
	}
}

void CWinsys::ChangeMode () {
	// this function is called when new_mode is set
	// terminate function of previous mode
	if (g_game.mode >= 0 &&  modefuncs[g_game.mode].term != 0) 
	    (modefuncs[g_game.mode].term) ();
	g_game.prev_mode = g_game.mode;

	// init function of new mode
	if (modefuncs[new_mode].init != 0) {
		(modefuncs[new_mode].init) ();
		clock_time = SDL_GetTicks() * 1.e-3;
	}

	g_game.mode = new_mode;
	// new mode is now the current mode.
}

void CWinsys::CallLoopFunction () {
		cur_time = SDL_GetTicks() * 1.e-3;
		g_game.time_step = cur_time - clock_time;
		if (g_game.time_step < 0.0001) g_game.time_step = 0.0001;
		clock_time = cur_time;

		if (modefuncs[g_game.mode].loop != 0) 
			(modefuncs[g_game.mode].loop) (g_game.time_step);	
}

void CWinsys::EventLoop () {
    while (true) {
	PollEvent ();
	if (ModePending()) ChangeMode ();
	CallLoopFunction ();
	Delay (g_game.loopdelay);
    }
}

unsigned char *CWinsys::GetSurfaceData () {
	return (unsigned char*)screen->pixels;
}

