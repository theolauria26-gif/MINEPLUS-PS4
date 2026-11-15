# MINEPLUS

Remake/protótipo independente em Godot 4, com identidade visual própria. Não inclui código, marcas, assets ou conteúdo proprietário de Minecraft.

## O que já está no repositório

- Lobby inicial escuro, translúcido e com botões arredondados.
- Cena 3D jogável com terreno em blocos, iluminação e céu.
- Personagem em primeira pessoa com colisão, gravidade e pulo.
- Interação por raycast: quebrar e colocar blocos.
- Seis materiais básicos para construção, incluindo tijolo fabricável.
- HUD com mira, instruções e indicação do bloco selecionado.
- Controles de teclado/mouse e botões virtuais de movimento para telas touch.
- Barra de seleção de seis blocos com botões touch e teclas 1–6.
- Inventário inicial por pilhas: colocar consome blocos e quebrar recupera o material.
- Bancada de fabricação com receitas: 2 Terras + 1 Pedra produzem 2 Tijolos; 3 Pedras + 2 Tijolos fabricam uma picareta com 40 usos.
- Sexto material (Tijolo) selecionável por toque e pela tecla 6.
- Inventário visual acessível pelo botão INVENTÁRIO ou tecla E, com contagem dos materiais e durabilidade da picareta.
- Picareta com durabilidade: minerar Pedra e Tijolo consome um uso; a ferramenta e sua durabilidade são salvas.
- Configurações de FOV e sensibilidade da câmera, ajustáveis no lobby e persistidas em `user://mineplus_settings.cfg`.
- Salvamento local do mundo em `user://mineplus_world.json`, com botão SALVAR MUNDO e salvamento ao fechar.
- Renderizador Compatibility para favorecer dispositivos móveis e hardware limitado.

## Controles atuais

| Ação | Teclado / PC | Touch |
|---|---|---|
| Andar | WASD | Botões direcionais na tela |
| Pular | Espaço | Botão PULAR |
| Olhar | Mouse | Arrastar no lado direito |
| Quebrar bloco | Clique esquerdo | Botão QUEBRAR ou toque na área de jogo |
| Colocar bloco | Clique direito | Botão COLOCAR |
| Selecionar material | Teclas 1–6 | Botões da barra inferior |
| Abrir inventário | E | Botão INVENTÁRIO |
| Liberar mouse | Esc | — |

## Abrir o projeto

1. Instale Godot 4.x.
2. Baixe/clique em **Code → Download ZIP** neste repositório.
3. Extraia a pasta.
4. No Godot, importe o arquivo `project.godot`.

## Exportação

- **Android:** requer templates de exportação da mesma versão do Godot e configuração do Android SDK/JDK.
- **Windows/Linux:** pode ser exportado pelo editor Godot após instalar os templates.
- **PS4:** não existe exportação oficial do Godot para PS4. Um PKG real depende de toolchain/homebrew compatível, SDK e ambiente de compilação específico. O projeto ainda não foi compilado nem testado em hardware PS4.

## Próximas etapas

- Mais receitas, categorias de itens e slots de equipamento no inventário.
- Testar e refinar controles touch, salvamento e carregamento em dispositivo real.
- Geração de terreno e árvores básicas já implementadas; recursos e biomas ainda pendentes.
- Skins, animações, áudio, opções gráficas adicionais e otimização.
- Pipeline de exportação Android e validação em dispositivo.
