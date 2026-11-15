#!/usr/bin/env bash
set -euo pipefail

# MINEPLUS PS4 homebrew package stage.
# This packages a real PS4 eboot.bin; it does NOT convert a Godot project to PS4.

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAGE="${ROOT}/ps4/staging"
OUT="${ROOT}/ps4/out"
TOOLCHAIN="${OO_PS4_TOOLCHAIN:-}"

TITLE="${PKG_TITLE:-MINEPLUS}"
TITLE_ID="${PKG_TITLE_ID:-BREW00001}"
CONTENT_ID="${PKG_CONTENT_ID:-IV0000-BREW00001_00-MINEPLUS00000001}"
VERSION="${PKG_VERSION:-1.00}"
PKG_NAME="${PKG_NAME:-MINEPLUS-PS4.pkg}"

fail() { echo "ERRO: $*" >&2; exit 1; }

[[ -n "$TOOLCHAIN" && -d "$TOOLCHAIN" ]] || fail "Defina OO_PS4_TOOLCHAIN para a pasta do OpenOrbis PS4 Toolchain."
PKG_TOOL="${TOOLCHAIN}/bin/linux/PkgTool.Core"
GP4_TOOL="${TOOLCHAIN}/bin/linux/create-gp4"
[[ -x "$PKG_TOOL" ]] || fail "PkgTool.Core não encontrado/executável: $PKG_TOOL"
[[ -x "$GP4_TOOL" ]] || fail "create-gp4 não encontrado/executável: $GP4_TOOL"
[[ -f "${STAGE}/eboot.bin" ]] || fail "Falta ps4/staging/eboot.bin. O projeto Godot atual ainda não possui um port nativo PS4."
[[ -f "${STAGE}/sce_sys/icon0.png" ]] || fail "Falta ps4/staging/sce_sys/icon0.png (PNG RGB 512x512, sem transparência)."

mkdir -p "${STAGE}/sce_sys" "$OUT"
command -v file >/dev/null 2>&1 || fail "Instale o utilitário 'file' para validar o ícone."
ICON_INFO="$(file "${STAGE}/sce_sys/icon0.png")"
echo "$ICON_INFO" | grep -q '512 x 512' || fail "icon0.png precisa ter 512x512 pixels."
echo "$ICON_INFO" | grep -Eqi 'RGB|24-bit' || fail "icon0.png precisa ser PNG RGB de 24 bits, sem alpha."

cd "$STAGE"
"$PKG_TOOL" sfo_new sce_sys/param.sfo
"$PKG_TOOL" sfo_setentry sce_sys/param.sfo APP_TYPE --type Integer --maxsize 4 --value 1
"$PKG_TOOL" sfo_setentry sce_sys/param.sfo APP_VER --type Utf8 --maxsize 8 --value "$VERSION"
"$PKG_TOOL" sfo_setentry sce_sys/param.sfo ATTRIBUTE --type Integer --maxsize 4 --value 0
"$PKG_TOOL" sfo_setentry sce_sys/param.sfo CATEGORY --type Utf8 --maxsize 4 --value gd
"$PKG_TOOL" sfo_setentry sce_sys/param.sfo CONTENT_ID --type Utf8 --maxsize 48 --value "$CONTENT_ID"
"$PKG_TOOL" sfo_setentry sce_sys/param.sfo DOWNLOAD_DATA_SIZE --type Integer --maxsize 4 --value 0
"$PKG_TOOL" sfo_setentry sce_sys/param.sfo SYSTEM_VER --type Integer --maxsize 4 --value 0
"$PKG_TOOL" sfo_setentry sce_sys/param.sfo TITLE --type Utf8 --maxsize 128 --value "$TITLE"
"$PKG_TOOL" sfo_setentry sce_sys/param.sfo TITLE_ID --type Utf8 --maxsize 12 --value "$TITLE_ID"
"$PKG_TOOL" sfo_setentry sce_sys/param.sfo VERSION --type Utf8 --maxsize 8 --value "$VERSION"

FILES="eboot.bin sce_sys/param.sfo sce_sys/icon0.png"
if [[ -f "sce_sys/about/right.sprx" ]]; then
  FILES="$FILES sce_sys/about/right.sprx"
fi
"$GP4_TOOL" -out pkg.gp4 --content-id="$CONTENT_ID" --files "$FILES"
"$PKG_TOOL" pkg_build pkg.gp4 "$OUT"
[[ -s "${OUT}/${PKG_NAME}" ]] || {
  # LibOrbisPkg may choose a name from the GP4 metadata; report the actual output.
  echo "PKG criado. Arquivos gerados em: $OUT"
  find "$OUT" -maxdepth 1 -type f -name '*.pkg' -print
  exit 0
}
echo "PKG criado: ${OUT}/${PKG_NAME}"
