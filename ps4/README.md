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

- `package.sh`: atalho compatível que chama `build-native.sh` para compilar e empacotar o cliente nativo.
- Pasta de entrada: `ps4/staging/`.
- Saída: `ps4/out/`.

## Requisitos para empacotar

1. Instalar o OpenOrbis PS4 Toolchain v0.5.4 e configurar `OO_PS4_TOOLCHAIN` para a pasta extraída.
2. Instalar `clang++`, `ld.lld`, `make`, Python 3 e o runtime .NET necessário ao `PkgTool.Core`.
3. Executar no Linux:

```bash
export OO_PS4_TOOLCHAIN="/caminho/para/OpenOrbis-PS4-Toolchain"
bash ps4/build-native.sh
```

O script compila o cliente nativo, cria `eboot.bin`, gera o ícone RGB 512×512, cria SFO/GP4 e empacota o PKG em `ps4/out/`. O workflow `.github/workflows/ps4-native.yml` também tenta fazer esse build automaticamente e publica o PKG como artefato do GitHub Actions.

## Bloqueio técnico que ainda falta

O cliente nativo inicial já foi iniciado em SDL2, mas ainda é um protótipo 2D simplificado e não uma conversão do jogo Godot 3D. O próximo trabalho é expandir o port (mundo 3D, inventário/crafting, saves, áudio e controles) e testar o PKG em hardware real. O Godot/GDScript original não é executado pelo EBOOT nativo.

Referências:
- OpenOrbis Toolchain: https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain
- Guia de criação de homebrew e empacotamento: https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/blob/master/docs/MD/Building%20Homebrew.md
- Suporte a consoles no Godot: https://docs.godotengine.org/en/4.5/tutorials/platform/consoles.html
