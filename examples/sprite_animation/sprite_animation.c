#include "villa2d.h"
#include "uv.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define MAX_FRAME 8

static villa2dImage *image;
static villa2dImage *backgroundImage;
static villa2dSpriteFrame *frames[8][MAX_FRAME];
static int frameIndex = 0;
static villa2dSpriteFrame *backgroundFrame;
static villa2dSpriteFrame *backgroundFrame2;
static int myDirect = 0;
static int myX = 0;
static int myY = 0;

static void onMouseEvent(int button, int state, int x, int y) {
  myDirect = villa2dWindowPosToDirect(myDirect, x, y);
  myX = x;
  myY = y;
}

static const char *directToString(int direct) {
  switch (direct) {
    case VILLA2D_NORTH: return "NORTH";
    case VILLA2D_SOUTH: return "SOUTH";
    case VILLA2D_WEST: return "WEST";
    case VILLA2D_EAST: return "EAST";
    case VILLA2D_NORTHWEST: return "NORTHWEST";
    case VILLA2D_NORTHEAST: return "NORTHEAST";
    case VILLA2D_SOUTHWEST: return "SOUTHWEST";
    case VILLA2D_SOUTHEAST: return "SOUTHEAST";
    default: return "";
  }
}

static int directToFrame(int direct) {
  switch (direct) {
    case VILLA2D_NORTH: return 2;
    case VILLA2D_SOUTH: return 6;
    case VILLA2D_WEST: return 0;
    case VILLA2D_EAST: return 4;
    case VILLA2D_NORTHWEST: return 1;
    case VILLA2D_NORTHEAST: return 3;
    case VILLA2D_SOUTHWEST: return 7;
    case VILLA2D_SOUTHEAST: return 5;
    default: return 0;
  }
}

static void onRender(void) {
  char debugInfo[512];
  int debugInfoOffsetY = 0;
  
  villa2dSetColor(0xffffff);
  
  sprintf(debugInfo, "frameIndex:%d", frameIndex);
  villa2dDrawText(20, 20 + debugInfoOffsetY, debugInfo);
  debugInfoOffsetY += 12;
  
  sprintf(debugInfo, "(%d,%d) %s", myX, myY, directToString(myDirect));
  villa2dDrawText(20, 20 + debugInfoOffsetY, debugInfo);
  debugInfoOffsetY += 12;
  
  villa2dSetColor(0xffffff);
  villa2dDrawRect(100, 100, 200, 200);
  
  villa2dBlitSpriteFrame(backgroundFrame, 128, 128);
  villa2dBlitSpriteFrame(backgroundFrame2, 128, 128);
  
  villa2dSetColor(0xff00ff);
  villa2dDrawRect(villa2dGetWindowWidth() / 2 - 100, villa2dGetWindowHeight() / 2 - 100, 200, 200);

  villa2dBlitSpriteFrame(frames[directToFrame(myDirect)][frameIndex], 
    villa2dGetWindowWidth() / 2, villa2dGetWindowHeight() / 2);
  ++frameIndex;
  if (frameIndex >= MAX_FRAME) {
    frameIndex = 0;
  }
}

static uv_timer_t frameTimer;

static void frame(uv_timer_t* handle) {
  villa2dPostFrame();
}

static void onIdle(void) {
  uv_run(uv_default_loop(), UV_RUN_ONCE);
}

int main(int agrc, char *argv[]) {
  int i, j;
  villa2dInit(1024, 768);
  villa2dSetRenderHandler(onRender);
  villa2dSetIdleHandler(onIdle);
  villa2dSetMouseEventHandler(onMouseEvent);
  image = villa2dCreateImage("skeleton_0.png");
  assert(image);
  backgroundImage = villa2dCreateImage("orc_regular_0.png");
  backgroundFrame = villa2dCreateSpriteFrameWithRect(backgroundImage, 0, 0, 128, 128);
  backgroundFrame2 = villa2dCreateSpriteFrameWithRect(backgroundImage, 0, 0, 128, 128);
  villa2dSetSpriteFrameFlipX(backgroundFrame, 1);
  villa2dSetSpriteFrameRotate(backgroundFrame, 15);
  villa2dSetSpriteFrameFlipX(backgroundFrame2, 1);
  villa2dSetSpriteFrameRotate(backgroundFrame2, 90);
  for (j = 0; j < 8; ++j) {
    for (i = 0; i < MAX_FRAME; ++i) {
      frames[j][i] = villa2dCreateSpriteFrameWithRect(image, (4 + i) * 128, j * 128, 128, 128);
    }
  }
  uv_timer_init(uv_default_loop(), &frameTimer);
  uv_timer_start(&frameTimer, frame, 100, 100);
  return villa2dRun();
}

