TITLE := PS5 Launcher
VERSION := 0.1.0
TITLE_ID := LCHR00001
CONTENT_ID := IV0000-LCHR00001_00-PS5LAUNCHER00001
TOOLCHAIN := $(OO_PS4_TOOLCHAIN)
INTDIR := build
CXX := clang++-18
LD := ld.lld-18
CXXFLAGS := --target=x86_64-pc-freebsd12-elf -std=c++17 -D_POSIX_C_SOURCE=200809L -O2 -fPIC -funwind-tables -c -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include
LIBS := -lc -lkernel -lc++ -lSceUserService -lSceLncUtil -lSceSystemService -lSceVideoOut -lSceAudioOut -lScePad -lSceSysmodule -lSDL2 -lSDL2_image -lm
LDFLAGS := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr -L$(TOOLCHAIN)/lib $(LIBS) $(TOOLCHAIN)/lib/crt1.o
.PHONY: all clean
all: eboot.bin
$(INTDIR):
	mkdir -p $(INTDIR)
$(INTDIR)/main.o: src/main.cpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -isystem $(TOOLCHAIN)/include/c++/v1 -o $@ $<
$(INTDIR)/launcher.elf: $(INTDIR)/main.o
	$(LD) $^ -o $@ $(LDFLAGS)
eboot.bin: $(INTDIR)/launcher.elf
	$(TOOLCHAIN)/bin/linux/create-fself -in=$< -out=$(INTDIR)/launcher.oelf --eboot "$@" --paid 0x3800000000000011
clean:
	rm -rf $(INTDIR) eboot.bin
