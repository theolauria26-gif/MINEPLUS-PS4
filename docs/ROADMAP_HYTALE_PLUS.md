# MINEPLUS — Roteiro para um sandbox voxel de aventura

Este roteiro usa jogos voxel modernos de aventura como referência de qualidade e amplitude, mas mantém identidade, arte, código e conteúdo originais. A prioridade é transformar a base atual em um jogo realmente 3D e estável no PS4, não mascarar limitações com promessas.

## Diagnóstico verificado no código atual

- **Renderização:** o cliente PS4 usa SDL2 e um renderer de colunas/height-field em perspectiva. O mesher greedy e o builder de vértices já existem como módulos CPU testados, mas ainda não estão ligados a um renderer GPU 3D.
- **Materiais:** existe paleta por bloco e padrões pixel-art determinísticos. Há contratos CPU para atlas (UV com half-texel), orientação das seis faces e conversão de quads greedy em vértices com normais/UVs locais; ainda não há textura gráfica final, shader, upload GPU ou integração no renderer.
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
- Integrar mesher greedy, atlas, orientação UV e builder de vértices ao renderer GPU real; criar buffers/shaders e validar faces/UV em cena.
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


## Estado dos fundamentos de geometria (novembro de 2026)

- [x] Mesher greedy CPU para chunks 16×16×128 com halo e limite de quads.
- [x] Metadados do atlas 8×4, tiles 16×16 e UVs com half-texel.
- [x] Base de orientação UV das seis faces com testes de produto vetorial.
- [x] Builder CPU de quatro vértices por quad, normais externas e UVs locais repetíveis.
- [ ] Texturas pixel-art originais exportadas em atlas real.
- [ ] Shader/material sampling tile-aware, buffers GPU e renderer 3D integrado ao jogo.
- [ ] Streaming, colisão, edição e saves conectados ao mundo 3D jogável.

Esses módulos são fundação técnica, não uma mudança visual já disponível no jogo. A próxima etapa de maior impacto é a integração de GPU/renderização; em paralelo, o conteúdo precisa evoluir para uma aventura com progressão e mundo vivo.
