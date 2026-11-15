# MINEPLUS no PS4 — estado real do port

## Situação atual

O repositório contém um jogo feito em **Godot 4.2/GDScript**. O Godot não oferece exportação oficial para PS4; para consoles é necessário um port específico do motor e templates privados/fornecidos por um portador autorizado. Um arquivo PKG não pode ser produzido apenas empacotando os arquivos `.gd`.

Para PS4 desbloqueado, a rota homebrew aberta é o **OpenOrbis PS4 Toolchain**. Ele compila aplicações nativas para PS4 e fornece ferramentas para criar EBOOT/SELF, GP4 e PKG. Isso ainda exige um executável nativo PS4 real. O código GDScript atual não é esse executável e não pode ser renomeado para `eboot.bin`.

## Port nativo inicial em andamento

- `native/main.cpp`: cliente SDL2 independente do runtime Godot, com menu inicial, mundo em blocos 2D, hotbar, inventário visual e controles de teclado/controle.
- `native/Makefile`: compilação do EBOOT com OpenOrbis.
- `build-native.sh`: gera o EBOOT, ícone RGB, metadados e PKG.
- `.github/workflows/ps4-native.yml`: tenta compilar e publica o PKG como artefato do GitHub Actions.

**Importante:** este é o início de um port nativo separado, não uma exportação do jogo Godot nem uma versão 3D completa. O primeiro build precisa passar na toolchain e ser testado no PS4 antes de ser considerado funcional.

## O que já está preparado neste repositório

- `package.sh`: valida as ferramentas OpenOrbis, o EBOOT e o ícone; cria `param.sfo`, `pkg.gp4` e tenta gerar o PKG.
- Pasta de entrada: `ps4/staging/`.
- Saída: `ps4/out/`.

## Requisitos para empacotar

1. Instalar o OpenOrbis PS4 Toolchain no computador de compilação e configurar `OO_PS4_TOOLCHAIN`.
2. Ter um port nativo PS4 do MINEPLUS que gere `ps4/staging/eboot.bin`.
3. Colocar `ps4/staging/sce_sys/icon0.png` (PNG RGB 512×512, sem alpha).
4. Executar no Linux:

```bash
export OO_PS4_TOOLCHAIN="/caminho/para/OpenOrbis-PS4-Toolchain"
bash ps4/package.sh
```

O script cria o SFO e o GP4 e chama o empacotador do OpenOrbis. O identificador de conteúdo e o Title ID podem ser substituídos pelas variáveis `PKG_CONTENT_ID` e `PKG_TITLE_ID`; os valores padrão são identificadores de homebrew de desenvolvimento e devem ser conferidos antes de distribuir.

## Bloqueio técnico que ainda falta

O passo principal é portar o runtime do jogo. O OpenOrbis não executa projetos Godot/GDScript por si só. É necessário um runtime/port de Godot compatível com PS4 ou reimplementar o cliente usando APIs nativas/SDL2 para PS4, incluindo renderização, input, áudio, arquivos e ciclo de vida. Sem esse EBOOT legítimo, qualquer PKG produzido seria apenas um pacote inválido/incompleto; este repositório não fabrica um falso PKG.

Referências:
- OpenOrbis Toolchain: https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain
- Guia de criação de homebrew e empacotamento: https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/blob/master/docs/MD/Building%20Homebrew.md
- Suporte a consoles no Godot: https://docs.godotengine.org/en/4.5/tutorials/platform/consoles.html
