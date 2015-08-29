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

#include "villa2d.h"
#include "lodepng.h"

#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/freeglut.h>

static int windowWidth = 0;
static int windowHeight = 0;
static renderHandler onUserDefinedRender = 0;
static idleHandler onUserDefinedIdle = 0;
static mouseEventHandler onUserDefinedMouseEvent = 0;
static double angleTans[2] = {0, 0};

static void destroyTexForImage(villa2dImage *image) {
  assert(image->texId);
  if (image->texId) {
    glDeleteTextures(1, &image->texId);
    image->texId = 0;
  }
}

static void refImage(villa2dImage *image) {
  ++image->refs;
}

static void unrefImage(villa2dImage *image) {
  --image->refs;
  if (0 == image) {
    if (image->texId) {
      destroyTexForImage(image);
    }
    free(image);
  }
}

void villa2dDestroyImage(villa2dImage *image) {
  unrefImage(image);
}

villa2dSpriteFrame *villa2dCreateSpriteFrameWithRect(villa2dImage *image,
    int left, int top, int width, int height) {
  villa2dPoint pointData[4] = {
    {left, top},
    {left, top + height - 1},
    {left + width - 1, top + height - 1},
    {left + width - 1, top}
  };
  return villa2dCreateSpriteFrame(image, left + width / 2, top + height / 2, 
    4, pointData);
}

villa2dSpriteFrame *villa2dCreateSpriteFrame(villa2dImage *image,
    int centerX, int centerY, int pointCount, villa2dPoint *pointData) {
  villa2dSpriteFrame *spriteFrame;
  int i;
  villa2dPoint *point;
  int minX;
  int minY;
  assert(pointCount > 0 && pointData);
  if (pointCount <= 0 || !pointData) {
    return 0;
  }
  spriteFrame = malloc(sizeof(*spriteFrame) + 
    (pointCount - 1) * sizeof(villa2dVertex));
  if (!spriteFrame) {
    fprintf(stderr, "villa2dCreateSpriteFrame::malloc failed(pointCount:%d)\n",
      pointCount);
    return 0;
  }
  spriteFrame->image = image;
  spriteFrame->flipX = 0;
  spriteFrame->flipY = 0;
  spriteFrame->rotate = 0;
  spriteFrame->centerX = centerX;
  spriteFrame->centerY = centerY;
  refImage(image);
  spriteFrame->pointCount = pointCount;
  point = &pointData[0];
  for (i = 0; i < pointCount; ++i) {
    villa2dVertex *vtx = &spriteFrame->vertexData[i];
    point = &pointData[i];
    vtx->xScale = (float)point->x / image->width;
    vtx->yScale = (float)point->y / image->height;
    vtx->xOffset = point->x - centerX;
    vtx->yOffset = point->y - centerY;
  }
  return spriteFrame;
}

void villa2dDestroySpriteFrame(villa2dSpriteFrame *spriteFrame) {
  unrefImage(spriteFrame->image);
  free(spriteFrame);
}

villa2dImage *villa2dCreateImage(const char *filename) {
  unsigned char *imageData = 0;
  int err;
  villa2dImage *image;
  long imageDataSize;
  int width;
  int height;
  err = lodepng_decode32_file(&imageData, &width, &height, filename);
  if (err) {
    fprintf(stderr, "villa2dCreateImage::lodepng_decode32_file(err:%u text:%s)\n", 
      err, lodepng_error_text(err));
    return 0;
  }
  imageDataSize = width * height * 4;
  image = malloc(sizeof(*image) - 1 + imageDataSize);
  if (!image) {
    fprintf(stderr, "villa2dCreateImage::malloc failed\n");
    free(imageData);
    return 0;
  }
  image->refs = 1;
  image->width = width;
  image->height = height;
  memcpy(image->imageData, imageData, imageDataSize);
  free(imageData);
  return image;
}

void villa2dSetImageTransparentColor(villa2dImage *image, unsigned int rgb) {
  unsigned char r = (rgb >> 16);
  unsigned char g = (rgb >> 8) & 0xff;
  unsigned char b = rgb & 0xff;
  unsigned long count;
  unsigned long i;
  
  // If the texture has already been specified to OpenGL, 
  // we delete it.
  if (image->texId) {
    destroyTexForImage(image);
  }

  // For all the pixels that correspond to the specifed color,
  // set the alpha channel to 0 (transparent) and reset the other
  // ones to 255.
  count = image->width * image->height * 4;
  for (i = 0; i < count; i += 4) {
    if ((image->imageData[i] == r) && (image->imageData[i+1] == g) 
        && (image->imageData[i+2] == b)) {
      image->imageData[i+3] = 0;
    } else {
      image->imageData[i+3] = 255;
    }
  }
}

static void createTexForImage(villa2dImage *image) {
  assert(0 == image->texId);
  if (!image->texId) {
    glGenTextures(1, &image->texId);
    // Make this texture the active one, so that each
    // subsequent glTex* calls will affect it.
    glBindTexture(GL_TEXTURE_2D, image->texId);

    // Specify a linear filter for both the minification and
    // magnification.
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);                       
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // Sets drawing mode to GL_MODULATE
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    // Finally, generate the texture data in OpenGL.
    glTexImage2D(GL_TEXTURE_2D, 0, 4, image->width, image->height,
      0, GL_RGBA, GL_UNSIGNED_BYTE, image);
  }
}

void villa2dSetColor(unsigned int rgb) {
  glColor3f((float)(rgb >> 16) / 256, (float)((rgb >> 8) & 0xff) / 256, 
    (float)(rgb & 0xff) / 256);
}

void villa2dDrawText(int x, int y, const char *text) {
  glDisable(GL_TEXTURE_2D);
  glRasterPos2f((float)x, (float)y);
  glutBitmapString(GLUT_BITMAP_8_BY_13, (const unsigned char*)text);
  glEnable(GL_TEXTURE_2D);
}

void villa2dSetSpriteFrameFlipX(villa2dSpriteFrame *spriteFrame, int needFlip) {
  spriteFrame->flipX = needFlip;
}

void villa2dSetSpriteFrameFlipY(villa2dSpriteFrame *spriteFrame, int needFlip) {
  spriteFrame->flipY = needFlip;
}

void villa2dSetSpriteFrameRotate(villa2dSpriteFrame *spriteFrame, int rotate) {
  spriteFrame->rotate = rotate;
}

void villa2dBlitSpriteFrame(villa2dSpriteFrame *spriteFrame, int x, int y) {
  int i;
  
  // http://www.gamedev.net/topic/390666-glcolor-and-glbindtexture/
  glColor3f(1.0, 1.0, 1.0);
  
  if (!spriteFrame->image->texId) {
    createTexForImage(spriteFrame->image);
  }
  
  glBindTexture(GL_TEXTURE_2D, spriteFrame->image->texId);
  
  glPushMatrix();
  
  glTranslatef(x, y, 0);
  if (spriteFrame->rotate) {
    glRotatef(spriteFrame->rotate, 0.0, 0.0, 1.0);
  }

  glBegin(GL_POLYGON);
  for (i = 0; i < spriteFrame->pointCount; ++i) {
    villa2dVertex *vtx = &spriteFrame->vertexData[i];
    glTexCoord2f(spriteFrame->flipX ? -vtx->xScale : vtx->xScale, 
      spriteFrame->flipY ? -vtx->yScale : vtx->yScale);
    glVertex3i(vtx->xOffset, vtx->yOffset, 0);
  }
  glEnd();
  
  glPopMatrix();
}

void villa2dDrawRect(int x, int y, int width, int height) {
  glDisable(GL_TEXTURE_2D);
  glBegin(GL_QUADS);
    glVertex2f((float)x, (float)y);
    glVertex2f((float)x + width, (float)y);
    glVertex2f((float)x + width, (float)y + height);
    glVertex2f((float)x, (float)y + height);
  glEnd();
  glEnable(GL_TEXTURE_2D);
}

void villa2dPostFrame(void) {
  glutPostRedisplay();
}

static void render(void) {
  if (onUserDefinedRender) {
    onUserDefinedRender();
  }
}

void villa2dSetRenderHandler(renderHandler handler) {
  onUserDefinedRender = handler;
}

void villa2dSetIdleHandler(idleHandler handler) {
  onUserDefinedIdle = handler;
}

void villa2dSetMouseEventHandler(mouseEventHandler handler) {
  onUserDefinedMouseEvent = handler;
}

static void draw(void) {
  glClear(GL_COLOR_BUFFER_BIT);
  glLoadIdentity();
  render();
  glutSwapBuffers();
}

static void idle(void) {
  if (onUserDefinedIdle) {
    onUserDefinedIdle();
  }
}

static void mouseClicks(int button, int state, int x, int y) {
  if (onUserDefinedMouseEvent) {
    int realButton = 0;
    int realState = 0;
    switch (button) {
      case GLUT_LEFT_BUTTON:
        realButton = VILLA2D_MOUSE_BUTTON_LEFT;
        break;
      case GLUT_RIGHT_BUTTON:
        realButton = VILLA2D_MOUSE_BUTTON_RIGHT;
        break;
    }
    switch (state) {
      case GLUT_DOWN:
        realState = VILLA2D_MOUSE_STATE_DOWN;
        break;
      case GLUT_UP:
        realState = VILLA2D_MOUSE_STATE_UP;
        break;
    }
    onUserDefinedMouseEvent(realButton, realState, x, y);
  }
}

static void enable2d(int width, int height) {
  // Enable 2D texturing
  glEnable(GL_TEXTURE_2D);
  // Choose a smooth shading model
  glShadeModel(GL_SMOOTH);
  // Set the clear color to black
  glClearColor(0.0, 0.0, 0.0, 0.0);

  // Enable the alpha test. This is needed 
  // to be able to have images with transparent 
  // parts.
  glEnable(GL_ALPHA_TEST);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  //glAlphaFunc(GL_GREATER, 0.0f);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
  
  // Sets the size of the OpenGL viewport
  glViewport(0, 0, width,height);

  // Select the projection stack and apply 
  // an orthographic projection
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, width, height, 0.0, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
}

static void reshape(int width, int height) {
  windowWidth = width;
  windowHeight = height;
  enable2d(width, height);
}

static void initAngleTans(void) {
  /*
   360 / 16 = 22.5(degree) 
   */
  #define PI 3.14159265
  angleTans[0] = tan(22.5 * PI / 180.0);
  angleTans[1] = tan(67.5 * PI / 180.0);
}

static void moseMoves(int x, int y) {
  if (onUserDefinedMouseEvent) {
    onUserDefinedMouseEvent(0, VILLA2D_MOUSE_STATE_MOVE, x, y);
  }
}

void villa2dInit(int width, int height) {
  int argc = 0;
  initAngleTans();
  windowWidth = width;
  windowHeight = height;
  glutInit(&argc, 0);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
  glutInitWindowSize(width, height);
  glutCreateWindow("villa2d");
  glutDisplayFunc(draw);
  glutMouseFunc(mouseClicks);
  glutMotionFunc(moseMoves);
  glutPassiveMotionFunc(moseMoves);
  glutReshapeFunc(reshape);
  glutIdleFunc(idle);
  enable2d(width, height);
}

int villa2dRun(void) {
  glutMainLoop();
  return 0;
}

int villa2dGetWindowWidth(void) {
  return windowWidth;
}

int villa2dGetWindowHeight(void) {
  return windowHeight;
}

static double tanVal(double a, double b) {
  if (b <= 0) {
    return 0;
  }
  return a / b;
}

int villa2dWindowPosToDirect(int oldDirect, int x, int y) {
  int direct = 0;
  double val;
  int width = windowWidth;
  int height = windowHeight;
  if (x <= width / 2) {
    if (y <= height / 2) {
      val = tanVal((double)((height / 2) - y),
        ((width / 2) - x));
      if (val <= 0) {
        direct = oldDirect;
      } else if (val < angleTans[0]) {
        direct = VILLA2D_WEST;
      } else if (val < angleTans[1]) {
        direct = VILLA2D_NORTHWEST;
      } else {
        direct = VILLA2D_NORTH;
      }
    } else {
      val = tanVal((double)(y - (height / 2)),
        ((width / 2) - x));
      if (val <= 0) {
        direct = oldDirect;
      } else if (val < angleTans[0]) {
        direct = VILLA2D_WEST;
      } else if (val < angleTans[1]) {
        direct = VILLA2D_SOUTHWEST;
      } else {
        direct = VILLA2D_SOUTH;
      }
    }
  } else {
    if (y <= height / 2) {
      val = tanVal((double)((height / 2) - y),
        (x - (width / 2)));
      if (val <= 0) {
        direct = oldDirect;
      } else if (val < angleTans[0]) {
        direct = VILLA2D_EAST;
      } else if (val < angleTans[1]) {
        direct = VILLA2D_NORTHEAST;
      } else {
        direct = VILLA2D_NORTH;
      }
    } else {
      val = tanVal((double)(y - (height / 2)),
        (x - (width / 2)));
      if (val <= 0) {
        direct = oldDirect;
      } else if (val < angleTans[0]) {
        direct = VILLA2D_EAST;
      } else if (val < angleTans[1]) {
        direct = VILLA2D_SOUTHEAST;
      } else {
        direct = VILLA2D_SOUTH;
      }
    }
  }
  return direct;
}

