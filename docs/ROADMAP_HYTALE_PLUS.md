# MINEPLUS — Roteiro para um sandbox voxel de aventura

Este roteiro usa jogos voxel modernos de aventura como referência de qualidade e amplitude, mas mantém identidade, arte, código e conteúdo originais. A prioridade é transformar a base atual em um jogo realmente 3D e estável no PS4, não mascarar limitações com promessas.

## Diagnóstico verificado no código atual

- **Renderização:** o cliente PS4 usa SDL2 e um renderer de colunas/height-field em perspectiva. Ainda não renderiza malhas 3D de voxels, faces cúbicas com UV ou atlas de materiais.
- **Materiais:** existe paleta por bloco e uma primeira camada de padrões pixel-art determinísticos desenhados em tempo real. Ainda não há arquivos de textura, atlas UV, mipmaps ou materiais por face.
- **Mundo:** a geração procedural consulta coordenadas globais, mas o loop jogável ainda mantém uma janela residente fixa de 2048×2048. O streaming por chunks existe como subsistema, mas não substitui o armazenamento do gameplay.
- **Verticalidade:** há amostras voxel 3D e cavernas procedurais no gerador, mas o cliente principal ainda não oferece exploração 3D completa de túneis e câmaras.
- **Conteúdo:** existem biomas, estruturas, inventário, receitas, criaturas e combate básico. Faltam variedade de comportamento, animações, áudio, progressão longa, NPCs/aldeias completas, missões e eventos sistêmicos.
- **Plataforma:** o GitHub Actions compila EBOOT/PKG e roda testes portáveis. Isso não substitui testes de instalação, FPS, memória, salvamento e estabilidade em hardware PS4 real.

## Plano de evolução

### P0 — Fundamentos visuais e estabilidade
- Consolidar paleta legível e padrões de material específicos por bloco.
- Criar atlas pixel-art original com variantes de superfície, laterais e topo; UV por face e filtragem nearest-neighbor.
- Adicionar iluminação direcional por face, fog por distância, oclusão ambiente econômica e ciclo dia/noite.
- Definir orçamento de frame, memória e draw calls para PS4; evitar efeitos caros sem medição.
- Automatizar validação de assets e testes de determinismo.

### P1 — Mundo 3D de verdade
- Trocar o height-field pelo renderer de chunks cúbicos.
- Construir malhas por chunk com remoção de faces ocultas e greedy meshing.
- Integrar streaming ao movimento, colisão, mineração, construção e persistência.
- Migrar saves preservando mundos antigos, seed, posição, criaturas, inventário e blocos alterados.
- Carregar e descarregar chunks com orçamento por frame; geração em worker sem chamadas SDL/GPU.

### P2 — Mundo vivo
- Estruturas compostas por salas conectadas, ruínas, vilas e pontos de interesse com regras por bioma.
- Rios, lagos, cachoeiras, vegetação em camadas, árvores com silhuetas variadas e transições graduais entre biomas.
- Criaturas com estados de IA, rotinas, percepção, fuga/caça, grupos, animação e drops equilibrados.
- Clima, partículas, sons ambientais, passos por material, música adaptativa e eventos locais.

### P3 — Aventura e progressão
- Combate com telegraph de ataques, esquiva, bloqueio, armas e chefes com padrões.
- Equipamentos, atributos, ferramentas, estações de crafting e progressão com objetivos.
- NPCs, facções, aldeias, comércio, missões, lore ambiental e recompensas de exploração.
- Agricultura, criação de animais, pesca e construção decorativa.

### P4 — Polimento e expansão
- Ferramentas criativas, prefabs editáveis e modding com formatos versionados.
- Multiplayer somente após autoridade do mundo, sincronização, segurança e persistência estarem desenhadas.
- Perfil de desempenho no PS4 base e Pro, testes de longa duração e regressão de saves.

## Critério de conclusão

Nenhum sistema é considerado pronto apenas porque compila. Cada etapa precisa de testes automatizados, evidência de build, validação visual e, para desempenho/estabilidade, teste no PS4 real. O renderer atual e o streaming não devem ser descritos como 3D/infinito enquanto a integração jogável não estiver concluída.
