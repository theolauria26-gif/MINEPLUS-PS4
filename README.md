# MINEPLUS

Remake/protótipo independente em Godot 4, com identidade visual própria. Não inclui código, marcas, assets ou conteúdo proprietário de Minecraft.

## O que já está no repositório

- Lobby inicial escuro, translúcido e com botões arredondados.
- Cena 3D jogável com terreno em blocos, iluminação e céu.
- Personagem em primeira pessoa com colisão, gravidade e pulo.
- Interação por raycast: quebrar e colocar blocos.
- Cinco materiais básicos para construção.
- HUD com mira, instruções e indicação do bloco selecionado.
- Controles de teclado/mouse e botões virtuais de movimento para telas touch.
- Renderizador Compatibility para favorecer dispositivos móveis e hardware limitado.

## Controles atuais

| Ação | Teclado / PC | Touch |
|---|---|---|
| Andar | WASD | Botões direcionais na tela |
| Pular | Espaço | Botão PULAR |
| Olhar | Mouse | Arrastar no lado direito |
| Quebrar bloco | Clique esquerdo | Botão QUEBRAR ou toque na área de jogo |
| Colocar bloco | Clique direito | Botão COLOCAR |
| Selecionar material | Teclas 1–5 | Em desenvolvimento |
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

- Melhorar controles touch e interação de colocação.
- Inventário funcional, salvamento e carregamento de mundos.
- Geração de terreno e árvores básicas já implementadas; recursos e biomas ainda pendentes.
- Animações, áudio, opções gráficas e otimização.
- Pipeline de exportação Android e validação em dispositivo.
