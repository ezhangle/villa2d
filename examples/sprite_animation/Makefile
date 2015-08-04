CC ?= gcc

CFLAGS += -g -I../../src
          
LDFLAGS += -lglut -lGLU -lGL -luv

OBJS = sprite_animation.o \
       ../../src/villa2d.o \
       ../../src/lodepng.o
       
HEADERS = ../../src/villa2d.h \
          ../../src/lodepng.h
       
TARGETS = sprite_animation

all: $(TARGETS)

clean:
	-$(RM) $(OBJS) $(TARGETS)

sprite_animation: $(OBJS) $(HEADERS)
	$(CC) -o sprite_animation $(OBJS) $(LDFLAGS)

$(OBJS): %.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<
