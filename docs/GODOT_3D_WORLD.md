# Mundo 3D da versão Godot — MINEPLUS

Este documento descreve o protótipo 3D Godot 4.3 do MINEPLUS. Ele é separado do cliente nativo PS4 em SDL2; não significa que o renderer 3D já esteja integrado ao PKG nativo.

## O que agora existe no protótipo 3D

- Terreno em chunks 16×16 com malhas de triângulos e colisão por chunk.
- Streaming limitado a um chunk novo por frame, descarte por distância e reaproveitamento de nós.
- Árvores 3D geradas deterministicamente por macro-células globais de 8×8, com propriedade de chunk para impedir duplicação nas bordas.
- Silhuetas de árvores de folhas largas, árvores antigas de copa em camadas, coníferas de tundra e árvores retorcidas de pântano.
- Raízes expostas, variação de copa e pequenos monólitos de pedra/cristal em florestas antigas e terras altas.
- Geometria de decoração agrupada em uma única malha visual por chunk, separada da malha de colisão do terreno. Folhas e galhos não viram obstáculos invisíveis.
- Ciclo de dia/noite contínuo de 15 minutos, com rotação/energia/cor do sol, céu procedural, luz ambiente e neblina atmosférica.
- Bloom/glow habilitado somente na qualidade gráfica alta.

## Decisões de performance

- As decorações usam SurfaceTool e malhas combinadas por chunk; não criam dezenas de MeshInstance3D ou corpos físicos por árvore.
- A geração é baseada em seed e coordenadas globais, então o resultado não depende da ordem de carregamento.
- O collider continua associado somente à superfície do terreno. A decoração visual não adiciona colisores.
- O streamer ainda constrói um chunk por frame e recicla nós; a próxima etapa deve medir tempo de geração, triângulos e memória em Android/PC antes de aumentar a densidade.
- Não há LOD de árvore ainda. Distâncias maiores devem futuramente usar silhuetas simplificadas e impostores.

## Limitações que permanecem

- As árvores são modelos voxelizados cúbicos construídos em runtime, não assets artísticos finais com atlas pintado, UVs detalhadas ou rigs.
- Não existem animações esqueléticas, vento físico nas copas, NPCs de aldeia ou animais 3D neste incremento.
- O terreno Godot ainda é uma superfície de altura, não um volume voxel completo com cavernas e blocos empilhados.
- O cliente nativo PS4 segue usando SDL2 e projeção pseudo-3D; este trabalho pertence ao protótipo Godot e não troca automaticamente o renderer do PKG.
- A iluminação e a neblina precisam de avaliação visual em dispositivos reais. CI headless valida parsing/instanciação, não qualidade de imagem nem FPS.

## Próximos marcos de alto impacto

1. Substituir a textura procedural mínima por atlas artesanal coerente, com materiais separados por face.
2. Converter ruínas, pontes, fazendas e moinhos em estruturas 3D modulares com colisão e interiores.
3. Criar personagens e criaturas com malhas próprias, esqueleto, animações e estados de IA.
4. Gerar volume voxel subterrâneo e paredes laterais reais, mantendo streaming e meshing por chunk.
5. Adicionar LOD, frustum culling e orçamento de triângulos para Android.
6. Criar testes visuais automatizados e medir FPS/memória em hardware alvo.

O objetivo é um mundo voxel dark fantasy com identidade própria, não uma cópia de assets ou marcas de outro jogo.

### Estruturas de superfície em malha 3D

- Âncoras PCG antigas agora são convertidas em modelos cúbicos 3D no chunk proprietário: árvores ancestrais, ruínas com paredes quebradas e pilares, e portais/portões de masmorra com degraus e pedra rúnica.
- A propriedade por chunk evita desenhar a mesma estrutura duas vezes; as malhas podem ultrapassar ligeiramente o limite do chunk para preservar a silhueta.
- Os modelos de ruína e entrada ainda são decoração visual sem colisão, interiores navegáveis, baús funcionais ou interação. A geometria de terreno continua com collider separado.

### Vento nas copas

- A malha de decoração usa um shader espacial com deslocamento de vértices de baixa amplitude, animado por TIME.
- O alpha da cor de vértice identifica folhas e musgo; troncos, pedra, cristais e ruínas permanecem rígidos.
- A animação é barata e compartilhada por chunk, sem criar nós ou corpos físicos extras. Não é uma simulação de vento nem uma animação esquelética.


### Entrada touch no mobile

- O gesto de olhar é associado ao índice do dedo que iniciou o toque na zona superior direita (acima de 78% da altura e à direita de 48% da largura).
- Somente arrastos desse mesmo dedo rotacionam a câmera; um segundo dedo usado nos controles não altera a visão.
- O toque inicial na zona de olhar não minera blocos. Mineração, colocação e pulo permanecem nos botões virtuais dedicados, evitando que um gesto de câmera consuma ferramentas ou altere o mundo.
- A captura é liberada quando o dedo correspondente é solto e também ao abrir inventário/pausa.
- O teste headless verifica a zona direita, a zona de movimento, a faixa inferior de ações, coordenadas fora da viewport e viewport sem dimensões. A CI não substitui teste de usabilidade em aparelho Android real.


### Persistência do ciclo dia/noite

- O save versão 7 registra a fase normalizada do relógio de 15 minutos junto com seed, alterações de terreno, blocos, inventário, ferramentas e posição do jogador.
- Ao carregar um mundo, o céu, a luz direcional e a neblina são atualizados imediatamente para a fase persistida, em vez de reiniciar sempre no mesmo horário.
- Saves até a versão 6 continuam migrando; quando não existe `day_clock`, é usada a fase padrão 0,5. Valores inválidos são substituídos pelo padrão e valores fora da faixa são limitados.
- Os testes headless verificam round-trip da fase, migração da versão 6, limite superior e fallback para dados inválidos. Isso valida o codec, não uma sessão de jogo prolongada em Android.
