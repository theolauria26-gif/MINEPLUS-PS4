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

O projeto Godot é um protótipo separado do cliente C++ que gera o PKG de PS4. A persistência do protótipo agora usa `scripts/world_save.gd`, com formato versão 9 e leitura/migração dos saves anteriores (até a versão 8). O inventário do protótipo separa sete blocos construíveis de oito pilhas coletáveis, mantendo madeira bruta e tábuas como itens distintos; saves antigos são migrados preservando as pilhas existentes e preenchendo os novos espaços com zero. Picareta e machado de pedra têm durabilidade própria, persistida no save; o machado é fabricado com 2 Pedras + 2 Terras e permite colher troncos, enquanto a picareta continua necessária para pedra e tijolo. A fase do ciclo dia/noite também é persistida, então cada mundo retoma o horário salvo; saves legados sem esse campo começam na fase padrão. O codec valida e limita quantidades, coordenadas, inventário, durabilidade e fase do relógio; grava primeiro em arquivo temporário e mantém uma cópia `.bak` para recuperação se a gravação for interrompida ou o JSON principal ficar inválido.

O workflow Godot executa o projeto em modo headless, instancia o mundo streamed e roda testes de round-trip, saneamento de dados, rejeição de versões futuras e recuperação do backup. Esses testes cobrem o protótipo Godot, incluindo migração do inventário legado, persistência das oito pilhas, durabilidade do machado e do novo bloco de tábuas; não alteram nem substituem o codec de save do cliente nativo PS4.

A geração dos chunks Godot agora pré-amostra uma grade de 18×18 (chunk 16×16 + halo) e reutiliza as quatro alturas vizinhas na construção das faces laterais. Em um chunk sem edições, isso reduz o pior caso de até 1.792 chamadas repetidas de amostragem para 324, mantendo o limite de um chunk construído por frame e as coordenadas globais determinísticas.
Os controles virtuais de movimento, pulo, mineração e colocação agora aparecem apenas em dispositivos com a feature mobile e dimensionam botões/espaçamento pelo menor eixo da tela; teclado e mouse não recebem mais a sobreposição touch.
No mobile, o gesto de olhar usa somente o dedo que começou na zona superior direita da tela; o dedo precisa começar nessa área para mover a câmera, e os botões inferiores de quebrar/colocar/pular mantêm suas ações próprias. Tocar para olhar não dispara mineração acidentalmente. O roteamento da zona touch tem testes de limites e viewport no workflow Godot.


### Lobby e slots de mundo (Godot)

O botão **JOGAR** agora abre um lobby próprio antes de carregar a partida. Ele apresenta três slots locais com estado **Novo mundo**, **Aventura salva** ou **Arquivo não validado**. Para saves válidos, mostra seed, blocos construídos, alterações do terreno, total de itens e horário do último arquivo gravado. O slot 1 mantém o caminho legado `user://mineplus_world.json`; os slots 2 e 3 usam arquivos separados.

Mundos novos recebem seeds determinísticas distintas por slot (o slot 1 mantém a seed-base 4839201). A escolha do slot é gravada nas configurações antes da troca de cena; a tela de configurações não duplica essa escolha, que fica centralizada no lobby. Se um arquivo e seu backup existirem, mas nenhum puder ser validado, o botão de entrada fica bloqueado: o lobby não trata o arquivo corrompido como slot vazio e não oferece sobrescrevê-lo. Quando há backup válido, o codec existente tenta recuperá-lo antes de apresentar o slot.

### Inventário interativo e filtros (Godot)

O inventário do protótipo agora possui filtros **Todos**, **Blocos** e **Recursos**. A lista usa um catálogo central (`scripts/inventory_catalog.gd`) para manter estável a relação entre os oito itens, suas categorias e os tipos de bloco construíveis. Madeira bruta aparece apenas em Recursos, enquanto tábuas aparecem em Blocos; tocar/clicar em um bloco no inventário seleciona o tipo correspondente na hotbar. A madeira bruta não pode ser equipada como bloco e apresenta a orientação para transformá-la em tábuas na bancada. As durabilidades da picareta e do machado continuam visíveis no painel, separadas das pilhas.

O teste `scripts/tests/inventory_test.gd` verifica a ordem das oito pilhas, os filtros, os mapeamentos entre item e bloco, a rejeição de IDs inválidos e a seleção pela interface. O workflow Godot executa esse teste com Godot 4.3 headless junto aos testes de save, crafting e lobby. A validação automatizada não substitui a avaliação visual em telas touch reais. A primeira execução completa desse conjunto passou em [Godot 4.3 headless — workflow 45685280839](https://github.com/theolauria26-gif/MINEPLUS-PS4/actions/runs/45685280839), incluindo importação do projeto, filtros e quick-equip, saves, receitas, lobby, controles touch e testes visuais.

### Fabricação transacional (Godot)

As quatro receitas do protótipo foram centralizadas em `scripts/crafting.gd`, separando as regras de fabricação da interface. Além de tijolos, picareta e machado, a receita de tábuas converte 1 Madeira em 4 Tábuas. A bancada apresenta os requisitos e desativa receitas sem materiais suficientes. Cada tentativa valida o inventário completo (oito pilhas limitadas a 999), a disponibilidade da ferramenta e o espaço da pilha de saída antes de consumir qualquer ingrediente. Tijolos, tábuas, picareta e machado usam a mesma transação: em caso de falha, nenhuma pilha ou durabilidade muda; em caso de sucesso, o novo inventário e a durabilidade da ferramenta são aplicados juntos e salvos. A fabricação retorna uma cópia do inventário, sem alterar a lista de entrada, o que também permite testar as regras sem instanciar a cena 3D.

O teste `scripts/tests/crafting_test.gd` cobre custos e resultados das quatro receitas, falta de materiais, pilha de saída cheia, ferramentas duplicadas, inventário inválido e ausência de mutação parcial; também verifica que o bloco de tábuas consome a pilha de tábuas, não a madeira bruta. O workflow Godot executa esse teste com Godot 4.3 headless junto aos testes de persistência e lobby. Isso valida a lógica de crafting, mas não substitui teste de interface/toque em aparelho físico.


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

Em 26/11/2026, o container local foi verificado novamente: Godot, `ANDROID_HOME`/`ANDROID_SDK_ROOT`, `adb`, `sdkmanager`, Gradle, .NET, `OO_PS4_TOOLCHAIN`, `create-fself` e `create-gp4` estão ausentes. O runner do GitHub consegue provisionar Godot 4.3 para validar o protótipo e OpenOrbis v0.5.4 + .NET 6 para compilar o cliente nativo C++/PS4. O último artefato nativo registrado continua sendo `MINEPLUS-PS4-PKG` (5.978.572 bytes; run [44549940509](https://github.com/theolauria26-gif/MINEPLUS-PS4/actions/runs/44549940509), artefato [14542986299](https://github.com/theolauria26-gif/MINEPLUS-PS4/actions/runs/44549940509)); ele passou pela validação estrutural, mas não foi instalado nem executado em console físico. Esse PKG contém o cliente C++/SDL2, não o protótipo Godot e seus sistemas de lobby, inventário e equipamento. O protótipo não tem `export_presets.cfg` nem preset de exportação Android; sem Android SDK e templates de exportação não há APK. Para uma versão final PS4 que contenha os sistemas Godot, ainda é necessário portá-los/integrá-los ao cliente C++ nativo ou disponibilizar uma toolchain legítima de exportação Godot para PS4; a pipeline OpenOrbis atual compila apenas o cliente C++ existente. A validação final do PKG também exige instalação e execução em um PS4 compatível.

Para concluir a etapa local, é necessário disponibilizar OpenOrbis v0.5.4, as ferramentas de empacotamento e dependências listadas acima. A validação final ainda exige instalar e executar o pacote em um PS4 compatível.

## Testes automatizados

O workflow executa testes C++ portáveis para:
- determinismo do PCG, coordenadas negativas, limites `int32` do espaço de coordenadas, fronteiras de chunks e estruturas subterrâneas;
- cache LRU, persistência de alterações e limites do journal;
- runtime, edição de voxels, exportação/importação e checksum de saves;
- adaptadores de mundo fixo e streamed.
- seleção dos três slots Godot, seeds distintas, classificação de saves inválidos e recuperação de backup no lobby.
- custos, saídas, limites de pilha, ferramentas duplicadas e rejeição segura de inventários inválidos nas receitas Godot.

O workflow Godot também verifica o script e a cena do lobby (`scripts/world_lobby.gd` e `scenes/world_lobby.tscn`), a instanciação do mundo streamed, os testes de recuperação do codec de save versão 8 e migração da versão 7, as regras de entrada touch em modo headless e a tela de lobby/seleção dos três slots, incluindo seeds distintas e proteção contra sobrescrita de saves inválidos. Os testes não substituem teste de desempenho, controles e instalação no hardware PS4.

A validação mais recente deste conjunto, no commit `7250a8fa`, concluiu com sucesso em [Godot 4.3 headless — workflow 44940853195](https://github.com/theolauria26-gif/MINEPLUS-PS4/actions/runs/44940853195). Ela inclui o novo teste transacional das receitas, além dos testes de save, lobby e decorações.

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
- `scripts/world_lobby.gd` e `scenes/world_lobby.tscn`: lobby local de seleção dos três mundos no protótipo Godot.
- `scripts/crafting.gd`: catálogo e transações de receitas do protótipo Godot.

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
- Em 20/11/2026, o workflow [44803837889](https://github.com/theolauria26-gif/MINEPLUS-PS4/actions/runs/44803837889), no commit `7f8f823f`, instalou Godot 4.3 headless e passou pela importação do projeto, recuperação do save versão 7, testes do lobby de três slots (save válido, arquivo corrompido, recuperação de backup e três cartões de mundo) e testes visuais determinísticos. O lobby é parte do protótipo Godot e ainda não está integrado ao cliente C++/PKG de PS4.

## Direção visual: materiais pixel-art

A direção visual usa pixel-art voxel dark fantasy, com materiais distinguíveis em vez de blocos de cor chapada. O renderer SDL2 ativo recebeu padrões ancorados nas coordenadas do mundo: grama com tufos e flores, terra em grânulos, pedra com juntas e fissuras, madeira com veios, folhagem em camadas, musgo com esporos, cristais facetados, osso estriado, areia ondulada, neve, gelo, lava, água animada, minério, obsidiana, metal e runas luminosas. O atlas de coordenadas atual reserva 21 materiais em uma grade de 8×4 células de 16×16 pixels (128×64 no total); ele ainda é um mapeamento CPU, não uma textura enviada à GPU.

O céu agora tem bancos de nuvens pixeladas que se deslocam com o vento, estrelas e ciclo solar/lunar. A amostragem horizontal do ray marcher passou de uma coluna a cada dois pixels para uma coluna por pixel. A superfície também recebe iluminação direcional aproximada pelas diferenças de altura, seguindo a posição do sol, e névoa de distância que mistura o terreno distante com a cor atual do céu.

**Limite visual importante:** a partida ainda usa o renderer SDL2 de colunas em perspectiva, não um renderer voxel 3D completo com shaders, iluminação dinâmica por voxel e malhas de chunks na GPU. As melhorias deixam o exterior mais definido e atmosférico, mas não significam que o MINEPLUS já alcançou a qualidade gráfica de Hytale. A próxima etapa estrutural é integrar o mundo em chunks, meshing, atlas e iluminação a um renderer 3D real, e depois avançar modelos, animações, áudio e comportamento de criaturas.


O atlas de coordenadas de materiais mede 128×64 pixels, com células de 16×16 (8×4) e 21 IDs sólidos mapeados. Ele ainda não é uma imagem de textura: a geração da imagem, o upload e a amostragem GPU permanecem pendentes. Os padrões visíveis atuais são desenhados proceduralmente pelo renderer SDL2.

**Limite visual importante:** a partida ainda usa o renderer SDL2 de colunas em perspectiva, não um renderer voxel 3D completo com shaders, iluminação dinâmica e malhas de chunks na GPU. Portanto, os novos padrões melhoram a leitura visual do renderer atual, mas não significam que o MINEPLUS já alcançou a qualidade gráfica de Hytale. A próxima etapa é evoluir para superfícies 3D reais, atlas com gutters, luz ambiente/direcional e modelos animados sem quebrar o jogo atual.


- A versão 7 do save introduziu a persistência da fase do ciclo dia/noite; a validação de migração da versão 6, round-trip, limites e fallback passou em [44596413847](https://github.com/theolauria26-gif/MINEPLUS-PS4/actions/runs/44596413847).
- O input touch foi corrigido nos commits `5f037535` e `c645483f`: somente o dedo que começou na zona de olhar controla a câmera; os botões inferiores continuam responsáveis por quebrar, colocar e pular. Os testes de zona, viewport e propriedade do dedo (multi-touch) passaram com Godot 4.3 em [44596776503](https://github.com/theolauria26-gif/MINEPLUS-PS4/actions/runs/44596776503), commit de testes `c2a9aad3`.


### Tábuas e sétimo bloco construível (Godot)

O inventário do protótipo agora tem oito pilhas: os seis materiais/blocos anteriores, madeira bruta e tábuas. A receita **1 Madeira → 4 Tábuas** é transacional e verifica o limite de 999 antes de consumir o tronco. Tábuas formam o sétimo bloco construível, aparecem no sétimo espaço da hotbar e podem ser selecionadas pela tecla 7. A conversão entre tipo de bloco e pilha de inventário é explícita: o item 6 continua sendo madeira bruta, enquanto o bloco de tábuas usa o item 7; colocar e minerar tábuas não consome nem devolve madeira bruta por engano.

O save do protótipo avançou para a versão 8. Saves de versões 1–7 continuam legíveis; a migração de um save versão 7 preserva a madeira e cria a nova pilha de tábuas vazia. O codec também aceita o novo tipo de bloco colocado e preserva a seleção do sétimo espaço. Os testes de crafting e de round-trip/migração cobrem essas regras.

- A validação Godot do novo inventário, [45060840743](https://github.com/theolauria26-gif/MINEPLUS-PS4/actions/runs/45060840743), no commit `7c552a46`, instalou Godot 4.3 headless e passou pela importação do projeto, migração e round-trip do save versão 8, quatro receitas (incluindo tábuas), mapeamento entre bloco e pilha, lobby de três slots e decorações 3D determinísticas. O bloco de tábuas está no protótipo Godot e ainda não está integrado ao cliente C++/PKG de PS4.

## Configurações persistentes e orçamento de chunks (23/11/2026)

O protótipo Godot agora centraliza a persistência das opções em `scripts/settings.gd` (`MineplusSettings`). As gravações são feitas como patches sobre o arquivo existente, em vez de criar um `ConfigFile` vazio: alterar FOV, qualidade, sensibilidade ou aparência não apaga a seleção do mundo, a opacidade da HUD nem chaves de outras versões. Se o arquivo estiver malformado, a interface informa o problema e recusa sobrescrevê-lo automaticamente.

A tela de configurações oferece limites de 30 FPS (economia), 60 FPS (equilibrado) e sem limite (PC). A opção sem limite é limitada a 60 FPS quando o jogo roda com a feature mobile. O perfil gráfico também controla a distância de streaming: baixa usa raio de 2 chunks; equilibrada, 3; alta, 4 no desktop. Em dispositivos mobile, os perfis equilibrado e alto ficam limitados a 3 chunks e o perfil baixo mantém 2. O raio de descarregamento conserva uma margem de um chunk para evitar descarte imediato na borda da área ativa.

- `scripts/main_menu.gd`: carrega/aplica o limite de quadros, expõe a opção e salva somente as seções alteradas.
- `scripts/world.gd`: aplica as opções ao iniciar a partida e configura o streaming conforme qualidade e plataforma.
- `scripts/chunk_streamer.gd`: raio de carga/descarregamento configurável, mantendo o orçamento de no máximo um chunk novo por frame.
- `scripts/tests/settings_test.gd`: verifica preservação de slot/HUD/chaves desconhecidas, proteção de arquivo malformado, limites de FPS e raios desktop/mobile.

A validação final desta etapa instalou Godot 4.3 headless e concluiu importação/parsing, teste das configurações, migração de saves, receitas, lobby e decorações no [workflow 45254441199](https://github.com/theolauria26-gif/MINEPLUS-PS4/actions/runs/45254441199), commit `1a08207f`. As tentativas intermediárias `45254377418` e `45254404777` falharam por diferenças de maiúsculas/minúsculas nos nomes das variáveis do raio; a referência foi corrigida antes da execução final aprovada. A aprovação é de CI headless e não substitui medição de FPS, temperatura, consumo ou teste touch em aparelhos reais.

### Exportação verificada em 23/11/2026

No container local continuam ausentes Godot, Android SDK (`ANDROID_HOME`/`ANDROID_SDK_ROOT`), `adb`, `sdkmanager`, Gradle, .NET, OpenOrbis (`OO_PS4_TOOLCHAIN`), `create-fself` e `create-gp4`. A rede do container também não conseguiu resolver `github.com`; por isso a validação Godot foi feita pelo runner do GitHub Actions, que baixou e executou a versão 4.3 headless. Nenhum APK foi gerado nesta etapa. O protótipo Godot ainda não está integrado ao cliente nativo C++/PS4; nenhum PKG novo foi gerado por estas mudanças. A validação de um PKG continua exigindo instalação e execução em hardware PS4 compatível.

## Animação de primeira pessoa e sementes por slot (23/11/2026)

A apresentação do personagem recebeu uma primeira camada de animação procedural. Os dois braços visíveis em primeira pessoa agora acompanham o deslocamento horizontal com fases opostas, enquanto a intensidade é normalizada pela velocidade real do jogador. O movimento é deliberadamente contido: o deslocamento máximo calculado fica abaixo de 0,044 unidade e a rotação combinada abaixo de 5,4 graus. Em repouso, permanece apenas uma oscilação muito discreta. Isso não é um rig esquelético de corpo inteiro nem uma animação de terceira pessoa.

As quatro opções de aparência também ganharam punhos de manga com paletas próprias (verde musgo, vinho, azul ardósia e violeta), além da cor de pele já aplicada aos braços. A opção continua sendo persistida em `mineplus_settings.cfg` e é aplicada quando o mundo é iniciado.

Durante a revisão do lobby, foi corrigido um detalhe: os cartões de slots vazios 2 e 3 exibiam a seed do slot 1. Agora cada cartão informa a seed determinística do próprio slot. Os testes verificam separadamente as seeds dos três mundos, os limites do movimento dos braços, as fases opostas e a intensidade máxima.

- `scripts/world.gd`: referências dos braços, punhos coloridos por aparência e oscilação de caminhada/repouso.
- `scripts/tests/world_visual_test.gd`: limites e comportamento matemático da animação de primeira pessoa, junto dos testes touch e visuais existentes.
- `scripts/world_lobby.gd` e `scripts/tests/world_lobby_test.gd`: seed correta por cartão de mundo vazio.
- `.github/workflows/godot-check.yml`: passo visual identificado explicitamente para touch, animação e decorações.

A validação final no [workflow Godot 4.3 headless 45255511940](https://github.com/theolauria26-gif/MINEPLUS-PS4/actions/runs/45255511940), commit `dfc21233`, instalou Godot 4.3, importou/analisou o projeto e passou pelos testes de configurações, persistência/migração, crafting, lobby (incluindo seed por slot), touch, animação de braços e malhas de decoração 3D. A execução é automatizada em Linux headless; não substitui teste visual em PC/celular nem medição de desempenho em hardware real.

Nenhum APK ou PKG foi produzido nesta etapa. O protótipo Godot continua separado do cliente nativo C++/PS4, e o ambiente local permanece sem Android SDK, templates de exportação, OpenOrbis e ferramentas PS4. A animação descrita existe apenas no protótipo Godot.

## Ferramentas exigidas ao desmontar blocos de madeira (24/11/2026)

A mineração de blocos construídos agora respeita a mesma regra de ferramenta usada para os recursos naturais: pedra e tijolos exigem picareta; madeira bruta e tábuas exigem machado. Antes desta correção, tábuas colocadas pelo jogador podiam ser removidas sem gastar durabilidade do machado, embora a madeira natural já exigisse essa ferramenta.

- `scripts/world.gd`: centraliza a resolução da ferramenta necessária a partir do item obtido e aplica consumo, animação e quebra de durabilidade também às tábuas colocadas.
- `scripts/tests/world_visual_test.gd`: cobre o mapeamento de pedra, tijolos, madeira, tábuas e blocos que não exigem ferramenta.
- Não houve alteração no formato do save: durabilidade das ferramentas e blocos colocados já são persistidos na versão 8.

A validação no [workflow Godot 4.3 headless 45468813439](https://github.com/theolauria26-gif/MINEPLUS-PS4/actions/runs/45468813439), commit `31c99469`, instalou Godot 4.3, importou/analisou o projeto e aprovou os testes de configurações, migração/recuperação de saves, crafting, lobby, controles touch, animação e o novo mapeamento de ferramentas. É validação automatizada headless; não comprova interação em aparelho ou console físico.

Nenhum APK ou PKG foi gerado nesta etapa. A alteração pertence ao protótipo Godot, ainda separado do cliente C++/PS4. A exportação Android e a integração do protótipo ao cliente PS4 permanecem pendentes.


## Ferramentas equipáveis e persistência de equipamento (26/11/2026)

Picareta e machado agora são ferramentas selecionáveis, não ficam mais visíveis simultaneamente nem são usados automaticamente apenas por existirem no inventário. A mochila oferece três ações: **Mão livre**, **Picareta** e **Machado**; no teclado, as teclas **8**, **9** e **0** selecionam respectivamente picareta, machado e mão livre. No mobile, os mesmos controles ficam acessíveis pela interface do inventário.

Mineração de pedra/tijolo exige que a picareta esteja selecionada e tenha durabilidade. Madeira natural e tábuas colocadas exigem o machado selecionado. O sistema impede o golpe se a ferramenta correta não estiver equipada, informa o motivo e não consome durabilidade. Ao fabricar uma ferramenta, ela é selecionada automaticamente; quando quebra, a mão livre volta a ficar ativa e o modelo da ferramenta some da câmera. O inventário mostra a durabilidade e destaca o equipamento ativo.

- `scripts/equipment.gd`: regras puras para seleção, normalização, durabilidade e validação da ferramenta exigida.
- `scripts/world.gd`: botões de equipamento na mochila, atalhos 8/9/0, modelos de primeira pessoa exclusivos e validação antes de minerar blocos naturais ou construídos.
- `scripts/world_save.gd`: formato versão 9 salva a ferramenta selecionada. Saves até a versão 8 migram com mão livre; seleções inválidas ou ferramentas sem durabilidade são descartadas para não carregar um estado impossível.
- `scripts/tests/equipment_test.gd` e `scripts/tests/world_save_test.gd`: verificam ferramenta correta, ferramenta errada, ferramenta quebrada, saneamento e migração do save.

A validação final instalou Godot 4.3 headless no runner e passou pela importação/parsing, instanciação real de `scenes/world.tscn`, verificação dos controles e modelos de equipamento na cena, migração/recuperação do save v9, regras de seleção/durabilidade, inventário, crafting, lobby, touch, animação e decorações. Workflow [45887180834](https://github.com/theolauria26-gif/MINEPLUS-PS4/actions/runs/45887180834), commit de código `b420075a`. A execução é headless e automatizada: não substitui teste visual de teclado/touch nem medição de desempenho em aparelho físico.

As alterações de equipamento e a nova verificação de inicialização da cena `world.tscn` são exclusivas do protótipo Godot e não alteram o conteúdo do PKG nativo existente. Nenhum APK ou PKG final foi produzido nesta etapa, para evitar publicar um artefato que não contenha os recursos recém-implementados.

## Animação de ações em primeira pessoa (27/11/2026)

A mineração e a construção no protótipo Godot agora compartilham uma pequena animação procedural de ação. Ao concluir uma colocação, quebrar um bloco do terreno ou desmontar um bloco construído, os braços fazem um movimento curto de avanço e retorno, sobreposto à animação de caminhada. O braço dominante executa o golpe principal e o braço de apoio faz um contramovimento menor. Isso também dá feedback visual ao usar a mão livre em materiais que não exigem ferramenta; picareta e machado mantêm seus próprios movimentos de ferramenta.

- `scripts/action_animation.gd`: funções puras e determinísticas para o envelope temporal e as poses dos braços, sem alocações por frame.
- `scripts/world.gd`: aciona a animação somente depois de uma interação válida; ações recusadas por falta de bloco, ferramenta, durabilidade ou espaço não iniciam o golpe.
- `scripts/tests/action_animation_test.gd`: verifica pose neutra no início/fim, pico no meio, limites do progresso e diferença entre braço dominante e braço de apoio.
- `.github/workflows/godot-check.yml`: executa o novo teste com Godot 4.3 headless.

A animação é uma camada procedural de primeira pessoa; não é rig esquelético, animação de corpo inteiro ou animação em terceira pessoa. Não altera o formato do save (continua versão 9) e ainda precisa de avaliação visual em PC e celular. Esta melhoria pertence ao protótipo Godot e não foi portada para o cliente C++/SDL2 que gera o PKG de PS4.
