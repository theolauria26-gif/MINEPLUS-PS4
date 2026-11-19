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


## Cavernas Primordiais (geração subterrânea)

As âncoras de marcos agora definem complexos subterrâneos determinísticos de cinco câmaras: uma sala central e quatro alas cardeais ligadas por corredores em cruz. Os quatro perfis derivados do tipo da âncora variam proporções das salas e largura dos corredores, sem perder a topologia conectada. O teto e o piso permanecem em DarkStone; veios raros de cristal aparecem nas áreas escavadas sem bloquear a passagem. A forma depende apenas da seed e das coordenadas globais, mantendo continuidade nas bordas dos chunks. Isso é geração de dados voxel: renderização 3D, iluminação volumétrica, criaturas e estruturas decorativas completas ainda precisam ser integradas ao gameplay.


## Direção visual e materiais procedurais

O renderer nativo agora usa uma paleta mais saturada e motivos pixel-art determinísticos por material. Grama recebe pequenos traços de vegetação; terra, pontos minerais; pedra, estratos; madeira, veios; musgo, manchas; cristal e runas, brilhos; areia, neve, gelo, água e lava recebem padrões próprios. As marcas são ancoradas em coordenadas do mundo para evitar que a textura deslize com a câmera.

**Limite atual:** estes são detalhes procedurais desenhados pelo renderer de colunas SDL2, não um atlas de texturas 3D pintadas nem um renderer de malhas voxel com iluminação por face. O próximo salto visual exige chunks 3D renderizados, atlas UV, modelos cúbicos, sombras/oclusão e efeitos com orçamento de GPU/CPU medido no PS4.


## Malhador de chunks voxel (base 3D)

O módulo `voxel_mesher.hpp/.cpp` converte dados de um chunk 16×16×128 em retângulos de faces expostas. Ele percorre os seis sentidos, usa uma máscara 2D por plano e mescla faces coplanares de mesmo material; o halo de um voxel permite ocultar faces compartilhadas com chunks vizinhos. O resultado é geometria compacta em coordenadas locais, ainda sem upload para GPU ou integração no renderer SDL do gameplay.

A API aceita um limite `max_quads` (padrão 65.536). Se o limite for excedido, retorna falha e limpa a saída, evitando que o chamador use uma malha parcial. O limite pode ser reduzido conforme o orçamento de memória medido no PS4. O CI compila o módulo com `-Werror` e executa testes de faces isoladas, fusão de blocos, materiais diferentes, volumes sólidos e halo de fronteira.


## Propriedade rural medieval

- Uma nova família de marco exterior gera uma pequena propriedade rural: celeiro de madeira e pedra escura, cerca perimetral com portão, caminho de acesso, canteiros em faixas e poço com água.
- A planta usa a seed e a âncora global; o mesmo terreno é reproduzido em qualquer ordem de carregamento. Os canteiros são composição visual de tiles existentes, não cultivo colhível, NPCs, economia ou simulação agrícola.
- A estrutura é desenhada pelo mapa 2D atual. Não representa ainda um celeiro 3D com telhado/interior navegável.


## Moinho medieval no mundo exterior

A família de marcos 11 pode gerar um moinho rural com torre de pedra escura, rotor de quatro pás em madeira e areia, pátio cercado, trilha de acesso e pequeno depósito de grãos. A orientação visual e a posição são derivadas da seed e da célula global, mantendo a estrutura estável entre gerações. No cliente atual, o rotor é uma silhueta estática de tiles 2D: não gira, não mói grãos e não possui interior 3D ou economia agrícola simulada.


## Entradas rurais conectadas à malha de estradas

Os portões da propriedade rural e do moinho agora escolhem a face voltada para o marco vizinho indicado pelo grafo determinístico de estradas. O celeiro da fazenda é reposicionado para o lado oposto à entrada, e o caminho interno acompanha o eixo do portão. Quando não existe ligação de estrada para a âncora, a orientação sul permanece como fallback estável. A representação ainda é uma composição 2D de tiles; não cria navegação de NPC nem uma estrada física 3D.
