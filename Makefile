CC  = ccache gcc
CXX = ccache g++

OPT = -O2 -lX11 -lz -fno-inline-functions # -std=c++11

PREFIX = /usr/local/share/frogatto

include Makefile.common

%.o : src/%.cpp
	$(CXX) $(CCFLAGS) -DIMPLEMENT_SAVE_PNG -fno-inline-functions `sdl-config --cflags` -D_GNU_SOURCE=1 -D_REENTRANT -Wnon-virtual-dtor -Werror=return-type -fthreadsafe-statics -g $(OPT) -c $<

# -lSDLmain -lSDL -lGL -lGLU -lGLEW -lSDL_image -lSDL_ttf -lSDL_mixer -lpng 
wte: $(objects)
	$(CXX) $(CCFLAGS) -D_GNU_SOURCE=1 -D_REENTRANT -Wnon-virtual-dtor -Werror=return-type $(objects) -o wte -L. -L/sw/lib -L. -L/usr/lib `sdl-config --libs` -lboost_regex-mt -lboost_system-mt -lboost_filesystem-mt -lpthread -g $(OPT) -fthreadsafe-statics

clean:
	rm -f *.o wte
