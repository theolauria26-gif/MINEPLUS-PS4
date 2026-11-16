# PCG nativo do MINEPLUS para PS4

O módulo procedural é C++17 puro e faz parte do cliente SDL2/OpenOrbis. Não depende de Godot, Unity ou runtime externo.

## Geração

- Seed determinística (padrão: `4839201`).
- Ruído interpolado e FBM multi-octave para relevo, temperatura e umidade.
- Oito biomas: oceano, planícies, floresta, deserto, tundra, terras altas, vulcânico e ermos cristalinos.
- Máscara de continente central e transição oceânica; as coordenadas continuam determinísticas além da região central.
- Âncoras de estruturas calculadas a partir de células globais, consultando vizinhos para evitar decisões diferentes nas bordas dos chunks.
- Consultas puras do gerador não dependem de estado SDL e podem ser executadas em workers.

## Camadas implementadas

- `pcg_world.hpp/.cpp`: hash, ruído, FBM, amostras de superfície/voxels e âncoras.
- `chunk_generator.hpp/.cpp`: chunks de 16×16 colunas, com coordenadas globais assinadas e divisão matemática correta para coordenadas negativas.
- `chunk_cache.hpp/.cpp`: cache LRU de 81 chunks, journal de até 2048 alterações de superfície e 4096 alterações de voxel. Edições não são descartadas silenciosamente quando o journal enche.
- `world_runtime.hpp/.cpp`: API de gameplay para atualizar centro, consultar/editar tiles e voxels e serializar snapshots.
- `world_grid.hpp`: contrato `WorldGridView`, adaptador fixo legado e adaptador `StreamedWorldGrid`.
- `save_codec.hpp/.cpp`: formato binário little-endian versionado, com checksum e validação antes de aceitar um snapshot.
- `tests/`: testes de determinismo, coordenadas negativas, fronteiras, eviction LRU, persistência de edições, voxels e saves corrompidos.

## Estado da integração com o jogo

A infraestrutura de chunks/runtime é compilada pelo Makefile e exercitada no workflow de testes, mas o loop SDL em `main.cpp` ainda usa o array residente de 2048×2048 células. Portanto:

1. O gerador pode consultar coordenadas globais sem limite de mapa.
2. O cache e a API streamed preservam alterações ao regenerar chunks.
3. **O cliente jogável ainda não faz streaming em torno do jogador**; a janela fixa permanece como armazenamento ativo.
4. O save do loop principal ainda usa o formato legado versionado (atualmente até versão 8). O `SaveCodec` v2 é uma camada separada e ainda não migra automaticamente os saves do gameplay.
5. As âncoras procedurais ainda não são prefabs completos renderizados no mundo streamed.

Não anuncie o mapa como infinito até a troca do armazenamento do gameplay, a migração de saves e os testes no console estarem concluídos.

## Parâmetros atuais

- Chunk: 16×16 colunas.
- Altura lógica: 128 níveis.
- Raio aproximado do continente central: 4200 unidades.
- Transição oceânica: aproximadamente 900 unidades.
- Seed padrão: 4839201.

## Próximas etapas seguras

1. Integrar `StreamedWorldGrid` ao gameplay por trás do contrato já usado por mineração, construção, colisão e renderização.
2. Migrar o save legado sem perder seed, posição, criaturas, inventário nem alterações feitas pelo jogador; manter leitura dos formatos antigos.
3. Aplicar orçamento de chunks por frame e geração assíncrona sem criar recursos SDL/GPU em workers.
4. Renderizar estruturas e volumes subterrâneos a partir de dados de chunk, com limites de memória medidos.
5. Medir FPS, RAM, tempo de carregamento e estabilidade no PS4 real antes de aumentar o raio ou a densidade.
