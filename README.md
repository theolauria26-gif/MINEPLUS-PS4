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

O projeto Godot é um protótipo separado do cliente C++ que gera o PKG de PS4. A persistência do protótipo agora usa `scripts/world_save.gd`, com formato versão 7 e leitura/migração dos saves anteriores (até a versão 6). O inventário do protótipo separa seis blocos construíveis de sete recursos coletáveis, incluindo madeira; saves antigos de seis itens recebem um slot de madeira vazio. Picareta e machado de pedra têm durabilidade própria, persistida no save; o machado é fabricado com 2 Pedras + 2 Terras e permite colher troncos, enquanto a picareta continua necessária para pedra e tijolo. A fase do ciclo dia/noite também é persistida, então cada mundo retoma o horário salvo; saves legados sem esse campo começam na fase padrão. O codec valida e limita quantidades, coordenadas, inventário, durabilidade e fase do relógio; grava primeiro em arquivo temporário e mantém uma cópia `.bak` para recuperação se a gravação for interrompida ou o JSON principal ficar inválido.

O workflow Godot executa o projeto em modo headless, instancia o mundo streamed e roda testes de round-trip, saneamento de dados, rejeição de versões futuras e recuperação do backup. Esses testes cobrem o protótipo Godot, incluindo migração do inventário legado, persistência dos sete recursos e durabilidade do machado; não alteram nem substituem o codec de save do cliente nativo PS4.

A geração dos chunks Godot agora pré-amostra uma grade de 18×18 (chunk 16×16 + halo) e reutiliza as quatro alturas vizinhas na construção das faces laterais. Em um chunk sem edições, isso reduz o pior caso de até 1.792 chamadas repetidas de amostragem para 324, mantendo o limite de um chunk construído por frame e as coordenadas globais determinísticas.
Os controles virtuais de movimento, pulo, mineração e colocação agora aparecem apenas em dispositivos com a feature mobile e dimensionam botões/espaçamento pelo menor eixo da tela; teclado e mouse não recebem mais a sobreposição touch.
No mobile, o gesto de olhar usa somente o dedo que começou na zona superior direita da tela; o dedo precisa começar nessa área para mover a câmera, e os botões inferiores de quebrar/colocar/pular mantêm suas ações próprias. Tocar para olhar não dispara mineração acidentalmente. O roteamento da zona touch tem testes de limites e viewport no workflow Godot.


### Lobby e slots de mundo (Godot)

O botão **JOGAR** agora abre um lobby próprio antes de carregar a partida. Ele apresenta três slots locais com estado **Novo mundo**, **Aventura salva** ou **Arquivo não validado**. Para saves válidos, mostra seed, blocos construídos, alterações do terreno, total de itens e horário do último arquivo gravado. O slot 1 mantém o caminho legado `user://mineplus_world.json`; os slots 2 e 3 usam arquivos separados.

Mundos novos recebem seeds determinísticas distintas por slot (o slot 1 mantém a seed-base 4839201). A escolha do slot é gravada nas configurações antes da troca de cena. Se um arquivo e seu backup existirem, mas nenhum puder ser validado, o botão de entrada fica bloqueado: o lobby não trata o arquivo corrompido como slot vazio e não oferece sobrescrevê-lo. Quando há backup válido, o codec existente tenta recuperá-lo antes de apresentar o slot.


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

Em 19/11/2026, o container local desta execução continua sem `OO_PS4_TOOLCHAIN`, `PkgTool.Core`, `create-fself`, `create-gp4` e .NET; há `clang++`, `ld.lld` e Make, mas sem SDK/header SDL2 eles não geram EBOOT/PKG. O workflow do GitHub provisiona OpenOrbis v0.5.4 e já produziu pacotes nativos estruturalmente validados (último artefato: `MINEPLUS-PS4-PKG`, 5.978.572 bytes, run [44549940509](https://github.com/theolauria26-gif/MINEPLUS-PS4/actions/runs/44549940509), artefato 14542986299). Isso não comprova instalação ou execução em console físico. Para Android, não existem `ANDROID_HOME`/`ANDROID_SDK_ROOT`, `adb`, `sdkmanager`, Gradle ou `export_presets.cfg` neste ambiente/repositório; o projeto ainda não tem preset de exportação Android e nenhum APK foi gerado. A validação final do PKG ainda exige instalação e execução em um PS4 compatível.

Para concluir a etapa local, é necessário disponibilizar OpenOrbis v0.5.4, as ferramentas de empacotamento e dependências listadas acima. A validação final ainda exige instalar e executar o pacote em um PS4 compatível.

## Testes automatizados

O workflow executa testes C++ portáveis para:
- determinismo do PCG, coordenadas negativas, limites `int32` do espaço de coordenadas, fronteiras de chunks e estruturas subterrâneas;
- cache LRU, persistência de alterações e limites do journal;
- runtime, edição de voxels, exportação/importação e checksum de saves;
- adaptadores de mundo fixo e streamed.

O workflow Godot também verifica a instanciação do mundo streamed, os testes de recuperação do codec de save versão 7 e as regras de entrada touch em modo headless. Os testes não substituem teste de desempenho, controles e instalação no hardware PS4.

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
- Em 17/11/2026, o workflow [Build MINEPLUS PS4 PKG](https://github.com/theolauria26-gif/MINEPLUS-PS4/actions/runs/44171203498) concluiu com sucesso para o commit `50c6ae5b`, incluindo testes portáveis, compilação do EBOOT/PKG, validação estrutural e publicação do artefato.
- A atualização documental desse marco também passou no workflow [de documentação](https://github.com/theolauria26-gif/MINEPLUS-PS4/actions/runs/44171215115).
- A melhoria visual `23fb65e3` acrescenta pequenas flores com caule, frondes e brilhos de micélio ciano no musgo; o workflow [correspondente](https://github.com/theolauria26-gif/MINEPLUS-PS4/actions/runs/44171985600) concluiu com sucesso.
- A etapa `db49876d`/`02653665`/`b054d82f` introduz árvores coníferas esparsas na tundra e em áreas úmidas das montanhas, com copas mais estreitas e testes de determinismo. O workflow final [44172801289](https://github.com/theolauria26-gif/MINEPLUS-PS4/actions/runs/44172801289) concluiu com sucesso: testes PCG, compilação do EBOOT/PKG, validação estrutural e upload do artefato `MINEPLUS-PS4-PKG` (5.978.576 bytes; artefato [14368597336](https://github.com/theolauria26-gif/MINEPLUS-PS4/actions/runs/44172801289)).
- Os workflows intermediários `44172738544` e `44172770796` falharam porque foram executados antes da atualização do teste que ainda esperava árvores somente em grama/musgo; a execução final com o teste corrigido passou. O pacote continua precisando de teste de instalação e execução em hardware PS4.
- [Acessar o último artefato validado](https://github.com/theolauria26-gif/MINEPLUS-PS4/actions/runs/44172801289)


- Atualização nativa mais recente: o workflow [44379732777](https://github.com/theolauria26-gif/MINEPLUS-PS4/actions/runs/44379732777), no commit `f9b85d84`, concluiu com sucesso testes portáveis, compilação do EBOOT/PKG, validação estrutural e publicação do artefato `MINEPLUS-PS4-PKG` (5.978.581 bytes; artefato [14463836288](https://github.com/theolauria26-gif/MINEPLUS-PS4/actions/runs/44379732777)). A validação não inclui instalação ou execução em console físico.
- A validação Godot mais recente, [44384221699](https://github.com/theolauria26-gif/MINEPLUS-PS4/actions/runs/44384221699), no commit `93f1f8e5`, instalou Godot 4.3 headless e passou pela importação/parsing do projeto, instanciação de `scenes/world.tscn` e testes de migração/recuperação do save versão 6. Ela inclui a nova madeira, durabilidade do machado e a interface de inventário com rolagem para telas estreitas.

## Direção visual: materiais pixel-art

A direção visual usa pixel-art voxel dark fantasy, com materiais distinguíveis em vez de blocos de cor chapada. O renderer SDL2 ativo recebeu padrões ancorados nas coordenadas do mundo: grama com tufos e flores, terra em grânulos, pedra com juntas e fissuras, madeira com veios, folhagem em camadas, musgo com esporos, cristais facetados, osso estriado, areia ondulada, neve, gelo, lava, água animada, minério, obsidiana, metal e runas luminosas. O atlas de coordenadas atual reserva 21 materiais em uma grade de 8×4 células de 16×16 pixels (128×64 no total); ele ainda é um mapeamento CPU, não uma textura enviada à GPU.

O céu agora tem bancos de nuvens pixeladas que se deslocam com o vento, estrelas e ciclo solar/lunar. A amostragem horizontal do ray marcher passou de uma coluna a cada dois pixels para uma coluna por pixel. A superfície também recebe iluminação direcional aproximada pelas diferenças de altura, seguindo a posição do sol, e névoa de distância que mistura o terreno distante com a cor atual do céu.

**Limite visual importante:** a partida ainda usa o renderer SDL2 de colunas em perspectiva, não um renderer voxel 3D completo com shaders, iluminação dinâmica por voxel e malhas de chunks na GPU. As melhorias deixam o exterior mais definido e atmosférico, mas não significam que o MINEPLUS já alcançou a qualidade gráfica de Hytale. A próxima etapa estrutural é integrar o mundo em chunks, meshing, atlas e iluminação a um renderer 3D real, e depois avançar modelos, animações, áudio e comportamento de criaturas.


O atlas de coordenadas de materiais mede 128×64 pixels, com células de 16×16 (8×4) e 21 IDs sólidos mapeados. Ele ainda não é uma imagem de textura: a geração da imagem, o upload e a amostragem GPU permanecem pendentes. Os padrões visíveis atuais são desenhados proceduralmente pelo renderer SDL2.

**Limite visual importante:** a partida ainda usa o renderer SDL2 de colunas em perspectiva, não um renderer voxel 3D completo com shaders, iluminação dinâmica e malhas de chunks na GPU. Portanto, os novos padrões melhoram a leitura visual do renderer atual, mas não significam que o MINEPLUS já alcançou a qualidade gráfica de Hytale. A próxima etapa é evoluir para superfícies 3D reais, atlas com gutters, luz ambiente/direcional e modelos animados sem quebrar o jogo atual.


- O save versão 7 agora persiste a fase do ciclo dia/noite; a validação de migração da versão 6, round-trip, limites e fallback passou em [44596413847](https://github.com/theolauria26-gif/MINEPLUS-PS4/actions/runs/44596413847).
- O input touch foi corrigido nos commits `5f037535` e `c645483f`: somente o dedo que começou na zona de olhar controla a câmera; os botões inferiores continuam responsáveis por quebrar, colocar e pular. Os testes de zona, viewport e propriedade do dedo (multi-touch) passaram com Godot 4.3 em [44596776503](https://github.com/theolauria26-gif/MINEPLUS-PS4/actions/runs/44596776503), commit de testes `c2a9aad3`.
