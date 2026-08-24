CXX    := c++
TARGET := pdfview

CXXFLAGS := -std=c++17 -O2 -Wall -Wextra \
            $(shell pkg-config --cflags poppler-cpp libpng)
LDFLAGS  := $(shell pkg-config --libs poppler-cpp libpng)

SRCS := src/main.cpp src/term.cpp src/render.cpp src/history.cpp \
        src/tui.cpp src/home.cpp src/viewer.cpp

OBJS := $(SRCS:src/%.cpp=build/%.o)
DEPS := $(OBJS:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

# Compile + auto-generate header dependency files
build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

build:
	mkdir -p build

# Include auto-generated deps so header changes trigger recompilation
-include $(DEPS)

clean:
	rm -rf build $(TARGET)

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

.PHONY: all clean install
