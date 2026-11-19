#include <time.h>
#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace {
constexpr int W=1920,H=1080;
struct Game { const char* title; SDL_Color color; };
struct Particle { float x,y,vx,vy,r,a; };
std::vector<Game> games={
 {"NIGHTFALL",{104,75,91,255}}, {"KINGDOMS",{75,103,112,255}},
 {"ABYSS",{71,83,119,255}}, {"ASHEN REALMS",{132,88,62,255}},
 {"LAST LIGHT",{81,116,91,255}}
};
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
void update(float dt){focus+=(1.0f-focus)*std::min(1.0f,dt*8.0f);bgMix+=(1.0f-bgMix)*std::min(1.0f,dt*2.5f);for(auto& p:particles){p.x+=p.vx*dt;p.y+=p.vy*dt;if(p.y<0){p.y=H;p.x=float((int(p.x)+W/3)%W);}if(p.x<0)p.x+=W;if(p.x>=W)p.x-=W;}}
void draw(){
 fill(0,0,W,H,{8,11,18,255});
 // Layered atmospheric gradients and subtle horizon glow.
 for(int y=0;y<H;y+=8){float t=float(y)/H;fill(0,y,W,8,{Uint8(9+10*t),Uint8(14+13*t),Uint8(24+20*t),255});}
 for(int i=0;i<12;++i){int yy=590+i*18;line(0,yy,W,yy,{Uint8(25+i),Uint8(34+i),Uint8(51+i),90});}
 for(const auto& p:particles){SDL_SetRenderDrawColor(renderer,145,177,205,Uint8(p.a*180));SDL_Rect q{int(p.x),int(p.y),int(p.r),int(p.r)};SDL_RenderFillRect(renderer,&q);}
 textBlock("LIBRARY",110,55,2,{146,168,190,255});\n textBlock("MINEPLUS",W-360,48,3,{220,229,239,255});
 // Horizontal game row, selected card grows and lifts.
 const int start=110,baseY=190,cardW=236,cardH=150,gap=24;
 for(int i=0;i<(int)games.size();++i){float active=(i==selected)?focus:0.0f;int w=cardW+int(active*28),h=cardH+int(active*18);int x=start+i*(cardW+gap)-int(active*14),y=baseY-int(active*26);auto c=games[i].color;
  fill(x-5,y-5,w+10,h+10,{Uint8(i==selected?220:65),Uint8(i==selected?226:76),Uint8(i==selected?237:91),255});fill(x,y,w,h,c);
  fill(x+14,y+14,w-28,4,{Uint8(255),Uint8(255),Uint8(255),Uint8(75)});
  fill(x+18,y+h-27,w-36,5,{Uint8(15),Uint8(18),Uint8(25),Uint8(160)});
  textBlock(games[i].title,x+16,y+h-55,2,{235,239,245,255});
 }
 const Game& g=games[selected];
 // Hero title and metadata zone.
 fill(112,510,5,130,{104,210,190,255});
 // Title shown as abstract title bars until a full font atlas is integrated.
 for(int i=0;i<8;++i)fill(140+i*26,530,18,7,{Uint8(210-i*5),Uint8(222-i*4),Uint8(231-i*3),255});
 fill(140,558,360,3,{90,117,137,190});fill(140,574,260,3,{70,94,113,160});
 textBlock("2026",140,610,3,{166,184,202,255});
 if(hub){fill(0,850,W,230,{10,15,24,245});
  textBlock("CONTROL CENTER",110,872,2,{204,219,232,255});fill(0,850,W,2,{102,145,164,210});fill(110,900,360,104,{222,232,239,255});fill(122,912,336,80,{18,26,37,255});fill(150,948,220,8,{232,239,245,255});fill(520,900,250,104,{28,39,54,255});fill(550,948,180,8,{175,191,207,255});fill(810,900,250,104,{28,39,54,255});fill(840,948,180,8,{175,191,207,255});}
 SDL_RenderPresent(renderer);
}
void move(int d){selected=(selected+d+(int)games.size())%(int)games.size();focus=0.0f;bgMix=0.0f;}
}
int main(int,char**){
 if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER|SDL_INIT_AUDIO)!=0)return 1;
 window=SDL_CreateWindow("PS5-style Homebrew Launcher",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,W,H,SDL_WINDOW_SHOWN);
 if(!window){SDL_Quit();return 2;}
 renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
 if(!renderer)renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
 if(!renderer){SDL_DestroyWindow(window);SDL_Quit();return 3;}
 SDL_GameController* pad=nullptr;if(SDL_NumJoysticks()>0&&SDL_IsGameController(0))pad=SDL_GameControllerOpen(0);
 initParticles();Uint64 prev=SDL_GetPerformanceCounter();
 while(running){Uint64 now=SDL_GetPerformanceCounter();float dt=float(double(now-prev)/double(SDL_GetPerformanceFrequency()));prev=now;dt=std::min(dt,0.05f);SDL_Event e;
  while(SDL_PollEvent(&e)){if(e.type==SDL_QUIT)running=false;if(e.type==SDL_CONTROLLERBUTTONDOWN){switch(e.cbutton.button){case SDL_CONTROLLER_BUTTON_DPAD_LEFT:if(hub)hubSelected=(hubSelected+3)%4;else move(-1);break;case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:if(hub)hubSelected=(hubSelected+1)%4;else move(1);break;case SDL_CONTROLLER_BUTTON_DPAD_DOWN:hub=true;break;case SDL_CONTROLLER_BUTTON_DPAD_UP:hub=false;break;case SDL_CONTROLLER_BUTTON_B:hub=false;break;case SDL_CONTROLLER_BUTTON_A:/* Launch backend intentionally not wired in prototype. */break;default:break;}}}
  update(dt);draw();
 }
 if(pad)SDL_GameControllerClose(pad);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return 0;
}
