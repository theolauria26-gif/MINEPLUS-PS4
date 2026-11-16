#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PS4_DIR="$ROOT/ps4"
NATIVE="$PS4_DIR/native"
STAGE="$PS4_DIR/staging"
OUT="$PS4_DIR/out"
TOOLCHAIN="${OO_PS4_TOOLCHAIN:-}"
fail(){ echo "ERRO: $*" >&2; exit 1; }
[[ -n "$TOOLCHAIN" && -d "$TOOLCHAIN" ]] || fail "Defina OO_PS4_TOOLCHAIN para OpenOrbis v0.5.4."
for tool in clang++ ld.lld make python3; do command -v "$tool" >/dev/null || fail "Ferramenta ausente: $tool"; done
for tool in create-fself create-gp4; do [[ -x "$TOOLCHAIN/bin/linux/$tool" ]] || fail "Ferramenta ausente: $TOOLCHAIN/bin/linux/$tool"; done
PKG="$TOOLCHAIN/bin/linux/PkgTool.Core"
[[ -x "$PKG" ]] || fail "PkgTool.Core ausente."
export OO_PS4_TOOLCHAIN="$TOOLCHAIN"
mkdir -p "$STAGE/sce_sys" "$STAGE/sce_module" "$OUT"
make -C "$NATIVE" clean all
cp "$NATIVE/eboot.bin" "$STAGE/eboot.bin"
python3 - "$STAGE/sce_sys/icon0.png" <<'PY'
import struct,zlib,sys
w=h=512
rows=[]
for y in range(h):
 row=bytearray([0])
 for x in range(w):
  if 80<x<432 and 105<y<407:
   c=(45,160,112) if x<112 or x>400 or y<137 or y>375 else ((25,34,42) if ((x//32+y//32)%2) else (34,48,57))
  else:c=(13,18,24)
  row.extend(c)
 rows.append(bytes(row))
def chunk(t,d):return struct.pack(">I",len(d))+t+d+struct.pack(">I",zlib.crc32(t+d)&0xffffffff)
png=b"\x89PNG\r\n\x1a\n"+chunk(b"IHDR",struct.pack(">2I5B",w,h,8,2,0,0,0))+chunk(b"IDAT",zlib.compress(b"".join(rows),9))+chunk(b"IEND",b"")
open(sys.argv[1],"wb").write(png)
PY
# Include the standard PS4 homebrew rights resource when the toolchain provides it.
RIGHT_SPRX="$(find "$TOOLCHAIN/samples" -type f -path '*/sce_sys/about/right.sprx' -print -quit)"
if [[ -n "$RIGHT_SPRX" ]]; then
 mkdir -p "$STAGE/sce_sys/about"
 cp "$RIGHT_SPRX" "$STAGE/sce_sys/about/right.sprx"
fi
# Bundle runtime modules shipped by the toolchain, if present.
for module in libc.prx libSceFios2.prx; do
 [[ ! -f "$TOOLCHAIN/bin/data/modules/$module" ]] || cp "$TOOLCHAIN/bin/data/modules/$module" "$STAGE/sce_module/$module"
done
cd "$STAGE"
"$PKG" sfo_new sce_sys/param.sfo
"$PKG" sfo_setentry sce_sys/param.sfo APP_TYPE --type Integer --maxsize 4 --value 1
"$PKG" sfo_setentry sce_sys/param.sfo APP_VER --type Utf8 --maxsize 8 --value 1.00
"$PKG" sfo_setentry sce_sys/param.sfo ATTRIBUTE --type Integer --maxsize 4 --value 0
"$PKG" sfo_setentry sce_sys/param.sfo CATEGORY --type Utf8 --maxsize 4 --value gd
"$PKG" sfo_setentry sce_sys/param.sfo CONTENT_ID --type Utf8 --maxsize 48 --value IV0000-MNPX00001_00-MINEPLUS00000001
"$PKG" sfo_setentry sce_sys/param.sfo DOWNLOAD_DATA_SIZE --type Integer --maxsize 4 --value 0
"$PKG" sfo_setentry sce_sys/param.sfo SYSTEM_VER --type Integer --maxsize 4 --value 0
"$PKG" sfo_setentry sce_sys/param.sfo TITLE --type Utf8 --maxsize 128 --value MINEPLUS
"$PKG" sfo_setentry sce_sys/param.sfo TITLE_ID --type Utf8 --maxsize 12 --value MNPX00001
"$PKG" sfo_setentry sce_sys/param.sfo VERSION --type Utf8 --maxsize 8 --value 1.00
FILES=(eboot.bin sce_sys/param.sfo sce_sys/icon0.png)
[[ ! -f sce_sys/about/right.sprx ]] || FILES+=(sce_sys/about/right.sprx)
for f in sce_module/*; do [[ ! -f "$f" ]] || FILES+=("$f"); done
"$TOOLCHAIN/bin/linux/create-gp4" -out pkg.gp4 --content-id=IV0000-MNPX00001_00-MINEPLUS00000001 --files "${FILES[*]}"
"$PKG" pkg_build pkg.gp4 "$OUT"
PKG_FILE="$(find "$OUT" -maxdepth 1 -type f -name '*.pkg' -print -quit)"
[[ -n "$PKG_FILE" && -s "$PKG_FILE" ]] || fail "Nenhum PKG válido foi gerado."
"$PKG" pkg_validate --verbose "$PKG_FILE"
python3 - "$PKG_FILE" <<'PY'
import os,sys
path=sys.argv[1]
with open(path,"rb") as f: magic=f.read(4)
if magic != bytes.fromhex("7f434e54"):
    raise SystemExit("ERRO: o arquivo gerado não possui assinatura PKG PS4 (7f434e54).")
size=os.path.getsize(path)
if size < 4096:
    raise SystemExit("ERRO: PKG muito pequeno; pacote provavelmente incompleto.")
print(f"PKG PS4 validado: {path} ({size} bytes)")
PY
echo "PKG criado em $PKG_FILE"
