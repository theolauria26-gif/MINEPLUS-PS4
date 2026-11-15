# MINEPLUS

Remake/protótipo independente em Godot 4, com identidade visual própria. Não inclui código, marcas, assets ou conteúdo proprietário de Minecraft.

## O que já está no repositório

- Lobby inicial escuro, translúcido e com botões arredondados.
- Cena 3D jogável com terreno em blocos, iluminação e céu.
- Personagem em primeira pessoa com colisão, gravidade, pulo e animação procedural suave de caminhada.
- Interação por raycast: quebrar e colocar blocos.
- Seis materiais básicos para construção, incluindo tijolo fabricável, com texturas pixeladas procedurais geradas em runtime.
- HUD com mira, instruções e indicação do bloco selecionado.
- Controles de teclado/mouse e botões virtuais de movimento para telas touch, com dimensionamento responsivo em telas estreitas.
- Barra de seleção de seis blocos com botões touch e teclas 1–6, incluindo destaque visual do slot ativo.
- Inventário inicial por pilhas: colocar consome blocos e quebrar recupera o material.
- Pilhas limitadas a 999 unidades por material, proteção contra colocar blocos dentro do jogador e bloqueio de mineração quando a pilha está cheia.
- Recuperação automática: ao cair abaixo do mapa, o jogador retorna ao ponto inicial com velocidade e câmera reiniciadas.
- Receitas fabricadas são gravadas imediatamente no arquivo do mundo para reduzir perda de progresso.
- Personalização da HUD: ajuste de opacidade entre 45% e 100%, salvo nas configurações e aplicado ao entrar no mundo.
- Bancada de fabricação com receitas: 2 Terras + 1 Pedra produzem 2 Tijolos; 3 Pedras + 2 Tijolos fabricam uma picareta com 40 usos.
- Sexto material (Tijolo) selecionável por toque e pela tecla 6.
- Inventário visual acessível pelo botão INVENTÁRIO ou tecla E, com contagem dos materiais e durabilidade da picareta.
- Menu de pausa por ESC ou botão PAUSAR, com continuar, salvar e continuar, e salvar e voltar ao lobby.
- Picareta visível em primeira pessoa, com animação de golpe e durabilidade: minerar Pedra e Tijolo consome um uso; a ferramenta e sua durabilidade são salvas.
- Configurações de FOV, sensibilidade da câmera e qualidade gráfica (baixa/equilibrada/alta), ajustáveis no lobby e persistidas em `user://mineplus_settings.cfg`.
- Perfil baixo desativa sombras dinâmicas; o perfil alto ativa sombras para dispositivos compatíveis.
- Quatro opções de aparência de personagem, com braços visíveis em primeira pessoa e escolha salva nas configurações.
- Três slots independentes de mundo, selecionáveis nas configurações; o primeiro mantém compatibilidade com o save antigo.
- Salvamento automático a cada 45 segundos e ao pausar o aplicativo, além do botão manual.
- Salvamento local do mundo em `user://mineplus_world.json`, com botão SALVAR MUNDO e salvamento ao fechar.
- Save versão 3: também preserva posição do jogador, direção da câmera, inclinação vertical e material selecionado; saves antigos continuam carregando com valores padrão.
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
- **PS4:** iniciado um cliente nativo separado em C++/SDL2 em `ps4/native/`, com Makefile OpenOrbis e workflow `.github/workflows/ps4-native.yml` para compilar EBOOT e publicar o PKG como artefato do GitHub Actions. Este primeiro port é um protótipo 2D de controles e mundo em blocos; ainda não reproduz o cliente 3D Godot. O build precisa passar na toolchain e ser testado no console.

## Próximas etapas

- Mais receitas, categorias de itens e slots de equipamento no inventário.
- Testar e refinar controles touch, salvamento e carregamento em dispositivo real; validar os três slots.
- Geração de terreno e árvores básicas já implementadas; recursos e biomas ainda pendentes.
- Modelo completo de personagem, animações de movimento adicionais, áudio e otimização avançada por chunks.
- Pipeline de exportação Android e validação em dispositivo.
