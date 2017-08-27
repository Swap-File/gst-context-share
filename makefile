OBJs := $(patsubst %.cpp, %.o, $(wildcard *.cpp)) 
DEPs := $(OBJs:%.o=%.d)
BIN  := glxgears
CC  = g++

#Gstreamer stuff
LDFLAGS = -L/usr/local/lib -lX11 -lXrender -lGL -lGLU -lglut -lpng -lgstapp-1.0 -lgstvideo-1.0 -lgstgl-1.0 -lgstreamer-1.0 -lgmodule-2.0 -lgthread-2.0 -lgobject-2.0 -lglib-2.0 -lm
CPPFLAGS = -pthread -I/usr/include/gstreamer-1.0 -I/usr/lib/arm-linux-gnueabihf/gstreamer-1.0/include -I/usr/include/glib-2.0 -I/usr/lib/arm-linux-gnueabihf/glib-2.0/include

# Use a modern language
CPPFLAGS += --std=c++11

# Turn on "all" warnings
CPPFLAGS += -Wall  

# Generate .d files with dependency info
CPPFLAGS += -MD -MP

# optimized.
CPPFLAGS += -O2 
	
.PHONY: all
all: $(BIN)

$(BIN): $(OBJs)

.PHONY: clean
clean:
	@rm -fv $(BIN) $(OBJs) $(DEPs) *.o
	
