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

## Protótipo Godot e persistência de mundos

O projeto Godot é um protótipo separado do cliente C++ que gera o PKG de PS4. A persistência do protótipo agora usa `scripts/world_save.gd`, com formato versão 5 e leitura/migração dos saves anteriores (até a versão 4). O codec valida e limita quantidades, coordenadas, inventário e durabilidade; grava primeiro em arquivo temporário e mantém uma cópia `.bak` para recuperação se a gravação for interrompida ou o JSON principal ficar inválido.

O workflow Godot executa o projeto em modo headless, instancia o mundo streamed e roda testes de round-trip, saneamento de dados, rejeição de versões futuras e recuperação do backup. Esses testes cobrem o protótipo Godot; não alteram nem substituem o codec de save do cliente nativo PS4.

A geração dos chunks Godot agora pré-amostra uma grade de 18×18 (chunk 16×16 + halo) e reutiliza as quatro alturas vizinhas na construção das faces laterais. Em um chunk sem edições, isso reduz o pior caso de até 1.792 chamadas repetidas de amostragem para 324, mantendo o limite de um chunk construído por frame e as coordenadas globais determinísticas.
Os controles virtuais de movimento, pulo, mineração e colocação agora aparecem apenas em dispositivos com a feature mobile e dimensionam botões/espaçamento pelo menor eixo da tela; teclado e mouse não recebem mais a sobreposição touch.

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

O workflow Godot também verifica a instanciação do mundo streamed e os testes do codec de save versão 5 em modo headless. Os testes não substituem teste de desempenho, controles e instalação no hardware PS4.

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
- Em 17/11/2026, o workflow [Build MINEPLUS PS4 PKG](https://github.com/theolauria26-gif/MINEPLUS-PS4/actions/runs/44163159779) concluiu com sucesso para o commit `328973cb` (nuvens móveis e amostragem horizontal em resolução integral): testes portáveis, compilação do EBOOT/PKG, validação estrutural do pacote e upload do artefato `MINEPLUS-PS4-PKG`.
- O PKG validado pelo script tinha 6.619.136 bytes. O ZIP do artefato publicado tem 5.978.577 bytes; SHA-256 do ZIP: `ec2aa5c254fabecd83cf9d51c6ccaee002957b96772f19cc9054bd509879cac2`.
- Esse build verde não inclui ainda os commits posteriores de iluminação direcional/névoa e ajuste da camada de nuvens; eles estão em validação. O pacote precisa ser instalado e testado em um PS4 compatível antes de ser considerado funcional.
- [Baixar/abrir o artefato no GitHub Actions](https://github.com/theolauria26-gif/MINEPLUS-PS4/actions/runs/44163159779)


## Direção visual: materiais pixel-art

A direção visual usa pixel-art voxel dark fantasy, com materiais distinguíveis em vez de blocos de cor chapada. O renderer SDL2 ativo recebeu padrões ancorados nas coordenadas do mundo: grama com tufos e flores, terra em grânulos, pedra com juntas e fissuras, madeira com veios, folhagem em camadas, musgo com esporos, cristais facetados, osso estriado, areia ondulada, neve, gelo, lava, água animada, minério, obsidiana, metal e runas luminosas. O atlas procedural tem 32×32 pixels por material e 21 materiais definidos.

O céu agora tem bancos de nuvens pixeladas que se deslocam com o vento, estrelas e ciclo solar/lunar. A amostragem horizontal do ray marcher passou de uma coluna a cada dois pixels para uma coluna por pixel. A superfície também recebe iluminação direcional aproximada pelas diferenças de altura, seguindo a posição do sol, e névoa de distância que mistura o terreno distante com a cor atual do céu.

**Limite visual importante:** a partida ainda usa o renderer SDL2 de colunas em perspectiva, não um renderer voxel 3D completo com shaders, iluminação dinâmica por voxel e malhas de chunks na GPU. As melhorias deixam o exterior mais definido e atmosférico, mas não significam que o MINEPLUS já alcançou a qualidade gráfica de Hytale. A próxima etapa estrutural é integrar o mundo em chunks, meshing, atlas e iluminação a um renderer 3D real, e depois avançar modelos, animações, áudio e comportamento de criaturas.


O atlas procedural de materiais foi elevado para 32×32 pixels por material (256×128 no total), com 21 materiais e padrões determinísticos específicos por família. Os testes verificam determinismo, transparência das células vazias e variação de cor em cada tile.

**Limite visual importante:** a partida ainda usa o renderer SDL2 de colunas em perspectiva, não um renderer voxel 3D completo com shaders, iluminação dinâmica e malhas de chunks na GPU. Portanto, os novos padrões melhoram a leitura visual do renderer atual, mas não significam que o MINEPLUS já alcançou a qualidade gráfica de Hytale. A próxima etapa é evoluir para superfícies 3D reais, atlas com gutters, luz ambiente/direcional e modelos animados sem quebrar o jogo atual.

