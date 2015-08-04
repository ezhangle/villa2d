/* Copyright (c) 2015, huxingyi@msn.com
*
* Permission to use, copy, modify, and/or distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
* WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
* ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
* ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
* OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#ifndef _VILLA2D_H
#define _VILLA2D_H

typedef enum villa2dDirect{
  VILLA2D_NORTH = 1,
  VILLA2D_SOUTH,
  VILLA2D_WEST,
  VILLA2D_EAST,
  VILLA2D_NORTHWEST,
  VILLA2D_NORTHEAST,
  VILLA2D_SOUTHWEST,
  VILLA2D_SOUTHEAST
} villa2dDirect;

typedef enum villa2dMouseButton {
  VILLA2D_MOUSE_BUTTON_LEFT = 1,
  VILLA2D_MOUSE_BUTTON_RIGHT
} villa2dMouseButton;

typedef enum villa2dMouseState {
  VILLA2D_MOUSE_STATE_DOWN = 1,
  VILLA2D_MOUSE_STATE_UP,
  VILLA2D_MOUSE_STATE_MOVE
} villa2dMouseState;

typedef struct villa2dImage {
  unsigned int texId;
  int refs;
  int width;
  int height;
  char imageData[1];
} villa2dImage;

typedef struct villa2dPoint {
  int x;
  int y;
} villa2dPoint;

typedef struct villa2dVertex {
  int xOffset;
  int yOffset;
  float xScale;
  float yScale;
} villa2dVertex;

typedef struct villa2dSpriteFrame {
  villa2dImage *image;
  villa2dPoint leftTop;
  int rotate;
  int flipX:1;
  int flipY:1;
  int centerX;
  int centerY;
  int pointCount;
  villa2dVertex vertexData[1];
} villa2dSpriteFrame;

typedef void (*renderHandler)(void);
typedef void (*idleHandler)(void);
typedef void (*mouseEventHandler)(int button, int state, int x, int y);

void villa2dInit(int width, int height);
int villa2dRun(void);
villa2dImage *villa2dCreateImage(const char *filename);
void villa2dDestroyImage(villa2dImage *image);
villa2dSpriteFrame *villa2dCreateSpriteFrame(villa2dImage *image,
  int centerX, int centerY, int pointCount, villa2dPoint *pointData);
villa2dSpriteFrame *villa2dCreateSpriteFrameWithRect(villa2dImage *image,
  int left, int top, int width, int height);
void villa2dSetSpriteFrameFlipX(villa2dSpriteFrame *spriteFrame, int needFlip);
void villa2dSetSpriteFrameFlipY(villa2dSpriteFrame *spriteFrame, int needFlip);
void villa2dSetSpriteFrameRotate(villa2dSpriteFrame *spriteFrame, int rotate);
void villa2dDestroySpriteFrame(villa2dSpriteFrame *spriteFrame);
void villa2dBlitSpriteFrame(villa2dSpriteFrame *spriteFrame, int x, int y);
void villa2dSetRenderHandler(renderHandler handler);
void villa2dSetIdleHandler(idleHandler handler);
void villa2dSetMouseEventHandler(mouseEventHandler handler);
void villa2dPostFrame(void);
void villa2dSetColor(unsigned int rgb);
void villa2dDrawRect(int x, int y, int width, int height);
void villa2dSetImageTransparentColor(villa2dImage *image, unsigned int rgb);
int villa2dGetWindowWidth(void);
int villa2dGetWindowHeight(void);
int villa2dWindowPosToDirect(int oldDirect, int x, int y);

#endif
