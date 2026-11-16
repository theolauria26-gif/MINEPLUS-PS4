# MINEPLUS — PCG e chunks infinitos

## Implementação adicionada

O protótipo Godot 4 agora usa dois módulos:

- `scripts/world_gen.gd` (`MineplusWorldGen`): geração determinística por coordenadas globais, ruído Simplex FBM em cinco oitavas, mapas de temperatura/umidade, biomas, oceano entre o continente central e terras exteriores e âncoras de estruturas.
- `scripts/chunk_streamer.gd` (`MineplusChunkStreamer`): streaming ao redor do jogador, malha/collider por chunk, fila de geração limitada a um chunk por frame, descarregamento e pool de até 12 nós de chunk.

O `scripts/world.gd` instancia o streamer, conecta o jogador, direciona mineração de terreno ao chunk correto e grava alterações do terreno no save versão 4. Saves anteriores continuam sendo lidos; quando não contêm seed/edições, são carregados com os valores padrão.

## Parâmetros atuais

| Parâmetro | Valor |
|---|---:|
| Seed inicial | 4839201 |
| Dimensão do chunk | 16 × 16 colunas |
| Raio de carregamento | 3 chunks |
| Área ativa nominal | 7 × 7 = 49 chunks |
| Raio do continente central | 420 blocos |
| Faixa oceânica | 76 blocos |
| Altura do mar | Y = 7 |
| Pool de chunks | 12 nós |

A geração não tem borda lógica: altura e bioma são funções de coordenadas globais e seed. O anel oceânico separa o continente central das macro-regiões exteriores. As estruturas usam âncoras em células de 64 blocos e cada chunk consulta as células vizinhas; por isso uma árvore ou ruína pode atravessar a borda sem ser gerada duas vezes ou ficar truncada.

## Como conectar

1. Abra o projeto Godot 4.2+ pelo `project.godot`.
2. A cena `scenes/world.tscn` já aponta para `scripts/world.gd`; não é necessário adicionar nós manualmente.
3. Ao entrar no mundo, `world.gd` cria `MineplusChunkStreamer`, define `player` depois de instanciar o personagem e o streamer carrega os chunks próximos.
4. A seed e as edições do terreno são guardadas em `user://mineplus_world.json` (ou no slot correspondente).
5. Para alterar escala/performance, edite `CHUNK_SIZE`, `LOAD_RADIUS`, `UNLOAD_RADIUS` em `chunk_streamer.gd`, e os raios/ruídos em `world_gen.gd`.

## Biomas e estruturas

No continente central, temperatura e umidade classificam planícies, florestas, desertos, tundras, pântanos e terras altas. Nas terras exteriores, uma hash de macro-célula escolhe biomas por coordenadas; a mesma seed sempre produz o mesmo resultado. A faixa oceânica é uma zona de transição entre as duas regiões.

As âncoras de árvore gigante e ruína são calculadas a partir da seed e de coordenadas de macro-célula. A função `surface_at(x,z)` avalia as âncoras vizinhas, mesmo quando estão fora do chunk em construção. Assim, todos os chunks concordam sobre a mesma estrutura.

## Performance: o que já está aplicado e o próximo passo

- **Já aplicado:** só os chunks próximos existem na cena; cada chunk tem uma malha e um collider, em vez de um corpo físico por bloco.
- **Já aplicado:** a fila prioriza chunks mais próximos do jogador e constrói no máximo um por frame para limitar picos de tempo.
- **Já aplicado:** descarregamento de chunks distantes e pool de 12 raízes com MeshInstance/StaticBody/CollisionShape reutilizados.
- **Próximo passo para multithreading:** mover apenas o cálculo puro de alturas, biomas, tipos e arrays de vértices para `WorkerThreadPool`/threads. Não criar, alterar ou liberar Nodes, Meshes, materiais ou CollisionShapes dentro da thread; enviar os arrays prontos para a thread principal e montar os recursos lá.
- **Próximo passo para maior escala:** separar malha visual e colisão, gerar colisão apenas perto do jogador, adicionar LOD para chunks distantes e usar um cache de amostras de altura para não recalcular ruído nas bordas.

## Limitações honestas desta etapa

Esta etapa gera terreno de superfície em colunas com relevo em degraus, não um voxel 3D completo com cavernas, blocos subterrâneos editáveis, fluidos simulados ou iluminação por voxel. A mineração atual reduz a altura da coluna em um bloco e persiste a edição. As árvores gigantes e ruínas são marcos de superfície simplificados. O sistema foi integrado ao projeto Godot; o cliente PS4 SDL2 em `ps4/native/` é um port separado e ainda não consome estes scripts GDScript.
