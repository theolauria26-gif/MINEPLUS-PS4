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
S=512
im=[[(10,16,22) for x in range(S)] for y in range(S)]
def pixel(x,y,c):
 if 0<=x<S and 0<=y<S: im[y][x]=c
def rect(x,y,w,h,c):
 for yy in range(max(0,y),min(S,y+h)):
  for xx in range(max(0,x),min(S,x+w)): pixel(xx,yy,c)
def poly(points,c):
 ymin=max(0,min(y for x,y in points)); ymax=min(S-1,max(y for x,y in points))
 for y in range(ymin,ymax+1):
  xs=[]
  for i in range(len(points)):
   x1,y1=points[i];x2,y2=points[(i+1)%len(points)]
   if y1==y2: continue
   if min(y1,y2)<=y<max(y1,y2): xs.append(int(x1+(y-y1)*(x2-x1)/(y2-y1)))
  xs.sort()
  for i in range(0,len(xs)-1,2): rect(xs[i],y,xs[i+1]-xs[i]+1,1,c)
def line(x1,y1,x2,y2,c,w=1):
 dx=abs(x2-x1);sx=1 if x1<x2 else -1;dy=-abs(y2-y1);sy=1 if y1<y2 else -1;err=dx+dy
 while True:
  rect(x1-w//2,y1-w//2,w,w,c)
  if x1==x2 and y1==y2: break
  e=2*err
  if e>=dy:err+=dy;x1+=sx
  if e<=dx:err+=dx;y1+=sy
for y in range(S):
 for x in range(S):
  d=((x-256)**2+(y-220)**2)**.5
  glow=max(0,1-d/370)
  pixel(x,y,(10+int(4*glow),16+int(22*glow),22+int(18*glow)))
for y in range(0,S,8): rect(0,y,S,1,(12,22,27))
rect(82,62,348,348,(5,10,14))
poly([(256,78),(392,154),(256,232),(120,154)],(43,202,133))
poly([(120,154),(256,232),(256,390),(120,312)],(23,130,91))
poly([(256,232),(392,154),(392,312),(256,390)],(18,92,68))
line(256,78,256,232,(115,255,187),7)
line(120,154,256,232,(93,245,169),7)
line(392,154,256,232,(93,245,169),7)
line(256,232,256,390,(93,245,169),7)
for yy in range(4):
 for xx in range(4): rect(153+xx*26,183+yy*26,12,12,(34,177,117) if (xx+yy)%2==0 else (20,105,77))
font={
'M':["10001","11011","10101","10101","10001","10001","10001"],
'I':["11111","00100","00100","00100","00100","00100","11111"],
'N':["10001","11001","10101","10011","10001","10001","10001"],
'E':["11111","10000","10000","11110","10000","10000","11111"],
'P':["11110","10001","10001","11110","10000","10000","10000"],
'L':["10000","10000","10000","10000","10000","10000","11111"],
'U':["10001","10001","10001","10001","10001","10001","01110"],
'S':["01111","10000","10000","01110","00001","00001","11110"]}
word="MINEPLUS";scale=7;spacing=8;total=len(word)*(5*scale+spacing)-spacing;x0=(S-total)//2
for ch in word:
 for yy,row in enumerate(font[ch]):
  for xx,v in enumerate(row):
   if v=="1":rect(x0+xx*scale,432+yy*scale,scale,scale,(225,245,236))
 x0+=5*scale+spacing
rect(175,498,162,4,(45,196,130))
def chunk(t,d):return struct.pack(">I",len(d))+t+d+struct.pack(">I",zlib.crc32(t+d)&0xffffffff)
rows=b"".join(b"\x00"+bytes(v for rgb in row for v in rgb) for row in im)
png=b"\x89PNG\r\n\x1a\n"+chunk(b"IHDR",struct.pack(">2I5B",S,S,8,2,0,0,0))+chunk(b"IDAT",zlib.compress(rows,9))+chunk(b"IEND",b"")
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
