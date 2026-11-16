# MINEPLUS PS4

Projeto focado exclusivamente no cliente nativo de PlayStation 4: C++17, SDL2 e toolchain OpenOrbis. O alvo é um jogo de exploração/sobrevivência com identidade dark-fantasy, mundo procedural, criaturas, construção, inventário e estruturas próprias.

## Build PS4

O workflow .github/workflows/ps4-native.yml compila o EBOOT e empacota o PKG usando OpenOrbis. O artefato MINEPLUS-PS4-PKG é publicado no GitHub Actions após um build concluído. O pacote ainda precisa ser testado no console; sucesso do CI não substitui teste real.

## Sistemas no cliente nativo

- Cliente SDL2 para PS4 com controles DualShock.
- Mundo atual com janela residente de 2048×2048 células, salvamento local e migração de saves.
- Renderização pseudo-3D por raycasting/height-field, materiais e minimapa.
- Inventário, receitas, coleta, mineração, construção, criaturas e combate básico.
- PCG nativo C++17: seed determinística, FBM multi-octave, relevo, temperatura, umidade, oito biomas, máscara de continente/oceano e âncoras determinísticas de estruturas.
- O PCG é consultado por coordenadas globais, mas o armazenamento ainda é uma janela fixa. Streaming infinito de chunks é a próxima etapa e ainda não está concluído.

## Documentação PCG

Consulte ps4/native/PCG.md para arquitetura, parâmetros, otimização e limitações.

## Estrutura principal

- ps4/native/main.cpp: loop de jogo e cliente SDL2.
- ps4/native/pcg_world.hpp e pcg_world.cpp: gerador procedural nativo.
- ps4/native/Makefile: build do EBOOT.
- .github/workflows/ps4-native.yml: pipeline de PKG.

Qualquer projeto Godot que ainda exista no repositório é legado/protótipo e não faz parte do alvo de desenvolvimento nem do PKG PS4.
