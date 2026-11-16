# MINEPLUS PS4

Cliente nativo de exploração e sobrevivência para PlayStation 4, escrito em C++17 com SDL2 e OpenOrbis. O projeto tem identidade dark-fantasy própria; não depende do protótipo Godot legado.

## Estado real do projeto

- Cliente SDL2 com loop de jogo, renderização pseudo-3D por raycasting/height-field, minimapa e interface própria.
- Controles DualShock, teclado e mouse; movimentação, mineração, construção, combate e pausa.
- Salvamento manual por F5 ou pelo botão X no menu de pausa, além de autosave periódico e ao sair; o HUD confirma sucesso ou falha do salvamento manual.
- Ritmo de frames limitado a aproximadamente 60 FPS sem somar um segundo atraso quando o VSync já bloqueou a apresentação.
- Inventário de recursos, nove receitas, coleta, criaturas e salvamento local com migração de formatos antigos. A coleta por mineração e combate limita cada pilha a 9.999 unidades para evitar overflow e manter consistência com a validação dos saves.
- PCG determinístico com seed, FBM multi-octave, relevo, temperatura, umidade, oito biomas, continente/oceano e âncoras de estruturas.
- O cliente jogável ainda usa uma janela residente de 2048×2048 células. **O mundo jogável ainda não é infinito.**
- A camada de streaming já existe em módulos isolados: chunks 16×16, cache LRU de 81 chunks, journal limitado de alterações, consulta de voxels e adaptador `StreamedWorldGrid`. Esses módulos são compilados e testados, mas ainda não substituíram o armazenamento fixo usado pelo loop em `main.cpp`.
- `SaveCodec` versão 2 serializa seed e alterações de superfície/voxels com checksum. Ainda não substitui o formato de save usado diretamente pelo loop principal.

Não considere streaming infinito, prefabs 3D completos, animações esqueléticas ou compatibilidade Android como recursos concluídos nesta versão.

## Build PS4

O workflow [Build MINEPLUS PS4 PKG](.github/workflows/ps4-native.yml) baixa OpenOrbis v0.5.4, compila os testes portáveis, gera o EBOOT, cria o PKG e executa validações estruturais. O artefato `MINEPLUS-PS4-PKG` só é publicado se todas as etapas passarem.

Um build verde confirma a compilação e a validação do pacote, **não** a instalação nem a execução no console. O PKG precisa ser testado em um PS4 compatível antes de ser considerado funcional.

### Build local

Requisitos:
- OpenOrbis PS4 Toolchain v0.5.4;
- `clang++`, `ld.lld`, `make`, `python3`;
- .NET 6 e `PkgTool.Core`;
- `create-fself`, `create-gp4` e fontes dos módulos de compatibilidade exigidos pelo script.

Defina `OO_PS4_TOOLCHAIN` apontando para a pasta da toolchain e execute:

```bash
bash ps4/build-native.sh
```

O script interrompe o processo se faltar uma dependência, se o EBOOT não for convertido em SELF ou se a validação do PKG falhar. Ele não cria pacote vazio como fallback.

### Bloqueio verificado no ambiente atual

Em 16/11/2026, o ambiente de trabalho usado nesta execução não possui OpenOrbis montado (`OO_PS4_TOOLCHAIN`/SDK), `PkgTool.Core`, `create-fself`, `create-gp4`, .NET ou os headers de desenvolvimento SDL2. Também não há `ANDROID_HOME`/`ANDROID_SDK_ROOT`, `adb`, `sdkmanager` ou Gradle, e esta branch nativa PS4 não contém um projeto Android configurado. Há compiladores C++ e Make disponíveis, mas isso não basta para gerar EBOOT/PKG nem APK. O workflow do GitHub está configurado para provisionar a toolchain PS4; nenhum artefato PKG ou APK foi gerado ou validado nesta execução e não há confirmação de instalação no console.

Para concluir a etapa local, é necessário disponibilizar OpenOrbis v0.5.4, as ferramentas de empacotamento e dependências listadas acima. A validação final ainda exige instalar e executar o pacote em um PS4 compatível.

## Testes automatizados

O workflow executa testes C++ portáveis para:
- determinismo do PCG, coordenadas negativas, limites `int32` do espaço de coordenadas, fronteiras de chunks e estruturas subterrâneas;
- cache LRU, persistência de alterações e limites do journal;
- runtime, edição de voxels, exportação/importação e checksum de saves;
- adaptadores de mundo fixo e streamed.

Os testes não substituem teste de desempenho, controles e instalação no hardware PS4.

## Estrutura principal

- `ps4/native/main.cpp`: loop SDL2 e gameplay atual.
- `ps4/native/pcg_world.hpp/.cpp`: geração procedural.
- `ps4/native/chunk_generator.hpp/.cpp`: geração de chunks determinísticos.
- `ps4/native/chunk_cache.hpp/.cpp`: cache LRU e journals de edição.
- `ps4/native/world_runtime.hpp/.cpp`: API de mundo streamed.
- `ps4/native/world_grid.hpp`: adaptador entre armazenamento fixo e streamed.
- `ps4/native/save_codec.hpp/.cpp`: snapshot versionado com checksum.
- `ps4/native/tests/`: testes C++ portáveis.
- `ps4/native/Makefile`: compilação do EBOOT.
- `ps4/build-native.sh`: geração e validação do PKG.

Consulte [ps4/native/PCG.md](ps4/native/PCG.md) para a arquitetura procedural e as limitações atuais.


## Cavernas Primordiais
- Marcos subterrâneos podem gerar complexos determinísticos de cinco câmaras (núcleo central + quatro alas), conectadas por corredores em cruz.
- As salas têm piso e teto de DarkStone, com veios raros de cristal na área escavada; a geração usa seed e coordenadas globais para manter consistência entre chunks.
- Os testes portáveis verificam as quatro alas e a continuidade da geração. Isso ainda é geometria voxel procedural, não uma implementação final de renderização 3D, iluminação volumétrica, chefes ou decoração avançada.

## Último build automatizado
- Em 16/11/2026, o workflow GitHub Actions **Build MINEPLUS PS4 PKG** concluiu com sucesso para o commit `2e52caf8`: testes portáveis de mundo, compilação do EBOOT/PKG e upload do artefato `MINEPLUS-PS4-PKG`.
- O PKG gerado é um artefato de CI; ainda precisa ser instalado e testado em um PS4 compatível. Build verde não comprova estabilidade em hardware real.
- Execução: https://github.com/theolauria26-gif/MINEPLUS-PS4/actions/runs/43958492489
