CXXFLAGS = -std=c++17 -Wall -fPIC -I./ini -I./importLibraries

SRCS = Gas.cpp GasAnes.cpp GasApplication.cpp GasDoc.cpp GasGlobal.cpp GasSample.cpp ColorUtils.cpp StringUtils.cpp gas_out.cpp

HDRS = framework.h Gas.h GasAnes.h GasApplication.h GasDoc.h GasGlobal.h GasPtrList.h GasSample.h ColorUtils.h importLibraries/SimpleIni.h StringUtils.h importLibraries/json.hpp framework.h trim.h

OUTDIR = ../compiled

OBJS = $(patsubst %.cpp,$(OUTDIR)/%.o,$(filter %.cpp,$(SRCS))) $(patsubst %.c,$(OUTDIR)/%.o,$(filter %.c,$(SRCS)))

TARGET = libgas

ifeq ($(OS),Windows_NT)
    TARGET_EXT = .dll
    CXXFLAGS += -D_WINDOWS
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
        TARGET_EXT = .dylib
        CXXFLAGS += -D_MACOS
    else
        TARGET_EXT = .so
        CXXFLAGS += -D_LINUX
    endif
endif

all: $(OUTDIR) $(OUTDIR)/$(TARGET)$(TARGET_EXT)

$(OUTDIR):
	mkdir -p $(OUTDIR)

$(OUTDIR)/$(TARGET)$(TARGET_EXT): $(OBJS)
	$(CXX) -shared -o $@ $(OBJS) -lstdc++ -lstdc++fs

$(OUTDIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUTDIR)/%.o: %.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OUTDIR)/*.o $(OUTDIR)/$(TARGET)$(TARGET_EXT)
