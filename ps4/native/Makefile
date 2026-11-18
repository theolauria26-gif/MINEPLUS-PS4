TITLE := MINEPLUS
VERSION := 1.00
TITLE_ID := MNPX00001
CONTENT_ID := IV0000-MNPX00001_00-MINEPLUS00000001
TOOLCHAIN := $(OO_PS4_TOOLCHAIN)
INTDIR := build
CXX := clang++
LD := ld.lld
CXXFLAGS := --target=x86_64-pc-freebsd12-elf -std=c++17 -D_POSIX_C_SOURCE=200809L -fPIC -funwind-tables -c -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include -isystem $(TOOLCHAIN)/include/c++/v1
LIBS := -lc -lkernel -lc++ -lSceUserService -lSceVideoOut -lSceAudioOut -lScePad -lSceSysmodule -lSDL2
LDFLAGS := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr -L$(TOOLCHAIN)/lib $(LIBS) $(TOOLCHAIN)/lib/crt1.o
.PHONY: all clean
all: eboot.bin
$(INTDIR):
	mkdir -p $(INTDIR)
$(INTDIR)/main.o: main.cpp pcg_world.hpp world_grid.hpp world_runtime.hpp dungeon_loot.hpp dungeon_graph.hpp equipment.hpp boss_combat.hpp surface_ecology.hpp surface_settlements.hpp surface_npcs.hpp surface_trade.hpp surface_quests.hpp surface_events.hpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ main.cpp
$(INTDIR)/pcg_world.o: pcg_world.cpp pcg_world.hpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ pcg_world.cpp
$(INTDIR)/chunk_generator.o: chunk_generator.cpp chunk_generator.hpp pcg_world.hpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ chunk_generator.cpp
$(INTDIR)/chunk_cache.o: chunk_cache.cpp chunk_cache.hpp chunk_generator.hpp pcg_world.hpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ chunk_cache.cpp
$(INTDIR)/save_codec.o: save_codec.cpp save_codec.hpp chunk_cache.hpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ save_codec.cpp
$(INTDIR)/world_runtime.o: world_runtime.cpp world_runtime.hpp chunk_cache.hpp save_codec.hpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ world_runtime.cpp
$(INTDIR)/voxel_mesher.o: voxel_mesher.cpp voxel_mesher.hpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ voxel_mesher.cpp
$(INTDIR)/material_atlas.o: material_atlas.cpp material_atlas.hpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ material_atlas.cpp
$(INTDIR)/procedural_material_atlas.o: procedural_material_atlas.cpp procedural_material_atlas.hpp material_atlas.hpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ procedural_material_atlas.cpp
$(INTDIR)/voxel_face_uv.o: voxel_face_uv.cpp voxel_face_uv.hpp voxel_mesher.hpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ voxel_face_uv.cpp
$(INTDIR)/voxel_mesh_vertices.o: voxel_mesh_vertices.cpp voxel_mesh_vertices.hpp voxel_face_uv.hpp material_atlas.hpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ voxel_mesh_vertices.cpp
$(INTDIR)/voxel_mesh_indices.o: voxel_mesh_indices.cpp voxel_mesh_indices.hpp voxel_mesher.hpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ voxel_mesh_indices.cpp
$(INTDIR)/dungeon_loot.o: dungeon_loot.cpp dungeon_loot.hpp dungeon_graph.hpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ dungeon_loot.cpp
$(INTDIR)/equipment.o: equipment.cpp equipment.hpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ equipment.cpp
$(INTDIR)/boss_combat.o: boss_combat.cpp boss_combat.hpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ boss_combat.cpp
$(INTDIR)/surface_ecology.o: surface_ecology.cpp surface_ecology.hpp pcg_world.hpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ surface_ecology.cpp
$(INTDIR)/surface_settlements.o: surface_settlements.cpp surface_settlements.hpp pcg_world.hpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ surface_settlements.cpp
$(INTDIR)/surface_npcs.o: surface_npcs.cpp surface_npcs.hpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ surface_npcs.cpp
$(INTDIR)/surface_trade.o: surface_trade.cpp surface_trade.hpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ surface_trade.cpp
$(INTDIR)/surface_quests.o: surface_quests.cpp surface_quests.hpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ surface_quests.cpp
$(INTDIR)/surface_events.o: surface_events.cpp surface_events.hpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ surface_events.cpp
$(INTDIR)/dungeon_graph.o: dungeon_graph.cpp dungeon_graph.hpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ dungeon_graph.cpp
$(INTDIR)/dungeon_voxel_builder.o: dungeon_voxel_builder.cpp dungeon_voxel_builder.hpp dungeon_graph.hpp voxel_mesher.hpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ dungeon_voxel_builder.cpp
$(INTDIR)/procedural_voxel_chunk.o: procedural_voxel_chunk.cpp procedural_voxel_chunk.hpp pcg_world.hpp voxel_mesher.hpp voxel_mesh_vertices.hpp voxel_mesh_indices.hpp world_runtime.hpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ procedural_voxel_chunk.cpp
$(INTDIR)/mineplus.elf: $(INTDIR)/main.o $(INTDIR)/pcg_world.o $(INTDIR)/chunk_generator.o $(INTDIR)/chunk_cache.o $(INTDIR)/save_codec.o $(INTDIR)/world_runtime.o $(INTDIR)/voxel_mesher.o $(INTDIR)/material_atlas.o $(INTDIR)/procedural_material_atlas.o $(INTDIR)/voxel_face_uv.o $(INTDIR)/voxel_mesh_vertices.o $(INTDIR)/voxel_mesh_indices.o $(INTDIR)/dungeon_graph.o $(INTDIR)/dungeon_voxel_builder.o $(INTDIR)/procedural_voxel_chunk.o $(INTDIR)/dungeon_loot.o $(INTDIR)/equipment.o $(INTDIR)/boss_combat.o $(INTDIR)/surface_ecology.o $(INTDIR)/surface_settlements.o $(INTDIR)/surface_npcs.o $(INTDIR)/surface_trade.o $(INTDIR)/surface_quests.o $(INTDIR)/surface_events.o
	$(LD) $^ -o $@ $(LDFLAGS)
eboot.bin: $(INTDIR)/mineplus.elf
	$(TOOLCHAIN)/bin/linux/create-fself -in=$< -out=$(INTDIR)/mineplus.oelf --eboot "$@" --paid 0x3800000000000011
clean:
	rm -rf $(INTDIR) eboot.bin
