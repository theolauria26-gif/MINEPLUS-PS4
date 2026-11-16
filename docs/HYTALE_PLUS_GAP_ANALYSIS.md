# MINEPLUS — análise de lacunas para superar a referência de Hytale

**Data da pesquisa:** 16 de novembro de 2026  
**Escopo:** comparação de pilares de jogo e sistemas publicados oficialmente pela equipe de Hytale. A meta é criar uma obra original; não copiar nomes, personagens, texturas, mapas, código ou assets proprietários.

## O que a referência oferece ou está desenvolvendo

A página oficial de jogo descreve um sandbox RPG centrado em exploração procedural, dungeons, criaturas, construção, agricultura, crafting, eventos aleatórios, pontos de interesse e mundos com rios, cachoeiras e cavernas. A página também cita ferramentas para criadores, minigames e servidores comunitários. Fonte: https://hytale.com/game

A atualização oficial de outubro de 2026, *Chapter I: Out of the Breach*, destaca uma dungeon artesanal com salas conectadas, desafios ambientais, puzzles, checkpoints, inimigos de elite e chefe de múltiplas fases. Fonte: https://hytale.com/news/2026/10/chapter-i-out-of-the-breach

O artigo oficial de julho de 2026 sobre Chapter 1 apresenta o Encounter Manager para coordenar fases, ondas, mudanças de arena e encontros; também descreve WorldGen V2 baseada em grafos para cavernas mais imersivas e, futuramente, dungeons procedurais, vilas/cidades coerentes, rios e estradas. O mesmo artigo destaca revisão de modelos, texturas e animações de criaturas. Fonte: https://hytale.com/news/2026/7/first-look-chapter-1-and-more

A comunicação oficial sobre WorldGen V2 reforça geração modular e coerência de biomas, com planos para rios, caminhos, arquitetura procedural, ruínas, vegetação e cachoeiras. Fonte: https://hytale.com/news/2026/1/the-future-of-world-generation/

## Comparação com o MINEPLUS verificado no repositório

| Pilar | Situação atual do MINEPLUS | Trabalho necessário |
|---|---|---|
| Mundo voxel 3D | Mesher e builder de vértices CPU, ainda desconectados do renderer; jogo ainda desenha height-field | Renderer GPU cúbico, shaders, buffers, chunks 3D jogáveis |
| Mundo procedural | PCG, perfis, cavernas primordiais e estruturas já existem | Biomas mais distintos, transições, rios, lagos, estradas, POIs encadeados |
| Cavernas | Geração subterrânea e câmaras definidas em subsistema | Túneis/câmaras navegáveis em 3D, ecossistemas, ruínas, puzzles e chefes |
| Dungeons | Estruturas procedurais básicas | Layouts por salas/grafo, chaves, armadilhas, checkpoints, encontros e recompensas |
| Criaturas | Elenco básico, movimento/combate simples e silhuetas desenhadas | IA por espécie, estados, percepção, rotinas, grupos, animação esquelética e áudio |
| Chefes | Não há sistema completo de encontros multi-fase | Encounter director orientado a dados, telegraphs, fases, arenas e loot |
| Progressão | Inventário e receitas iniciais | Equipamentos por tier, atributos, habilidades, estações, quests e objetivos |
| Mundo vivo | Sem ecossistema completo de NPCs | Vilas, facções, comércio, relações, rotinas, eventos e histórias ambientais |
| Sobrevivência | Coleta e crafting iniciais | Agricultura completa, culinária, domesticação, pesca, clima e estações |
| Construção | Colocar/quebrar blocos básicos | Construção 3D livre, ferramentas, prefabs, decoração e estruturas funcionais |
| Criadores | Não há pipeline público de mods | Formatos versionados, prefabs, scripts/dados, ferramentas de criação |
| Multiplayer | Não implementado | Arquitetura de autoridade, replicação, sincronização e persistência |

## Ordem recomendada para MINEPLUS 2.0

### A — Tornar o mundo realmente 3D
1. Escolher e validar uma API gráfica suportada pela build OpenOrbis/SDL2 atual.
2. Integrar vértices, atlas e shader com repetição de textura por tile.
3. Renderizar chunks cúbicos com iluminação, sombras/fog e transparência.
4. Ligar streaming, colisão, mineração, construção e save ao mesmo mundo 3D.
5. Validar no PS4 real antes de substituir o renderer antigo.

### B — Fazer a exploração gerar histórias
1. Gerador de POIs por grafo: entrada, salas, conexões, bloqueios, segredos e saída.
2. Ruínas e dungeons temáticas por bioma e profundidade.
3. Encontros dirigidos por dados: ondas, fases, arena, boss, checkpoints e recompensa.
4. Eventos emergentes no mundo: caravanas, ataques, acampamentos, criaturas migratórias e descobertas.

### C — Dar vida ao mundo
1. IA com estados e necessidades por espécie.
2. Animação com locomotion, ataques, dano, morte, interação e variação por criatura.
3. NPCs com rotina, diálogo, comércio, facções e missões.
4. Agricultura, culinária, domesticação, pesca, clima e ciclos ambientais.

### D — Profundidade e rejogabilidade
1. Progressão por exploração e equipamentos, não apenas mineração repetitiva.
2. Chefes opcionais e principais, biomas de alto risco e loot único.
3. Desafios configuráveis, modo hardcore, construção avançada e conteúdo criativo.
4. Ferramentas de modding e multiplayer somente depois da base de mundo/saves estar sólida.

## Diferencial próprio que pode levar além

- Cavernas primordiais em camadas, com ecossistemas e história ambiental próprios.
- Mundo com regiões que evoluem: assentamentos crescem, rotas mudam e ameaças ocupam áreas.
- Dungeons sistêmicas e recombináveis, não apenas salas aleatórias sem lógica.
- Criaturas com comportamento emergente, relações predador/presa e ciclos dia/noite.
- Progressão com escolhas reais, relíquias, habilidades e builds distintas.
- Ferramentas de criação dentro do próprio jogo, para construir e compartilhar aventuras.

## Critério de honestidade técnica

Não declarar um pilar como concluído porque existe apenas um módulo isolado. Cada sistema precisa estar conectado ao gameplay, ter testes, passar no build do PKG e ser validado visualmente no console. O tamanho do PKG não é uma meta de qualidade: conteúdo, variedade, profundidade e apresentação são.
