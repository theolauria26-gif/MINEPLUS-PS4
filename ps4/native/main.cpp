#include <time.h>
#include <SDL2/SDL.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#include "pcg_world.hpp"
#include "surface_ecology.hpp"
#include "surface_settlements.hpp"
#include "surface_npcs.hpp"
#include "surface_trade.hpp"
#include "surface_quests.hpp"
#include "surface_events.hpp"
#include "world_grid.hpp"
#include "dungeon_loot.hpp"
#include "equipment.hpp"
#include "weapon_abilities.hpp"
#include "boss_combat.hpp"
namespace {
constexpr int W=1280,H=720,COLS=2048,ROWS=2048,LEGACY_COLS=1024,LEGACY_ROWS=1024,TILE=36,VIEW_COLS=24,VIEW_ROWS=14;
enum Tile:unsigned char{Air=0,Grass=1,Dirt=2,Stone=3,Wood=4,Moss=5,Crystal=6,Bone=7,DarkStone=8,Sand=9,Snow=10,Ice=11,Lava=12,Water=13,IronOre=14,Obsidian=15,Ember=16,Rune=17,IronBar=18,ObsidianCore=19,EmberCatalyst=20,AncientRune=21,ChestClosed=22,ChestOpened=23,Leaves=24};
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
int px=COLS/2,py=ROWS/2,selected=0,faceX=1,faceY=0,playerHealth=10,selectedRecipe=0,selectedEquipment=0;
mineplus::EquipmentState equipmentState{};
mineplus::BossEncounter primordialBoss{};
mineplus::BossAttack lastBossAttack=mineplus::BossAttack::None;
Uint32 lastBossAttackAt=0;
bool lastBossAttackHitPlayer=false;
constexpr int BOSS_X=COLS/2+16,BOSS_Y=ROWS/2;
bool equipmentPage=false;
std::uint64_t worldSeed=4839201ULL;
Uint32 lastDamage=0,damageCooldown=0,lastCreatureHit=0,lastBossHitAt=0,lastSaveAt=0,lastWeaponArtAt=0,weaponArtFlashUntil=0,worldVisualTicks=0;
Uint32 bossClockMs=0,lastBossClockTick=0;bool lastSaveSucceeded=true;int lastCreatureHitX=-100,lastCreatureHitY=-100;
struct Creature{int x,y,type,hp;};std::array<Creature,96> creatures{};
std::array<mineplus::SettlementNpc,64> settlementNpcs{};std::size_t settlementNpcCount=0;
std::array<mineplus::SurfaceQuestProgress,mineplus::SURFACE_QUEST_ROLES> surfaceQuestProgress{};
std::uint64_t surfaceEventClockMs=0;Uint32 lastSurfaceEventTick=0;
mineplus::SurfaceEventInfo currentSurfaceEvent{mineplus::SurfaceEventType::None,"",""};
Uint32 lastNpcUpdate=0,npcDialogueUntil=0;const char* activeNpcName=nullptr;const char* activeNpcLine=nullptr;
bool tradeOpen=false;std::uint8_t tradeRole=0,selectedTradeOffer=0;
std::array<int,31> stock{{24,18,12,8,6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
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
case 'Q':return "01110""10001""10001""10001""10101""10010""01101";
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
void build_boss_arena(){
 for(int oy=-8;oy<=8;++oy)for(int ox=-8;ox<=8;++ox){const int d=ox*ox+oy*oy,x=BOSS_X+ox,y=BOSS_Y+oy;if(d>64||x<2||y<2||x>=COLS-2||y>=ROWS-2)continue;if(d>=49){if(!(ox==-7&&oy==0))world[y*COLS+x]=DarkStone;}else if(d<=4)world[y*COLS+x]=(d==0)?AncientRune:ObsidianCore;else if((ox==0||oy==0)&&d<49)world[y*COLS+x]=AncientRune;else world[y*COLS+x]=Moss;}
 world[BOSS_Y*COLS+BOSS_X]=AncientRune;
 // Guarantee a traversable causeway from the spawn clearing to the arena gate.
 for(int x=COLS/2;x<=BOSS_X-7;++x)world[BOSS_Y*COLS+x]=AncientRune;
}
void init_world(){
 // PS4-native PCG sampling. Every coordinate is reproducible from one seed;
 // this 2048x2048 resident window is a transitional cache, not the final world limit.
 mineplus::PcgWorld pcg(worldSeed);
 settlementNpcCount=0;surfaceQuestProgress.fill({});activeNpcName=nullptr;activeNpcLine=nullptr;npcDialogueUntil=0;
 for(int y=0;y<ROWS;y++)for(int x=0;x<COLS;x++){
  const int wx=x-COLS/2, wy=y-ROWS/2;
  const auto sample=pcg.sample_surface(wx,wy);
  unsigned char tile=sample.surface;
  // Trees form clustered groves with open clearings instead of uniform noise.
  if((tile==Grass||tile==Moss||tile==Snow||tile==Stone)&&
     pcg.surface_tree_at(wx,wy,sample))
   tile=Wood;
  const unsigned h=hash2(x,y);
  // Sparse resources are coordinate-hashed, so revisiting an area never rerolls it.
  if(tile==Stone && h%389u==0u) tile=IronOre;
  else if((tile==Stone||tile==DarkStone)&&h%12007u==0u) tile=Rune;
  world[y*COLS+x]=tile;
  heightMap[y*COLS+x]=static_cast<unsigned char>(std::clamp<int>(sample.height,0,255));
 }
 // Build varied leaf crowns around natural forest trunks. Crown width, depth
 // and silhouette come from the trunk's world coordinate, so groves contain
 // compact round trees, broad oaks, ancient giants and narrow wind-shaped crowns.
 // Rare old-growth trees are restricted to Forest; their broad crowns make
 // distant woodland silhouettes more varied without adding tile or save IDs.
 // Leaves remain walk-through foliage; the trunk is the collision point.
 for(int y=5;y<ROWS-5;++y)for(int x=5;x<COLS-5;++x){
  if(world[y*COLS+x]!=Wood)continue;
  const int wx=x-COLS/2,wy=y-ROWS/2;
  const auto column=pcg.sample_surface(wx,wy);
  if(!pcg.surface_tree_at(wx,wy,column))continue;
  const unsigned crownSeed=hash2(wx,wy);
  const auto biome=static_cast<mineplus::Biome>(column.biome);
  const bool conifer=biome==mineplus::Biome::Tundra||
                     biome==mineplus::Biome::Highlands;
  const bool ancient=biome==mineplus::Biome::Forest&&
                     (crownSeed%31u)==0u;
  const int radiusX=conifer?1:(ancient?4+static_cast<int>((crownSeed>>3)%2u):
                                      1+static_cast<int>(crownSeed%3u));
  const int radiusY=conifer?2:(ancient?3:
                                      1+static_cast<int>((crownSeed>>5)%2u));
  const int crownStyle=static_cast<int>((crownSeed>>9)%3u);
  for(int oy=-3;oy<=3;++oy)for(int ox=-5;ox<=5;++ox){
   if((ox==0&&oy==0)||std::abs(oy)>radiusY)continue;
   const int rowWidth=conifer?std::max(1,radiusX+radiusY-std::abs(oy)-1):
                     (ancient?std::max(1,radiusX-(std::abs(oy)/2)):radiusX);
   if(std::abs(ox)>rowWidth)continue;
   const int nx=x+ox,ny=y+oy;
   const int ellipse=ox*ox*radiusY*radiusY+oy*oy*radiusX*radiusX;
   const int limit=radiusX*radiusX*radiusY*radiusY;
   if(!conifer&&!ancient&&ellipse>limit+(crownStyle==0?0:radiusX*radiusY))continue;
   if(ancient&&((ox+2*oy+int(crownSeed>>16))%11u)==0u&&
      std::abs(ox)>radiusX-2)continue;
   if(!conifer&&!ancient&&crownStyle==1&&((ox+oy+crownSeed)%5u)==0u)continue;
   if(!conifer&&!ancient&&crownStyle==2&&std::abs(ox)==radiusX&&std::abs(oy)==radiusY)continue;
   const auto ground=world[ny*COLS+nx];
   if(ground==Grass||ground==Moss||ground==Snow||
      (conifer&&ground==Stone))world[ny*COLS+nx]=Leaves;
  }
 }
   // Undergrowth fills the forest floor in small, irregular shrub clusters.
  // Leaves remain walk-through, and the hash makes every patch stable on reload.
  for(int y=2;y<ROWS-2;++y)for(int x=2;x<COLS-2;++x){
   const int wx=x-COLS/2,wy=y-ROWS/2;
   const auto ground=world[y*COLS+x];
   if(ground!=Grass&&ground!=Moss)continue;
   const auto sample=pcg.sample_surface(wx,wy);
   const auto biome=static_cast<mineplus::Biome>(sample.biome);
   if(biome!=mineplus::Biome::Forest&&
      !(biome==mineplus::Biome::Plains&&sample.moisture>164u))continue;
   const unsigned shrub=hash2(wx+0x2B1,wy-0x517);
   const unsigned chance=biome==mineplus::Biome::Forest?83u:211u;
   if(shrub%chance!=0u)continue;
   const int radius=1+static_cast<int>((shrub>>9)%2u);
   for(int oy=-radius;oy<=radius;++oy)for(int ox=-radius;ox<=radius;++ox){
    if((ox==0&&oy==0)||std::abs(ox)+std::abs(oy)>radius+1)continue;
    const unsigned shape=hash2(wx+ox*17,wy+oy*23);
    if((shape&3u)==0u)continue;
    const int nx=x+ox,ny=y+oy;
    const auto under=world[ny*COLS+nx];
    if(under==Grass||under==Moss)world[ny*COLS+nx]=Leaves;
   }
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
    case 5: // Abandoned surface camp with a fire pit and timber crossbeams.
     if(d>=r2-2)tile=DarkStone;
     else if(d<3)tile=Ember;
     else if((ox==0&&abs(oy)<=3)||(oy==0&&abs(ox)<=3))tile=Wood;
     else if(d<r2/3&&((ox+oy)&1)==0)tile=Moss;
     break;
    case 6: // Ruined frontier watch-post with a timber lookout and a marked approach.
     if(d>=r2-2)tile=DarkStone;
     else if(d==0)tile=ChestClosed;
     else if((abs(ox)<=1&&abs(oy)<=radius/2)||
             (abs(oy)<=1&&abs(ox)<=radius/2))tile=Wood;
     else if(oy>radius/2&&abs(ox)<=1)tile=AncientRune;
     else if((ox==0&&abs(oy)==radius-2)||(oy==0&&abs(ox)==radius-2))
      tile=Crystal;
     else if(d<r2/4&&((ox+oy+int(anchor.x))&3)==0)tile=Moss;
     break;
    default: // Bone altar, dark ring and central ancient relic.
     if(d>=r2-2)tile=Bone;
     else if(cardinal&&d>r2/4)tile=DarkStone;
     else if(d==0)tile=ChestClosed;
      else if(d<5)tile=AncientRune;
     break;
   }
   if(tile!=Air)world[ly*COLS+lx]=tile;
  }
 }
 // Connect nearby landmarks with coordinate-stable dirt roads. Links are
 // chosen by the PCG world, drawn once at generation time and never rerolled.
 for(int cy=-13;cy<=13;++cy)for(int cx=-13;cx<=13;++cx){
  mineplus::StructureAnchor from{},to{};
  if(!pcg.surface_road_link_for_cell(cx,cy,from,to))continue;
  if(to.y<from.y||(to.y==from.y&&to.x<from.x))continue;
  const int vx=to.x-from.x,vy=to.y-from.y;
  const float length=std::sqrt(float(vx*vx+vy*vy));
  if(length<=float(from.radius+to.radius+4))continue;
  const int sx=from.x+static_cast<int>(std::lround(vx/length*(from.radius+2)));
  const int sy=from.y+static_cast<int>(std::lround(vy/length*(from.radius+2)));
  const int ex=to.x-static_cast<int>(std::lround(vx/length*(to.radius+2)));
  const int ey=to.y-static_cast<int>(std::lround(vy/length*(to.radius+2)));
  int x0=sx+COLS/2,y0=sy+ROWS/2,x1=ex+COLS/2,y1=ey+ROWS/2;
  if(x0<2||x0>=COLS-2||y0<2||y0>=ROWS-2||
     x1<2||x1>=COLS-2||y1<2||y1>=ROWS-2)continue;
  // Roads may cross a narrow river at a shallow ford/causeway, but never
  // cut through lakes, broad channels or lava fields.
  int ax=std::abs(x1-x0),ay=std::abs(y1-y0),stepX=x0<x1?1:-1,stepY=y0<y1?1:-1;
  int error=ax-ay,tx=x0,ty=y0,waterRun=0,maxWaterRun=0;bool blocked=false;
  for(;;){
   const auto ground=tile_at(tx,ty);
   if(ground==Lava){blocked=true;break;}
   if(ground==Water){++waterRun;maxWaterRun=std::max(maxWaterRun,waterRun);}
   else waterRun=0;
   if(maxWaterRun>5){blocked=true;break;}
   if(tx==x1&&ty==y1)break;
   const int twice=2*error;
   if(twice>-ay){error-=ay;tx+=stepX;}
   if(twice<ax){error+=ax;ty+=stepY;}
  }
  if(blocked)continue;
  ax=std::abs(x1-x0);ay=std::abs(y1-y0);error=ax-ay;tx=x0;ty=y0;
  for(;;){
   const auto ground=tile_at(tx,ty);
   if(ground==Water){
    // Sand causeway tiles are walkable and visually distinct from open water.
    world[ty*COLS+tx]=Sand;
   }else if(ground!=ChestClosed&&ground!=DarkStone&&ground!=AncientRune&&
      ground!=Crystal&&ground!=ObsidianCore&&ground!=Bone)
    world[ty*COLS+tx]=Sand;
   if(tx==x1&&ty==y1)break;
   const int twice=2*error;
   if(twice>-ay){error-=ay;tx+=stepX;}
   if(twice<ax){error+=ax;ty+=stepY;}
  }
 }
 // Large ancient keeps, watchtowers, shrines and circular ruins.
 for(int cy=24;cy<ROWS-24;cy+=61)for(int cx=24;cx<COLS-24;cx+=73){
  unsigned seed=hash2(cx,cy);int w=9+(seed%13),h=8+(hash2(cy,cx)%11);
  for(int y=cy;y<cy+h;y++)for(int x=cx;x<cx+w;x++)
   if(x==cx||x==cx+w-1||y==cy||y==cy+h-1)world[y*COLS+x]=DarkStone;
  world[(cy+h/2)*COLS+cx+w/2]=(seed%7u==0u)?ChestClosed:Air;
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
 // Secondary landmark pass: roadside shrines, collapsed arches, wells and camps.
 // Each site is coordinate-hashed, so the same seed always recreates the same world.
 for(int cy=42;cy<ROWS-42;cy+=47)for(int cx=42;cx<COLS-42;cx+=53){
  const unsigned h=hash2(cx+1201,cy+701);
  if(h%6u!=0u)continue;
  const int kind=int((h>>5)%4u),radius=4+int((h>>11)%4u);
  for(int oy=-radius;oy<=radius;++oy)for(int ox=-radius;ox<=radius;++ox){
   const int x=cx+ox,y=cy+oy,d=ox*ox+oy*oy;
   if(x<2||y<2||x>=COLS-2||y>=ROWS-2)continue;
   if(kind==0){ // Mossy well with a broken stone rim and water core.
    if(d>=radius*radius-2&&d<=radius*radius+2)world[y*COLS+x]=Moss;
    if(d<=2)world[y*COLS+x]=Water;
    if((ox==0&&abs(oy)==radius-1)||(oy==0&&abs(ox)==radius-1))world[y*COLS+x]=Bone;
   }else if(kind==1){ // Twin-pillar arch and a cracked path.
    if(abs(ox)==radius/2&&abs(oy)<=2)world[y*COLS+x]=DarkStone;
    if(oy==0&&abs(ox)<=radius)world[y*COLS+x]=((ox&1)==0)?Stone:Sand;
    if(ox==0&&oy==-2)world[y*COLS+x]=Rune;
   }else if(kind==2){ // Abandoned hunter camp with firepit and timber stakes.
    if(abs(ox)==radius||abs(oy)==radius)world[y*COLS+x]=((ox+oy)&1)?Wood:Bone;
    if(d<=2)world[y*COLS+x]=Ember;
    if(d==radius*radius)world[y*COLS+x]=DarkStone;
   }else{ // Small crystal altar protected by four broken monoliths.
    if((abs(ox)==radius&&oy==0)||(abs(oy)==radius&&ox==0))world[y*COLS+x]=Crystal;
    if(d<=2)world[y*COLS+x]=AncientRune;
    if(d>radius*radius/2&&d<radius*radius/2+radius)world[y*COLS+x]=Moss;
   }
  }
 }
 // Winding ancient causeways: short deterministic segments link nearby landmarks.
 for(int cy=80;cy<ROWS-80;cy+=181)for(int cx=80;cx<COLS-80;cx+=193){
  const unsigned h=hash2(cx+1901,cy+2309);
  if(h%3u!=0u)continue;
  const int length=24+int(h%28u),direction=int((h>>8)%4u);
  for(int step=0;step<length;++step){
   const int x=cx+(direction==0?step:(direction==1?-step:0));
   const int y=cy+(direction==2?step:(direction==3?-step:0));
   for(int side=-1;side<=1;++side){
    const int tx=x+(direction<2?0:side),ty=y+(direction<2?side:0);
    if(tx>1&&ty>1&&tx<COLS-1&&ty<ROWS-1&&world[ty*COLS+tx]!=Water&&world[ty*COLS+tx]!=Lava)
     world[ty*COLS+tx]=(side==0)?DarkStone:((step%5==0)?Moss:Stone);
   }
  }
 }
 // Small frontier settlements occupy gentle, temperate clearings and
 // preserve their footprints after the road pass. Their deterministic five-house
 // layouts are data-driven; resident NPC actors are added by the next gameplay pass.
 for(int cy=-3;cy<=3;++cy)for(int cx=-3;cx<=3;++cx){
  mineplus::SurfaceSettlement settlement{};
  if(!mineplus::surface_settlement_for_cell(pcg,cx,cy,settlement))continue;
  const int centerX=settlement.x+COLS/2,centerY=settlement.y+ROWS/2;
  if(centerX<20||centerY<20||centerX>=COLS-20||centerY>=ROWS-20)continue;
  bool blocked=false;
  for(int oy=-17;oy<=17&&!blocked;++oy)for(int ox=-17;ox<=17;++ox){
   if(ox*ox+oy*oy>17*17)continue;
   const auto ground=tile_at(centerX+ox,centerY+oy);
   if(ground==Water||ground==Lava||ground==ChestClosed||ground==DarkStone||
      ground==AncientRune||ground==Crystal||ground==ObsidianCore){blocked=true;break;}
  }
  if(blocked)continue;
  for(int oy=-17;oy<=17;++oy)for(int ox=-17;ox<=17;++ox){
   if(ox*ox+oy*oy>17*17)continue;
   const int x=centerX+ox,y=centerY+oy;
   world[y*COLS+x]=((ox+oy)&3)==0?Moss:Grass;
  }
  // A compact walkable village green connects the five entrances. Four
  // planted beds break up the empty square without adding blocking props.
  for(int offset=-5;offset<=5;++offset){
   world[centerY*COLS+centerX+offset]=Sand;
   world[(centerY+offset)*COLS+centerX]=Sand;
  }
  for(int bedY : {-5,5})for(int bedX : {-5,5}){
   for(int oy=-1;oy<=1;++oy)for(int ox=-1;ox<=1;++ox){
    const int gx=centerX+bedX+ox,gy=centerY+bedY+oy;
    if(gx<2||gy<2||gx>=COLS-2||gy>=ROWS-2)continue;
    world[gy*COLS+gx]=(ox==0&&oy==0)?Moss:Leaves;
   }
  }
  for(std::uint8_t i=0;i<5u;++i){
   mineplus::SettlementBuilding building{};
   if(!mineplus::surface_settlement_building(settlement,i,building))continue;
   const int bx=centerX+building.offset_x,by=centerY+building.offset_y;
   const int extent=building.half_extent;
   for(int oy=-extent;oy<=extent;++oy)for(int ox=-extent;ox<=extent;++ox){
    const int x=bx+ox,y=by+oy;
    if(x<2||y<2||x>=COLS-2||y>=ROWS-2)continue;
    const bool edge=std::abs(ox)==extent||std::abs(oy)==extent;
    if(edge)world[y*COLS+x]=(building.role==0||building.role==2)?Stone:Wood;
    else if(ox==0&&oy==0)world[y*COLS+x]=building.role==0?Ember:Moss;
    else if((ox==0||oy==0)&&((ox+oy+int(building.role))&1)==0)
     world[y*COLS+x]=Wood;
   }
   // Doorway faces the central square and remains walkable.
   world[(by+extent)*COLS+bx]=Grass;
  }
  for(std::uint8_t resident=0;resident<settlement.resident_count&&settlementNpcCount<settlementNpcs.size();++resident){
   int hx=centerX,hy=centerY;std::uint8_t role=static_cast<std::uint8_t>(resident%5u);
   if(resident<5u){mineplus::SettlementBuilding building{};if(!mineplus::surface_settlement_building(settlement,resident,building))continue;hx+=building.offset_x;hy+=building.offset_y+building.half_extent+1;role=building.role;}
   else{hx+=resident==5u?3:-3;hy+=resident==5u?-3:3;}
   const auto ground=tile_at(hx,hy);
   if(hx<2||hy<2||hx>=COLS-2||hy>=ROWS-2||ground==DarkStone||ground==Stone||ground==Wood||ground==Bone||ground==Crystal||ground==IronOre||ground==Obsidian||ground==Rune||ground==Water||ground==Lava)continue;
   mineplus::SettlementNpc npc{};npc.x=hx;npc.y=hy;npc.home_x=hx;npc.home_y=hy;
   npc.seed=hash2(settlement.x+int(resident)*97,settlement.y-int(resident)*53);
   npc.role=role;npc.dialogue_stage=0;settlementNpcs[settlementNpcCount++]=npc;
  }
 }
 for(int i=0;i<(int)creatures.size();i++){
  auto &m=creatures[i];
  m.x=-100;m.y=-100;m.type=0;m.hp=0;
  // Spawn wildlife on valid surface clearings and choose its family from the
  // local biome. Bounded attempts keep world initialization deterministic.
  for(unsigned attempt=0;attempt<64u;++attempt){
   const unsigned salt=static_cast<unsigned>(i)*131u+attempt*977u;
   const int sx=8+static_cast<int>(hash2(static_cast<int>(salt),91)%(COLS-16));
   const int sy=8+static_cast<int>(hash2(91,static_cast<int>(salt))%(ROWS-16));
   const int tile=world[sy*COLS+sx];
   if(tile==Water||tile==Lava||tile==Wood||tile==Stone||tile==DarkStone||
      tile==Bone||tile==Crystal||tile==IronOre||tile==Obsidian||tile==Rune||
      tile==ChestClosed||tile==AncientRune||tile==ObsidianCore)continue;
   if(abs(sx-COLS/2)+abs(sy-ROWS/2)<20)continue;
   if(abs(sx-BOSS_X)+abs(sy-BOSS_Y)<20)continue;
   const auto surface=pcg.sample_surface(sx-COLS/2,sy-ROWS/2);
   const auto biome=static_cast<mineplus::Biome>(surface.biome);
   if(biome==mineplus::Biome::Ocean)continue;
   const unsigned family=hash2(sx,sy);
   const auto species=mineplus::choose_surface_species(
       biome,surface.moisture,surface.temperature,
       surface.surface,family);
   if(species>=12u)continue;
   m.type=static_cast<int>(species);
   m.x=sx;m.y=sy;m.hp=3;
   break;
  }
 }
 build_boss_arena();
 mineplus::BossCombatDirector::start(primordialBoss,SDL_GetTicks(),1200);
 world[py*COLS+px]=Air;
}
constexpr const char* SAVE_DIR="/data/MINEPLUS";
constexpr const char* SAVE_FILE="/data/MINEPLUS/world.sav";
struct SaveHeader{char magic[8];unsigned version;int x,y,selected,fx,fy;int stock[9];};
bool save_world(bool feedback=true){
 mkdir(SAVE_DIR,0777);
 const char* temp="/data/MINEPLUS/world.sav.tmp";
 FILE* f=std::fopen(temp,"wb");if(!f){if(feedback){lastSaveSucceeded=false;lastSaveAt=SDL_GetTicks();}return false;}
 SaveHeader h{{'M','P','L','U','S','3','D','\0'},12,px,py,selected,faceX,faceY,{}};
 for(int i=0;i<9;i++)h.stock[i]=stock[i];
 bool ok=std::fwrite(&h,sizeof(h),1,f)==1&&std::fwrite(&worldSeed,sizeof(worldSeed),1,f)==1&&std::fwrite(world.data(),1,world.size(),f)==world.size()&&std::fwrite(creatures.data(),sizeof(Creature),creatures.size(),f)==creatures.size()&&std::fwrite(&playerHealth,sizeof(playerHealth),1,f)==1&&std::fwrite(stock.data()+9,sizeof(int),13,f)==13&&std::fwrite(stock.data()+22,sizeof(int),9,f)==9&&std::fwrite(equipmentState.owned.data(),sizeof(std::uint8_t),equipmentState.owned.size(),f)==equipmentState.owned.size()&&std::fwrite(equipmentState.equipped.data(),sizeof(mineplus::EquipmentId),equipmentState.equipped.size(),f)==equipmentState.equipped.size()&&std::fwrite(equipmentState.durability.data(),sizeof(std::uint16_t),equipmentState.durability.size(),f)==equipmentState.durability.size()&&
   std::fwrite(&primordialBoss.max_health,sizeof(primordialBoss.max_health),1,f)==1&&std::fwrite(&primordialBoss.health,sizeof(primordialBoss.health),1,f)==1&&std::fwrite(&primordialBoss.state_started_at,sizeof(primordialBoss.state_started_at),1,f)==1&&std::fwrite(&primordialBoss.state_duration_ms,sizeof(primordialBoss.state_duration_ms),1,f)==1&&std::fwrite(&primordialBoss.attack_index,sizeof(primordialBoss.attack_index),1,f)==1&&std::fwrite(&primordialBoss.phase,sizeof(primordialBoss.phase),1,f)==1&&std::fwrite(&primordialBoss.state,sizeof(primordialBoss.state),1,f)==1&&std::fwrite(&primordialBoss.pending_attack,sizeof(primordialBoss.pending_attack),1,f)==1&&[&](){const std::uint8_t claimed=primordialBoss.reward_claimed?1u:0u;if(std::fwrite(&claimed,sizeof(claimed),1,f)!=1)return false;for(const auto& q:surfaceQuestProgress)if(std::fwrite(&q.state,sizeof(q.state),1,f)!=1||std::fwrite(&q.gathered,sizeof(q.gathered),1,f)!=1)return false;return true;}();
 if(std::fflush(f)!=0)ok=false;
 if(ok&&::fsync(::fileno(f))!=0)ok=false;
 if(std::fclose(f)!=0)ok=false;
 if(ok&&std::rename(temp,SAVE_FILE)!=0)ok=false;
 if(!ok)std::remove(temp);
 if(feedback){lastSaveSucceeded=ok;lastSaveAt=SDL_GetTicks();}
 return ok;
}
bool load_world(){
  FILE* f=std::fopen(SAVE_FILE,"rb");if(!f)return false;
  SaveHeader h{};
  bool ok=std::fread(&h,sizeof(h),1,f)==1&&std::memcmp(h.magic,"MPLUS3D",7)==0&&(h.version>=1&&h.version<=12);
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
   std::array<int,9> extraLoot{};if(ok&&h.version>=9)ok=std::fread(extraLoot.data(),sizeof(int),extraLoot.size(),f)==extraLoot.size();
  mineplus::EquipmentState restoredEquipment{};
  if(ok&&h.version>=10){
   ok=std::fread(restoredEquipment.owned.data(),sizeof(std::uint8_t),restoredEquipment.owned.size(),f)==restoredEquipment.owned.size()&&
      std::fread(restoredEquipment.equipped.data(),sizeof(mineplus::EquipmentId),restoredEquipment.equipped.size(),f)==restoredEquipment.equipped.size()&&
      std::fread(restoredEquipment.durability.data(),sizeof(std::uint16_t),restoredEquipment.durability.size(),f)==restoredEquipment.durability.size();
  }
  mineplus::BossEncounter restoredBoss{};std::uint8_t restoredRewardClaimed=0;
  if(ok&&h.version>=11){ok=std::fread(&restoredBoss.max_health,sizeof(restoredBoss.max_health),1,f)==1&&std::fread(&restoredBoss.health,sizeof(restoredBoss.health),1,f)==1&&std::fread(&restoredBoss.state_started_at,sizeof(restoredBoss.state_started_at),1,f)==1&&std::fread(&restoredBoss.state_duration_ms,sizeof(restoredBoss.state_duration_ms),1,f)==1&&std::fread(&restoredBoss.attack_index,sizeof(restoredBoss.attack_index),1,f)==1&&std::fread(&restoredBoss.phase,sizeof(restoredBoss.phase),1,f)==1&&std::fread(&restoredBoss.state,sizeof(restoredBoss.state),1,f)==1&&std::fread(&restoredBoss.pending_attack,sizeof(restoredBoss.pending_attack),1,f)==1&&std::fread(&restoredRewardClaimed,sizeof(restoredRewardClaimed),1,f)==1;restoredBoss.reward_claimed=restoredRewardClaimed!=0;}
  std::array<mineplus::SurfaceQuestProgress,mineplus::SURFACE_QUEST_ROLES> restoredQuests{};
  if(ok&&h.version>=12)for(auto& q:restoredQuests)if(std::fread(&q.state,sizeof(q.state),1,f)!=1||std::fread(&q.gathered,sizeof(q.gathered),1,f)!=1){ok=false;break;}
  if(ok&&std::fgetc(f)!=EOF)ok=false;
  std::fclose(f);if(!ok)return false;
  // Reject impossible tile IDs before they reach rendering, collision or inventory.
  // This protects the live world from structurally valid but semantically corrupt saves.
  for(const auto tile:restored)if(tile>Leaves)return false;
  if((h.version>=12&&!mineplus::surface_quest_progress_valid(restoredQuests))||restoredRewardClaimed>1||restoredBoss.max_health==0||restoredBoss.max_health>100000000u||restoredBoss.health>restoredBoss.max_health||restoredBoss.phase<1||restoredBoss.phase>3||static_cast<unsigned>(restoredBoss.state)>static_cast<unsigned>(mineplus::BossState::Defeated)||static_cast<unsigned>(restoredBoss.pending_attack)>static_cast<unsigned>(mineplus::BossAttack::RiftCharge)||(restoredBoss.state==mineplus::BossState::Defeated&&restoredBoss.health!=0)||(restoredBoss.health==0&&restoredBoss.state!=mineplus::BossState::Defeated))return false;
  for(std::size_t i=0;i<restoredEquipment.owned.size();++i){
   if(restoredEquipment.owned[i]>1)return false;
   const auto* def=mineplus::EquipmentCatalog::find(static_cast<mineplus::EquipmentId>(i));
   if(i==0){if(restoredEquipment.durability[i]!=0)return false;continue;}
   if(!def||restoredEquipment.durability[i]>def->durability)return false;
   if(!restoredEquipment.owned[i]&&restoredEquipment.durability[i]!=0)return false;
  }
  for(std::size_t slot=0;slot<restoredEquipment.equipped.size();++slot){
   const auto id=restoredEquipment.equipped[slot];const auto raw=static_cast<std::size_t>(id);
   if(id==mineplus::EquipmentId::None)continue;
   const auto* def=mineplus::EquipmentCatalog::find(id);
   if(!def||raw>=restoredEquipment.owned.size()||!restoredEquipment.owned[raw]||static_cast<std::size_t>(def->slot)!=slot)return false;
  }
  if(h.version>=2){
   for(auto &m:restoredCreatures){
    if(m.type<0||m.type>(h.version>=4?11:5)||m.hp<0||m.hp>3||m.x< -100||m.x>=(legacy?LEGACY_COLS:COLS)||m.y< -100||m.y>=(legacy?LEGACY_ROWS:ROWS))return false;
    if(legacy&&m.x>=0&&m.y>=0){m.x+=COLS/2-LEGACY_COLS/2;m.y+=ROWS/2-LEGACY_ROWS/2;}
   }
   creatures=restoredCreatures;
  }
  world=restored;
  worldSeed=restoredSeed;
  playerHealth=std::max(1,restoredHealth);lastDamage=0;damageCooldown=0;
  px=std::clamp(legacy?h.x+COLS/2-LEGACY_COLS/2:h.x,1,COLS-2);
  py=std::clamp(legacy?h.y+ROWS/2-LEGACY_ROWS/2:h.y,1,ROWS-2);
  selected=std::clamp(h.selected,0,7);faceX=h.fx;faceY=h.fy;
  if(abs(faceX)+abs(faceY)!=1){faceX=1;faceY=0;}
  stock.fill(0);
  for(int i=0;i<9;i++)stock[i]=std::clamp(h.stock[i],0,9999);
  for(int i=0;i<13;i++)stock[i+9]=std::clamp(extraStock[i],0,9999);
   for(int i=0;i<9;i++)stock[i+22]=std::clamp(extraLoot[i],0,9999);
  equipmentState=restoredEquipment;primordialBoss=restoredBoss;surfaceQuestProgress=restoredQuests;primordialBoss.state_started_at=SDL_GetTicks();
  playerHealth=std::clamp(playerHealth,1,static_cast<int>(mineplus::EquipmentCatalog::calculate(equipmentState).max_health));
  return true;
}
void tile_color(int t,Uint8&r,Uint8&g,Uint8&b){switch(t){
 case Grass:r=63;g=112;b=67;break;case Dirt:r=121;g=83;b=59;break;
 case Stone:r=105;g=111;b=119;break;case Wood:r=112;g=76;b=49;break;
 case Moss:r=65;g=100;b=61;break;case Crystal:r=103;g=177;b=190;break;
 case Bone:r=179;g=164;b=128;break;case DarkStone:r=48;g=51;b=66;break;
 case Sand:r=190;g=157;b=94;break;case Snow:r=208;g=222;b=225;break;
 case Ice:r=105;g=171;b=190;break;case Lava:r=196;g=75;b=36;break;
 case Water:r=44;g=112;b=148;break;case IronOre:r=139;g=112;b=86;break;
 case Obsidian:r=53;g=43;b=82;break;case Ember:r=235;g=103;b=43;break;
 case Rune:r=90;g=190;b=166;break;case IronBar:r=183;g=192;b=197;break;
 case ObsidianCore:r=82;g=55;b=116;break;case EmberCatalyst:r=255;g=133;b=49;break;
 case AncientRune:r=124;g=231;b=190;break;
  case ChestClosed:r=154;g=91;b=48;break;case ChestOpened:r=78;g=58;b=42;break;case Leaves:r=48;g=116;b=55;break;
  case 25:r=100;g=190;b=105;break;case 26:r=190;g=181;b=150;break;
  case 27:r=150;g=158;b=168;break;case 28:r=105;g=190;b=210;break;case 29:r=150;g=105;b=210;break;case 30:r=245;g=125;b=55;break;default:r=16;g=23;b=29;break;
}}
void inventory_item_color(int id,Uint8& r,Uint8& g,Uint8& b){
 switch(id){
  case 22:r=235;g=190;b=72;break;
  case 23:r=100;g=190;b=105;break;
  case 24:r=190;g=181;b=150;break;
  case 25:r=150;g=158;b=168;break;
  case 26:r=105;g=190;b=210;break;
  case 27:r=150;g=105;b=210;break;
  case 28:r=245;g=125;b=55;break;
  case 29:r=205;g=180;b=130;break;
  case 30:r=220;g=80;b=95;break;
  default:tile_color(id,r,g,b);break;
 }
}
void draw_material_texture(int t,int sx,int top,int bottom,int wx,int wy,unsigned seed){
 // Pixel-art de alta densidade, ancorada no espaço do mundo para não "nadar" na câmera.
 // Os padrões são baratos: hash inteiro, sem alocação e sem assets externos por frame.
 const unsigned cellX=static_cast<unsigned>(wx)&15u;
 const unsigned cellY=static_cast<unsigned>(wy)&15u;
 for(int py=top+2;py<bottom;py+=3){
  const unsigned v=static_cast<unsigned>((py-top)/3)&7u;
  const unsigned pattern=hash2(int(cellX*37u+cellY*11u+unsigned(t)*97u+seed),
                               int(v*53u+cellY*19u+unsigned(t)*31u));
  Uint8 r=0,g=0,b=0,alpha=155;bool draw=false;
  switch(t){
   case Grass: {
    const unsigned tuft=hash2(int(cellX*37u+cellY*11u+unsigned(t)*97u+seed),
                              int(cellY*19u+unsigned(t)*31u));
    const unsigned blade=(tuft>>5)&7u;
    if((tuft%47u)==0u){
     // Tiny wildflowers have a stem pixel and a brighter bloom at the crown.
     draw=(v==1u||v==2u);
     switch((tuft>>8)%4u){
      case 0:r=v==1u?116:242;g=v==1u?157:218;b=v==1u?72:126;break;
      case 1:r=v==1u?116:218;g=v==1u?157:166;b=v==1u?72:194;break;
      case 2:r=v==1u?116:225;g=v==1u?157:229;b=v==1u?72:205;break;
      default:r=v==1u?116:173;g=v==1u?157:188;b=v==1u?72:232;break;
     }
    }else if((tuft%83u)==1u){
     // Sparse fern-like fronds break up the uniform grass line.
     draw=(v==blade||v==((blade+2u)&7u)||v==((blade+6u)&7u));
     r=static_cast<Uint8>(79u+(tuft>>12)%35u);
     g=static_cast<Uint8>(151u+(tuft>>17)%48u);
     b=static_cast<Uint8>(63u+(tuft>>22)%23u);
    }else{
     draw=(v==blade||v==((blade+1u)&7u))&&((tuft>>12)%3u)!=0u;
     const unsigned shade=(tuft>>16)%4u;
     r=static_cast<Uint8>(shade==0u?83u:shade==1u?105u:shade==2u?128u:151u);
     g=static_cast<Uint8>(shade==0u?132u:shade==1u?157u:shade==2u?177u:193u);
     b=static_cast<Uint8>(shade==0u?55u:shade==1u?67u:shade==2u?78u:88u);
    }
    break;
   }
   case Dirt: {
    const unsigned cluster=(cellX*3u+cellY*5u+v*7u+pattern%5u)%13u;
    draw=cluster<4u||(pattern%19u)==0u;
    r=cluster==0u?184:static_cast<Uint8>(cluster<4u?157:190);
    g=cluster==0u?111:static_cast<Uint8>(cluster<4u?99:132);
    b=cluster==0u?76:static_cast<Uint8>(cluster<4u?72:91);
    if((pattern%37u)==0u){r=218;g=159;b=111;}
    break;
   }
   case Stone: case DarkStone: {
    const bool seam=(v==3u&&(pattern%3u)!=0u)||
        (v==7u&&((cellX+(cellY&1u))%4u==0u));
    const bool crack=(cellX*5u+v*3u+cellY)%29u==0u;
    draw=seam||crack;
    r=t==Stone?151:91;g=t==Stone?154:91;b=t==Stone?158:108;
    if(seam){r=t==Stone?76:34;g=t==Stone?82:37;b=t==Stone?91:49;}
    if(crack){r=t==Stone?185:118;g=t==Stone?188:108;b=t==Stone?194:145;}
    break;
   }
   case Wood:
    draw=(cellX%4u==0u&&(pattern%2u)==0u)||
         (cellX==9u&&(v%3u==0u))||((pattern%23u)==0u);
    r=(cellX%4u==0u)?174:145;g=(cellX%4u==0u)?119:91;b=(cellX%4u==0u)?69:57;
    if((pattern%23u)==0u){r=207;g=154;b=96;}
    break;
   case Leaves: {
    const unsigned canopy=hash2(wx/3,wy/3);
    const unsigned palette=(canopy%29u==0u)?4u:
        ((canopy%23u==0u)?5u:((pattern>>7)&3u));
    const unsigned shade=(pattern>>13)&15u;
    draw=(pattern%5u)!=0u||v==1u||(v==5u&&(pattern%7u)==0u);
    if(palette==0u){r=static_cast<Uint8>(43u+shade);g=static_cast<Uint8>(104u+shade*3u);b=static_cast<Uint8>(39u+shade);}
    else if(palette==1u){r=static_cast<Uint8>(56u+shade*2u);g=static_cast<Uint8>(132u+shade*3u);b=static_cast<Uint8>(49u+shade);}
    else if(palette==2u){r=static_cast<Uint8>(83u+shade*2u);g=static_cast<Uint8>(119u+shade*2u);b=static_cast<Uint8>(43u+shade);}
    else if(palette==3u){r=static_cast<Uint8>(116u+shade*2u);g=static_cast<Uint8>(91u+shade*2u);b=static_cast<Uint8>(48u+shade);}
    else if(palette==4u){r=static_cast<Uint8>(151u+shade*2u);g=static_cast<Uint8>(91u+shade*2u);b=static_cast<Uint8>(38u+shade);}
    else{r=static_cast<Uint8>(151u+shade*2u);g=static_cast<Uint8>(65u+shade);b=static_cast<Uint8>(49u+shade);}
    if((pattern%19u)==0u){r=static_cast<Uint8>(std::min(255,int(r)+35));g=static_cast<Uint8>(std::min(255,int(g)+38));b=static_cast<Uint8>(std::min(255,int(b)+24));}
    break;
   }
   case Moss: {
    const unsigned patch=(pattern>>6)&3u;
    draw=(pattern%4u)==0u||(v==1u&&(pattern%9u)==0u);
    r=static_cast<Uint8>(patch==0u?72u:patch==1u?94u:patch==2u?119u:137u);
    g=static_cast<Uint8>(patch==0u?112u:patch==1u?137u:patch==2u?163u:176u);
    b=static_cast<Uint8>(patch==0u?54u:patch==1u?63u:patch==2u?77u:83u);
    if((pattern%29u)==0u){r=151;g=211;b=174;}
    // Rare cool-toned mycelium glints give shaded forest floors a subtle arcane feel.
    if((pattern%71u)==0u){draw=true;r=119;g=202;b=218;alpha=205;}
    break;
   }
   case Crystal: case ObsidianCore: {
    const unsigned facet=(cellX+v*2u+(pattern>>9))%11u;
    draw=facet<4u||(pattern%17u)==0u;
    r=t==Crystal?(facet<2u?218:156):(facet<2u?190:129);
    g=t==Crystal?(facet<2u?255:218):(facet<2u?153:79);
    b=t==Crystal?(facet<2u?255:226):(facet<2u?224:173);
    alpha=220;break;
   }
   case Bone:
    draw=(v==2u||v==6u||(pattern%13u)==0u);
    r=(v==2u||v==6u)?142:222;g=(v==2u||v==6u)?133:206;b=(v==2u||v==6u)?112:164;
    break;
   case Sand: {
    const unsigned ripple=(v+cellX/3u)%6u;
    draw=ripple==0u||(pattern%9u)==0u;
    r=ripple==0u?167:224;g=ripple==0u?137:191;b=ripple==0u?91:125;
    if((pattern%31u)==0u){r=239;g=214;b=158;}
    break;
   }
   case Snow:
    draw=(pattern%7u)==0u||(v==1u&&(pattern%13u)==0u);
    r=255;g=255;b=255;alpha=210;break;
   case Ice:
    draw=((cellX+v*2u)%13u==0u)||(pattern%11u)==0u;
    r=188;g=239;b=255;alpha=205;break;
   case Lava: case Ember: case EmberCatalyst: {
    const unsigned fissure=(cellX*3u+v*5u+pattern%7u)%12u;
    draw=fissure<4u||(pattern%17u)==0u;
    r=255;g=fissure<2u?231:static_cast<Uint8>(t==Lava?151:183);b=fissure<2u?143:65;
    alpha=225;break;
   }
   case Water: {
    const unsigned phase=(worldVisualTicks/180u+cellX*3u+cellY*5u)&7u;
    const bool crest=((v+phase+cellX/3u)&7u)==0u;
    draw=crest||(v%3u==0u&&(pattern%5u)==0u);
    r=crest?175:103;g=crest?225:177;b=crest?245:204;alpha=190;
    break;
   }
   case IronOre:
    draw=((cellX*7u+v*11u)%17u<4u)||(pattern%19u)==0u;
    r=207;g=164;b=111;break;
   case Obsidian:
    draw=((cellX+v*2u)%9u<3u)||(pattern%17u)==0u;
    r=132;g=112;b=177;alpha=195;break;
   case IronBar:
    draw=(v%3u==0u&&(pattern%2u)==0u)||(cellX%5u==0u);
    r=231;g=239;b=240;break;
   case Rune: case AncientRune: {
    const bool glyph=(cellX%8u==3u)||(v%4u==1u)||((cellX+v)%7u==0u);
    draw=glyph&&(pattern%4u)!=0u;
    r=t==AncientRune?204:168;g=255;b=t==AncientRune?244:216;alpha=235;
    break;
   }
   case ChestClosed: draw=(v==2u||v==6u||cellX==1u||cellX==6u);r=224;g=151;b=73;break;
   case ChestOpened: draw=(v==2u||v==6u);r=139;g=103;b=73;break;
   default: break;
  }
  if(draw){color(r,g,b,alpha);SDL_RenderDrawPoint(renderer,sx,py);}
  // Wind moves a sparse layer of canopy pixels by at most one screen pixel.
  // The phase is coordinate-stable per tree and uses integer time buckets,
  // avoiding per-pixel trigonometry and keeping the base material texture fixed.
  if(t==Leaves && draw && (pattern&7u)==0u){
   const unsigned gust=(worldVisualTicks/160u+cellX*3u+cellY*5u)&3u;
   const int sway=gust==0u?-1:(gust==2u?1:0);
   if(sway!=0){color(116,171,91,105);SDL_RenderDrawPoint(renderer,sx+sway,py);}
  }
 }
}
bool solid_tile(int t){return t==DarkStone||t==Stone||t==Wood||t==Bone||t==Crystal||t==IronOre||t==Obsidian||t==Rune;}
bool creature_occupies(int x,int y,const Creature*skip=nullptr){if(x==BOSS_X&&y==BOSS_Y&&primordialBoss.state!=mineplus::BossState::Defeated)return true;for(const auto &m:creatures)if(&m!=skip&&m.hp>0&&m.x==x&&m.y==y)return true;return false;}
void move_player(int dx,int dy){if(!dx&&!dy)return;int nx=std::clamp(px+dx,1,COLS-2),ny=std::clamp(py+dy,1,ROWS-2);if((nx==BOSS_X&&ny==BOSS_Y&&primordialBoss.state!=mineplus::BossState::Defeated)||solid_tile(tile_at(nx,ny)))return;px=nx;py=ny;faceX=dx;faceY=dy;}
// Inventory counts are persisted as signed integers; cap every reward at the
// same limit enforced by save loading and crafting to prevent overflow.
void add_stock(int item,int amount=1){
 if(item<0||item>=static_cast<int>(stock.size())||amount<=0)return;
 if(item>=0&&item<=255&&amount<9999)amount=std::min(9999,amount+int(mineplus::surface_event_gather_bonus(currentSurfaceEvent.type,static_cast<std::uint8_t>(item))));
 stock[item]=std::clamp(stock[item],0,9999-std::min(amount,9999))+std::min(amount,9999);
 if(item>=0&&item<=255){bool questChanged=false;for(std::size_t i=0;i<surfaceQuestProgress.size();++i){const auto q=mineplus::surface_quest_definition(static_cast<std::uint8_t>(i));if(surfaceQuestProgress[i].state==static_cast<std::uint8_t>(mineplus::SurfaceQuestState::Active)&&q.item_id==item)questChanged=true;}mineplus::surface_quest_add_progress(surfaceQuestProgress,static_cast<std::uint8_t>(item),static_cast<std::uint16_t>(std::min(amount,65535)));if(questChanged)save_world(false);}
}
void turn_player(int direction){if(direction>0){int old=faceX;faceX=-faceY;faceY=old;}else if(direction<0){int old=faceX;faceX=faceY;faceY=-old;}}
Uint32 lastCreatureUpdate=0;
void update_player_combat(){
 Uint32 now=SDL_GetTicks();
 const auto stats=mineplus::EquipmentCatalog::calculate(equipmentState);
 const int maxHealth=static_cast<int>(stats.max_health);
 if(playerHealth<=0){playerHealth=maxHealth;px=COLS/2;py=ROWS/2;lastDamage=now;damageCooldown=now+1400;return;}
 playerHealth=std::clamp(playerHealth,1,maxHealth);
 if(now<damageCooldown)return;
 for(const auto &m:creatures){
  if(m.hp<=0||m.type%4==3)continue;
  const int distance=abs(m.x-px)+abs(m.y-py);
  if(distance<=1&&now-lastDamage>=(currentSurfaceEvent.type==mineplus::SurfaceEventType::PredatorWarning?700u:950u)){
   const bool elemental=m.type==6||m.type==9||m.type==10;
   const int incoming=mineplus::EquipmentCatalog::mitigate_damage(3,stats,elemental);
   playerHealth=std::max(0,playerHealth-incoming);
   for(std::size_t slot=1;slot<equipmentState.equipped.size();++slot)
    mineplus::EquipmentCatalog::wear(equipmentState,static_cast<mineplus::EquipmentSlot>(slot),1);
   save_world(false);
   lastDamage=now;damageCooldown=now+500;return;
  }
 }
}
void update_primordial_boss(Uint32 now){
 const int distance=abs(BOSS_X-px)+abs(BOSS_Y-py);
 if(primordialBoss.state==mineplus::BossState::Dormant&&distance>9)return;
 const auto event=mineplus::BossCombatDirector::update(primordialBoss,now);
 if(event==mineplus::BossEvent::AttackResolved){const auto attack=mineplus::BossCombatDirector::attack_profile(primordialBoss.pending_attack,primordialBoss.phase);const int dx=abs(BOSS_X-px),dy=abs(BOSS_Y-py);const bool hit=mineplus::BossCombatDirector::attack_hits_player(attack,static_cast<std::uint16_t>(dx),static_cast<std::uint16_t>(dy));lastBossAttack=attack.attack;lastBossAttackAt=SDL_GetTicks();lastBossAttackHitPlayer=hit;
  if(hit){const auto stats=mineplus::EquipmentCatalog::calculate(equipmentState);const int incoming=mineplus::EquipmentCatalog::mitigate_damage(attack.damage,stats,true);playerHealth=std::max(0,playerHealth-incoming);for(std::size_t slot=1;slot<equipmentState.equipped.size();++slot)mineplus::EquipmentCatalog::wear(equipmentState,static_cast<mineplus::EquipmentSlot>(slot),1);}
 }
 if(primordialBoss.state==mineplus::BossState::Defeated&&mineplus::BossCombatDirector::claim_reward(primordialBoss)){add_stock(30,1);add_stock(29,3);add_stock(28,2);save_world(false);return;}
 if(event!=mineplus::BossEvent::None)save_world(false);
}
void update_settlement_npcs(){
 const Uint32 now=SDL_GetTicks();if(now-lastNpcUpdate<800u)return;lastNpcUpdate=now;
 const std::uint32_t schedule=now/800u;
 for(std::size_t i=0;i<settlementNpcCount;++i){
  auto& npc=settlementNpcs[i];std::int32_t gx=npc.home_x,gy=npc.home_y;
  mineplus::settlement_npc_goal(npc,schedule,gx,gy);
  const int dx=gx-npc.x,dy=gy-npc.y;if(std::abs(dx)+std::abs(dy)==0)continue;
  int sx=0,sy=0;
  if(std::abs(dx)>std::abs(dy))sx=(dx>0)-(dx<0);
  else if(std::abs(dy)>std::abs(dx))sy=(dy>0)-(dy<0);
  else if((npc.seed+schedule)&1u)sx=(dx>0)-(dx<0);else sy=(dy>0)-(dy<0);
  const int nx=npc.x+sx,ny=npc.y+sy;
  if(nx<1||ny<1||nx>=COLS-1||ny>=ROWS-1||(nx==px&&ny==py)||solid_tile(tile_at(nx,ny))||tile_at(nx,ny)==Water||tile_at(nx,ny)==Lava)continue;
  bool occupied=false;for(std::size_t j=0;j<settlementNpcCount;++j)if(j!=i&&settlementNpcs[j].x==nx&&settlementNpcs[j].y==ny){occupied=true;break;}
  if(!occupied){npc.x=nx;npc.y=ny;}
 }
}
std::size_t nearest_settlement_npc(){
 std::size_t nearest=settlementNpcCount;int best=3;
 for(std::size_t i=0;i<settlementNpcCount;++i){const int d=std::abs(settlementNpcs[i].x-px)+std::abs(settlementNpcs[i].y-py);if(d<best){best=d;nearest=i;}}
 return nearest;
}
void open_surface_trade(){
 const auto nearest=nearest_settlement_npc();
 if(nearest==settlementNpcCount){activeNpcName="NO MERCHANT";activeNpcLine="STAND CLOSER TO A RESIDENT";npcDialogueUntil=SDL_GetTicks()+3000u;return;}
 tradeRole=settlementNpcs[nearest].role;selectedTradeOffer=0;tradeOpen=true;
}
void buy_selected_trade_offer(){
 if(!tradeOpen)return;
 const auto discount=mineplus::surface_event_trade_discount(currentSurfaceEvent.type);
 if(mineplus::surface_trade_buy(tradeRole,selectedTradeOffer,stock.data(),stock.size(),discount)){
  const auto offer=mineplus::surface_trade_offer(tradeRole,selectedTradeOffer);
  activeNpcName="TRADE COMPLETE";activeNpcLine=offer.name;npcDialogueUntil=SDL_GetTicks()+2600u;mineplus::surface_quest_add_progress(surfaceQuestProgress,offer.item_id,1);save_world(false);
 }else{activeNpcName="NOT ENOUGH COINS";activeNpcLine="OR INVENTORY SLOT IS FULL";npcDialogueUntil=SDL_GetTicks()+2600u;}
}
void talk_to_nearest_npc(){
 const auto nearest=nearest_settlement_npc();
 if(nearest==settlementNpcCount){activeNpcName="NO RESIDENT";activeNpcLine="COME CLOSER TO A VILLAGER";}
 else{
  const auto role=settlementNpcs[nearest].role;
  const auto def=mineplus::surface_quest_definition(role);
  const auto action=mineplus::surface_quest_interact(role,surfaceQuestProgress,stock.data(),stock.size());
  auto& progress=surfaceQuestProgress[role];
  activeNpcName=def.title;
  if(action==mineplus::SurfaceQuestAction::Accepted)activeNpcLine=def.description;
  else if(action==mineplus::SurfaceQuestAction::Progress){static char status[48];std::snprintf(status,sizeof(status),"GATHERED %u OF %u",unsigned(progress.gathered),unsigned(def.required));activeNpcLine=status;}
  else if(action==mineplus::SurfaceQuestAction::Claimed){activeNpcLine="QUEST COMPLETE  /  REWARD RECEIVED";save_world(false);}
  else if(action==mineplus::SurfaceQuestAction::AlreadyClaimed)activeNpcLine="THANK YOU  /  THIS TASK IS COMPLETE";
  else activeNpcLine="QUEST UNAVAILABLE";
  if(action==mineplus::SurfaceQuestAction::Accepted)save_world(false);
 }
 npcDialogueUntil=SDL_GetTicks()+5200u;
}
void update_creatures(){
 Uint32 now=SDL_GetTicks();const auto interval=mineplus::surface_event_creature_interval(currentSurfaceEvent.type);if(now-lastCreatureUpdate<interval)return;lastCreatureUpdate=now;
 const unsigned tick=now/interval;
 for(auto &m:creatures){
  if(m.hp<=0)continue;
  int dx=px-m.x,dy=py-m.y,distance=abs(dx)+abs(dy);
  const auto* species=mineplus::surface_species(static_cast<std::uint8_t>(m.type));
  const bool passive=species && (species->behavior==mineplus::SurfaceBehavior::Grazer || species->behavior==mineplus::SurfaceBehavior::Skittish);
  const auto alertDistance=mineplus::surface_event_alert_distance(currentSurfaceEvent.type,passive);
  const bool wandering=distance>alertDistance;
  if(!wandering&&!dx&&!dy)continue;
  int sx=0,sy=0;
  if(wandering){
   // Distant creatures patrol instead of freezing; no mutable RNG state is needed.
   const unsigned roam=hash2(m.x+int(tick*17u),m.y-int(tick*11u)+m.type*31);
   if((roam&3u)!=0u)continue;
   switch((roam>>4)&3u){case 0:sx=1;break;case 1:sx=-1;break;case 2:sy=1;break;default:sy=-1;break;}
  }else if(passive){
   // Skittish wildlife flees when approached instead of attacking the player.
   if(abs(dx)>=abs(dy))sx=dx>0?-1:1;else sy=dy>0?-1:1;
  }else{
   // Aggressive species pursue the player with axis-prioritized pathing.
   if(abs(dx)>=abs(dy))sx=(dx>0)-(dx<0);else sy=(dy>0)-(dy<0);
  }
  int nx=m.x+sx,ny=m.y+sy;
  if(nx==px&&ny==py)continue;
  if(nx<1||nx>=COLS-1||ny<1||ny>=ROWS-1||solid_tile(tile_at(nx,ny))||creature_occupies(nx,ny,&m)){
   sx=0;sy=0;
   if(wandering){
    const unsigned turn=hash2(m.x+int(tick),m.y-m.type);
    if(turn&1u)sx=(turn&2u)?1:-1;else sy=(turn&2u)?1:-1;
   }else if(abs(dx)<abs(dy)){sx=passive?(dy>0?-1:1):(dy>0?1:-1);}else{sy=passive?(dx>0?-1:1):(dx>0?1:-1);}
   nx=m.x+sx;ny=m.y+sy;
  }
  if((nx!=px||ny!=py)&&nx>=1&&nx<COLS-1&&ny>=1&&ny<ROWS-1&&!solid_tile(tile_at(nx,ny))&&!creature_occupies(nx,ny,&m)){m.x=nx;m.y=ny;}
 }
}
bool target_tile(int& x,int& y){x=px+faceX;y=py+faceY;return worldGrid.contains(x,y);}
void place_block(){int x=0,y=0;int block=selected+1;if(target_tile(x,y)&&tile_at(x,y)==Air&&block<=8&&stock[block]>0&&set_tile_at(x,y,static_cast<unsigned char>(block)))--stock[block];}
void change_recipe(int direction){selectedRecipe=(selectedRecipe+direction+9)%9;}
void change_equipment(int direction){selectedEquipment=(selectedEquipment+direction+25)%25;}
mineplus::EquipmentId selected_equipment_id(){return static_cast<mineplus::EquipmentId>(selectedEquipment+1);}
bool craft_equipment(){
 const auto id=selected_equipment_id();
 if(!mineplus::EquipmentCatalog::craft(equipmentState,id,stock.data(),stock.size()))return false;
 const auto* def=mineplus::EquipmentCatalog::find(id);
 const auto slot=static_cast<std::size_t>(def->slot);
 if(slot<equipmentState.equipped.size()&&equipmentState.equipped[slot]==mineplus::EquipmentId::None)
  mineplus::EquipmentCatalog::equip(equipmentState,id);
 save_world(false);
 return true;
}
bool equip_selected_equipment(){
 const bool equipped=mineplus::EquipmentCatalog::equip(equipmentState,selected_equipment_id());
 if(equipped)save_world(false);
 return equipped;
}
bool repair_selected_equipment(){
 const bool repaired=mineplus::EquipmentCatalog::repair(equipmentState,selected_equipment_id(),stock.data(),stock.size());
 if(repaired)save_world(false);
 return repaired;
}
void craft_item(){
 // Five deterministic survival recipes, all using resources already in the world.
 const int ironBarsBefore=stock[IronBar];
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
 const int craftedIronBars=stock[IronBar]-ironBarsBefore;
 if(craftedIronBars>0){mineplus::surface_quest_add_progress(surfaceQuestProgress,18,static_cast<std::uint16_t>(craftedIronBars));save_world(false);}
}
bool open_treasure_chest(int x,int y){
  if(tile_at(x,y)!=ChestClosed)return false;
  mineplus::DungeonRoom room{};room.id=hash2(x,y);room.x=x;room.y=y;room.width=7;room.height=7;room.type=mineplus::DungeonRoomType::Treasure;
  mineplus::DungeonChestState chest{};
  if(!mineplus::DungeonLootGenerator::generate_chest(worldSeed,room,chest))return false;
  if(!set_tile_at(x,y,ChestOpened))return false;
  for(const auto& drop:chest.contents)add_stock(21+static_cast<int>(drop.item),drop.count);
  add_stock(22,4+static_cast<int>(hash2(x,y)%5u));
  save_world();
  return true;
}
void mine_block(){
 int x=0,y=0;if(!target_tile(x,y))return;
 if(tile_at(x,y)==ChestClosed){open_treasure_chest(x,y);return;}
 if(x==BOSS_X&&y==BOSS_Y&&primordialBoss.state!=mineplus::BossState::Defeated){const auto stats=mineplus::EquipmentCatalog::calculate(equipmentState);const auto damage=static_cast<std::uint32_t>(std::max(1,static_cast<int>(stats.damage)/6));mineplus::BossCombatDirector::damage(primordialBoss,damage);lastBossHitAt=SDL_GetTicks();mineplus::EquipmentCatalog::wear(equipmentState,mineplus::EquipmentSlot::Weapon,1);if(primordialBoss.state==mineplus::BossState::Defeated&&mineplus::BossCombatDirector::claim_reward(primordialBoss)){add_stock(30,1);add_stock(29,3);add_stock(28,2);}save_world(false);return;}
 for(auto &m:creatures)if(m.x==x&&m.y==y&&m.hp>0){
  lastCreatureHit=SDL_GetTicks();lastCreatureHitX=x;lastCreatureHitY=y;
  const auto stats=mineplus::EquipmentCatalog::calculate(equipmentState);
  const int damage=std::max(1,static_cast<int>(stats.damage)/6);
  m.hp-=damage;
  mineplus::EquipmentCatalog::wear(equipmentState,mineplus::EquipmentSlot::Weapon,1);
  if(m.hp<=0){m.hp=0;if(m.type<4)add_stock(Bone,2);else if(m.type<8)add_stock(Crystal);else add_stock(DarkStone);m.x=-100;m.y=-100;}
  save_world(false);return;
 }
 int block=tile_at(x,y);
 if(block>0){int drop=block;if(drop==Sand||drop==Snow||drop==Water||drop==Ice)drop=Dirt;else if(drop==Lava)drop=Stone;if(drop==Leaves)add_stock(Moss);else if(drop>=1&&drop<=8)add_stock(drop);else if(drop>=IronOre&&drop<=Rune)add_stock(drop);set_tile_at(x,y,Air);}
}
bool use_weapon_art(){
 const auto weapon=equipmentState.equipped[static_cast<std::size_t>(mineplus::EquipmentSlot::Weapon)];
 const auto profile=mineplus::weapon_art(weapon);
 const Uint32 now=SDL_GetTicks();
 if(!mineplus::weapon_art_ready(profile,now,lastWeaponArtAt))return false;
 const auto raw=static_cast<std::size_t>(weapon);
 if(raw>=equipmentState.durability.size()||equipmentState.durability[raw]<profile.durability_cost)return false;
 const auto stats=mineplus::EquipmentCatalog::calculate(equipmentState);
 const int damage=std::max(1,static_cast<int>(stats.damage)/6)+profile.bonus_damage;
 bool hit=false;
 for(unsigned step=1;step<=profile.range;++step){const int bx=px+faceX*static_cast<int>(step),by=py+faceY*static_cast<int>(step);if(bx==BOSS_X&&by==BOSS_Y&&primordialBoss.state!=mineplus::BossState::Defeated){mineplus::BossCombatDirector::damage(primordialBoss,static_cast<std::uint32_t>(damage));lastBossHitAt=now;hit=true;if(primordialBoss.state==mineplus::BossState::Defeated&&mineplus::BossCombatDirector::claim_reward(primordialBoss)){add_stock(30,1);add_stock(29,3);add_stock(28,2);}}
  const int tx=px+faceX*static_cast<int>(step),ty=py+faceY*static_cast<int>(step);
  for(auto &m:creatures)if(m.hp>0&&m.x==tx&&m.y==ty){
   m.hp=std::max(0,m.hp-damage);lastCreatureHit=now;lastCreatureHitX=tx;lastCreatureHitY=ty;hit=true;
   if(m.hp==0){if(m.type<4)add_stock(Bone,2);else if(m.type<8)add_stock(Crystal);else add_stock(DarkStone);m.x=-100;m.y=-100;}
  }
 }
 if(!hit)return false;
 lastWeaponArtAt=now;weaponArtFlashUntil=now+260u;
 mineplus::EquipmentCatalog::wear(equipmentState,mineplus::EquipmentSlot::Weapon,profile.durability_cost);
 save_world(false);return true;
}
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
 switch(t){case Grass:h+=0.12f;break;case Dirt:h+=0.04f;break;case Stone:h+=0.34f;break;case Wood:h+=0.72f;break;case Leaves:h+=0.96f;break;case Moss:h+=0.18f;break;case Crystal:h+=0.92f;break;case Bone:h+=0.48f;break;case DarkStone:h+=0.42f;break;case Sand:h-=0.08f;break;case Snow:h+=0.24f;break;case Ice:h+=0.12f;break;case Lava:h-=0.08f;break;case Water:h-=0.22f;break;case IronOre:h+=0.36f;break;case Obsidian:h+=0.46f;break;case Ember:h+=0.02f;break;case Rune:h+=0.58f;break;case IronBar:h+=0.3f;break;case ObsidianCore:h+=0.6f;break;case EmberCatalyst:h+=0.5f;break;case AncientRune:h+=0.8f;break;default:break;}
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
 if(primordialBoss.state!=mineplus::BossState::Defeated){
  const int bx=std::clamp(BOSS_X-px,-radius,radius),by=std::clamp(BOSS_Y-py,-radius,radius);
  const int markerX=ox+(bx+radius)*cell+2,markerY=oy+(by+radius)*cell+2;
  rect(markerX,markerY,5,5,225,65,92);
  color(255,175,180);SDL_RenderDrawLine(renderer,markerX-2,markerY+2,markerX+6,markerY+2);
 }
 draw_text("MAP",ox,oy+cell*17+7,1,190,204,190);
 if(primordialBoss.state!=mineplus::BossState::Defeated)draw_text("GUARDIAN",ox+38,oy+cell*17+7,1,220,105,120);
 char coords[32];std::snprintf(coords,sizeof(coords),"%d %d",px,py);
 draw_text(coords,ox+35,oy+cell*17+7,1,190,204,190);
}
bool boss_visible_from_player(){
 const int dx=BOSS_X-px,dy=BOSS_Y-py,steps=std::max(abs(dx),abs(dy));
 for(int i=1;i<steps;++i){const int x=px+(dx*i)/steps,y=py+(dy*i)/steps;if(solid_tile(tile_at(x,y)))return false;}
 return true;
}
void draw_surface_event_effects(){
 // Lightweight, deterministic screen-space weather and event particles.
 // These effects are deliberately confined to the current SDL renderer path.
 const Uint32 now=SDL_GetTicks();
 const float pulse=0.5f+0.5f*std::sin(float(now)*0.006f);
 if(currentSurfaceEvent.type==mineplus::SurfaceEventType::RuneStorm){
  // Slanted cyan rune rain with a small bright head and a fading trail.
  for(unsigned i=0;i<34u;++i){
   const unsigned seed=hash2(int(i*73u+19u),int(worldSeed));
   const int x=int((seed+now/5u*(1u+(i%3u)))%unsigned(W));
   const int y=int((hash2(int(i*29u+7u),int(seed>>3))+now/3u*(2u+i%3u))%unsigned(H-90));
   const int length=5+int(seed%13u);
   color(49,150,159,75);SDL_RenderDrawLine(renderer,x,y-length,x-3,y);
   color(135,245,220,static_cast<Uint8>(105u+(seed%90u)));
   SDL_RenderDrawLine(renderer,x,y-2,x-2,y+2);
   if((i%5u)==0u)rect(x-1,y-1,3,3,170,255,226,110);
  }
 }else if(currentSurfaceEvent.type==mineplus::SurfaceEventType::HarvestFestival){
  // Warm pollen and drifting leaf motes; positions loop without per-frame allocation.
  for(unsigned i=0;i<30u;++i){
   const unsigned seed=hash2(int(i*97u+31u),int(worldSeed>>4));
   const int x=int((seed+now/9u*(1u+i%2u))%unsigned(W));
   const int y=90+int((hash2(int(seed),int(i*41u))+unsigned(H*4)-now/7u*(1u+i%3u))%unsigned(H-110));
   if(i%3u==0u)rect(x,y,3,2,224,194,112,125);
   else rect(x,y,2,2,151,197,113,105);
  }
 }else if(currentSurfaceEvent.type==mineplus::SurfaceEventType::PredatorWarning){
  // Subtle pulsing edge warning communicates danger without obscuring the view.
  const Uint8 alpha=static_cast<Uint8>(18u+unsigned(pulse*28.0f));
  rect(0,64,W,7,154,38,43,alpha);rect(0,H-82,W,7,154,38,43,alpha);
  rect(0,64,7,H-145,154,38,43,alpha);rect(W-7,64,7,H-145,154,38,43,alpha);
  if((now/420u)%2u==0u){
   color(224,91,77,145);
   SDL_RenderDrawLine(renderer,18,88,31,101);SDL_RenderDrawLine(renderer,31,101,18,114);
   SDL_RenderDrawLine(renderer,W-18,88,W-31,101);SDL_RenderDrawLine(renderer,W-31,101,W-18,114);
  }
 }else if(currentSurfaceEvent.type==mineplus::SurfaceEventType::CaravanMarket){
  // A small animated market pennant reinforces the temporary trading event.
  const int sway=int(std::sin(float(now)*0.004f)*4.0f);
  rect(W-92,142,3,30,128,103,65,180);
  rect(W-89,144+sway,22,12,211,165,76,205);
  rect(W-89,156+sway,15,3,113,75,48,190);
 }else if(currentSurfaceEvent.type==mineplus::SurfaceEventType::WildlifeMigration){
  // A few soft drifting motes suggest movement across the open landscape.
  for(unsigned i=0;i<16u;++i){
   const unsigned seed=hash2(int(i*53u+11u),int(worldSeed>>7));
   const int x=int((seed+now/13u*(1u+i%2u))%unsigned(W));
   const int y=120+int((hash2(int(seed>>2),int(i*17u))+unsigned(H)-now/17u)%unsigned(H-180));
   rect(x,y,2,2,187,204,151,75);
  }
 }
}
void draw_surface_weather(){
 // Weather is selected in deterministic 90-second windows from local climate.
 // This is a screen-space pass for the current SDL renderer, not simulated fluid.
 const Uint32 now=SDL_GetTicks();
 const mineplus::PcgWorld climateWorld(worldSeed);
 const auto climate=climateWorld.sample_surface(px-COLS/2,py-ROWS/2);
 const unsigned window=now/90000u;
 const unsigned roll=hash2(static_cast<int>(window),static_cast<int>(worldSeed));
 const auto biome=static_cast<mineplus::Biome>(climate.biome);
 enum class Weather{Clear,Rain,Snow,Ash};
 Weather weather=Weather::Clear;
 if(biome==mineplus::Biome::Volcanic){
  if(roll%100u<62u)weather=Weather::Ash;
 }else if(biome==mineplus::Biome::Tundra ||
          (biome==mineplus::Biome::Highlands&&climate.temperature<105u)){
  if(roll%100u<58u)weather=Weather::Snow;
 }else if(biome==mineplus::Biome::Plains||biome==mineplus::Biome::Forest||
          biome==mineplus::Biome::Highlands){
  if(roll%100u<43u)weather=Weather::Rain;
 }
 if(weather==Weather::Rain){
  for(unsigned i=0;i<76u;++i){
   const unsigned seed=hash2(static_cast<int>(i*61u+window*17u),static_cast<int>(worldSeed>>3));
   const int x=static_cast<int>((seed+now/3u*(2u+i%3u))%static_cast<unsigned>(W));
   const int y=static_cast<int>((hash2(static_cast<int>(seed>>2),static_cast<int>(i*13u))+now/2u*(3u+i%2u))%static_cast<unsigned>(H-70));
   color(116,165,190,static_cast<Uint8>(55u+seed%75u));
   SDL_RenderDrawLine(renderer,x,y,x-3,y+9+static_cast<int>(seed%7u));
  }
  if((now/11000u)%13u==static_cast<Uint32>(roll%13u)&&now%11000u<75u)
   rect(0,64,W,H-64,190,211,225,24);
 }else if(weather==Weather::Snow){
  for(unsigned i=0;i<42u;++i){
   const unsigned seed=hash2(static_cast<int>(i*47u+window*29u),static_cast<int>(worldSeed>>5));
   const int drift=static_cast<int>(std::sin(static_cast<double>(now)*0.0012+static_cast<double>(i))*5.0);
   const int x=static_cast<int>((seed+now/18u*(1u+i%2u))%static_cast<unsigned>(W))+drift;
   const int y=static_cast<int>((hash2(static_cast<int>(seed),static_cast<int>(i*19u))+now/12u*(1u+i%3u))%static_cast<unsigned>(H-72));
   rect(x,y,1+(seed%4u==0u),1+(seed%5u==0u),205,220,226,static_cast<Uint8>(95u+seed%100u));
  }
 }else if(weather==Weather::Ash){
  for(unsigned i=0;i<34u;++i){
   const unsigned seed=hash2(static_cast<int>(i*83u+window*11u),static_cast<int>(worldSeed>>7));
   const int x=static_cast<int>((seed+now/15u*(1u+i%2u))%static_cast<unsigned>(W));
   const int y=static_cast<int>((hash2(static_cast<int>(seed>>1),static_cast<int>(i*23u))+now/9u*(1u+i%3u))%static_cast<unsigned>(H-72));
   rect(x,y,2,2,static_cast<Uint8>(165u+seed%75u),static_cast<Uint8>(75u+seed%55u),48,static_cast<Uint8>(90u+seed%100u));
  }
 }
}
void draw_world(){
 // Perspective height-field renderer for the native PS4 build.
 // A complete day/night cycle drives sky palette, celestial bodies and material emission.
 const Uint32 worldTicks=SDL_GetTicks();
 worldVisualTicks=worldTicks;
 const float dayPhase=float(worldTicks%480000u)/480000.0f*6.2831853f;
 const float daylight=std::clamp(static_cast<float>(0.5+0.5*std::sin(static_cast<double>(dayPhase)-1.5707963)),0.0f,1.0f);
 const float dawn=std::max(0.0f,static_cast<float>(1.0-std::fabs(static_cast<double>(daylight)-0.5)*3.4));
 const float glowPulse=0.92f+0.08f*std::sin(float(worldTicks)*0.004f);
 const float sunDirX=std::cos(dayPhase),sunDirY=std::sin(dayPhase);
 const int skyR=int(7+daylight*67+dawn*65),skyG=int(12+daylight*112+dawn*37),skyB=int(24+daylight*157+dawn*20);
 rect(0,0,W,H,skyR,skyG,skyB);
 for(int y=0;y<330;y+=3){
  const float blend=float(y)/330.0f;
  const int r=int(skyR+(31+daylight*118-skyR)*blend);
  const int g=int(skyG+(42+daylight*119-skyG)*blend);
  const int b=int(skyB+(53+daylight*105-skyB)*blend);
  rect(0,y,W,3,r,g,b);
 }
 // A deterministic star field fades in as the daylight falls.
 const float night=1.0f-daylight;
 for(unsigned i=0;i<72u;++i){
  const unsigned seed=hash2(static_cast<int>(i*37u+13u),static_cast<int>(worldSeed>>9));
  const int starX=static_cast<int>(seed%static_cast<unsigned>(W));
  const int starY=18+static_cast<int>((seed>>12)%170u);
  const Uint8 starAlpha=static_cast<Uint8>(night*static_cast<float>(92u+(seed>>20)%120u));
  if(starAlpha>12u)rect(starX,starY,1+(seed%5u==0u),1,178,198,222,starAlpha);
 }
 // Sun and moon travel in opposite halves of the same sky path.
 const float celestial=dayPhase;
 const int celestialX=int(W*0.5f+std::cos(celestial)*W*0.39f);
 const int celestialY=115+int((1.0f-std::sin(celestial))*62.0f);
 const bool sunVisible=daylight>0.08f;
 const Uint8 bodyAlpha=static_cast<Uint8>(150+int(daylight*105.0f));
 for(int ring=4;ring>=1;--ring){
  const int radius=ring*5;
  for(int yy=-radius;yy<=radius;yy+=2)for(int xx=-radius;xx<=radius;xx+=2)
   if(xx*xx+yy*yy<=radius*radius){
    if(sunVisible)color(255,static_cast<Uint8>(150+ring*13),static_cast<Uint8>(75+ring*8),static_cast<Uint8>(16+ring*7));
    else color(115,150,178,static_cast<Uint8>(18+ring*9));
    SDL_RenderDrawPoint(renderer,celestialX+xx,celestialY+yy);
   }
 }
 for(int yy=-11;yy<=11;yy++)for(int xx=-11;xx<=11;xx++)if(xx*xx+yy*yy<=121){
  if(sunVisible)color(255,218,151,bodyAlpha);
  else color(207,220,225,bodyAlpha);
  SDL_RenderDrawPoint(renderer,celestialX+xx,celestialY+yy);
 }
 // Moving voxel-style cloud banks add parallax and soften the horizon.
 // Their positions are seed-stable; only the wind offset changes over time.
 const Uint8 cloudAlpha=static_cast<Uint8>(28.0f+daylight*54.0f);
 for(unsigned i=0;i<22u;++i){
  const unsigned cloudSeed=hash2(static_cast<int>(i*131u+47u),
                                 static_cast<int>(worldSeed>>7));
  const unsigned travel=worldTicks/95u*(1u+(i%3u));
  const int cloudX=static_cast<int>((cloudSeed+travel)%
      static_cast<unsigned>(W+360))-180;
  const int cloudY=28+static_cast<int>((cloudSeed>>11)%178u);
  const int cloudW=72+static_cast<int>((cloudSeed>>19)%126u);
  const int cloudH=10+static_cast<int>((cloudSeed>>25)%17u);
  const Uint8 cloudR=static_cast<Uint8>(daylight>0.55f?118:45);
  const Uint8 cloudG=static_cast<Uint8>(daylight>0.55f?139:58);
  const Uint8 cloudB=static_cast<Uint8>(daylight>0.55f?157:83);
  rect(cloudX,cloudY,cloudW,cloudH,cloudR,cloudG,cloudB,cloudAlpha);
  rect(cloudX+cloudW/5,cloudY-cloudH/2,cloudW/2,cloudH/2+2,
       static_cast<Uint8>(cloudR+12u),static_cast<Uint8>(cloudG+12u),
       static_cast<Uint8>(cloudB+12u),static_cast<Uint8>(cloudAlpha+12u));
  rect(cloudX+cloudW/3,cloudY+cloudH-2,cloudW/2,3,
       22,31,43,static_cast<Uint8>(cloudAlpha/2u));
 }
 // Layered horizon haze becomes brighter at sunrise and sunset.
 for(int i=0;i<7;i++){
  const int warm=int(dawn*28.0f);
  rect(0,278+i*17,W,4,Uint8(27+warm),Uint8(42+warm/2),Uint8(52+int(daylight*12)),Uint8(18+int(daylight*9)));
 }
 const float dirX=float(faceX),dirY=float(faceY);
 const float rightX=-dirY,rightY=dirX;
 const float focal=720.0f, horizon=H*0.48f, cameraH=1.48f;
 const float fov=0.92f;
 // Full horizontal sampling removes the alternating-column gaps of the
 // previous half-resolution ray pass. The PS4 build is intentionally sharper.
 for(int sx=0;sx<W;++sx){
  float cameraX=(2.0f*(sx+0.5f)/float(W)-1.0f)*fov;
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
    // Height gradients provide cheap directional terrain lighting without
    // requiring a full 3D GPU pipeline; slopes catch the moving sun naturally.
    const int heightDx=int(heightMap[wy*COLS+wx+1])-int(heightMap[wy*COLS+wx-1]);
    const int heightDy=int(heightMap[(wy+1)*COLS+wx])-int(heightMap[(wy-1)*COLS+wx]);
    const float slopeLight=std::clamp(1.0f+
        (float(heightDx)*sunDirX+float(heightDy)*sunDirY)*0.006f,0.72f,1.18f);
    light*=slopeLight;
    if(t==Lava)light=std::min(1.0f,light+0.48f);
    if(t==Crystal)light=std::min(1.0f,light+0.16f);
    int strata=((wx+wy*3)%7==0)?9:0;
    int fleck=(tex%23==0)?22:0;
    int rr=int((int(r)+grain+strata+fleck)*light);
    int gg=int((int(g)+grain+strata+fleck)*light);
    int bb=int((int(b)+grain+strata+fleck)*light);
    // Localized material emission adds a readable glow to rare resources and hazards.
    const int emission=int(glowPulse*(t==Lava?48:t==Ember||t==EmberCatalyst?35:
                         t==Crystal||t==ObsidianCore?25:t==Rune||t==AncientRune?22:0));
    if(emission>0){
     if(t==Lava||t==Ember||t==EmberCatalyst){rr+=emission;gg+=emission/2;bb+=emission/7;}
     else if(t==Crystal||t==ObsidianCore){rr+=emission/2;gg+=emission;bb+=emission;}
     else{rr+=emission/3;gg+=emission;bb+=emission*3/4;}
    }
    // Distance haze blends the far terrain into the current sky palette.
    // This avoids a hard black horizon and makes valleys feel deeper.
    const float fog=std::clamp((dist-10.0f)/28.0f,0.0f,0.52f);
    rr=int(float(rr)*(1.0f-fog)+float(skyR)*fog);
    gg=int(float(gg)*(1.0f-fog)+float(skyG)*fog);
    bb=int(float(bb)*(1.0f-fog)+float(skyB)*fog);
    r=Uint8(std::clamp(rr,0,255));
    g=Uint8(std::clamp(gg,0,255));
    b=Uint8(std::clamp(bb,0,255));
    color(r,g,b);SDL_RenderDrawLine(renderer,sx,sy,sx,lastY);
    // Hand-painted-inspired pixel motifs are generated from world coordinates,
    // so grain, mineral seams and foliage marks do not crawl while moving.
    draw_material_texture(t,sx,sy,lastY,wx,wy,tex);
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
 // Project creatures with deterministic species silhouettes and time-based animation.
 const float animTime=float(SDL_GetTicks())*0.006f;
 for(const auto &m:creatures)if(m.hp>0){
  float dx=float(m.x-px),dy=float(m.y-py),forward=dx*dirX+dy*dirY,side=dx*rightX+dy*rightY;
  if(forward<0.7f||forward>22.0f)continue;
  int sx=int(W*0.5f+(side/forward)*focal/fov);
  int size=int(430.0f/forward);
  const float phase=animTime+float(hash2(m.x,m.y)%628u)*0.01f;
  const int bob=int(std::sin(phase*(m.type%3==0?1.8f:2.7f))*std::max(1,size/22));
  int sy=int(horizon+focal*0.65f/forward-size*0.8f)+bob;
  if(sx>8&&sx<W-8&&size>3){
   static const int speciesColor[12]={75,105,61,130,150,92,112,83,158,67,137,98};
   const bool recentlyHit=(m.x==lastCreatureHitX&&m.y==lastCreatureHitY&&SDL_GetTicks()-lastCreatureHit<190u);
   const int rr=speciesColor[std::clamp(m.type,0,11)];
   const int bodyW=std::max(2,size*(m.type==8||m.type==10?4:2)/3);
   const int bodyH=std::max(2,size*(m.type==3||m.type==8?5:4)/4);
   const int legSwing=int(std::sin(phase*5.0f)*std::max(1,size/12));
   // Ground shadow and alternating legs make movement readable at distance.
   rect(sx-bodyW/2,sy+bodyH-1,bodyW, std::max(1,size/14),8,10,11,100);
   rect(sx-bodyW/3,sy+bodyH-std::max(2,size/8),std::max(2,bodyW/4),std::max(2,size/7)+legSwing,rr-17,rr-15,rr-10);
   rect(sx+bodyW/8,sy+bodyH-std::max(2,size/8),std::max(2,bodyW/4),std::max(2,size/7)-legSwing,rr-17,rr-15,rr-10);
   rect(sx-bodyW/2,sy,bodyW,bodyH,recentlyHit?245:rr,recentlyHit?220:rr-8,recentlyHit?190:rr+5);
   if(recentlyHit&&size>9){draw_text("1",sx,sy-std::max(9,size/3),std::max(1,size/32),255,225,180);}
   // Species 6/9 carry glowing mineral plates; 8/10 have raised horns.
   if(m.type==6||m.type==9){
    const int glow=110+int((std::sin(phase*3.0f)+1.0f)*35.0f);
    rect(sx-bodyW/2,sy+bodyH/3,bodyW, std::max(2,size/18),210,Uint8(glow),115,210);
    rect(sx-1,sy-std::max(2,size/8),2,std::max(2,size/7),120,210,205,190);
   }
   if(m.type==8||m.type==10){
    const int horn=std::max(2,size/5);
    rect(sx-bodyW/2,sy-horn, std::max(2,bodyW/4),horn,rr+25,rr+12,rr+8);
    rect(sx+bodyW/4,sy-horn, std::max(2,bodyW/4),horn,rr+25,rr+12,rr+8);
   }
   // Species silhouettes gain a readable head, ears/tail and breathing motion.
   const int headSize=std::max(2,size/(m.type==8||m.type==10?5:7));
   const int headX=sx+bodyW/3,headY=sy+bodyH/7+int(std::sin(phase*2.0f)*std::max(1,size/45));
   rect(headX-headSize/2,headY,headSize,headSize,recentlyHit?245:rr+12,recentlyHit?220:rr,recentlyHit?190:rr+7);
   if(m.type==1||m.type==3||m.type==7||m.type==9){
    rect(headX-headSize/2,headY-headSize/2,std::max(1,headSize/3),std::max(2,headSize/2),rr+18,rr+12,rr+5);
    rect(headX+headSize/5,headY-headSize/2,std::max(1,headSize/3),std::max(2,headSize/2),rr+18,rr+12,rr+5);
   }
   if(m.type==0||m.type==4||m.type==5||m.type==8||m.type==10){
    const int tailSwing=int(std::sin(phase*3.2f)*std::max(1,size/14));
    rect(sx-bodyW/2-std::max(2,size/7),sy+bodyH/2+tailSwing,std::max(2,size/7),std::max(2,size/12),rr-12,rr-10,rr-7);
   }
   const int eye=std::max(2,size/18);
   rect(headX-headSize/4,headY+headSize/3,eye,eye,235,226,190);
   rect(headX+headSize/8,headY+headSize/3,eye,eye,235,226,190);
  }
 }
 // Surface residents: role-colored pseudo-3D figures with an idle bob.
 for(std::size_t i=0;i<settlementNpcCount;++i){
  const auto& npc=settlementNpcs[i];const float dx=float(npc.x-px),dy=float(npc.y-py);
  const float forward=dx*dirX+dy*dirY,side=dx*rightX+dy*rightY;
  if(forward<0.7f||forward>18.0f)continue;
  const int sx=int(W*0.5f+(side/forward)*focal/fov),size=int(390.0f/forward);
  const float phase=animTime+float(npc.seed%628u)*0.01f;
  const int bob=int(std::sin(phase*2.1f)*std::max(1,size/30));
  const int sy=int(horizon+focal*0.62f/forward-size*0.82f)+bob;
  if(sx<8||sx>=W-8||size<4)continue;
  static const int coats[5][3]={{103,117,133},{133,78,55},{83,128,77},{105,91,135},{153,129,83}};
  const auto& coat=coats[npc.role%5u];const int bw=std::max(3,size/2),bh=std::max(4,size*2/3);
  rect(sx-bw/2,sy+bh-1,bw,std::max(1,size/18),7,9,11,120);
  rect(sx-bw/2,sy+size/4,bw,bh/2,coat[0],coat[1],coat[2]);
  rect(sx-bw/3,sy+bh/2,std::max(2,bw/4),std::max(2,size/5),55,43,39);
  rect(sx+bw/8,sy+bh/2,std::max(2,bw/4),std::max(2,size/5),55,43,39);
  rect(sx-size/5,sy,std::max(3,size*2/5),std::max(3,size/3),191,151,116);
  rect(sx-size/5,sy-size/12,std::max(3,size*2/5),std::max(2,size/9),npc.role==1u?74:coat[0],npc.role==1u?61:coat[1],npc.role==1u?52:coat[2]);
  rect(sx-size/10,sy+size/8,std::max(1,size/18),std::max(1,size/18),35,30,28);
  if(npc.role==1u)rect(sx+bw/3,sy+size/3,std::max(2,size/8),std::max(2,size/3),105,112,119);
  if(npc.role==3u)rect(sx+bw/2,sy+size/8,std::max(1,size/14),std::max(2,size/2),173,145,94);
 }
 // Primordial Guardian: oversized pseudo-3D silhouette with glowing runic eyes.
 if(primordialBoss.state!=mineplus::BossState::Defeated&&boss_visible_from_player()){const float dx=float(BOSS_X-px),dy=float(BOSS_Y-py),forward=dx*dirX+dy*dirY,side=dx*rightX+dy*rightY;if(forward>0.7f&&forward<28.0f){const int sx=int(W*0.5f+(side/forward)*focal/fov),size=int(1050.0f/forward),sy=int(horizon+focal*0.72f/forward-size*0.78f);if(sx>12&&sx<W-12&&size>5){const int pulse=int((std::sin(float(SDL_GetTicks())*0.004f)+1.0f)*18.0f);const bool struck=SDL_GetTicks()-lastBossHitAt<180u;const int body= struck?130:53;rect(sx-size/3,sy+size/2,size*2/3,size/2,25,19,34);rect(sx-size/2,sy+size/5,size,size/2,body,43,67);rect(sx-size/3,sy-size/4,size*2/3,size/2,struck?185:74,60,89);rect(sx-size/2,sy-size/5,std::max(2,size/7),size/3,92,67,106);rect(sx+size/2-size/7,sy-size/5,std::max(2,size/7),size/3,92,67,106);rect(sx-size/5,sy-size/10,std::max(2,size/9),std::max(2,size/14),210,83+pulse,116);rect(sx+size/10,sy-size/10,std::max(2,size/9),std::max(2,size/14),210,83+pulse,116);}}}
 // Event-specific weather and ambience overlays sit behind the first-person tools and HUD.
 draw_surface_event_effects();
  draw_surface_weather();
 // First-person blocky arms and tool silhouette.
 rect(W/2+115,H-158,75,180,91,65,49);rect(W/2+125,H-166,58,28,155,119,82);
 rect(W/2+164,H-190,12,95,151,159,163);rect(W/2+132,H-198,75,13,181,190,190);
 // Telegraph tint and attack-specific impact cues make boss timing readable.
 const Uint32 now=SDL_GetTicks();
 if(primordialBoss.state==mineplus::BossState::Telegraph)rect(0,64,W,H-64,135,24,35,24);
 if(lastBossAttackAt&&now-lastBossAttackAt<260u){
  if(lastBossAttackHitPlayer)rect(0,64,W,H-64,185,24,32,58);
  const int center=W/2,base=H*2/3;
  if(lastBossAttack==mineplus::BossAttack::GroundSlam){color(245,104,78,220);for(int k=0;k<4;++k)SDL_RenderDrawLine(renderer,center-110-k*14,base+k*8,center+110+k*14,base+k*8);}
  else if(lastBossAttack==mineplus::BossAttack::RuneVolley){color(128,220,235,230);for(int k=-2;k<=2;++k)SDL_RenderDrawLine(renderer,center+k*18,base-70,center+k*18,base+55);}
  else if(lastBossAttack==mineplus::BossAttack::RiftCharge){color(205,112,235,230);SDL_RenderDrawLine(renderer,center-145,base+55,center+145,base-65);SDL_RenderDrawLine(renderer,center-120,base+75,center+170,base-45);}
 }
 // Crosshair and weapon-art impact flash.
 color(225,232,222);SDL_RenderDrawLine(renderer,W/2-9,H/2,W/2+9,H/2);SDL_RenderDrawLine(renderer,W/2,H/2-9,W/2,H/2+9);
 if(SDL_GetTicks()<weaponArtFlashUntil){color(120,235,210,220);SDL_RenderDrawLine(renderer,W/2-28,H/2+22,W/2+28,H/2-22);SDL_RenderDrawLine(renderer,W/2-20,H/2+28,W/2+20,H/2-28);}
 if(activeNpcName&&SDL_GetTicks()<npcDialogueUntil){rect(250,526,780,92,7,13,17,238);outline(250,526,780,92,111,155,127);draw_text(activeNpcName,275,543,2,151,224,177);draw_text(activeNpcLine?activeNpcLine:"",275,574,2,224,226,210);}
 if(tradeOpen){rect(350,205,580,310,8,16,19,246);outline(350,205,580,310,111,155,127);draw_text("FRONTIER TRADING",390,232,3,151,224,177);draw_text("COINS",390,280,2,220,205,135);char coinsText[20];std::snprintf(coinsText,sizeof(coinsText),"%d",stock[22]);draw_text(coinsText,510,280,2,235,225,190);if(mineplus::surface_event_trade_discount(currentSurfaceEvent.type)>0)draw_text("CARAVAN -25 PERCENT",650,280,1,145,224,177);for(std::uint8_t i=0;i<3u;++i){const auto offer=mineplus::surface_trade_offer(tradeRole,i);const int y=322+int(i)*48;rect(385,y,510,39,i==selectedTradeOffer?52:24,i==selectedTradeOffer?78:38,i==selectedTradeOffer?58:43);outline(385,y,510,39,i==selectedTradeOffer?180:70,i==selectedTradeOffer?205:105,i==selectedTradeOffer?145:83);draw_text(offer.name,405,y+10,2,220,225,210);char cost[24];std::snprintf(cost,sizeof(cost),"%u COINS",unsigned(mineplus::surface_trade_price(offer,mineplus::surface_event_trade_discount(currentSurfaceEvent.type))));draw_text(cost,760,y+12,1,230,195,125);}draw_text("LEFT/RIGHT SELECT  ENTER BUY  ESC CLOSE",390,478,1,180,205,190);}
  const int aimedX=px+faceX,aimedY=py+faceY;if(worldGrid.contains(aimedX,aimedY)&&tile_at(aimedX,aimedY)==ChestClosed)draw_text("TREASURE CHEST  /  PRESS F OR X",W/2-145,H/2+25,2,245,205,130);
 rect(0,0,W,64,7,12,16,235);rect(20,13,7,38,52,212,132);draw_text("MINEPLUS  /  THE FORGOTTEN WILDS",44,15,3);draw_text("3D VOXEL SURVIVAL",45,43,1,160,190,174);
 draw_text("HP",500,20,2,210,218,208);rect(535,19,110,14,36,25,26);const int maxHealth=static_cast<int>(mineplus::EquipmentCatalog::calculate(equipmentState).max_health);rect(537,21,std::clamp(playerHealth*110/maxHealth,0,110),10,playerHealth>3?75:190,playerHealth>3?183:66,playerHealth>3?111:53);
 draw_text("MOVE  /  R-STICK LOOK  /  F OPEN/BREAK  /  E BAG",650,25,2,190,198,192);
 draw_text("G / R3 SPECIAL ATTACK",650,47,1,145,215,190);
 draw_text("T TALK TO RESIDENT",44,54,1,145,215,190);
 if(currentSurfaceEvent.type!=mineplus::SurfaceEventType::None){rect(34,78,490,45,9,23,25,220);outline(34,78,490,45,112,157,119);draw_text(currentSurfaceEvent.title,45,82,2,235,211,145);draw_text(currentSurfaceEvent.description,45,105,1,195,210,180);char eventTimer[24];const auto seconds=(mineplus::surface_event_time_remaining(surfaceEventClockMs)+999u)/1000u;std::snprintf(eventTimer,sizeof(eventTimer),"%02u:%02u",unsigned(seconds/60u),unsigned(seconds%60u));draw_text(eventTimer,480,91,2,235,211,145);}
 if(primordialBoss.state!=mineplus::BossState::Dormant&&primordialBoss.state!=mineplus::BossState::Defeated){draw_text("PRIMORDIAL GUARDIAN",490,82,2,220,180,210);rect(490,101,300,13,34,25,38);rect(492,103,std::max(0,296*int(primordialBoss.health)/int(primordialBoss.max_health)),9,165,65,112);char phaseLabel[24];std::snprintf(phaseLabel,sizeof(phaseLabel),"PHASE %u / 3",unsigned(primordialBoss.phase));draw_text(phaseLabel,800,101,1,195,175,205);if(primordialBoss.state==mineplus::BossState::Telegraph){const char* warning="MOVE OUT OF RANGE";switch(primordialBoss.pending_attack){case mineplus::BossAttack::GroundSlam:warning="GROUND SLAM - MOVE AWAY";break;case mineplus::BossAttack::RuneVolley:warning="RUNE VOLLEY - BREAK LINE";break;case mineplus::BossAttack::RiftCharge:warning="RIFT CHARGE - RETREAT";break;default:break;}draw_text(warning,490,119,1,245,145,115);}}
 if(lastSaveAt&&SDL_GetTicks()-lastSaveAt<2400u)draw_text(lastSaveSucceeded?"WORLD SAVED":"SAVE FAILED",44,132,2,lastSaveSucceeded?120:235,lastSaveSucceeded?220:100,lastSaveSucceeded?165:90);
 draw_minimap();
 int start=(W-8*62)/2;for(int i=0;i<8;i++){int x=start+i*62;rect(x,H-72,54,54,i==selected?44:23,i==selected?66:34,i==selected?49:40);outline(x,H-72,54,54,i==selected?90:55,i==selected?224:77,i==selected?142:91);Uint8 r,g,b;tile_color(i+1,r,g,b);rect(x+15,H-57,24,24,r,g,b);char qty[8];std::snprintf(qty,sizeof(qty),"%d",stock[i+1]);draw_text(qty,x+17,H-20,1,225,230,220);}
 if(inventory||paused){rect(240,130,1000,480,8,14,18,245);outline(240,130,1000,480,68,166,111);draw_text(inventory?"INVENTORY":"PAUSED",310,195,5,132,235,166);
  if(inventory){
   if(equipmentPage){
    static const char* equipmentNames[25]={"STONE AXE","BONE CAP","HIDE VEST","TRAIL LEGS","MOSS BOOTS","IRON SWORD","IRON HELM","IRON CHEST","IRON LEGS","IRON BOOTS","OBSIDIAN BLADE","OBS HELM","OBS CHEST","OBS LEGS","OBS BOOTS","RUNE SWORD","RUNE HELM","RUNE CHEST","RUNE LEGS","RUNE BOOTS","PRIM BLADE","PRIM HELM","PRIM CHEST","PRIM LEGS","PRIM BOOTS"};
    for(int i=0;i<25;++i){
     const int x=265+(i%5)*137,y=245+(i/5)*49;
     const bool owned=equipmentState.owned[static_cast<std::size_t>(i+1)]!=0;
     const bool equipped=std::find(equipmentState.equipped.begin(),equipmentState.equipped.end(),static_cast<mineplus::EquipmentId>(i+1))!=equipmentState.equipped.end();
     rect(x,y,128,41,owned?35:22,owned?58:31,owned?48:36);
     outline(x,y,128,41,i==selectedEquipment?232:70,i==selectedEquipment?206:105,i==selectedEquipment?119:83);
     draw_text(equipmentNames[i],x+5,y+7,1,owned?220:125,owned?225:135,owned?205:140);
     if(equipped)draw_text("ON",x+93,y+26,1,130,235,166);
     else if(owned)draw_text("OWNED",x+65,y+26,1,165,205,170);
    }
    const auto id=selected_equipment_id();
    const auto* def=mineplus::EquipmentCatalog::find(id);
    const auto totals=mineplus::EquipmentCatalog::calculate(equipmentState);
    draw_text("EQUIPMENT",965,218,2,132,235,166);
    draw_text(equipmentNames[selectedEquipment],965,250,1,230,220,180);
    static const char* slots[5]={"WEAPON","HEAD","CHEST","LEGS","BOOTS"};
    if(def){
     draw_text(slots[static_cast<std::size_t>(def->slot)],965,272,1,165,195,180);
     char line[48];
     std::snprintf(line,sizeof(line),"DMG %u  DEF %u",unsigned(def->stats.damage),unsigned(def->stats.defense));draw_text(line,965,296,1,215,220,205);
     std::snprintf(line,sizeof(line),"HP %u  ARC %u",unsigned(def->stats.vitality),unsigned(def->stats.arcane));draw_text(line,965,316,1,215,220,205);
     std::snprintf(line,sizeof(line),"RES %u",unsigned(def->stats.resistance));draw_text(line,965,336,1,215,220,205);
     std::snprintf(line,sizeof(line),"DUR %u/%u",unsigned(equipmentState.durability[static_cast<std::size_t>(id)]),unsigned(def->durability));draw_text(line,965,356,1,215,220,205);
     static const char* materialNames[31]={"AIR","GRASS","DIRT","STONE","WOOD","MOSS","CRYSTAL","BONE","DARKSTONE","SAND","SNOW","ICE","LAVA","WATER","IRON ORE","OBSIDIAN","EMBER","RUNE","IRON BAR","OBS CORE","EMBER CAT","ANCIENT RUNE","COINS","HERBS","BONE SHARDS","DUNGEON ORE","CRYSTAL SHARDS","RUNE FRAG","EMBER CORES","ANCIENT RELIC","BOSS SIGIL"};
     const auto* recipe=mineplus::EquipmentCatalog::recipe(id);
     if(recipe){draw_text("RECIPE",965,386,1,132,235,166);for(unsigned ri=0;ri<recipe->ingredient_count;++ri){const auto ingredient=recipe->ingredients[ri];char cost[48];std::snprintf(cost,sizeof(cost),"%s %u",materialNames[ingredient.inventory_index],unsigned(ingredient.count));draw_text(cost,965,405+int(ri)*17,1,175,190,180);}}
    }
    char totalsLine[64];std::snprintf(totalsLine,sizeof(totalsLine),"TOTAL DMG %u DEF %u",unsigned(totals.damage),unsigned(totals.defense));draw_text(totalsLine,265,510,1,170,220,185);
    std::snprintf(totalsLine,sizeof(totalsLine),"MAX HP %u ARC %u RES %u",unsigned(totals.max_health),unsigned(totals.arcane),unsigned(totals.resistance));draw_text(totalsLine,265,530,1,170,220,185);
    draw_text("LEFT/RIGHT SELECT  A CRAFT  RIGHT STICK EQUIP",265,565,1,184,205,184);
    draw_text("R / CONTROLLER X REPAIR WITH TIER MATERIALS",265,580,1,184,205,184);
    draw_text("V PAGE  /  U EQUIP  /  BACK CLOSE",650,585,1,184,205,184);
   }else{
    static const int itemIds[25]={Grass,Dirt,Stone,Wood,Moss,Crystal,Bone,DarkStone,IronOre,Obsidian,Ember,Rune,IronBar,ObsidianCore,EmberCatalyst,AncientRune,22,23,24,25,26,27,28,29,30};
    static const char* itemNames[25]={"GRASS","DIRT","STONE","WOOD","MOSS","CRYSTAL","BONE","DARKSTONE","IRON ORE","OBSIDIAN","EMBER","RUNE","IRON BARS","OBSIDIAN CORE","EMBER CATALYST","ANCIENT RUNE","COINS","HERBS","BONE SHARDS","DUNGEON ORE","CRYSTAL SHARDS","RUNE FRAGMENTS","EMBER CORES","ANCIENT RELICS","BOSS SIGILS"};
    for(int i=0;i<25;i++){int x=265+(i%4)*188,y=245+(i/4)*40,id=itemIds[i];rect(x,y,170,36,27,43,43);Uint8 r,g,b;if(id>=22)inventory_item_color(id,r,g,b);else tile_color(id,r,g,b);rect(x+7,y+4,27,27,r,g,b);outline(x,y,170,36,75,145,98);draw_text(itemNames[i],x+41,y+5,1,208,215,208);char amount[12];std::snprintf(amount,sizeof(amount),"%d",stock[id]);draw_text(amount,x+41,y+21,1,160,220,174);}
    static const char* recipes[9]={"DARKSTONE: 3 STONE + 2 WOOD","MOSS: 3 DIRT + 1 WOOD","CRYSTAL: 4 STONE + 1 BONE","BONE: 2 WOOD + 1 STONE","STONE: 3 DIRT + 1 BONE","IRON BARS: 3 ORE + 1 EMBER","OBSIDIAN CORE: 3 OBSIDIAN + 1 ORE","EMBER CATALYST: 3 EMBER + 1 CRYSTAL","ANCIENT RUNE: 2 RUNES + 2 CRYSTAL"};
    draw_text("CRAFTING",1010,218,2,132,235,166);
    for(int ri=0;ri<9;ri++)draw_text(recipes[ri],1010,245+ri*25,1,ri==selectedRecipe?245:145,ri==selectedRecipe?220:165,ri==selectedRecipe?145:160);
    draw_text("L/R SELECT  /  A CRAFT",1010,485,1,184,205,184);
    draw_text("F OPENS TREASURE CHESTS  /  BACK CLOSE",650,565,1,184,205,184);
   }
  }
  else{draw_text("A / ENTER  CONTINUE",350,310,3);draw_text("X / F5  SAVE WORLD",350,375,3);draw_text("B / ESC  BACK TO WORLD",350,440,2);}
 }
}
void handle_key(SDL_Keycode k){
 if(!playing){if(k==SDLK_RETURN||k==SDLK_SPACE)playing=true;if(k==SDLK_ESCAPE)running=false;return;}
 if(k==SDLK_F5){save_world();return;}
 if(tradeOpen){if(k==SDLK_ESCAPE||k==SDLK_b){tradeOpen=false;return;}if(k==SDLK_LEFT||k==SDLK_a)selectedTradeOffer=static_cast<std::uint8_t>((selectedTradeOffer+2u)%3u);if(k==SDLK_RIGHT||k==SDLK_d)selectedTradeOffer=static_cast<std::uint8_t>((selectedTradeOffer+1u)%3u);if(k==SDLK_RETURN)buy_selected_trade_offer();return;}
 if(k==SDLK_ESCAPE){paused=!paused;inventory=false;return;}
 if(paused){if(k==SDLK_RETURN)paused=false;return;}
 if(k==SDLK_TAB||k==SDLK_e){inventory=!inventory;return;}
 if(!inventory){if(k==SDLK_q)turn_player(-1);if(k==SDLK_r)turn_player(1);if(k==SDLK_g)use_weapon_art();if(k==SDLK_t)talk_to_nearest_npc();if(k==SDLK_b)open_surface_trade();}
 if(inventory){
  if(k==SDLK_v){equipmentPage=!equipmentPage;return;}
  if(equipmentPage){
   if(k==SDLK_RIGHT||k==SDLK_d||k==SDLK_c)change_equipment(1);
   if(k==SDLK_LEFT||k==SDLK_a)change_equipment(-1);
   if(k==SDLK_RETURN)craft_equipment();
   if(k==SDLK_u)equip_selected_equipment();
   if(k==SDLK_r)repair_selected_equipment();
   return;
  }
  if(k>=SDLK_1&&k<=SDLK_8)selected=k-SDLK_1;
  if(k==SDLK_c||k==SDLK_RIGHT){change_recipe(1);return;}
  if(k==SDLK_LEFT){change_recipe(-1);return;}
  if(k==SDLK_RETURN){craft_item();return;}
  return;
 }
 if(k==SDLK_w||k==SDLK_UP)move_player(0,-1);
 if(k==SDLK_s||k==SDLK_DOWN)move_player(0,1);
 if(k==SDLK_a||k==SDLK_LEFT)move_player(-1,0);
 if(k==SDLK_d||k==SDLK_RIGHT)move_player(1,0);
 if(k>=SDLK_1&&k<=SDLK_8)selected=k-SDLK_1;
 if(k==SDLK_SPACE)place_block();if(k==SDLK_f)mine_block();
}
}
int main(int,char**){if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER|SDL_INIT_EVENTS)!=0)return 1;window=SDL_CreateWindow("MINEPLUS",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,W,H,SDL_WINDOW_SHOWN);if(!window){SDL_Quit();return 2;}renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);if(!renderer)renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);if(!renderer){SDL_DestroyWindow(window);SDL_Quit();return 3;}SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);SDL_RenderSetLogicalSize(renderer,W,H);if(SDL_NumJoysticks()>0&&SDL_IsGameController(0))pad=SDL_GameControllerOpen(0);init_world();load_world();build_boss_arena();bossClockMs=SDL_GetTicks();lastBossClockTick=bossClockMs;lastSurfaceEventTick=bossClockMs;currentSurfaceEvent=mineplus::surface_event_at(worldSeed,surfaceEventClockMs);primordialBoss.state_started_at=bossClockMs;Uint32 lastAutosave=SDL_GetTicks();while(running){const Uint32 frameStart=SDL_GetTicks();const Uint32 wallNow=frameStart;if(lastBossClockTick==0)lastBossClockTick=wallNow;if(playing&&!paused&&!inventory)bossClockMs+=wallNow-lastBossClockTick;lastBossClockTick=wallNow;if(lastSurfaceEventTick==0)lastSurfaceEventTick=wallNow;if(playing&&!paused&&!inventory)surfaceEventClockMs+=wallNow-lastSurfaceEventTick;lastSurfaceEventTick=wallNow;currentSurfaceEvent=mineplus::surface_event_at(worldSeed,surfaceEventClockMs);SDL_Event e;while(SDL_PollEvent(&e)){if(e.type==SDL_QUIT)running=false;if(e.type==SDL_KEYDOWN&&e.key.repeat==0)handle_key(e.key.keysym.sym);if(e.type==SDL_MOUSEBUTTONDOWN&&playing&&!paused&&!inventory&&!tradeOpen){if(e.button.button==SDL_BUTTON_LEFT)mine_block();else if(e.button.button==SDL_BUTTON_RIGHT)place_block();}if(e.type==SDL_CONTROLLERBUTTONDOWN){switch(e.cbutton.button){case SDL_CONTROLLER_BUTTON_START:if(tradeOpen)tradeOpen=false;else if(!playing)playing=true;else{paused=!paused;inventory=false;}break;case SDL_CONTROLLER_BUTTON_BACK:if(playing&&!paused)inventory=!inventory;break;case SDL_CONTROLLER_BUTTON_A:if(!playing)playing=true;else if(paused)paused=false;else if(inventory){if(equipmentPage)craft_equipment();else craft_item();}else place_block();break;case SDL_CONTROLLER_BUTTON_X:if(tradeOpen)buy_selected_trade_offer();else if(playing&&!inventory&&nearest_settlement_npc()!=settlementNpcCount)open_surface_trade();else if(paused)save_world();else if(playing&&inventory&&equipmentPage)repair_selected_equipment();else if(playing&&!inventory)mine_block();break;case SDL_CONTROLLER_BUTTON_B:if(tradeOpen)tradeOpen=false;else if(inventory)inventory=false;else if(paused)paused=false;else if(playing)paused=true;else running=false;break;case SDL_CONTROLLER_BUTTON_DPAD_UP:if(playing&&!paused&&!inventory)move_player(0,-1);break;case SDL_CONTROLLER_BUTTON_DPAD_DOWN:if(playing&&!paused&&!inventory)move_player(0,1);break;case SDL_CONTROLLER_BUTTON_DPAD_LEFT:if(tradeOpen)selectedTradeOffer=static_cast<std::uint8_t>((selectedTradeOffer+2u)%3u);else if(playing&&!paused&&!inventory)move_player(-1,0);break;case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:if(tradeOpen)selectedTradeOffer=static_cast<std::uint8_t>((selectedTradeOffer+1u)%3u);else if(playing&&!paused&&!inventory)move_player(1,0);break;case SDL_CONTROLLER_BUTTON_Y:if(playing&&!paused)inventory=!inventory;break;case SDL_CONTROLLER_BUTTON_RIGHTSTICK:if(inventory){if(equipmentPage)equip_selected_equipment();else craft_item();}else if(playing&&!paused)use_weapon_art();break;case SDL_CONTROLLER_BUTTON_LEFTSTICK:if(inventory)equipmentPage=!equipmentPage;else if(playing&&!paused)talk_to_nearest_npc();break;case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:if(inventory){if(equipmentPage)change_equipment(-1);else change_recipe(-1);}else selected=(selected+7)%8;break;case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:if(inventory){if(equipmentPage)change_equipment(1);else change_recipe(1);}else selected=(selected+1)%8;break;default:break;}}}if(playing&&!paused&&!inventory&&!tradeOpen){update_creatures();update_settlement_npcs();update_primordial_boss(bossClockMs);}if(playing&&!paused&&!inventory&&!tradeOpen)update_player_combat();if(playing&&!paused&&!inventory&&!tradeOpen&&pad){Uint32 now=SDL_GetTicks();if(now-lastTurn>230){Sint16 rx=SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_RIGHTX);if(rx>18000){turn_player(1);lastTurn=now;}else if(rx< -18000){turn_player(-1);lastTurn=now;}}if(now-lastMove>155){Sint16 ax=SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_LEFTX),ay=SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_LEFTY);if(ax>18000){move_player(1,0);lastMove=now;}else if(ax< -18000){move_player(-1,0);lastMove=now;}else if(ay>18000){move_player(0,1);lastMove=now;}else if(ay< -18000){move_player(0,-1);lastMove=now;}}}if(playing&&!paused&&SDL_GetTicks()-lastAutosave>30000){save_world(false);lastAutosave=SDL_GetTicks();}if(playing)draw_world();else draw_menu();SDL_RenderPresent(renderer);const Uint32 frameElapsed=SDL_GetTicks()-frameStart;if(frameElapsed<16u)SDL_Delay(16u-frameElapsed);}save_world(false);if(pad)SDL_GameControllerClose(pad);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return 0;}
