CXXFLAGS = $(shell GraphicsMagick++-config --cxxflags --cppflags)
CXXFLAGS += -std=c++17
LDFLAGS = $(shell GraphicsMagick++-config --ldflags --libs)

.PHONY: all

all: mbmapper

mbmapper: main.cpp
	c++ $(CXXFLAGS) main.cpp -o mbmapper $(LDFLAGS)
