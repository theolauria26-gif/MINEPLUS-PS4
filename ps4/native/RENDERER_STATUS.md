# Renderer status and voxel integration plan

## Current PS4 presentation path

- The live game uses SDL2's accelerated 2D renderer when available, with a software fallback.
- `draw_world()` performs a first-person height-field/ray-march over the resident 2D tile map. It projects terrain columns and creatures into a pseudo-3D scene.
- Material appearance is currently generated procedurally in `draw_material_texture()` and `tile_color()`; there is no shipped Hytale texture atlas or imported third-party art.
- The new stone/darkstone surface pattern uses staggered masonry joints; wood uses directional grain; obsidian and iron bars have dedicated highlights.
- The CPU voxel pipeline (`VoxelMesher`, `VoxelMeshVertexBuilder`, and `ProceduralVoxelChunkBuilder`) is not yet submitted to a GPU draw path.

## Overworld surface generation

- The deterministic PCG surface sampler now carves meandering river channels through moist temperate land, with sandy banks that transition into plains, forests and highlands.
- River fields use global coordinates and the world seed, so neighboring chunks and reloads reproduce the same channels. The biome identity remains intact for climate and later surface ecology.
- Surface voxel sampling reports river water as liquid and river banks as solid sand. This is CPU world-generation data consumed by the current surface tile map; it is not a new GPU water renderer.
- Inland lakes now use seed-derived macro-cell basins with varied radii and anisotropic shorelines; water lowers the local surface profile and nearby eligible terrain transitions to sand. Lake water is exposed as liquid voxel data and regression-tested as multi-sample pools across deterministic world instances. These remain CPU-generated surface tiles, not animated GPU water or simulated fluid.
- Surface elevation now combines broad ridged mountain-chain fields with a separate winding valley field. Mountain uplift influences both biome selection and column height, while valley carving lowers connected corridors; regression tests verify deterministic samples, highland presence and a meaningful elevation range. This shapes the existing height-field terrain and does not add true 3D cliff meshes or erosion simulation.
- Warm coastal shelves now transition into sandy beaches before the ocean boundary; cold regions retain their snow/ice surface and desert identity is kept separate. The coastline regression scan checks repeatability and solid sand voxel samples. Beaches use the current surface height-field, not a separate shoreline mesh or wave simulation.
- Forest biome trees now form broad deterministic groves with clearings; moist plains can contain sparse tree clusters. Desert, ocean, volcanic and crystal-waste regions do not receive these trees. Placement is derived from seed/global coordinates and is covered by portable PCG tests.
- Natural tree crowns now vary deterministically in width, depth and edge silhouette, producing compact, broad and wind-shaped canopy profiles within the same forest groves. The trunk remains the collision point and leaves remain walk-through; this is a tile-map silhouette variation, not skeletal foliage or wind simulation.
- Rare ancient trees now appear in Forest biome groves with broad, layered 4–5 tile crowns and irregular outer edges. Their placement and silhouette are derived from the world-coordinate hash, while ordinary forest trees and cold-biome conifers retain their existing shapes. This is still a 2D walk-through canopy silhouette around a single solid trunk, not a giant 3D tree model, branch system or new collision footprint.
- Grass material textures include sparse, deterministic wildflower flecks in several muted colors. They are visual pixels in the existing SDL material pass, not collectible flower entities or new saved tile types.
- Surface landmark generation now includes abandoned camps as a fifth deterministic structure family. The rasterized camp uses a dark-stone perimeter, timber crossbeams, moss patches and an ember hearth; its appearance is static in the current height-field renderer.
- Surface landmark generation now also includes ruined frontier watch-posts as a sixth deterministic family. Their footprint combines a broken dark-stone ring, timber lookout braces, an ancient-rune approach, crystal markers, moss and a central chest tile. These are static tile-map landmarks, not multi-level enterable buildings or animated towers.
- Nearby surface landmarks are now joined by deterministic nearest-neighbor dirt tracks. The generator selects stable anchor pairs within a bounded radius, draws a narrow sand-colored route between their clear perimeters, and leaves lake/lava crossings untouched. Roads are static terrain tiles in the current height-field renderer; there are no dynamic traffic, bridges, or pathfinding agents yet.
- Leaf canopy pixels now derive their palette from a larger world-coordinate cluster: most remain in the forest green range, while rare deterministic patches use muted amber or rust tones. This is a visual variation in the existing renderer, not a seasonal simulation or new foliage entity.
- Forest canopy now has a restrained wind pass: a sparse subset of leaf pixels shifts by at most one screen pixel on deterministic time buckets, with per-tree coordinate phase. The base texture remains world-anchored and the effect allocates no memory or per-pixel trigonometric calls.
- The surface renderer now places a dedicated leafy canopy tile around deterministic forest trunks. Canopy foliage is walk-through, has its own green pixel texture and raised height-field profile, and can be harvested for moss. This is still the existing SDL height-field renderer, not GPU foliage or wind animation.
- Initial creature placement now searches for valid land clearings, avoids liquid/solid landmark tiles and the spawn/boss arenas, then assigns creature families from the local biome (woodland/plains, desert/tundra, or highland/hostile regions). Failed bounded searches leave that creature slot inactive instead of embedding it in terrain.

## Requirements before true voxel rendering

1. Confirm the exact SDL2/OpenOrbis renderer capabilities and available GPU APIs in the pinned PS4 toolchain.
2. Add a PS4-compatible shader/texture upload path only after its API and linkage are verified.
3. Upload greedy chunk meshes and material atlas data, with explicit vertex/index budgets.
4. Add opaque and translucent passes separately; liquids are intentionally omitted from the current opaque chunk builder.
5. Integrate chunk visibility, camera transforms, depth testing, and rebuilds after player edits.
6. Keep the existing renderer as a fallback until the new path passes device testing.

## Honesty boundary

A successful CI PKG proves that the native application compiles and packages. It does not prove that voxel chunks are rendered in-game or that the package has been tested on a physical PS4. The current material-pattern improvements affect the existing height-field renderer; the CPU voxel mesh remains a separate pipeline.

## OpenOrbis GPU API audit

The pinned OpenOrbis toolchain source tree was inspected directly:

- `include/orbis/GnmDriver.h` declares low-level GNM entry points including `sceGnmDrawIndex`, `sceGnmDrawIndexAuto`, `sceGnmSetVsShader`, `sceGnmSetPsShader`, `sceGnmSubmitCommandBuffers`, and `sceGnmSubmitDone`.
- The declarations confirm that a native GNM draw/submit route exists in the toolchain. They do not, by themselves, provide a complete renderer: the header contains many untyped/empty declarations, and the project does not yet own command-buffer allocation, GPU memory mapping, shader binaries, pipeline state, render targets, synchronization, or presentation.
- The OpenOrbis SDL2 sample confirms SDL2 is packaged and linked as `-lSDL2`; it is a 2D SDL sample, not evidence of a voxel GPU mesh renderer.
- The current MINEPLUS Makefile links `-lSDL2` and PS4 system libraries, but not a GNM rendering module. No GNM calls are made by the game.

### Safe implementation sequence

1. Obtain and inspect the exact v0.5.4 SDK headers and import libraries used by CI, including GNM and kernel memory APIs; confirm exported symbols and signatures rather than relying on placeholder declarations.
2. Build a tiny isolated GNM triangle proof-of-concept with a known shader binary, command buffers, GPU-visible allocations and a video-out flip. Keep it outside the gameplay renderer until it compiles and is device-validated.
3. Only then add voxel vertex/index uploads, depth-tested opaque geometry, atlas sampling, and separate translucent passes.
4. Retain SDL2 as the fallback and UI path. Do not link speculative GNM calls into the shipping game.

This audit narrows the next task: the toolchain exposes low-level GNM draw/submit symbols, but a safe GPU renderer still needs a verified memory/command/shader/presentation foundation. A CI package alone cannot validate GPU execution on PS4 hardware.


## Surface wildlife ecology

- Surface wildlife now uses a standalone deterministic species catalog for the twelve existing creature IDs. The catalog gives each species a name, biome affinity, climate bounds and behavior role (grazer, skittish, predator or territorial).
- Initial placement now asks the ecology catalog to select a species compatible with the sampled biome, moisture, temperature and land surface. Ocean, water and sandy shoreline cells are rejected; if no species fits the sampled climate, the bounded spawn attempt is retried.
- Creature movement reads the catalog behavior role: grazers and skittish wildlife flee when approached, while predators and territorial species pursue. Distant grazers now loosely regroup toward nearby members of the same species when at least two herd companions are within seven tiles; movement remains deterministic and uses the existing tile-step collision checks. This is a lightweight local cohesion rule, not advanced pathfinding, persistent herd identity, pack tactics or a new rendered model.
- Portable tests cover all twelve species IDs, biome/climate eligibility, invalid surfaces, deterministic selection and behavior roles.


## Frontier settlements

- Added deterministic settlement site selection on temperate Plains/Forest clearings with moisture and temperature bounds and a five-point height/surface check.
- Each accepted site has one of four five-building layouts and a stable resident-count target (5–7). The current gameplay integration rasterizes five distinct house/workshop footprints, a central hearth and walkable doorways after the road pass.
- Each accepted settlement now populates 5–7 named residents at building entrances and around the central square. Five roles (elder, smith, herbalist, scout and warden) have distinct pseudo-3D silhouettes, short deterministic patrol schedules and three rotating dialogue lines each. Keyboard T or controller L3 talks to the nearest resident within two tiles.
- Resident movement is bounded to a two-tile patrol around each home and checks solid/liquid terrain plus other residents before stepping. Actors and dialogue are regenerated deterministically at world initialization; per-NPC schedules and dialogue stages are not persisted.
- Residents now offer role-based trades for coins: the elder exchanges runic relics, the smith offers metalwork, the herbalist trades natural materials, the scout offers exploration resources and the warden supplies frontier materials. Purchases validate coin balance and inventory capacity atomically; successful trades save immediately. Keyboard B opens trading, arrows select and Enter buys. On controller, approach a resident and press X to open/buy; D-pad selects and Circle closes. Offers are currently unlimited rather than depleting merchant stock.
- Five role-specific surface quests are now available through the same resident interaction (keyboard T or controller L3): the elder requests Rune Fragments, the smith Iron Bars, the herbalist Herbs, the scout Crystal Shards and the warden Bone Shards. Gathering matching materials while a quest is active advances its counter; returning to that resident claims the reward. Quest state and progress are appended to save version 12, with older versions defaulting to an empty quest journal.
- Timed overworld events rotate deterministically during active play: Caravan Market (25% trade discount), Harvest Festival (+1 bonus for frontier gathering materials), Wildlife Migration (faster movement and a wider roaming/flee radius for passive wildlife), Rune Storm (bonus runic/crystal materials) and Predator Warning (faster creature updates and a wider detection/pursuit radius for hostile species). Events last 60 seconds in a 180-second cycle and pause with the game/inventory; the HUD displays a live mm:ss countdown for the active event or the intermission. The schedule is session-time based and is not persisted.
- The SDL height-field renderer draws stylized billboard-like figures, not skeletal 3D character models. Dialogue stage and patrol schedules remain regenerated at world initialization; quest completion is persisted.

## Surface presentation milestone: event atmosphere and wildlife silhouettes

- The SDL scene now adds event-specific ambient overlays: descending cyan rune streaks during Rune Storm, drifting pollen/leaf motes during Harvest Festival, a pulsing edge warning during Predator Warning, a small moving market pennant during Caravan Market, and drifting motes during Wildlife Migration.
- The effects are deterministic in their spatial seeds and animated from the game clock; they allocate no per-frame particle objects and are drawn behind the first-person tool and HUD.
- Wildlife billboards now include a separately animated head, species-dependent ears and tails, and retain their idle bob/leg swing. This improves readability but does not turn the billboard figures into rigged 3D models or skeletal animation.
- This is a first visual pass toward the larger art/animation/atmosphere upgrade. It does not yet add imported texture assets, a texture atlas, full 3D meshes, GPU particles, dynamic lighting, shadows, water simulation, or a true voxel renderer.

## Biome-aware surface weather

- The overworld now selects a deterministic weather state in 90-second windows using the player's local biome, climate sample and world seed.
- Temperate plains/forests/highlands can receive rain, tundra and cold highlands can receive snowfall, and volcanic regions can receive drifting ash; other climates remain clear.
- Rain uses slanted droplets and rare, brief lightning flashes; snow uses drifting flakes; volcanic ash uses warm embers. Particle positions are derived from stable hashes and the game clock, with no per-frame particle allocation.
- This is a screen-space weather presentation layer. It does not yet change moisture, crop growth, visibility, movement, water levels, or world simulation, and it is not volumetric weather or a GPU particle system.

## Surface grass detail pass

- Grass material pixels now combine short two-pixel blade tufts, four restrained green palettes and occasional warm/cool wildflower flecks. Tuft positions are hash-derived from the tile texture coordinates so the pattern is repeatable and requires no particle/entity allocation.
- This is a material-detail pass in the existing SDL height-field renderer. It does not add collectible plant entities, new collision, a foliage mesh, wind simulation or imported texture assets.

## Forest-floor undergrowth

- World initialization now places deterministic irregular shrub clusters in forest biomes and in especially moist plains. The clusters reuse the existing walk-through leaf material, so they add visible undergrowth without introducing new item IDs, collision rules or save-format changes.
- Patches are generated from global tile coordinates and the world seed, keeping their placement stable when the same world is regenerated. This remains a 2D surface-tile representation, not a 3D shrub mesh or wind animation.

## Surface water shimmer

- Water material pixels now include a subtle time-shifted ripple crest, using the shared frame clock and tile texture coordinates. The highlight moves across visible river and lake columns without allocating particle objects.
- This is a lightweight screen-space material shimmer, not wave geometry, flow simulation, reflections, refraction or shoreline interaction.

## Dynamic surface sky cycle

- The current SDL height-field presentation now has a 480-second visual day/night cycle.
- Sky colors transition through daylight and warmer dawn/dusk palettes; a celestial disc travels across the horizon and switches from sun to moon styling at night.
- Horizon haze responds to the transition, while existing material emission remains animated.
- A deterministic field of small stars fades in as daylight falls; star positions are stable for the world seed and opacity follows the night level.
- This is a screen-space atmosphere pass for the current renderer, not a physically based sky, volumetric atmosphere, dynamic shadow system, or true 3D lighting solution.


## Layered surface canopy materials

- Leaf and shrub pixels now select among four restrained deterministic palettes (deep woodland, emerald, olive and muted bronze), with scattered brighter leaf tips and denser cluster patterns.
- Palette and highlight placement are hash-derived from the existing material texture pattern, so the pass adds canopy depth without extra entities, tile IDs, collision or save changes.
- This remains procedural SDL material detail on the existing height-field renderer; it is not imported foliage art, individual 3D leaves, seasonal simulation or wind animation.


## Moss and sand material refinement

- Moss now uses four restrained green variations with rare cool spore flecks, while sand adds deterministic fine-grain and pebble pixels. Both patterns are anchored to world/tile texture coordinates and reuse the existing SDL material pass.
- This is surface material detail only: it adds no new tile IDs, entities, collision, save fields, imported texture assets or 3D geometry.


## Narrow river road fords

- Deterministic landmark roads can now cross narrow water channels using a short sand causeway, while lava and water stretches wider than five tiles remain barriers. This makes some surface routes continuous without allowing roads to erase lakes or broad rivers.
- The ford reuses the existing walkable sand tile and therefore adds no new tile ID, save migration, bridge mesh or water simulation. It is a simple low-profile crossing, not a timber bridge model.
\n\n## Village courtyards and planted beds\n\n- Frontier settlements now receive a walkable sand footpath cross linking the central green to the building entrances, plus four small planted beds made from the existing leaf and moss materials. The beds add visual structure to the village square without introducing new tile IDs or blocking resident movement.\n- The courtyard is rasterized deterministically during world generation and remains part of the existing tile-map renderer; these are stylized surface plots, not crop simulation, harvestable plants, 3D props or animated farm machinery.\n

## Wildflower and fern micro-variation

- Grass material now mixes the existing small flower flecks with sparse fern-like fronds. Flower pixels use a distinct stem/bloom palette so they read as tiny plants rather than isolated color noise.
- Moss receives rare cool cyan mycelium glints in addition to its green patch variations, reinforcing the damp/arcane forest-floor identity.
- All details are generated from the existing deterministic material hash and current SDL pixel pass. They add no tile IDs, entities, collision, inventory items, save fields, per-frame allocations or imported assets; they are visual micro-details, not collectible flora or a simulated ecosystem.

## Cold-biome and alpine tree silhouettes

- Surface tree placement now includes sparse conifers in sufficiently moist tundra pockets and sheltered highland ledges. Forest groves and occasional moist-plains trees retain their existing densities; desert, ocean, volcanic and crystal-waste biomes remain excluded.
- The active SDL world initialization uses narrow, layered crown silhouettes for tundra/highland trees and the broader randomized crowns for temperate forest trees. Existing snow and stone ground tiles can host these trunks/canopies without adding material IDs.
- Portable PCG regression coverage now checks deterministic placement and confirms that tundra and highlands both contain tree candidates while the excluded biomes remain tree-free.
- This is still a 2D tile-map representation with billboard/column projection. It does not yet create species-specific 3D tree meshes, branches, roots, collision volumes or seasonal growth.


## Distinct moth and wisp silhouettes

- Glasswing Moths now render with two translucent fluttering wings and a pulsing luminous body accent. Pale Wisps receive a broken drifting halo and a faint trailing afterimage, separating their silhouettes from grounded fauna.
- Both effects use the existing species IDs, deterministic world placement and time-based SDL projection. No new save fields, tile IDs, entity types, imported assets or per-frame allocations were added.
- These remain stylized 2D projected sprites in the current renderer; they are not skeletal animations, volumetric particles or full 3D creature models.


## More recognizable surface-fauna silhouettes

- Mossback Grazers now have branching antlers; Glimmer Hares have upright ears; Frost Prowlers have a jagged icy dorsal ridge; Burrow Foxes have a fuller animated brush tail; and Shardbacks have three faceted back plates.
- These are lightweight additions to the existing projected SDL creature pass, keyed to existing species IDs. They do not add entity types, save data, tile IDs or imported assets.
- The silhouettes remain 2D projected shapes rather than rigged 3D models. Their animation is procedural screen-space motion, not skeletal animation.


## Vida luminosa noturna no exterior

- Florestas temperadas agora recebem vaga-lumes discretos durante a noite; os campos de cristais recebem partículas frias azuladas. A densidade e a intensidade acompanham a noite, e posições/deriva usam seed e tempo para evitar sorteios e alocações por frame.
- O efeito é desenhado em espaço de tela sobre o terreno, condicionado ao bioma na posição do jogador. É atmosfera visual leve, não entidades persistentes, IA de insetos, partículas volumétricas ou iluminação dinâmica real.


## Detalhes de margens de rios e lagos

- Bancos de areia em contato direto com água doce podem receber pequenos juncos e seixos. A colocação usa coordenadas globais e hash determinístico; a consulta aos quatro vizinhos evita vegetação em areia seca/desértica.
- O detalhe é desenhado no passe de textura SDL existente, sem novos IDs de bloco, entidades, alocações por pixel ou alterações no formato de save. Continua sendo decoração 2D projetada, não vegetação 3D com colisão.



## Formações naturais de rochas na superfície

- O mundo externo agora distribui pequenos campos de blocos rochosos em planícies, clareiras de floresta e encostas frias. Cada agrupamento deriva de coordenadas globais e hash determinístico, então permanece estável ao regenerar o mesmo seed.
- Os agrupamentos reutilizam o material Stone existente, evitam a área inicial e não sobrescrevem troncos, copas, água ou materiais especiais. Não introduzem IDs, entidades, dados de save ou assets externos.
- No cliente PS4 atual, são formações de terreno na grade 2D projetada; ainda não são pedregulhos 3D com malha, física ou fragmentação.

## Madeira levada pela correnteza

- Margens arenosas diretamente ligadas a água doce agora podem receber raros troncos curtos de madeira à deriva, com extremidade escura e veio claro. O padrão é determinístico por coordenada e complementa os juncos e seixos já existentes.
- Os troncos são detalhes desenhados no passe de textura, sem bloco novo, colisão, entidade persistente ou alteração de save. São decoração projetada em 2D, não troncos físicos flutuando com simulação de correnteza.

## Árvores mortas e galhos expostos nas florestas

- Clareiras de florestas podem receber árvores mortas esparsas, com troncos sem copa, galhos quebrados e alguns detalhes claros de madeira envelhecida. A colocação é determinística por coordenada, evita árvores próximas, o início e a arena do chefe.
- A decoração reutiliza os materiais Wood e Bone existentes, sem novos IDs, entidades, colisões especiais ou campos de save. No renderizador atual, são silhuetas da grade 2D projetada, não árvores 3D com física, queda ou decomposição simulada.
