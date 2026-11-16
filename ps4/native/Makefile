TITLE := MINEPLUS
VERSION := 1.00
TITLE_ID := MNPX00001
CONTENT_ID := IV0000-MNPX00001_00-MINEPLUS00000001
TOOLCHAIN := $(OO_PS4_TOOLCHAIN)
INTDIR := build
CXX := clang++
LD := ld.lld
CXXFLAGS := --target=x86_64-pc-freebsd12-elf -std=c++17 -D_POSIX_C_SOURCE=200809L -fPIC -funwind-tables -c -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include -isystem $(TOOLCHAIN)/include/c++/v1
LIBS := -lSDL2 -lSceUserService -lSceVideoOut -lSceAudioOut -lScePad -lSceSysmodule -lkernel -lc++ -lc
LDFLAGS := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr -L$(TOOLCHAIN)/lib $(LIBS) $(TOOLCHAIN)/lib/crt1.o
.PHONY: all clean
all: eboot.bin
$(INTDIR):
	mkdir -p $(INTDIR)
$(INTDIR)/main.o: main.cpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ $<
$(INTDIR)/mineplus.elf: $(INTDIR)/main.o
	$(LD) $^ -o $@ $(LDFLAGS)
eboot.bin: $(INTDIR)/mineplus.elf
	$(TOOLCHAIN)/bin/linux/create-fself -in=$< -out=$(INTDIR)/mineplus.oelf --eboot "$@" --paid 0x3800000000000011
clean:
	rm -rf $(INTDIR) eboot.bin
