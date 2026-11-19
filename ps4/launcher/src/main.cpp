#include <time.h>
#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace {
constexpr int W=1920,H=1080;
struct Game { std::string title; SDL_Color color; std::string titleId; };
struct Particle { float x,y,vx,vy,r,a; };
std::vector<Game> games;
std::string libraryStatus="SCANNING LIBRARY";
std::vector<Particle> particles;
int selected=0,hubSelected=0; bool hub=false,running=true;
float focus=0.0f, bgMix=1.0f;
SDL_Window* window=nullptr; SDL_Renderer* renderer=nullptr;
void fill(int x,int y,int w,int h,SDL_Color c){SDL_SetRenderDrawColor(renderer,c.r,c.g,c.b,c.a);SDL_Rect r{x,y,w,h};SDL_RenderFillRect(renderer,&r);}
void line(int x1,int y1,int x2,int y2,SDL_Color c){SDL_SetRenderDrawColor(renderer,c.r,c.g,c.b,c.a);SDL_RenderDrawLine(renderer,x1,y1,x2,y2);}
void textBlock(const char* s,int x,int y,int scale,SDL_Color c){
 // Embedded 5x7 alphabet keeps typography crisp without external font dependencies.
 static const char* glyphs[36]={
 "01110100011000111111100011000110001","11110100011000111110100011000111110",
 "01111100001000010000100001000001111","11110100011000110001100011000111110",
 "11111100001000011110100001000011111","11111100001000011110100001000010000",
 "01111100001000010111100011000101111","10001100011000111111100011000110001",
 "11111001000010000100001000010011111","00111000100001000010100101001001100",
 "10001100101010011000101001001010001","10000100001000010000100001000011111",
 "10001110111010110101100011000110001","10001110011010110011100011000110001",
 "01110100011000110001100011000101110","11110100011000111110100001000010000",
 "01110100011000110001101011001001101","11110100011000111110101001001010001",
 "01111100001000001110000010000111110","11111001000010000100001000010000100",
 "10001100011000110001100011000101110","10001100011000110001010100101000100",
 "10001100011000110101101011010101010","10001100010101000100010101000110001",
 "10001100010101000100001000010000100","11111000010001000100010001000011111",
 "01110100011001110101110011000101110","00100011000010000100001000010001110",
 "01110100011000100010001000100011111","11110000010000101110000010000111110",
 "00010001100101010010111110001000010","11111100001000011110000010000111110",
 "01110100001000011110100011000101110","11111000010001000100010000100001000",
 "01110100011000101110100011000101110","01110100011000101111000010000101110"
 };
 for(;*s;++s,x+=6*scale){
  char ch=*s;int index=-1;
  if(ch>='A'&&ch<='Z')index=ch-'A';
  else if(ch>='0'&&ch<='9')index=26+ch-'0';
  if(index<0)continue;
  const char* p=glyphs[index];
  for(int row=0;row<7;++row)for(int col=0;col<5;++col)
   if(p[row*5+col]=='1')fill(x+col*scale,y+row*scale,scale,scale,c);
 }
}
void initParticles(){particles.clear();for(int i=0;i<110;++i){float f=float(i);particles.push_back({float((i*173)%W),float((i*97)%H),float(std::sin(f*1.7f)*7.0),-8.0f-float(i%9),1.0f+float(i%3),0.15f+float(i%5)*0.06f});}}
uint32_t le32(const std::vector<uint8_t>& b,size_t p){return uint32_t(b[p])|(uint32_t(b[p+1])<<8)|(uint32_t(b[p+2])<<16)|(uint32_t(b[p+3])<<24);}
uint16_t le16(const std::vector<uint8_t>& b,size_t p){return uint16_t(b[p])|(uint16_t(b[p+1])<<8);}
std::string sfoString(const std::vector<uint8_t>& b,const char* wanted){
 if(b.size()<20||std::memcmp(b.data(),"\0PSF",4)!=0)return {};
 uint32_t keys=le32(b,8),data=le32(b,12),count=le32(b,16);
 if(count>512||keys>=b.size()||data>=b.size())return {};
 for(uint32_t i=0;i<count;++i){size_t e=20+size_t(i)*16;if(e+16>b.size())break;
  uint16_t ko=le16(b,e);uint32_t len=le32(b,e+4),off=le32(b,e+12);
  size_t kp=size_t(keys)+ko,dp=size_t(data)+off;
  if(kp>=b.size()||dp>b.size()||len>b.size()-dp)continue;
  const char* key=reinterpret_cast<const char*>(b.data()+kp);
  const void* keyEnd=std::memchr(key,0,b.size()-kp);if(!keyEnd)continue;
  if(std::string(key,static_cast<const char*>(keyEnd)-key)==wanted){size_t n=0;while(n<len&&b[dp+n])++n;return std::string(reinterpret_cast<const char*>(b.data()+dp),n);}
 }
 return {};
}
void scanInstalledGames(){
 std::vector<Game> found;
 DIR* dir=opendir("/user/app");
 if(!dir){libraryStatus="LIBRARY ACCESS DENIED";games.clear();selected=0;return;}
 {
  dirent* ent;
  while((ent=readdir(dir))!=nullptr&&found.size()<40){
   if(ent->d_name[0]=='.')continue;
   std::string root="/user/app/"+std::string(ent->d_name);
   struct stat st{};if(stat(root.c_str(),&st)!=0||!S_ISDIR(st.st_mode))continue;
   std::string path=root+"/sce_sys/param.sfo";
   FILE* file=std::fopen(path.c_str(),"rb");if(!file)continue;
   if(std::fseek(file,0,SEEK_END)!=0){std::fclose(file);continue;}
   long fileSize=std::ftell(file);
   if(fileSize<20||fileSize>1024*1024||std::fseek(file,0,SEEK_SET)!=0){std::fclose(file);continue;}
   std::vector<uint8_t> bytes(static_cast<size_t>(fileSize));
   size_t readBytes=std::fread(bytes.data(),1,bytes.size(),file);std::fclose(file);
   if(readBytes!=bytes.size())continue;
   std::string title=sfoString(bytes,"TITLE");
   std::string id=sfoString(bytes,"TITLE_ID");
   if(title.empty()||id.empty())continue;
   found.push_back({title,{72,94,112,255},id});
  }
  closedir(dir);
 }
 games.swap(found);selected=0;
 if(!games.empty())libraryStatus="INSTALLED GAMES FOUND";
 else libraryStatus="NO READABLE GAMES FOUND";
}

void update(float dt){focus+=(1.0f-focus)*std::min(1.0f,dt*8.0f);bgMix+=(1.0f-bgMix)*std::min(1.0f,dt*2.5f);for(auto& p:particles){p.x+=p.vx*dt;p.y+=p.vy*dt;if(p.y<0){p.y=H;p.x=float((int(p.x)+W/3)%W);}if(p.x<0)p.x+=W;if(p.x>=W)p.x-=W;}}
void draw(){
 fill(0,0,W,H,{8,11,18,255});
 // Layered atmospheric gradients and subtle horizon glow.
 for(int y=0;y<H;y+=8){float t=float(y)/H;fill(0,y,W,8,{Uint8(9+10*t),Uint8(14+13*t),Uint8(24+20*t),255});}
 for(int i=0;i<12;++i){int yy=590+i*18;line(0,yy,W,yy,{Uint8(25+i),Uint8(34+i),Uint8(51+i),90});}
 for(const auto& p:particles){SDL_SetRenderDrawColor(renderer,145,177,205,Uint8(p.a*180));SDL_Rect q{int(p.x),int(p.y),int(p.r),int(p.r)};SDL_RenderFillRect(renderer,&q);}
 textBlock("LIBRARY",110,55,2,{146,168,190,255});
 textBlock("MINEPLUS",W-360,48,3,{220,229,239,255});
 // Horizontal game row, selected card grows and lifts.
 const int start=110,baseY=190,cardW=236,cardH=150,gap=24;
 for(int i=0;i<(int)games.size();++i){float active=(i==selected)?focus:0.0f;int w=cardW+int(active*28),h=cardH+int(active*18);int x=start+i*(cardW+gap)-int(active*14),y=baseY-int(active*26);auto c=games[i].color;
  fill(x-5,y-5,w+10,h+10,{Uint8(i==selected?220:65),Uint8(i==selected?226:76),Uint8(i==selected?237:91),255});fill(x,y,w,h,c);
  fill(x+14,y+14,w-28,4,{Uint8(255),Uint8(255),Uint8(255),Uint8(75)});
  fill(x+18,y+h-27,w-36,5,{Uint8(15),Uint8(18),Uint8(25),Uint8(160)});
  textBlock(games[i].title.c_str(),x+16,y+h-55,2,{235,239,245,255});
 }
 // Hero area reports real scan results; never presents fake games as installed titles.
 fill(112,510,5,130,{104,210,190,255});
 if(!games.empty()){
  const Game& g=games[selected];
  textBlock(g.title.c_str(),140,530,5,{226,234,242,255});
  textBlock("INSTALLED GAME",140,590,2,{117,184,180,255});
  textBlock(g.titleId.c_str(),140,630,2,{166,184,202,255});
 }else{
  textBlock("NO GAMES FOUND",140,530,4,{226,234,242,255});
  textBlock("LIBRARY SCAN RESULT",140,590,2,{117,184,180,255});
 }
 textBlock(libraryStatus.c_str(),110,790,2,{166,184,202,255});
 if(hub){
  fill(0,850,W,230,{10,15,24,245});
  textBlock("CONTROL CENTER",110,872,2,{204,219,232,255});
  fill(0,850,W,2,{102,145,164,210});
  const char* labels[4]={"GAMES","MEDIA","SETTINGS","PROFILE"};
  const char* hints[4]={"YOUR LIBRARY","MUSIC AND VIDEO","SYSTEM OPTIONS","PLAYER ACCOUNT"};
  for(int i=0;i<4;++i){int x=110+i*420;bool active=(i==hubSelected);
   fill(x,910,380,112,active?SDL_Color{222,232,239,255}:SDL_Color{28,39,54,255});
   if(active)fill(x+10,920,360,92,{18,26,37,255});
   SDL_Color tc=active?SDL_Color{238,244,249,255}:SDL_Color{174,190,207,255};
   textBlock(labels[i],x+28,940,2,tc);
   textBlock(hints[i],x+28,976,1,tc);
  }
 }
 SDL_RenderPresent(renderer);
}
void move(int d){if(games.empty())return;selected=(selected+d+(int)games.size())%(int)games.size();focus=0.0f;bgMix=0.0f;}
}
int main(int,char**){
 if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER|SDL_INIT_AUDIO)!=0)return 1;
 window=SDL_CreateWindow("PS5-style Homebrew Launcher",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,W,H,SDL_WINDOW_SHOWN);
 if(!window){SDL_Quit();return 2;}
 renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
 if(!renderer)renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
 if(!renderer){SDL_DestroyWindow(window);SDL_Quit();return 3;}
 SDL_GameController* pad=nullptr;if(SDL_NumJoysticks()>0&&SDL_IsGameController(0))pad=SDL_GameControllerOpen(0);
 initParticles();scanInstalledGames();Uint64 prev=SDL_GetPerformanceCounter();
 while(running){Uint64 now=SDL_GetPerformanceCounter();float dt=float(double(now-prev)/double(SDL_GetPerformanceFrequency()));prev=now;dt=std::min(dt,0.05f);SDL_Event e;
  while(SDL_PollEvent(&e)){if(e.type==SDL_QUIT)running=false;if(e.type==SDL_CONTROLLERBUTTONDOWN){switch(e.cbutton.button){case SDL_CONTROLLER_BUTTON_DPAD_LEFT:if(hub)hubSelected=(hubSelected+3)%4;else move(-1);break;case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:if(hub)hubSelected=(hubSelected+1)%4;else move(1);break;case SDL_CONTROLLER_BUTTON_DPAD_DOWN:hub=true;break;case SDL_CONTROLLER_BUTTON_DPAD_UP:hub=false;break;case SDL_CONTROLLER_BUTTON_B:hub=false;break;case SDL_CONTROLLER_BUTTON_A:/* Launch backend intentionally not wired in prototype. */break;default:break;}}}
  update(dt);draw();
 }
 if(pad)SDL_GameControllerClose(pad);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return 0;
}
