#include <time.h>
#include <SDL2/SDL.h>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#include "pcg_world.hpp"
#include "world_grid.hpp"
namespace {
constexpr int W=1280,H=720,COLS=2048,ROWS=2048,LEGACY_COLS=1024,LEGACY_ROWS=1024,TILE=36,VIEW_COLS=24,VIEW_ROWS=14;
enum Tile:unsigned char{Air=0,Grass=1,Dirt=2,Stone=3,Wood=4,Moss=5,Crystal=6,Bone=7,DarkStone=8,Sand=9,Snow=10,Ice=11,Lava=12,Water=13,IronOre=14,Obsidian=15,Ember=16,Rune=17,IronBar=18,ObsidianCore=19,EmberCatalyst=20,AncientRune=21};
std::array<unsigned char,COLS*ROWS> world{};
mineplus::FixedWorldGrid fixedWorldGrid(world.data(), COLS, ROWS);
mineplus::WorldGridView& worldGrid = fixedWorldGrid;
inline unsigned char tile_at(int x,int y){unsigned char t=Air;worldGrid.get(x,y,t);return t;}
inline bool set_tile_at(int x,int y,unsigned char t){return worldGrid.set(x,y,t);}
// Cached procedural elevation avoids recomputing multi-octave noise for every ray step.
std::array<unsigned char,COLS*ROWS> heightMap{};
// Large save buffers live in static storage, not on the PS4 main-thread stack.
// A 2048x2048 byte world is 4 MiB and can overflow the small homebrew thread stack.
// Keep migration and restore scratch space out of stack frames on every save version.
std::array<unsigned char,COLS*ROWS> loadWorldBuffer{};
std::array<unsigned char,LEGACY_COLS*LEGACY_ROWS> legacyWorldBuffer{};
int px=COLS/2,py=ROWS/2,selected=0,faceX=1,faceY=0,playerHealth=10,selectedRecipe=0;
std::uint64_t worldSeed=4839201ULL;
Uint32 lastDamage=0,damageCooldown=0;
struct Creature{int x,y,type,hp;};std::array<Creature,96> creatures{};std::array<int,22> stock{{24,18,12,8,6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
unsigned hash2(int x,int y){unsigned n=(unsigned)(x*374761393u+y*668265263u+0x9e3779b9u);n=(n^(n>>13))*1274126177u;return n^(n>>16);}
bool running=true,playing=false,inventory=false,paused=false;
Uint32 lastMove=0,lastTurn=0;
SDL_Window* window=nullptr;SDL_Renderer* renderer=nullptr;SDL_GameController* pad=nullptr;
void color(Uint8 r,Uint8 g,Uint8 b,Uint8 a=255){SDL_SetRenderDrawColor(renderer,r,g,b,a);}
void rect(int x,int y,int w,int h,Uint8 r,Uint8 g,Uint8 b,Uint8 a=255){color(r,g,b,a);SDL_Rect q{x,y,w,h};SDL_RenderFillRect(renderer,&q);}
void outline(int x,int y,int w,int h,Uint8 r,Uint8 g,Uint8 b){color(r,g,b);SDL_Rect q{x,y,w,h};SDL_RenderDrawRect(renderer,&q);}
const char* glyph(char c){switch(c){
case 'A':return "01110""10001""10001""11111""10001""10001""10001";
case 'B':return "11110""10001""10001""11110""10001""10001""11110";
case 'C':return "01111""10000""10000""10000""10000""10000""01111";
case 'D':return "11110""10001""10001""10001""10001""10001""11110";
case 'E':return "11111""10000""10000""11110""10000""10000""11111";
case 'F':return "11111""10000""10000""11110""10000""10000""10000";
case 'G':return "01111""10000""10000""10111""10001""10001""01111";
case 'H':return "10001""10001""10001""11111""10001""10001""10001";
case 'I':return "11111""00100""00100""00100""00100""00100""11111";
case 'K':return "10001""10010""10100""11000""10100""10010""10001";
case 'L':return "10000""10000""10000""10000""10000""10000""11111";
case 'M':return "10001""11011""10101""10101""10001""10001""10001";
case 'N':return "10001""11001""10101""10011""10001""10001""10001";
case 'O':return "01110""10001""10001""10001""10001""10001""01110";
case 'P':return "11110""10001""10001""11110""10000""10000""10000";
case 'R':return "11110""10001""10001""11110""10100""10010""10001";
case 'S':return "01111""10000""10000""01110""00001""00001""11110";
case 'T':return "11111""00100""00100""00100""00100""00100""00100";
case 'U':return "10001""10001""10001""10001""10001""10001""01110";
case 'V':return "10001""10001""10001""10001""10001""01010""00100";
case 'W':return "10001""10001""10001""10101""10101""10101""01010";
case 'Y':return "10001""10001""01010""00100""00100""00100""00100";
case '0':return "01110""10001""10011""10101""11001""10001""01110";
case '1':return "00100""01100""00100""00100""00100""00100""01110";
case '2':return "01110""10001""00001""00010""00100""01000""11111";
case '3':return "11110""00001""00001""01110""00001""00001""11110";
case '4':return "00010""00110""01010""10010""11111""00010""00010";
case '5':return "11111""10000""10000""11110""00001""00001""11110";
case ' ':return "00000""00000""00000""00000""00000""00000""00000";
default:return "00000""00000""00000""00000""00000""00000""00000";}}
void draw_text(const char*s,int x,int y,int scale,Uint8 r=230,Uint8 g=240,Uint8 b=236){color(r,g,b);for(;*s;s++,x+=6*scale){const char*p=glyph(*s);for(int yy=0;yy<7;yy++)for(int xx=0;xx<5;xx++)if(p[yy*5+xx]=='1'){SDL_Rect q{x+xx*scale,y+yy*scale,scale,scale};SDL_RenderFillRect(renderer,&q);}}}
void init_world(){
 // PS4-native PCG sampling. Every coordinate is reproducible from one seed;
 // this 2048x2048 resident window is a transitional cache, not the final world limit.
 mineplus::PcgWorld pcg(worldSeed);
 for(int y=0;y<ROWS;y++)for(int x=0;x<COLS;x++){
  const int wx=x-COLS/2, wy=y-ROWS/2;
  const auto sample=pcg.sample_surface(wx,wy);
  unsigned char tile=sample.surface;
  const unsigned h=hash2(x,y);
  // Sparse resources are coordinate-hashed, so revisiting an area never rerolls it.
  if(tile==Stone && h%389u==0u) tile=IronOre;
  else if((tile==Stone||tile==DarkStone)&&h%12007u==0u) tile=Rune;
  world[y*COLS+x]=tile;
  heightMap[y*COLS+x]=static_cast<unsigned char>(std::clamp<int>(sample.height,0,255));
 }
 // Resolve each 96x96 structure cell once, then rasterize a distinct
 // landmark silhouette. Every pattern is seed/coordinate deterministic.
 for(int cy=-13;cy<=13;++cy)for(int cx=-13;cx<=13;++cx){
  mineplus::StructureAnchor anchor{};
  if(!pcg.structure_anchor_for_cell(cx,cy,anchor))continue;
  const int radius=anchor.radius,r2=radius*radius;
  for(int oy=-radius;oy<=radius;++oy)for(int ox=-radius;ox<=radius;++ox){
   const int d=ox*ox+oy*oy;
   if(d>r2)continue;
   const int lx=anchor.x+ox+COLS/2,ly=anchor.y+oy+ROWS/2;
   if(lx<0||ly<0||lx>=COLS||ly>=ROWS)continue;
   unsigned char tile=Air;
   const bool cardinal=(ox==0||oy==0);
   switch(anchor.kind){
    case 1: // Standing-stone circle with four rune pillars.
     if(d>=r2-2)tile=DarkStone;
     else if(cardinal&&d>r2/3)tile=Bone;
     else if(d<4)tile=Rune;
     break;
    case 2: // Ruined shrine: broken wall, corner crystals and a gate.
     if(d>=r2-3)tile=DarkStone;
     if((abs(ox)==radius||abs(oy)==radius)&&d<r2)tile=DarkStone;
     if(abs(ox)>=radius-2&&abs(oy)>=radius-2)tile=Crystal;
     if(oy==-radius&&abs(ox)<=1)tile=Air;
     if(d<9&&tile==Air)tile=Rune;
     break;
    case 3: // Crystal grove with mineral spires radiating from its core.
     if(d>=r2-3)tile=Moss;
     else if((abs(ox)<=1&&abs(oy)>2)||(abs(oy)<=1&&abs(ox)>2))tile=Crystal;
     else if(d<4)tile=ObsidianCore;
     break;
    default: // Bone altar, dark ring and central ancient relic.
     if(d>=r2-2)tile=Bone;
     else if(cardinal&&d>r2/4)tile=DarkStone;
     else if(d<5)tile=AncientRune;
     break;
   }
   if(tile!=Air)world[ly*COLS+lx]=tile;
  }
 }
 // Large ancient keeps, watchtowers, shrines and circular ruins.
 for(int cy=24;cy<ROWS-24;cy+=61)for(int cx=24;cx<COLS-24;cx+=73){
  unsigned seed=hash2(cx,cy);int w=9+(seed%13),h=8+(hash2(cy,cx)%11);
  for(int y=cy;y<cy+h;y++)for(int x=cx;x<cx+w;x++)
   if(x==cx||x==cx+w-1||y==cy||y==cy+h-1)world[y*COLS+x]=DarkStone;
  world[(cy+h/2)*COLS+cx+w/2]=Air;
  world[(cy+1)*COLS+cx+1]=Crystal;
  for(int k=0;k<6;k++){int tx=cx+(int)(hash2(cx+k,cy)%w),ty=cy+(int)(hash2(cy+k,cx)%h);world[ty*COLS+tx]=Bone;}
  // Four corner towers.
  world[(cy+1)*COLS+cx+1]=DarkStone;world[(cy+1)*COLS+cx+w-2]=DarkStone;
  world[(cy+h-2)*COLS+cx+1]=DarkStone;world[(cy+h-2)*COLS+cx+w-2]=DarkStone;
 }
 // Dense dead forests, crystal groves and scattered natural landmarks.
 for(int y=3;y<ROWS-3;y++)for(int x=3;x<COLS-3;x++){
  unsigned h=hash2(x,y);
  if(h%137==0&&(world[y*COLS+x]==Grass||world[y*COLS+x]==Snow))world[y*COLS+x]=Wood;
  if(h%1201==0)world[y*COLS+x]=Crystal;
  if(h%1807==0)world[y*COLS+x]=Bone;
 }
 // Natural water basins and volcanic pools seeded across the expanded continent.
 for(int cy=32;cy<ROWS-32;cy+=83)for(int cx=32;cx<COLS-32;cx+=89){
  unsigned seed=hash2(cx,cy);if(seed%5!=0)continue;
  int radius=4+int(seed%7);bool volcanic=((seed>>8)%4)==0;
  for(int oy=-radius-2;oy<=radius+2;oy++)for(int ox=-radius-2;ox<=radius+2;ox++){
   int x=cx+ox,y=cy+oy,d=ox*ox+oy*oy;
   if(d<=radius*radius)world[y*COLS+x]=volcanic?Lava:Water;
   else if(d<=(radius+2)*(radius+2))world[y*COLS+x]=volcanic?DarkStone:Sand;
  }
 }
 // Rare monumental citadels with courtyards, corner towers and central relics.
 for(int cy=128;cy<ROWS-128;cy+=257)for(int cx=128;cx<COLS-128;cx+=263){
  unsigned seed=hash2(cx+71,cy+193);if(seed%3!=0)continue;
  int w=25+int(seed%17),h=23+int(hash2(cy,cx)%15);
  for(int y=cy;y<cy+h;y++)for(int x=cx;x<cx+w;x++){
   bool edge=x==cx||x==cx+w-1||y==cy||y==cy+h-1;
   if(edge)world[y*COLS+x]=DarkStone;
   else if((x==cx+3||x==cx+w-4)&&(y%5==0))world[y*COLS+x]=Bone;
  }
  for(int k=0;k<4;k++){
   int tx=cx+(k%2)*(w-1),ty=cy+(k/2)*(h-1);
   for(int oy=-2;oy<=2;oy++)for(int ox=-2;ox<=2;ox++)
    world[(ty+oy)*COLS+tx+ox]=DarkStone;
  }
  int mx=cx+w/2,my=cy+h/2;
  world[my*COLS+mx]=Crystal;
  world[my*COLS+mx-1]=Bone;world[my*COLS+mx+1]=Bone;
  world[(my-1)*COLS+mx]=Bone;world[(my+1)*COLS+mx]=Bone;
 }
 // Standing-stone circles, meteor scars and abandoned camps.
 for(int cy=74;cy<ROWS-74;cy+=149)for(int cx=69;cx<COLS-69;cx+=157){unsigned seed=hash2(cx+313,cy+127);if(seed%4!=0)continue;int radius=5+int(seed%4);
  for(int oy=-radius;oy<=radius;oy++)for(int ox=-radius;ox<=radius;ox++){int d=ox*ox+oy*oy;if(d>=radius*radius-3&&d<=radius*radius+3)world[(cy+oy)*COLS+cx+ox]=DarkStone;}
  world[(cy-2)*COLS+cx]=Bone;world[(cy+2)*COLS+cx]=Bone;world[cy*COLS+cx-2]=Bone;world[cy*COLS+cx+2]=Bone;world[cy*COLS+cx]=Rune;
 }
 for(int cy=112;cy<ROWS-112;cy+=223)for(int cx=101;cx<COLS-101;cx+=239){unsigned seed=hash2(cx+811,cy+419);if(seed%5!=0)continue;int radius=4+int(seed%3);
  for(int oy=-radius;oy<=radius;oy++)for(int ox=-radius;ox<=radius;ox++){int x=cx+ox,y=cy+oy,d=ox*ox+oy*oy;if(d<=radius*radius)world[y*COLS+x]=(d>radius*radius-radius*2)?Obsidian:Ember;}world[cy*COLS+cx]=Rune;
 }
 for(int cy=55;cy<ROWS-55;cy+=131)for(int cx=55;cx<COLS-55;cx+=137){unsigned seed=hash2(cx+17,cy+29);if(seed%7!=0)continue;
  for(int oy=-4;oy<=4;oy++)for(int ox=-4;ox<=4;ox++){int x=cx+ox,y=cy+oy;if(abs(ox)==4||abs(oy)==4)world[y*COLS+x]=((ox+oy)&1)?Wood:DarkStone;else if((ox==0&&oy==0)||((ox*ox+oy*oy)<5&&seed%2))world[y*COLS+x]=Bone;}world[(cy-1)*COLS+cx]=Crystal;
 }
 for(int y=4;y<ROWS-4;y++)for(int x=4;x<COLS-4;x++){unsigned ore=hash2(x/2+991,y/2+577);if(world[y*COLS+x]==Stone&&ore%389==0)world[y*COLS+x]=IronOre;if((world[y*COLS+x]==DarkStone||world[y*COLS+x]==Stone)&&ore%12007==0)world[y*COLS+x]=Rune;}
 for(int i=0;i<(int)creatures.size();i++){auto &m=creatures[i];m.x=8+(int)(hash2(i,91)%(COLS-16));m.y=8+(int)(hash2(91,i)%(ROWS-16));m.type=(int)(hash2(m.x,m.y)%12);m.hp=3;}
 world[py*COLS+px]=Air;
}
constexpr const char* SAVE_DIR="/data/MINEPLUS";
constexpr const char* SAVE_FILE="/data/MINEPLUS/world.sav";
struct SaveHeader{char magic[8];unsigned version;int x,y,selected,fx,fy;int stock[9];};
bool save_world(){
 mkdir(SAVE_DIR,0777);
 const char* temp="/data/MINEPLUS/world.sav.tmp";
 FILE* f=std::fopen(temp,"wb");if(!f)return false;
 SaveHeader h{{'M','P','L','U','S','3','D','\0'},8,px,py,selected,faceX,faceY,{}};
 for(int i=0;i<9;i++)h.stock[i]=stock[i];
 bool ok=std::fwrite(&h,sizeof(h),1,f)==1&&std::fwrite(&worldSeed,sizeof(worldSeed),1,f)==1&&std::fwrite(world.data(),1,world.size(),f)==world.size()&&std::fwrite(creatures.data(),sizeof(Creature),creatures.size(),f)==creatures.size()&&std::fwrite(&playerHealth,sizeof(playerHealth),1,f)==1&&std::fwrite(stock.data()+9,sizeof(int),13,f)==13;
 if(std::fflush(f)!=0)ok=false;
 if(ok&&::fsync(::fileno(f))!=0)ok=false;
 if(std::fclose(f)!=0)ok=false;
 if(ok&&std::rename(temp,SAVE_FILE)!=0)ok=false;
 if(!ok)std::remove(temp);
 return ok;
}
bool load_world(){
  FILE* f=std::fopen(SAVE_FILE,"rb");if(!f)return false;
  SaveHeader h{};
  bool ok=std::fread(&h,sizeof(h),1,f)==1&&std::memcmp(h.magic,"MPLUS3D",7)==0&&(h.version>=1&&h.version<=8);
  const bool legacy=h.version<5;
  auto &restored=loadWorldBuffer;
  restored.fill(0);
  std::uint64_t restoredSeed=4839201ULL;
  if(ok&&h.version>=8)ok=std::fread(&restoredSeed,sizeof(restoredSeed),1,f)==1;
  if(ok&&legacy){
   auto &oldWorld=legacyWorldBuffer;
   ok=std::fread(oldWorld.data(),1,oldWorld.size(),f)==oldWorld.size();
   if(ok)for(int y=0;y<LEGACY_ROWS;y++)
    std::copy_n(oldWorld.data()+size_t(y)*LEGACY_COLS,LEGACY_COLS,
                restored.data()+size_t(y+ROWS/2-LEGACY_ROWS/2)*COLS+(COLS/2-LEGACY_COLS/2));
  }else if(ok)ok=std::fread(restored.data(),1,restored.size(),f)==restored.size();
  std::array<Creature,96> restoredCreatures{};
  if(ok&&(h.version==2||h.version==3)){
   std::array<Creature,48> legacyCreatures{};
   ok=std::fread(legacyCreatures.data(),sizeof(Creature),legacyCreatures.size(),f)==legacyCreatures.size();
   if(ok)std::copy(legacyCreatures.begin(),legacyCreatures.end(),restoredCreatures.begin());
  }else if(ok&&h.version>=4)ok=std::fread(restoredCreatures.data(),sizeof(Creature),restoredCreatures.size(),f)==restoredCreatures.size();
  int restoredHealth=10;if(ok&&h.version>=3)ok=std::fread(&restoredHealth,sizeof(restoredHealth),1,f)==1;
  std::array<int,13> extraStock{};if(ok&&h.version>=6){size_t extraCount=h.version>=7?13:9;ok=std::fread(extraStock.data(),sizeof(int),extraCount,f)==extraCount;}
  if(ok&&std::fgetc(f)!=EOF)ok=false;
  std::fclose(f);if(!ok)return false;
  // Reject impossible tile IDs before they reach rendering, collision or inventory.
  // This protects the live world from structurally valid but semantically corrupt saves.
  for(const auto tile:restored)if(tile>AncientRune)return false;
  if(h.version>=2){
   for(auto &m:restoredCreatures){
    if(m.type<0||m.type>(h.version>=4?11:5)||m.hp<0||m.hp>3||m.x< -100||m.x>=(legacy?LEGACY_COLS:COLS)||m.y< -100||m.y>=(legacy?LEGACY_ROWS:ROWS))return false;
    if(legacy&&m.x>=0&&m.y>=0){m.x+=COLS/2-LEGACY_COLS/2;m.y+=ROWS/2-LEGACY_ROWS/2;}
   }
   creatures=restoredCreatures;
  }
  world=restored;
  worldSeed=restoredSeed;
  playerHealth=std::clamp(restoredHealth,1,10);lastDamage=0;damageCooldown=0;
  px=std::clamp(legacy?h.x+COLS/2-LEGACY_COLS/2:h.x,1,COLS-2);
  py=std::clamp(legacy?h.y+ROWS/2-LEGACY_ROWS/2:h.y,1,ROWS-2);
  selected=std::clamp(h.selected,0,7);faceX=h.fx;faceY=h.fy;
  if(abs(faceX)+abs(faceY)!=1){faceX=1;faceY=0;}
  for(int i=0;i<9;i++)stock[i]=std::clamp(h.stock[i],0,9999);
  for(int i=0;i<13;i++)stock[i+9]=std::clamp(extraStock[i],0,9999);
  return true;
}
void tile_color(int t,Uint8&r,Uint8&g,Uint8&b){switch(t){case Grass:r=43;g=76;b=61;break;case Dirt:r=83;g=62;b=52;break;case Stone:r=79;g=86;b=94;break;case Wood:r=65;g=58;b=54;break;case Moss:r=39;g=55;b=48;break;case Crystal:r=125;g=139;b=151;break;case Bone:r=137;g=133;b=119;break;case DarkStone:r=35;g=39;b=47;break;case Sand:r=133;g=119;b=85;break;case Snow:r=155;g=164;b=166;break;case Ice:r=95;g=132;b=145;break;case Lava:r=142;g=62;b=39;break;case Water:r=35;g=72;b=91;break;case IronOre:r=112;g=105;b=91;break;case Obsidian:r=39;g=34;b=57;break;case Ember:r=198;g=75;b=38;break;case Rune:r=94;g=164;b=155;break;case IronBar:r=164;g=171;b=177;break;case ObsidianCore:r=57;g=43;b=78;break;case EmberCatalyst:r=232;g=103;b=49;break;case AncientRune:r=116;g=206;b=185;break;default:r=16;g=23;b=29;break;}}
bool solid_tile(int t){return t==DarkStone||t==Stone||t==Wood||t==Bone||t==Crystal||t==IronOre||t==Obsidian||t==Rune;}
bool creature_occupies(int x,int y,const Creature*skip=nullptr){for(const auto &m:creatures)if(&m!=skip&&m.hp>0&&m.x==x&&m.y==y)return true;return false;}
void move_player(int dx,int dy){if(!dx&&!dy)return;int nx=std::clamp(px+dx,1,COLS-2),ny=std::clamp(py+dy,1,ROWS-2);if(solid_tile(tile_at(nx,ny)))return;px=nx;py=ny;faceX=dx;faceY=dy;}
void turn_player(int direction){if(direction>0){int old=faceX;faceX=-faceY;faceY=old;}else if(direction<0){int old=faceX;faceX=faceY;faceY=-old;}}
Uint32 lastCreatureUpdate=0;
void update_player_combat(){
 Uint32 now=SDL_GetTicks();
 if(playerHealth<=0){playerHealth=10;px=COLS/2;py=ROWS/2;lastDamage=now;damageCooldown=now+1400;return;}
 if(now<damageCooldown)return;
 for(const auto &m:creatures){if(m.hp<=0||m.type%4==3)continue;int distance=abs(m.x-px)+abs(m.y-py);if(distance<=1&&now-lastDamage>=950){--playerHealth;lastDamage=now;damageCooldown=now+500;return;}}
}
void update_creatures(){
 Uint32 now=SDL_GetTicks();if(now-lastCreatureUpdate<650)return;lastCreatureUpdate=now;
 for(auto &m:creatures){
  if(m.hp<=0)continue;
  int dx=px-m.x,dy=py-m.y,distance=abs(dx)+abs(dy);
  const bool passive=(m.type%4==3);
  if((!passive&&distance>14)||(!dx&&!dy))continue;
  if(passive&&distance>7)continue;
  int sx=0,sy=0;
  if(passive){
   // Skittish wildlife flees when approached instead of attacking the player.
   if(abs(dx)>=abs(dy))sx=dx>0?-1:1;else sy=dy>0?-1:1;
  }else{
   if(abs(dx)>=abs(dy))sx=(dx>0)-(dx<0);else sy=(dy>0)-(dy<0);
  }
  int nx=m.x+sx,ny=m.y+sy;
  if(nx==px&&ny==py)continue;
  if(nx<1||nx>=COLS-1||ny<1||ny>=ROWS-1||solid_tile(tile_at(nx,ny))||creature_occupies(nx,ny,&m)){
   sx=0;sy=0;
   if(abs(dx)<abs(dy)){sx=passive?(dy>0?-1:1):(dy>0?1:-1);}else{sy=passive?(dx>0?-1:1):(dx>0?1:-1);}
   nx=m.x+sx;ny=m.y+sy;
  }
  if((nx!=px||ny!=py)&&nx>=1&&nx<COLS-1&&ny>=1&&ny<ROWS-1&&!solid_tile(tile_at(nx,ny))&&!creature_occupies(nx,ny,&m)){m.x=nx;m.y=ny;}
 }
}
bool target_tile(int& x,int& y){x=px+faceX;y=py+faceY;return worldGrid.contains(x,y);}
void place_block(){int x=0,y=0;int block=selected+1;if(target_tile(x,y)&&tile_at(x,y)==Air&&block<=8&&stock[block]>0&&set_tile_at(x,y,static_cast<unsigned char>(block)))--stock[block];}
void change_recipe(int direction){selectedRecipe=(selectedRecipe+direction+9)%9;}
void craft_item(){
 // Five deterministic survival recipes, all using resources already in the world.
 switch(selectedRecipe){
  case 0: // Reinforced darkstone
   if(stock[Stone]>=3&&stock[Wood]>=2&&stock[DarkStone]<=9997){stock[Stone]-=3;stock[Wood]-=2;stock[DarkStone]+=2;}break;
  case 1: // Forest moss
   if(stock[Dirt]>=3&&stock[Wood]>=1&&stock[Moss]<=9997){stock[Dirt]-=3;stock[Wood]-=1;stock[Moss]+=2;}break;
  case 2: // Refined crystal
   if(stock[Stone]>=4&&stock[Bone]>=1&&stock[Crystal]<=9998){stock[Stone]-=4;stock[Bone]-=1;stock[Crystal]+=1;}break;
  case 3: // Bone stakes
   if(stock[Wood]>=2&&stock[Stone]>=1&&stock[Bone]<=9997){stock[Wood]-=2;stock[Stone]-=1;stock[Bone]+=2;}break;
  case 4: // Carved stone
   if(stock[Dirt]>=3&&stock[Bone]>=1&&stock[Stone]<=9997){stock[Dirt]-=3;stock[Bone]-=1;stock[Stone]+=2;}break;
  case 5: if(stock[IronOre]>=3&&stock[Ember]>=1&&stock[IronBar]<=9997){stock[IronOre]-=3;stock[Ember]-=1;stock[IronBar]+=2;}break;
  case 6: if(stock[Obsidian]>=3&&stock[IronOre]>=1&&stock[ObsidianCore]<=9998){stock[Obsidian]-=3;stock[IronOre]-=1;stock[ObsidianCore]+=1;}break;
  case 7: if(stock[Ember]>=3&&stock[Crystal]>=1&&stock[EmberCatalyst]<=9998){stock[Ember]-=3;stock[Crystal]-=1;stock[EmberCatalyst]+=1;}break;
  case 8: if(stock[Rune]>=2&&stock[Crystal]>=2&&stock[AncientRune]<=9998){stock[Rune]-=2;stock[Crystal]-=2;stock[AncientRune]+=1;}break;
 }
}
void mine_block(){int x=0,y=0;if(!target_tile(x,y))return;for(auto &m:creatures)if(m.x==x&&m.y==y&&m.hp>0){if(--m.hp<=0){if(m.type<4)stock[Bone]+=2;else if(m.type<8)stock[Crystal]+=1;else stock[DarkStone]+=1;m.x=-100;m.y=-100;}return;}int block=tile_at(x,y);if(block>0){int drop=block;if(drop==Sand||drop==Snow||drop==Water||drop==Ice)drop=Dirt;else if(drop==Lava)drop=Stone;if(drop>=1&&drop<=8)++stock[drop];else if(drop>=IronOre&&drop<=Rune)++stock[drop];set_tile_at(x,y,Air);}}
void draw_menu(){
 // Dark-fantasy title screen: moonlit ruins, distant towers and drifting embers.
 rect(0,0,W,H,5,8,14);
 for(int y=0;y<H;y+=4){Uint8 shade=static_cast<Uint8>(8+(y*13/H));rect(0,y,W,4,shade,shade+3,shade+9);}
 rect(0,430,W,290,9,13,18);
 // Broken skyline and black-stone spires.
 rect(0,455,1280,265,8,12,16);rect(95,350,58,180,12,18,24);rect(108,305,32,45,18,25,31);rect(1010,330,72,210,11,17,23);rect(1028,285,36,45,18,24,30);
 rect(205,405,34,130,13,19,25);rect(222,375,8,30,29,36,40);rect(875,390,45,150,12,18,24);
 // Moon and mist bands.
 color(184,194,194,210);SDL_Rect moon{955,82,112,112};SDL_RenderFillRect(renderer,&moon);rect(980,68,110,120,5,8,14);
 for(int i=0;i<7;i++)rect(0,510+i*19,W,2,static_cast<Uint8>(30+i*3),static_cast<Uint8>(39+i*2),static_cast<Uint8>(48+i*2),65);
 for(int i=0;i<34;i++){int x=(i*173+37)%W,y=(i*97+61)%H;rect(x,y,2,2,static_cast<Uint8>(105+(i%3)*35),static_cast<Uint8>(83+(i%3)*25),static_cast<Uint8>(66+(i%3)*20),190);}
 rect(70,55,1140,610,9,13,19,235);outline(70,55,1140,610,91,99,103);
 rect(105,95,5,150,225,226,220);draw_text("MINEPLUS",145,100,8,245,246,239);
 draw_text("DARKNESS FALLS  /  BUILD YOUR LEGEND",150,180,2,174,180,178);
 rect(130,270,600,275,14,19,25,240);outline(130,270,600,275,95,103,108);
 // Monochrome creeper-like sigil framed as carved stone.
 rect(165,310,112,112,22,27,32);outline(165,310,112,112,196,199,194);rect(184,329,74,74,205,207,201);rect(197,344,15,17,16,20,25);rect(230,344,15,17,16,20,25);rect(211,361,19,25,16,20,25);rect(201,379,12,15,16,20,25);rect(228,379,12,15,16,20,25);
 draw_text("ENTER THE WILDS",310,320,3,225,228,222);
 draw_text("ANCIENT RUINS  -  FORGOTTEN LANDS",310,360,1,154,163,164);
 rect(310,420,300,70,170,174,170);outline(310,420,300,70,245,245,238);draw_text("START",410,445,4,14,18,23);
 rect(780,270,350,275,13,18,24,245);outline(780,270,350,275,91,99,104);draw_text("CONTROLS",820,305,3,220,223,218);
 draw_text("DPAD MOVE",820,355,2,177,184,182);draw_text("A PLACE X BREAK",820,390,2,177,184,182);draw_text("Y INVENTORY",820,425,2,177,184,182);draw_text("START PAUSE",820,460,2,177,184,182);
 draw_text("MINEPLUS PS4  /  DARK FANTASY EDITION",105,610,1,151,158,160);
}
float smooth_noise(float x,float y){
 int ix=int(std::floor(x)),iy=int(std::floor(y));
 float fx=x-ix,fy=y-iy;
 fx=fx*fx*(3.0f-2.0f*fx);fy=fy*fy*(3.0f-2.0f*fy);
 auto sample=[](int ax,int ay){return float(hash2(ax,ay)&0xffffu)/65535.0f;};
 float a=sample(ix,iy)*(1.0f-fx)+sample(ix+1,iy)*fx;
 float b=sample(ix,iy+1)*(1.0f-fx)+sample(ix+1,iy+1)*fx;
 return a*(1.0f-fy)+b*fy;
}
float terrain_height(int t,int x,int y){
 // Elevation was generated once with the chunk-independent PCG world seed.
 // Reusing it here keeps the first-person ray marcher cheap enough for PS4.
 float h=float(heightMap[y*COLS+x])/48.0f;
 switch(t){case Grass:h+=0.12f;break;case Dirt:h+=0.04f;break;case Stone:h+=0.34f;break;case Wood:h+=0.72f;break;case Moss:h+=0.18f;break;case Crystal:h+=0.92f;break;case Bone:h+=0.48f;break;case DarkStone:h+=0.42f;break;case Sand:h-=0.08f;break;case Snow:h+=0.24f;break;case Ice:h+=0.12f;break;case Lava:h-=0.08f;break;case Water:h-=0.22f;break;case IronOre:h+=0.36f;break;case Obsidian:h+=0.46f;break;case Ember:h+=0.02f;break;case Rune:h+=0.58f;break;case IronBar:h+=0.3f;break;case ObsidianCore:h+=0.6f;break;case EmberCatalyst:h+=0.5f;break;case AncientRune:h+=0.8f;break;default:break;}
 // Multi-scale elevation creates broad mountain ranges, rolling foothills and rocky peaks.
 float continental=smooth_noise(x/310.0f,y/310.0f);
 float ridge=1.0f-std::fabs(2.0f*smooth_noise(x/118.0f+37.0f,y/118.0f-19.0f)-1.0f);
 float detail=smooth_noise(x/27.0f+11.0f,y/27.0f+53.0f);
 float mountain=std::max(0.0f,(continental-0.48f)*2.25f);
 mountain=mountain*mountain*(0.65f+ridge*0.85f);
 h+=mountain*2.25f+detail*0.16f;
 unsigned n=hash2(x/3,y/3);h+=float(n%17)/100.0f;
 if(t==Wood||t==Crystal||t==Bone)h+=0.45f;
 return h;
}
void draw_minimap(){
 // Compact local map: nearby terrain, ruins and the player's exact position.
 const int ox=W-178,oy=82,cell=9,radius=8;
 rect(ox-5,oy-5,cell*17+10,cell*17+10,5,9,13,225);
 outline(ox-5,oy-5,cell*17+10,cell*17+10,91,113,105);
 for(int my=-radius;my<=radius;my++)for(int mx=-radius;mx<=radius;mx++){
  int wx=px+mx,wy=py+my;Uint8 r=13,g=18,b=22;
  if(wx>=0&&wy>=0&&wx<COLS&&wy<ROWS)tile_color(tile_at(wx,wy),r,g,b);
  float shade=(mx==0&&my==0)?1.35f:0.72f;
  rect(ox+(mx+radius)*cell,oy+(my+radius)*cell,cell-1,cell-1,
       Uint8(std::min(255,int(r*shade))),Uint8(std::min(255,int(g*shade))),Uint8(std::min(255,int(b*shade))));
 }
 // Player marker remains visible over every terrain type.
 rect(ox+radius*cell+2,oy+radius*cell+2,5,5,245,240,210);
 rect(ox+radius*cell+2+faceX*3,oy+radius*cell+2+faceY*3,3,3,245,190,110);
 draw_text("MAP",ox,oy+cell*17+7,1,190,204,190);
 char coords[32];std::snprintf(coords,sizeof(coords),"%d %d",px,py);
 draw_text(coords,ox+35,oy+cell*17+7,1,190,204,190);
}
void draw_world(){
 // Perspective height-field renderer for the native PS4 build.
 rect(0,0,W,H,8,13,22);
 for(int y=0;y<330;y+=3){Uint8 v=static_cast<Uint8>(13+y/28);rect(0,y,W,3,v,v+7,v+15);}
 // Moon, distant haze and layered atmospheric fog.
 rect(930,82,82,82,145,157,157,175);rect(950,70,82,82,8,13,22,255);
 for(int i=0;i<6;i++)rect(0,285+i*19,W,4,38,52,58,25);
 const float dirX=float(faceX),dirY=float(faceY);
 const float rightX=-dirY,rightY=dirX;
 const float focal=720.0f, horizon=H*0.48f, cameraH=1.48f;
 const float fov=0.92f;
 for(int sx=0;sx<W;sx+=2){
  float cameraX=(2.0f*(sx+1)/float(W)-1.0f)*fov;
  float rayX=dirX+rightX*cameraX,rayY=dirY+rightY*cameraX;
  float len=std::sqrt(rayX*rayX+rayY*rayY);rayX/=len;rayY/=len;
  int ceiling=int(horizon),lastY=H;
  for(float dist=0.48f;dist<38.0f&&ceiling<lastY;dist+=0.18f+dist*0.0025f){
   int wx=int(px+rayX*dist),wy=int(py+rayY*dist);
   if(wx<1||wy<1||wx>=COLS-1||wy>=ROWS-1)break;
   int t=tile_at(wx,wy);
   float height=terrain_height(t,wx,wy);
   float corrected=dist*(0.82f+0.18f*(1.0f-std::fabs(cameraX)));
   int sy=int(horizon-(height-cameraH)*focal/corrected);
   if(sy<ceiling)sy=ceiling;
   if(sy<lastY){
    Uint8 r,g,b;tile_color(t,r,g,b);
    // Procedural material pass: deterministic grain, mineral flecks and depth fog.
    unsigned tex=hash2(unsigned(wx*31+int(dist*13.0f)),unsigned(wy*29-int(dist*9.0f)));
    int grain=int(tex%29)-14;
    float light=1.0f/(1.0f+dist*0.043f);
    if(t==Lava)light=std::min(1.0f,light+0.48f);
    if(t==Crystal)light=std::min(1.0f,light+0.16f);
    int strata=((wx+wy*3)%7==0)?9:0;
    int fleck=(tex%23==0)?22:0;
    r=Uint8(std::clamp(int((int(r)+grain+strata+fleck)*light),0,255));
    g=Uint8(std::clamp(int((int(g)+grain+strata+fleck)*light),0,255));
    b=Uint8(std::clamp(int((int(b)+grain+strata+fleck)*light),0,255));
    color(r,g,b);SDL_RenderDrawLine(renderer,sx,sy,sx,lastY);
    // Fine surface marks break up flat polygons and give each material a readable grain.
    for(int ty=sy+4+(tex%5);ty<lastY;ty+=7){
     unsigned mark=hash2(int(tex+unsigned(ty*13+sx*7)),int(sx+ty*3));
     if(mark%5==0){color(Uint8(std::min(255,int(r)+18)),Uint8(std::min(255,int(g)+18)),Uint8(std::min(255,int(b)+16)));SDL_RenderDrawPoint(renderer,sx,ty);}
    }
    // Thin illuminated top edge gives the terrain a block-built silhouette.
    if(sy<lastY-2){color(Uint8(std::min(255,int(r)+28)),Uint8(std::min(255,int(g)+30)),Uint8(std::min(255,int(b)+27)));SDL_RenderDrawLine(renderer,sx,sy,sx,sy+1);}
    if(t==Crystal&&sy<lastY-8){color(176,220,224,150);SDL_RenderDrawPoint(renderer,sx,sy+3);}
    lastY=sy;
   }
   if(t==Lava&&dist<12.0f){color(190,75,38,45);SDL_RenderDrawPoint(renderer,sx,int(horizon+70.0f/dist));}
  }
  if(lastY<H){color(9,14,19);SDL_RenderDrawLine(renderer,sx,lastY,sx,H);}
 }
 // Distant silhouettes create scale and a recognizable horizon.
 for(int i=0;i<18;i++){int x=(i*173+91)%W;int base=300+(i%4)*13;rect(x,base-24-(i%5)*13,5+(i%3)*4,base>horizon?H-base:25,13,19,24,150);}
 // Project living creatures as perspective billboards.
 for(const auto &m:creatures)if(m.hp>0){
  float dx=float(m.x-px),dy=float(m.y-py),forward=dx*dirX+dy*dirY,side=dx*rightX+dy*rightY;
  if(forward<0.7f||forward>22.0f)continue;
  int sx=int(W*0.5f+(side/forward)*focal/fov);
  int size=int(430.0f/forward);
  int sy=int(horizon+focal*0.65f/forward-size*0.8f);
  if(sx>8&&sx<W-8&&size>3){static const int speciesColor[12]={75,105,61,130,150,92,112,83,158,67,137,98};
  int rr=speciesColor[std::clamp(m.type,0,11)];
  int bodyW=std::max(2,size*(m.type==8||m.type==10?4:2)/3);
  int bodyH=std::max(2,size*(m.type==3||m.type==8?5:4)/4);
  rect(sx-bodyW/2,sy,bodyW,bodyH,rr,rr-8,rr+5);
  if(m.type==6||m.type==9)rect(sx-bodyW/2,sy+bodyH/3,bodyW,2,210,174,115);
  if(m.type==8||m.type==10)rect(sx-bodyW/2,sy-std::max(2,size/5),bodyW,std::max(2,size/5),rr+25,rr+12,rr+8);rect(sx-size/5,sy+size/4,3,3,230,225,210);rect(sx+size/8,sy+size/4,3,3,230,225,210);}
 }
 // First-person blocky arms and tool silhouette.
 rect(W/2+115,H-158,75,180,91,65,49);rect(W/2+125,H-166,58,28,155,119,82);
 rect(W/2+164,H-190,12,95,151,159,163);rect(W/2+132,H-198,75,13,181,190,190);
 // Crosshair and interaction hint.
 color(225,232,222);SDL_RenderDrawLine(renderer,W/2-9,H/2,W/2+9,H/2);SDL_RenderDrawLine(renderer,W/2,H/2-9,W/2,H/2+9);
 rect(0,0,W,64,7,12,16,235);rect(20,13,7,38,52,212,132);draw_text("MINEPLUS  /  THE FORGOTTEN WILDS",44,15,3);draw_text("3D VOXEL SURVIVAL",45,43,1,160,190,174);
 draw_text("HP",500,20,2,210,218,208);rect(535,19,110,14,36,25,26);rect(537,21,playerHealth*10+6,10,playerHealth>3?75:190,playerHealth>3?183:66,playerHealth>3?111:53);
 draw_text("MOVE  /  R-STICK LOOK  /  F BREAK  /  E BAG",650,25,2,190,198,192);
 draw_minimap();
 int start=(W-8*62)/2;for(int i=0;i<8;i++){int x=start+i*62;rect(x,H-72,54,54,i==selected?44:23,i==selected?66:34,i==selected?49:40);outline(x,H-72,54,54,i==selected?90:55,i==selected?224:77,i==selected?142:91);Uint8 r,g,b;tile_color(i+1,r,g,b);rect(x+15,H-57,24,24,r,g,b);char qty[8];std::snprintf(qty,sizeof(qty),"%d",stock[i+1]);draw_text(qty,x+17,H-20,1,225,230,220);}
 if(inventory||paused){rect(250,150,780,430,8,14,18,245);outline(250,150,780,430,68,166,111);draw_text(inventory?"INVENTORY":"PAUSED",310,195,5,132,235,166);
  if(inventory){
   static const int itemIds[16]={Grass,Dirt,Stone,Wood,Moss,Crystal,Bone,DarkStone,IronOre,Obsidian,Ember,Rune,IronBar,ObsidianCore,EmberCatalyst,AncientRune};
   static const char* itemNames[16]={"GRASS","DIRT","STONE","WOOD","MOSS","CRYSTAL","BONE","DARKSTONE","IRON ORE","OBSIDIAN","EMBER","RUNE","IRON BARS","OBSIDIAN CORE","EMBER CATALYST","ANCIENT RUNE"};
   for(int i=0;i<16;i++){int x=265+(i%4)*188,y=245+(i/4)*49,id=itemIds[i];rect(x,y,170,43,27,43,43);Uint8 r,g,b;tile_color(id,r,g,b);rect(x+7,y+6,30,30,r,g,b);outline(x,y,170,43,75,145,98);draw_text(itemNames[i],x+43,y+7,1,208,215,208);char amount[12];std::snprintf(amount,sizeof(amount),"%d",stock[id]);draw_text(amount,x+43,y+24,1,160,220,174);}
   static const char* recipes[9]={"DARKSTONE  3 STONE + 2 WOOD","MOSS  3 DIRT + 1 WOOD","CRYSTAL  4 STONE + 1 BONE","BONE  2 WOOD + 1 STONE","STONE  3 DIRT + 1 BONE","IRON BARS  3 ORE + 1 EMBER","OBSIDIAN CORE  3 OBSIDIAN + 1 ORE","EMBER CATALYST  3 EMBER + 1 CRYSTAL","ANCIENT RUNE  2 RUNES + 2 CRYSTAL"};
   for(int ri=0;ri<9;ri++)draw_text(recipes[ri],292,448+ri*13,1,ri==selectedRecipe?245:145,ri==selectedRecipe?220:165,ri==selectedRecipe?145:160);
   draw_text("L/R SELECT RECIPE  /  A CRAFT  /  BACK CLOSE",650,565,1,184,205,184);
  }
  else{draw_text("ENTER CONTINUE",350,320,3);draw_text("ESC BACK TO WORLD",350,385,3);}
 }
}
void handle_key(SDL_Keycode k){if(!playing){if(k==SDLK_RETURN||k==SDLK_SPACE)playing=true;if(k==SDLK_ESCAPE)running=false;return;}if(k==SDLK_ESCAPE){paused=!paused;inventory=false;return;}if(paused){if(k==SDLK_RETURN)paused=false;return;}if(k==SDLK_q)turn_player(-1);if(k==SDLK_r)turn_player(1);if(k==SDLK_TAB||k==SDLK_e)inventory=!inventory;if(inventory){if(k>=SDLK_1&&k<=SDLK_8)selected=k-SDLK_1;if(k==SDLK_c||k==SDLK_RIGHT){change_recipe(1);return;}if(k==SDLK_LEFT){change_recipe(-1);return;}if(k==SDLK_RETURN){craft_item();return;}return;}if(k==SDLK_w||k==SDLK_UP)move_player(0,-1);if(k==SDLK_s||k==SDLK_DOWN)move_player(0,1);if(k==SDLK_a||k==SDLK_LEFT)move_player(-1,0);if(k==SDLK_d||k==SDLK_RIGHT)move_player(1,0);if(k>=SDLK_1&&k<=SDLK_8)selected=k-SDLK_1;if(k==SDLK_SPACE)place_block();if(k==SDLK_f)mine_block();}
}
int main(int,char**){if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER|SDL_INIT_EVENTS)!=0)return 1;window=SDL_CreateWindow("MINEPLUS",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,W,H,SDL_WINDOW_SHOWN);if(!window){SDL_Quit();return 2;}renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);if(!renderer)renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);if(!renderer){SDL_DestroyWindow(window);SDL_Quit();return 3;}SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);SDL_RenderSetLogicalSize(renderer,W,H);if(SDL_NumJoysticks()>0&&SDL_IsGameController(0))pad=SDL_GameControllerOpen(0);init_world();load_world();Uint32 lastAutosave=SDL_GetTicks();while(running){SDL_Event e;while(SDL_PollEvent(&e)){if(e.type==SDL_QUIT)running=false;if(e.type==SDL_KEYDOWN&&e.key.repeat==0)handle_key(e.key.keysym.sym);if(e.type==SDL_MOUSEBUTTONDOWN&&playing&&!paused&&!inventory){if(e.button.button==SDL_BUTTON_LEFT)mine_block();else if(e.button.button==SDL_BUTTON_RIGHT)place_block();}if(e.type==SDL_CONTROLLERBUTTONDOWN){switch(e.cbutton.button){case SDL_CONTROLLER_BUTTON_START:if(!playing)playing=true;else paused=!paused;break;case SDL_CONTROLLER_BUTTON_BACK:inventory=!inventory;break;case SDL_CONTROLLER_BUTTON_A:if(!playing)playing=true;else if(inventory)craft_item();else if(!paused)place_block();break;case SDL_CONTROLLER_BUTTON_X:if(playing&&!paused&&!inventory)mine_block();break;case SDL_CONTROLLER_BUTTON_B:if(paused)paused=false;else if(playing)paused=true;else running=false;break;case SDL_CONTROLLER_BUTTON_DPAD_UP:if(playing&&!paused&&!inventory)move_player(0,-1);break;case SDL_CONTROLLER_BUTTON_DPAD_DOWN:if(playing&&!paused&&!inventory)move_player(0,1);break;case SDL_CONTROLLER_BUTTON_DPAD_LEFT:if(playing&&!paused&&!inventory)move_player(-1,0);break;case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:if(playing&&!paused&&!inventory)move_player(1,0);break;case SDL_CONTROLLER_BUTTON_Y:inventory=!inventory;break;case SDL_CONTROLLER_BUTTON_RIGHTSTICK:if(inventory)craft_item();break;case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:if(inventory)change_recipe(-1);else selected=(selected+7)%8;break;case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:if(inventory)change_recipe(1);else selected=(selected+1)%8;break;default:break;}}}if(playing&&!paused&&!inventory)update_creatures();if(playing&&!paused&&!inventory)update_player_combat();if(playing&&!paused&&!inventory&&pad){Uint32 now=SDL_GetTicks();if(now-lastTurn>230){Sint16 rx=SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_RIGHTX);if(rx>18000){turn_player(1);lastTurn=now;}else if(rx< -18000){turn_player(-1);lastTurn=now;}}if(now-lastMove>155){Sint16 ax=SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_LEFTX),ay=SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_LEFTY);if(ax>18000){move_player(1,0);lastMove=now;}else if(ax< -18000){move_player(-1,0);lastMove=now;}else if(ay>18000){move_player(0,1);lastMove=now;}else if(ay< -18000){move_player(0,-1);lastMove=now;}}}if(playing&&!paused&&SDL_GetTicks()-lastAutosave>30000){save_world();lastAutosave=SDL_GetTicks();}if(playing)draw_world();else draw_menu();SDL_RenderPresent(renderer);SDL_Delay(16);}save_world();if(pad)SDL_GameControllerClose(pad);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return 0;}
