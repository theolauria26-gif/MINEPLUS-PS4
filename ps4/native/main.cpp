#include <time.h>
#include <SDL2/SDL.h>
#include <algorithm>
#include <array>
namespace {
constexpr int W=1280,H=720,COLS=128,ROWS=72,TILE=36,VIEW_COLS=24,VIEW_ROWS=14;
enum Tile:unsigned char{Air=0,Grass=1,Dirt=2,Stone=3,Wood=4,Moss=5,Crystal=6,Bone=7,DarkStone=8};
std::array<unsigned char,COLS*ROWS> world{};
int px=COLS/2,py=ROWS/2,selected=0,faceX=1,faceY=0;
struct Creature{int x,y,type,hp;};std::array<Creature,12> creatures{};
unsigned hash2(int x,int y){unsigned n=(unsigned)(x*374761393u+y*668265263u+0x9e3779b9u);n=(n^(n>>13))*1274126177u;return n^(n>>16);}
bool running=true,playing=false,inventory=false,paused=false;
Uint32 lastMove=0;
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
 for(int y=0;y<ROWS;y++)for(int x=0;x<COLS;x++){unsigned h=hash2(x,y);unsigned char t=Grass;if(h%100<18)t=Stone;if(h%100<9)t=Dirt;if(h%100<5)t=Moss;if(h%997<4)t=Crystal;if((x%31==0||y%27==0)&&h%5==0)t=DarkStone;world[y*COLS+x]=t;}
 // Ruined keeps, broken walls, crystal shrines and burial stones.
 for(int cy=9;cy<ROWS-8;cy+=19)for(int cx=12;cx<COLS-10;cx+=27){int w=7+(hash2(cx,cy)%5),h=6+(hash2(cy,cx)%4);for(int y=cy;y<cy+h;y++)for(int x=cx;x<cx+w;x++)if(x==cx||x==cx+w-1||y==cy||y==cy+h-1)world[y*COLS+x]=DarkStone;world[(cy+h/2)*COLS+cx+w/2]=Air;world[(cy+1)*COLS+cx+1]=Crystal;for(int k=0;k<4;k++){int tx=cx+(int)(hash2(cx+k,cy)%w),ty=cy+(int)(hash2(cy+k,cx)%h);world[ty*COLS+tx]=Bone;}}
 // Dead forests and scattered trunks.
 for(int y=2;y<ROWS-2;y++)for(int x=2;x<COLS-2;x++)if(hash2(x,y)%83==0)world[y*COLS+x]=Wood;
 for(int i=0;i<(int)creatures.size();i++){auto &m=creatures[i];m.x=8+(int)(hash2(i,91)%(COLS-16));m.y=8+(int)(hash2(91,i)%(ROWS-16));m.type=(int)(hash2(m.x,m.y)%3);m.hp=3;}
 world[py*COLS+px]=Air;
}
void tile_color(int t,Uint8&r,Uint8&g,Uint8&b){switch(t){case Grass:r=43;g=76;b=61;break;case Dirt:r=83;g=62;b=52;break;case Stone:r=79;g=86;b=94;break;case Wood:r=65;g=58;b=54;break;case Moss:r=39;g=55;b=48;break;case Crystal:r=125;g=139;b=151;break;case Bone:r=137;g=133;b=119;break;case DarkStone:r=35;g=39;b=47;break;default:r=16;g=23;b=29;break;}}
void move_player(int dx,int dy){if(!dx&&!dy)return;px=std::clamp(px+dx,1,COLS-2);py=std::clamp(py+dy,1,ROWS-2);faceX=dx;faceY=dy;for(auto &m:creatures)if(hash2((int)SDL_GetTicks()+m.x,m.y)%5==0){if(abs(m.x-px)>abs(m.y-py))m.x+=m.x<px?1:-1;else m.y+=m.y<py?1:-1;}}
int target_index(){int x=px+faceX,y=py+faceY;if(x<0||x>=COLS||y<0||y>=ROWS)return -1;return y*COLS+x;}
void place_block(){int i=target_index();if(i>=0&&world[i]==Air)world[i]=static_cast<unsigned char>(selected+1);}
void mine_block(){int i=target_index();if(i>=0)world[i]=Air;}
void draw_menu(){
 // Dark-fantasy title screen: moonlit ruins, distant towers and drifting embers.
 rect(0,0,W,H,5,8,14);
 for(int y=0;y<H;y+=4){Uint8 shade=static_cast<Uint8>(8+(y*13/H));rect(0,y,W,4,shade,shade+3,shade+9);}
 rect(0,430,W,290,9,13,18);
 // Broken skyline and black-stone spires.
 polyline_dummy:
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
void draw_world(){
 rect(0,0,W,H,7,11,17);for(int y=70;y<H;y+=6)rect(0,y,W,3,static_cast<Uint8>(9+y/90),static_cast<Uint8>(14+y/95),static_cast<Uint8>(21+y/80));
 // Cold moonlight and low-contrast fog behind the block field.
 rect(920,92,74,74,104,115,119,80);for(int i=0;i<5;i++)rect(0,555+i*23,W,7,26,34,41,35);
 int ox=(W-VIEW_COLS*TILE)/2,oy=90;int camX=std::clamp(px-VIEW_COLS/2,0,COLS-VIEW_COLS),camY=std::clamp(py-VIEW_ROWS/2,0,ROWS-VIEW_ROWS);
 for(int vy=0;vy<VIEW_ROWS;vy++)for(int vx=0;vx<VIEW_COLS;vx++){int x=camX+vx,y=camY+vy;Uint8 r,g,b;tile_color(world[y*COLS+x],r,g,b);int sx=ox+vx*TILE,sy=oy+vy*TILE;rect(sx+1,sy+1,TILE-2,TILE-2,r,g,b);outline(sx,sy,TILE,TILE,20,27,31);if(world[y*COLS+x]==Grass){rect(sx+3,sy+3,TILE-6,3,83,126,91);rect(sx+5,sy+9,4,3,111,143,106);}else if(world[y*COLS+x]==Stone){rect(sx+5,sy+7,8,2,119,128,134);rect(sx+21,sy+22,6,2,47,55,63);}else if(world[y*COLS+x]==Wood){rect(sx+8,sy+3,3,TILE-6,123,98,76);}}
 int tx=px+faceX,ty=py+faceY;if(tx>=camX&&tx<camX+VIEW_COLS&&ty>=camY&&ty<camY+VIEW_ROWS)outline(ox+(tx-camX)*TILE+2,oy+(ty-camY)*TILE+2,TILE-4,TILE-4,240,226,105);
 for(const auto &m:creatures)if(m.x>=camX&&m.x<camX+VIEW_COLS&&m.y>=camY&&m.y<camY+VIEW_ROWS){int mx=ox+(m.x-camX)*TILE,my=oy+(m.y-camY)*TILE;rect(mx+7,my+8,22,24,m.type==0?82:m.type==1?105:65,m.type==0?78:m.type==1?91:72,m.type==0?86:m.type==1?101:81);rect(mx+10,my+13,5,5,220,220,215);rect(mx+22,my+13,5,5,220,220,215);}
 int sxp=ox+(px-camX)*TILE,syp=oy+(py-camY)*TILE;rect(sxp+9,syp+5,18,26,224,224,231);rect(sxp+11,syp+8,5,5,30,35,43);rect(sxp+20,syp+8,5,5,30,35,43);
 rect(0,0,W,70,7,12,16,245);rect(20,16,8,38,52,212,132);draw_text("MINEPLUS",45,17,3);draw_text("THE FORGOTTEN WILDS  /  128 x 72",45,45,1,160,166,165);
 draw_text("DPAD MOVE  A PLACE  X BREAK",650,27,2,190,194,190);
 int start=(W-5*62)/2;for(int i=0;i<5;i++){int x=start+i*62;rect(x,H-88,54,54,i==selected?44:23,i==selected?66:34,i==selected?49:40);outline(x,H-88,54,54,i==selected?90:55,i==selected?224:77,i==selected?142:91);Uint8 r,g,b;tile_color(i+1,r,g,b);rect(x+15,H-73,24,24,r,g,b);}
 if(inventory||paused){rect(250,150,780,430,8,14,18,245);outline(250,150,780,430,68,166,111);draw_text(inventory?"INVENTORY":"PAUSED",310,195,5,132,235,166);
 if(inventory){for(int i=0;i<5;i++){int x=320+i*125;rect(x,300,90,90,27,43,43);Uint8 r,g,b;tile_color(i+1,r,g,b);rect(x+23,323,44,44,r,g,b);outline(x,300,90,90,75,145,98);}}
 else{draw_text("START CONTINUE",350,320,3);draw_text("B BACK TO MENU",350,385,3);}
 }
}
void handle_key(SDL_Keycode k){if(!playing){if(k==SDLK_RETURN||k==SDLK_SPACE)playing=true;if(k==SDLK_ESCAPE)running=false;return;}if(k==SDLK_ESCAPE){paused=!paused;inventory=false;return;}if(paused){if(k==SDLK_RETURN)paused=false;return;}if(k==SDLK_TAB||k==SDLK_e)inventory=!inventory;if(inventory){if(k>=SDLK_1&&k<=SDLK_5)selected=k-SDLK_1;return;}if(k==SDLK_w||k==SDLK_UP)move_player(0,-1);if(k==SDLK_s||k==SDLK_DOWN)move_player(0,1);if(k==SDLK_a||k==SDLK_LEFT)move_player(-1,0);if(k==SDLK_d||k==SDLK_RIGHT)move_player(1,0);if(k>=SDLK_1&&k<=SDLK_5)selected=k-SDLK_1;if(k==SDLK_SPACE)place_block();if(k==SDLK_f)mine_block();}
}
int main(int,char**){if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER|SDL_INIT_EVENTS)!=0)return 1;window=SDL_CreateWindow("MINEPLUS",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,W,H,SDL_WINDOW_SHOWN);if(!window){SDL_Quit();return 2;}renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);if(!renderer)renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);if(!renderer){SDL_DestroyWindow(window);SDL_Quit();return 3;}SDL_RenderSetLogicalSize(renderer,W,H);if(SDL_NumJoysticks()>0&&SDL_IsGameController(0))pad=SDL_GameControllerOpen(0);init_world();while(running){SDL_Event e;while(SDL_PollEvent(&e)){if(e.type==SDL_QUIT)running=false;if(e.type==SDL_KEYDOWN&&e.key.repeat==0)handle_key(e.key.keysym.sym);if(e.type==SDL_CONTROLLERBUTTONDOWN){switch(e.cbutton.button){case SDL_CONTROLLER_BUTTON_START:if(!playing)playing=true;else paused=!paused;break;case SDL_CONTROLLER_BUTTON_BACK:inventory=!inventory;break;case SDL_CONTROLLER_BUTTON_A:if(!playing)playing=true;else if(!paused&&!inventory)place_block();break;case SDL_CONTROLLER_BUTTON_X:if(playing&&!paused&&!inventory)mine_block();break;case SDL_CONTROLLER_BUTTON_B:if(paused)paused=false;else if(playing)paused=true;else running=false;break;case SDL_CONTROLLER_BUTTON_DPAD_UP:if(playing&&!paused&&!inventory)move_player(0,-1);break;case SDL_CONTROLLER_BUTTON_DPAD_DOWN:if(playing&&!paused&&!inventory)move_player(0,1);break;case SDL_CONTROLLER_BUTTON_DPAD_LEFT:if(playing&&!paused&&!inventory)move_player(-1,0);break;case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:if(playing&&!paused&&!inventory)move_player(1,0);break;case SDL_CONTROLLER_BUTTON_Y:inventory=!inventory;break;case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:selected=(selected+4)%5;break;case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:selected=(selected+1)%5;break;default:break;}}}if(playing&&!paused&&!inventory&&pad){Uint32 now=SDL_GetTicks();if(now-lastMove>155){Sint16 ax=SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_LEFTX),ay=SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_LEFTY);if(ax>18000){move_player(1,0);lastMove=now;}else if(ax< -18000){move_player(-1,0);lastMove=now;}else if(ay>18000){move_player(0,1);lastMove=now;}else if(ay< -18000){move_player(0,-1);lastMove=now;}}}if(playing)draw_world();else draw_menu();SDL_RenderPresent(renderer);SDL_Delay(16);}if(pad)SDL_GameControllerClose(pad);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return 0;}
