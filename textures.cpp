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

#include "textures.h"
#include "spx.h"
#include <fstream>
#include "course.h"

// --------------------------------------------------------------------
//				class CImage
// --------------------------------------------------------------------

CImage::CImage () { 
	data = NULL;
	nx = 0;	
    ny = 0; 
    depth = 0;
    pitch = 0;
}

CImage::~CImage () {
	DisposeData ();
}

void CImage::DisposeData () {
	if (data != NULL) {
		free (data);
		data = NULL;
	}
}

bool CImage::LoadPng (const char *filepath, bool mirroring) {
	SDL_Surface *sdlImage;
	unsigned char *sdlData;
	
	sdlImage = IMG_Load (filepath);
   	if (sdlImage == 0) {
   		Message ("could not load image", filepath);
		return false;
   	}

	nx    = sdlImage->w;
	ny    = sdlImage->h;
	depth = sdlImage->format->BytesPerPixel;
	pitch = sdlImage->pitch;
	DisposeData ();
	data  = (unsigned char *) malloc (pitch * ny * sizeof (unsigned char));

   	if (SDL_MUSTLOCK (sdlImage)) {
   	    if (SDL_LockSurface (sdlImage) < 0) {
	   		Message ("mustlock error");
			return false;
		};
   	}

	sdlData = (unsigned char *) sdlImage->pixels;

	if (mirroring) {
		for (int y=0; y<ny; y++) {
#if 1
			memcpy( data + y * pitch, sdlData + ( ny - 1 - y ) * pitch, pitch*sizeof(sdlData[0]) );
#else
			for (int x=0; x<pitch; x++) {
				data [y * pitch + x] = sdlData [(ny-1-y) * pitch + x];	
			}
#endif
		}
	} else {
#if 1
		memcpy( data, sdlData, ny*pitch*sizeof(sdlData[0]) );
#else
		for (int y=0; y<ny; y++) {
			for (int x=0; x<pitch; x++) {
				data [y * pitch + x] = sdlData [y * pitch + x];	
			}
		}
#endif
	}

	if (SDL_MUSTLOCK (sdlImage)) SDL_UnlockSurface (sdlImage);
	SDL_FreeSurface (sdlImage);
	return true;
}

bool CImage::LoadPng (const char *dir, const char *filename, bool mirroring) {
	string path = dir;
	path += SEP;
	path += filename;
	return LoadPng (path.c_str(), mirroring);
}

// ------------------ read framebuffer --------------------------------

#if !defined(HAVE_GL_GLES1) && !defined(HAVE_GL_GLES2) // TODO
bool CImage::ReadFrameBuffer_PPM () {
	int viewport[4];
	glGetIntegerv (GL_VIEWPORT, viewport);
	
	nx = viewport[2];
	ny = viewport[3];
	depth = 3;

	DisposeData ();
	data  = (unsigned char *) malloc (nx * ny * depth * sizeof (unsigned char));
	
	glReadBuffer (GL_FRONT);
	
	for (int i=0; i<viewport[3]; i++){
		glReadPixels (viewport[0], viewport[1] + viewport[3] - 1 - i,
			viewport[2], 1, GL_RGB, GL_UNSIGNED_BYTE, data + viewport[2] * i * 3);
	}
	
	return true;
}
#endif

#if !defined(HAVE_GL_GLES1) && !defined(HAVE_GL_GLES2) // TODO
void CImage::ReadFrameBuffer_TGA () {
	nx = param.x_resolution;
	ny = param.y_resolution;
	depth = 3;

	DisposeData ();
	data  = (unsigned char *) malloc (nx * ny * depth * sizeof (unsigned char));

	glReadBuffer (GL_FRONT);
	glReadPixels (0, 0, nx, ny, GL_BGR, GL_UNSIGNED_BYTE, data);	
}
#endif

#if !defined(HAVE_GL_GLES1) && !defined(HAVE_GL_GLES2) // TODO
void CImage::ReadFrameBuffer_BMP () {
	nx = param.x_resolution;
	ny = param.y_resolution;
	depth = 4;

	DisposeData ();
	data  = (unsigned char *) malloc (nx * ny * depth * sizeof (unsigned char));
	glReadBuffer (GL_FRONT);
	glReadPixels (0, 0, nx, ny, GL_BGRA, GL_UNSIGNED_BYTE, data);	
}
#endif

// ---------------------------

void CImage::WritePPM (const char *filepath) {
	if (data == NULL) return;
	std::ofstream file;
	file.open (filepath);

	file << "P6\n# A Raw PPM file"
		 << "\n# width\n" << nx
		 << "\n# height\n" << ny
		 << "\n# max component value\n255"<< std::endl;

	file.write ((const char*) data, nx * ny * depth);
	file.close ();
}

void CImage::WritePPM (const char *dir, const char *filename) {
	string path = dir;
	path += SEP;
	path += filename;
	WritePPM (path.c_str());
}

void CImage::WriteTGA (const char *filepath) {
	if (data == NULL) return;
	FILE *out = fopen (filepath, "w");
	short TGAhead[] = {0, 2, 0, 0, 0, 0, nx, ny, 24};

	fwrite (&TGAhead, sizeof (TGAhead), 1, out);
	fwrite (data, 3 * nx * ny, 1, out);
	fclose (out); 
}

void CImage::WriteTGA (const char *dir, const char *filename) {
	string path = dir;
	path += SEP;
	path += filename;
	WriteTGA (path.c_str());
}

void CImage::WriteTGA_H (const char *filepath) {
	if (data == NULL) return;
	TTgaHeader header;

	header.tfType = 0;
    header.tfColorMapType = 0;
    header.tfImageType = 2;
    for (int i=0; i<5; i++) header.tfColorMapSpec[i] = 0;
    header.tfOrigX = 0;
    header.tfOrigY = 0;
    header.tfWidth = param.x_resolution;
    header.tfHeight = param.y_resolution;
    header.tfBpp = 24;
    header.tfImageDes = 0;

	FILE   *out = fopen (filepath, "w");
	fwrite (&header, sizeof (TTgaHeader), 1, out);
	fwrite (data, 3 * nx * ny, 1, out);
	fclose (out); 
}

void CImage::WriteTGA_H (const char *dir, const char *filename) {
	string path = dir;
	path += SEP;
	path += filename;
	WriteTGA_H (path.c_str());
}

void CImage::WriteBMP (const char *filepath) {
	if (data == NULL) return;
	TBmpInfo info;
    FILE *fp;       
    int  infosize; 
	unsigned int bitsize;   

	info.biSize = 40;
	info.biWidth = nx;
	info.biHeight = ny;
	info.biPlanes = 1;
	info.biBitCount = 8 * depth;
	info.biCompression = 0;
	info.biSizeImage = nx * ny * depth; 
	info.biXPelsPerMeter = 0;
	info.biYPelsPerMeter= 0;
	info.biClrUsed = 0;
	info.biClrImportant = 0;

    if ((fp = fopen (filepath, "wb")) == NULL) {
		Message ("could not open bmp file", filepath);
		return;
	}

	int imgsize = info.biSizeImage;
	int width = info.biWidth;
	int height = info.biHeight;
	int bitcnt = info.biBitCount; // 24 or 32

	// (width * bitcnt + 7) / 8 = width * depth
    if (imgsize == 0) bitsize = (width * bitcnt + 7) / 8 * height;
    else bitsize = imgsize;

    infosize = info.biSize; // 40
	if (infosize != 40 || info.biCompression != 0) {
		Message ("wrong bmp header");
		return;
	}

    write_word  (fp, 0x4D42); 
    write_dword (fp, 14 + infosize + bitsize);    
    write_word  (fp, 0);        
    write_word  (fp, 0);        
    write_dword (fp, 54);

    write_dword (fp, info.biSize);
    write_long  (fp, info.biWidth);
    write_long  (fp, info.biHeight);
    write_word  (fp, info.biPlanes);
    write_word  (fp, info.biBitCount);
    write_dword (fp, info.biCompression);
    write_dword (fp, info.biSizeImage);
    write_long  (fp, info.biXPelsPerMeter);
    write_long  (fp, info.biYPelsPerMeter);
    write_dword (fp, info.biClrUsed);
    write_dword (fp, info.biClrImportant);

    if (fwrite (data, 1, bitsize, fp) != bitsize) {
		Message ("error on writing bmp data");
        fclose (fp);
        return;
    }

    fclose(fp);
    return;
}

void CImage::WriteBMP (const char *dir, const char *filename) {
	string path = dir;
	path += SEP;
	path += filename;
	WriteBMP (path.c_str());
}

// --------------------------------------------------------------------
//				class CTexture
// --------------------------------------------------------------------

CTexture Tex;

CTexture::CTexture () {
	for (int i=0; i<MAX_COMMON_TEX; i++) {
		CommonTex[i].id = 0;
		CommonTex[i].width = 0;
		CommonTex[i].height = 0;
	}

	numTextures = 0;
	TextureIndex = "";	
	forientation = OR_TOP;
}

CTexture::~CTexture () {}

int CTexture::LoadTexture (const char *filename, GLuint *width, GLuint *height) {
	CImage texImage;
	GLuint texid;

	if (texImage.LoadPng (filename, true) == false) return 0;
	glGenTextures (1, &texid);
	glBindTexture (GL_TEXTURE_2D, texid);		
	glPixelStorei (GL_UNPACK_ALIGNMENT, 4); 
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 

	GLenum format = (texImage.depth == 3 ? GL_RGB : GL_RGBA);
	if (width) *width = texImage.nx;
	if (height) *height = texImage.ny;

	glTexImage2D 
		(GL_TEXTURE_2D, 0, texImage.depth, texImage.nx,
		texImage.ny, 0, format, GL_UNSIGNED_BYTE, texImage.data);

	texImage.DisposeData();
	return texid;
}

int CTexture::LoadTexture (const char *dir, const char *filename, GLuint *width, GLuint *height) {
	string path = dir;
	path += SEP;
	path += filename;
	return LoadTexture (path.c_str(), width, height);
}

int CTexture::LoadTexture (const string dir, const string filename, GLuint *width, GLuint *height) {
	return LoadTexture (dir.c_str(), filename.c_str (), width, height);
}

int CTexture::LoadMipmapTexture (const char *filename, bool repeatable, GLuint *width, GLuint *height) {
	CImage texImage;
	GLuint texid;

	if (texImage.LoadPng (filename, true) == false) return 0;
	glGenTextures (1, &texid);
	glBindTexture (GL_TEXTURE_2D, texid);		
	glPixelStorei (GL_UNPACK_ALIGNMENT, 4);
 
	if  (repeatable) {
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	} else {
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST); 

	GLenum format = (texImage.depth == 3 ? GL_RGB : GL_RGBA);
	if (width) *width = texImage.nx;
	if (height) *height = texImage.ny;

#if defined(GL_VERSION_1_4) && GL_VERSION_1_4
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	glTexImage2D
		(GL_TEXTURE_2D, 0, texImage.depth, texImage.nx,
		texImage.ny, 0, format, GL_UNSIGNED_BYTE, texImage.data);
#else
	gluBuild2DMipmaps 
		(GL_TEXTURE_2D, texImage.depth, texImage.nx,
		texImage.ny, format, GL_UNSIGNED_BYTE, texImage.data);
#endif	

	texImage.DisposeData();
	return texid;    
}

int CTexture::LoadMipmapTexture (const char *dir, const char *filename, bool repeatable, GLuint *width, GLuint *height) {
	string path = dir;
	path += SEP;
	path += filename;
	return LoadMipmapTexture (path.c_str(), repeatable, width, height);
}

void CTexture::LoadTextureList () {
	int rep, id;
	string texfile, line, name;

	TextureIndex = "";
	CSPList list (200);
	if (list.Load (param.tex_dir, "textures.lst")) {
		for (int i=0; i<list.Count(); i++) {
			line = list.Line (i);
			name = SPStrN (line, "name", "");		
			id = SPIntN (line, "id", 0);
			texfile = SPStrN (line, "file", "");
			rep = SPIntN (line, "repeat", 0);
			if (id >= 0 && id < MAX_COMMON_TEX) {
				GLuint w, h;
				if (rep>0) CommonTex[id].id = 
					LoadMipmapTexture (param.tex_dir.c_str(), texfile.c_str(), rep, &w, &h);
				else CommonTex[id].id = 
					LoadTexture (param.tex_dir.c_str(), texfile.c_str(), &w, &h);
				if (CommonTex[id].id > 0) {
					CommonTex[id].width = w;
					CommonTex[id].height = h;
					TextureIndex = TextureIndex + "[" + name + "]" + Int_StrN (CommonTex[id].id);
					numTextures++;
				}
			} else Message ("wrong texture id in textures.lst");	
		}
	} else {
		Message ("failed to load common textures");
	}
}

void CTexture::FreeTextureList () {
	for (int i=0; i<MAX_COMMON_TEX; i++) {
		if (CommonTex[i].id > 0) {
			glDeleteTextures (1, &CommonTex[i].id);
			CommonTex[i].id = 0;
			CommonTex[i].width = 0;
			CommonTex[i].height = 0;
		}
	}
}

GLuint CTexture::TexWidth (int idx) {
	if (idx >= MAX_COMMON_TEX || idx < 0) return 0;
	return CommonTex[idx].width;
}

GLuint CTexture::TexHeight (int idx) {
	if (idx >= MAX_COMMON_TEX || idx < 0) return 0;
	return CommonTex[idx].height;
}

GLuint CTexture::TexID (int idx) {
	if (idx >= MAX_COMMON_TEX || idx < 0) return 0;
	return CommonTex[idx].id;
}

GLuint CTexture::TexID (string name) {
	return SPIntN (TextureIndex, name, 0);
}

bool CTexture::BindTex (int idx) {
	if (idx < 0 || idx >= MAX_COMMON_TEX) return false;
	glBindTexture (GL_TEXTURE_2D, CommonTex[idx].id);
	return true;
}

bool CTexture::BindTex (string name) {
	GLuint id = SPIntN (TextureIndex, name, 0);
	if (id == 0) return false;
	glBindTexture (GL_TEXTURE_2D, id);
	return true;
}

// ---------------------------- Draw ----------------------------------

void CTexture::DrawDirect (GLuint texid, int x, int y, float width, float height) {
	GLfloat top, bott, left, right;

	glEnable (GL_TEXTURE_2D);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindTexture (GL_TEXTURE_2D, texid);

	if (forientation == OR_TOP) {
		top = param.y_resolution - y;
		bott = top - height;
	} else {
		bott = y;
		top = bott + height;
	}

	left = (x >= 0 ? x : (param.x_resolution - width) / 2 );
	right = left + width;

	const GLfloat tex[] = { 0,0, 1,0, 1,1, 0,1 };
	const GLfloat vtx[] = {
		 left, bott,
		right, bott,
		right, top,
		 left, top
	};

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
 
	glColor4f (1.0, 1.0, 1.0, 1.0);
	glVertexPointer (2, GL_FLOAT, 0, vtx);
	glTexCoordPointer (2, GL_FLOAT, 0, tex);
	glDrawArrays (GL_TRIANGLE_FAN,0,4);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void CTexture::Draw (int idx, int x, int y, float size) {
	DrawDirect (TexID (idx), x, y, size * TexWidth (idx), size * TexHeight (idx) );
}

void CTexture::Draw (int idx, int x, int y, int width, int height) {
	DrawDirect (TexID (idx), x, y, width, height);
}

void CTexture::DrawDirectFrame (GLuint texid, int x, int y, double w, double h, int frame, TColor col) {
	GLint ww = GLint (w);
	GLint hh = GLint (h);
	GLint xx = x;
	GLint yy = param.y_resolution - hh - y;

	if (texid < 1) return;
	glBindTexture (GL_TEXTURE_2D, texid);

	if (frame > 0) {
		const GLfloat vtx[] = {
		    xx - frame, yy - frame,
		    xx + ww + frame, yy - frame,
		    xx + ww + frame, yy + hh + frame,
		    xx - frame, yy + hh + frame
		};

		glDisable (GL_TEXTURE_2D);
		glEnableClientState(GL_VERTEX_ARRAY);
 
		glColor4f (col.r, col.g, col.b, 1.0);
		glVertexPointer (2, GL_FLOAT, 0, vtx);
		glDrawArrays (GL_TRIANGLE_FAN,0,4);

		glDisableClientState(GL_VERTEX_ARRAY);
		glEnable (GL_TEXTURE_2D);
	}

	const GLfloat tex[] = { 0,0, 1,0, 1,1, 0,1 };
	const GLfloat vtx[] = {
		xx, yy,
		xx + ww, yy,
		xx + ww, yy + hh,
		xx, yy + hh
	};

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
 
	glColor4f (1.0, 1.0, 1.0, 1.0);
	glVertexPointer (2, GL_FLOAT, 0, vtx);
	glTexCoordPointer (2, GL_FLOAT, 0, tex);
	glDrawArrays (GL_TRIANGLE_FAN,0,4);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void CTexture::DrawFrame (int idx, int x, int y, double w, double h, int frame, TColor col) {
	DrawDirectFrame (TexID (idx), x, y, w, h, frame, col);
}

void CTexture::DrawFrame (string name, int x, int y, double w, double h, int frame, TColor col) {
	DrawDirectFrame (TexID (name), x, y, w, h, frame, col);
}

void CTexture::SetOrientation (int orientation) {
	forientation = orientation;
}

// -------------------------- numeric strings -------------------------

void CTexture::DrawNumChr (char c, int x, int y, int w, int h, TColor col) {
	static string numidxstr = "[0]0[1]1[2]2[3]3[4]4[5]5[6]6[7]7[8]8[9]9[:]10[ ]11";
	float texleft, texright, texw;
	char chrname[2]; 	
	TVector2 bl, tr;
	chrname[0]= c;
	chrname[1]= 0;

	int idx = SPIntN (numidxstr, chrname, 11);	

	// quad coords
	bl.x = x;
	bl.y = param.y_resolution - y - h;
	tr.x = x + w * 0.9;
	tr.y = param.y_resolution - y;

	// texture coords
	texw = 22.0 / 256.0;
	texleft = idx * texw;
	texright = (idx + 1) * texw;

	const GLfloat tex[] = {
		texleft, 0,
		texright, 0,
		texright, 1,
		texleft, 1
	};
	const GLfloat vtx[] = {
		bl.x, bl.y,
		tr.x, bl.y,
		tr.x, tr.y,
		bl.x, tr.y
	};

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
 
	glColor4f (col.r, col.g, col.b, col.a);
	glVertexPointer (2, GL_FLOAT, 0, vtx);
	glTexCoordPointer (2, GL_FLOAT, 0, tex);
	glDrawArrays (GL_TRIANGLE_FAN,0,4);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
} 

void CTexture::DrawNumStr (const char *s, int x, int y, float size, TColor col) {
	if (!BindTex ("ziff032")) {
		Message ("DrawNumStr: missing texture");
		return;
	}
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable (GL_TEXTURE_2D);
	int qw = (int)(22 * size);
	int qh = (int)(32 * size);

	for (int i=0; i<int(strlen(s)); i++) {
		DrawNumChr (s[i], x + i*qw, y, qw, qh, col); 
	}
}

// --------------------------------------------------------------------
//				screenshot
// --------------------------------------------------------------------

// 0 ppm, 1 tga, 2 tga_header, 3 bmp
#define SCREENSHOT_PROC 3

void ScreenshotN () {
#if !defined(HAVE_GL_GLES1) && !defined(HAVE_GL_GLES2) // TODO
	CImage image;
	string path = param.screenshot_dir;
	path += SEP;
	path += Course.CourseList[g_game.course_id].dir;
	path += "_";
	path += GetTimeString1 ();
	int type = SCREENSHOT_PROC;

	switch (type) {
		case 0:
			path += ".ppm";
			image.ReadFrameBuffer_PPM ();
			image.WritePPM (path.c_str());
			image.DisposeData ();
			break;
		case 1:
			path += ".tga";
			image.ReadFrameBuffer_TGA ();
			image.WriteTGA (path.c_str());
			image.DisposeData ();
			break;
		case 2:
			path += ".tga";
			image.ReadFrameBuffer_TGA ();
			image.WriteTGA_H (path.c_str());
			image.DisposeData ();
			break;
		case 3:
			path += ".bmp";
			image.ReadFrameBuffer_BMP ();
			image.WriteBMP (path.c_str());
			image.DisposeData ();
			break;
	}
#endif
} 

