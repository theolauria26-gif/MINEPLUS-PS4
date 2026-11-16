# PCG nativo do MINEPLUS para PS4

Este módulo é C++17 puro e faz parte do cliente SDL2/OpenOrbis. Não depende de Godot, Unity ou runtime externo.

## Estado atual da integração

- Seed fixa e determinística: 4839201.
- Ruído interpolado + FBM de múltiplas oitavas para relevo, temperatura e umidade.
- Oito biomas: oceano, planícies, floresta, deserto, tundra, terras altas, vulcânico e ermos cristalinos.
- Máscara de continente central e transição oceânica; coordenadas além dela continuam gerando biomas.
- Âncoras de estruturas derivadas de células globais, consultando células vizinhas para evitar cortes lógicos entre chunks.
- Gerador stateless, seguro para consultas concorrentes sem estado mutável compartilhado.

## Arquivos

- pcg_world.hpp: API e estruturas de dados.
- pcg_world.cpp: hash, ruído, FBM, biomas, elevação e âncoras.
- main.cpp: inicializa a janela residente com amostras do PCG.
- Makefile: compila o módulo no EBOOT PS4.

## Limitação desta etapa

O gerador é infinito por coordenadas, mas o cliente ainda mantém uma janela residente fixa de 2048×2048 células. Portanto, o mapa jogável ainda não é infinito na prática: falta substituir o array global por cache de chunks com carregamento/descarregamento ao redor do jogador. Estruturas ainda são âncoras/silhuetas de superfície, não prefabs completos nem volumes subterrâneos editáveis.

## Arquitetura de streaming prevista

1. Cada chunk contém 16×16 colunas e usa coordenadas inteiras globais.
2. Divisão de coordenadas negativas usa floor division, não truncamento para zero.
3. Workers recebem seed e coordenadas e calculam arrays de altura, bioma, material e estruturas. Não chamam SDL.
4. A thread principal consome chunks prontos com orçamento por frame e atualiza buffers de renderização.
5. Chunks fora do raio de visão retornam a um pool; saves guardam seed e deltas do jogador, não o mundo inteiro.
6. Estruturas são decididas em células macro globais e consultam vizinhos antes de preencher o chunk.
7. Nunca criar/destruir objetos SDL, texturas ou recursos de GPU na worker thread.

## Parâmetros iniciais

- Chunk: 16×16 colunas.
- Altura lógica: 128 unidades.
- Raio aproximado do continente central: 4200 unidades.
- Faixa de transição oceânica: aproximadamente 900 unidades.
- Seed padrão: 4839201.

A próxima etapa é trocar o armazenamento fixo por cache de chunks e adaptar mineração/salvamento para deltas por coordenada. Até isso passar no build e em teste no console, o mapa infinito não deve ser considerado concluído.


## Gerador de chunks nativo (primeira camada)

`chunk_generator.hpp/.cpp` adiciona `ChunkData` e `ChunkGenerator`. Cada chunk contém 16×16 amostras de superfície e referências determinísticas às estruturas que cruzam suas células. A geração usa coordenadas globais com `int64_t` para calcular a origem, e `floor_div` corrige a divisão de coordenadas negativas (por exemplo, -1 pertence ao chunk -1, não ao chunk 0).

Este módulo já é compilado no EBOOT pelo Makefile. Nesta etapa ele ainda é uma camada de geração isolada: o loop de jogo e as rotinas de mineração/salvamento continuam usando o array residente antigo. Não confundir o gerador de chunks compilado com streaming ativo no console.
