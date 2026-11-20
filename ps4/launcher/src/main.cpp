#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <orbis/libkernel.h>
#include <GoldHEN.h>
#include <orbis/SystemService.h>
#include <orbis/UserService.h>
#include <orbis/Sysmodule.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cerrno>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <vector>

// Guard the PS4/OpenOrbis LNC parameter ABI against accidental layout changes.
static_assert(sizeof(LncAppParam) == 32, "Unexpected LncAppParam size");
static_assert(offsetof(LncAppParam, crash_report) == 16, "Unexpected LncAppParam crash_report offset");
static_assert(offsetof(LncAppParam, LaunchAppCheck_flag) == 24, "Unexpected LncAppParam flag offset");

namespace {
constexpr int W=1920,H=1080;
struct Game { std::string title; SDL_Color color; std::string titleId; std::string iconPath; SDL_Texture* icon=nullptr; bool iconAttempted=false; SDL_Texture* background=nullptr; bool backgroundAttempted=false; bool favorite=false; };
struct Particle { float x,y,vx,vy,r,a; };
// Button indices follow the OpenOrbis PS4 SDL2 joystick sample.
enum Ps4PadButton : Uint8 {
 PadCross=0,PadCircle=1,PadSquare=2,PadTriangle=3,
 PadL1=4,PadR1=5,PadOptions=9,PadL3=11,PadR3=12,
 PadUp=13,PadDown=14,PadLeft=15,PadRight=16,PadTouchpad=17
};
std::vector<Game> games;
std::vector<std::string> favoriteIds;
SDL_Texture* previousHeroBackground=nullptr;
std::string libraryStatus="SCANNING LIBRARY";
std::vector<Particle> particles;
int selected=0,hubSelected=0,settingsCursor=0; bool favoritesOnly=false; bool hub=false,settingsPage=false,detailsPage=false,running=true; bool particlesEnabled=true,showTitleId=true,reduceMotion=false;
std::string notice; float noticeTimer=0.0f;
bool launchServicesReady=false; int32_t launchUserId=-1; std::string launchServiceStatus="LAUNCH SERVICE NOT INITIALIZED";
float focus=0.0f, bgMix=1.0f;
SDL_Window* window=nullptr; SDL_Renderer* renderer=nullptr;
void fill(int x,int y,int w,int h,SDL_Color c){SDL_SetRenderDrawColor(renderer,c.r,c.g,c.b,c.a);SDL_Rect r{x,y,w,h};SDL_RenderFillRect(renderer,&r);}
void line(int x1,int y1,int x2,int y2,SDL_Color c){SDL_SetRenderDrawColor(renderer,c.r,c.g,c.b,c.a);SDL_RenderDrawLine(renderer,x1,y1,x2,y2);}
void drawAmbientGlow(){
 // Clean blue-black console-style ambience: no landscape or fantasy scenery.
 for(int i=0;i<18;++i){
  int y=250+i*12;
  Uint8 a=Uint8(3+(17-i)/2);
  fill(0,y,W,8,{28,82,156,a});
 }
 for(int i=0;i<7;++i){
  int y=610+i*10;
  fill(0,y,W,5,{20,48,86,Uint8(5+i)});
 }
}
void textBlock(const char* s,int x,int y,int scale,SDL_Color c){
 static const char* glyphs[36]={
 "01110100011000111111100011000110001","11110100011000111110100011000111110","01111100001000010000100001000001111","11110100011000110001100011000111110","11111100001000011110100001000011111","11111100001000011110100001000010000","01111100001000010111100011000101111","10001100011000111111100011000110001","11111001000010000100001000010011111","00111000100001000010100101001001100","10001100101010011000101001001010001","10000100001000010000100001000011111","10001110111010110101100011000110001","10001110011010110011100011000110001","01110100011000110001100011000101110","11110100011000111110100001000010000","01110100011000110001101011001001101","11110100011000111110101001001010001","01111100001000001110000010000111110","11111001000010000100001000010000100","10001100011000110001100011000101110","10001100011000110001010100101000100","10001100011000110101101011010101010","10001100010101000100010101000110001","10001100010101000100001000010000100","11111000010001000100010001000011111","01110100011001110101110011000101110","00100011000010000100001000010001110","01110100011000100010001000100011111","11110000010000101110000010000111110","00010001100101010010111110001000010","11111100001000011110000010000111110","01110100001000011110100011000101110","11111000010001000100010000100001000","01110100011000101110100011000101110","01110100011000101111000010000101110"
 };
 std::vector<SDL_Rect> pixels;pixels.reserve(std::strlen(s)*18);
 for(;*s;++s,x+=6*scale){
  char ch=*s;int index=-1;if(ch>='a'&&ch<='z')ch=char(ch-'a'+'A');
  if(ch>='A'&&ch<='Z')index=ch-'A';else if(ch>='0'&&ch<='9')index=26+ch-'0';
  if(index<0){if(ch=='-')pixels.push_back({x,y+3*scale,4*scale,scale});else if(ch=='.')pixels.push_back({x,y+6*scale,scale,scale});else if(ch==':'){pixels.push_back({x,y+2*scale,scale,scale});pixels.push_back({x,y+6*scale,scale,scale});}else if(ch=='/')for(int k=0;k<6;++k)pixels.push_back({x+(5-k)*scale,y+k*scale,scale,scale});else if(ch=='_')pixels.push_back({x,y+6*scale,5*scale,scale});continue;}
  const char* p=glyphs[index];for(int row=0;row<7;++row)for(int col=0;col<5;++col)if(p[row*5+col]=='1')pixels.push_back({x+col*scale,y+row*scale,scale,scale});
 }
 if(!pixels.empty()){SDL_SetRenderDrawColor(renderer,c.r,c.g,c.b,c.a);SDL_RenderFillRects(renderer,pixels.data(),static_cast<int>(pixels.size()));}
}
void textBlockFit(const std::string& value,int x,int y,int scale,int maxWidth,SDL_Color c){
 size_t maxChars=maxWidth>0?size_t(maxWidth/(6*scale)):value.size();
 std::string clipped=value;
 if(clipped.size()>maxChars){if(maxChars>3)clipped=clipped.substr(0,maxChars-3)+"...";else clipped=clipped.substr(0,maxChars);}
 textBlock(clipped.c_str(),x,y,scale,c);
}
void initParticles(){particles.clear();for(int i=0;i<110;++i){float f=float(i);particles.push_back({float((i*173)%W),float((i*97)%H),float(std::sin(f*1.7f)*7.0),-8.0f-float(i%9),1.0f+float(i%3),0.15f+float(i%5)*0.06f});}}
uint32_t le32(const std::vector<uint8_t>& b,size_t p){return uint32_t(b[p])|(uint32_t(b[p+1])<<8)|(uint32_t(b[p+2])<<16)|(uint32_t(b[p+3])<<24);}
uint16_t le16(const std::vector<uint8_t>& b,size_t p){return uint16_t(b[p])|(uint16_t(b[p+1])<<8);}
bool validSfoStructure(const std::vector<uint8_t>& b){
 if(b.size()<20||std::memcmp(b.data(),"\0PSF",4)!=0)return false;
 const uint32_t keys=le32(b,8),data=le32(b,12),count=le32(b,16);
 if(keys<20||data<keys||data>b.size()||count>512||count>(keys-20)/16)return false;
 for(uint32_t i=0;i<count;++i){
  const size_t entry=20+size_t(i)*16;
  const uint16_t keyOffset=le16(b,entry);
  const uint32_t length=le32(b,entry+4),maxLength=le32(b,entry+8),dataOffset=le32(b,entry+12);
  const size_t keyPosition=size_t(keys)+keyOffset,valuePosition=size_t(data)+dataOffset;
  if(keyPosition>=data||valuePosition>b.size()||length>maxLength||maxLength>b.size()-valuePosition)return false;
  if(!std::memchr(b.data()+keyPosition,0,size_t(data)-keyPosition))return false;
 }
 return true;
}
std::string sfoString(const std::vector<uint8_t>& b,const char* wanted){
 if(b.size()<20||std::memcmp(b.data(),"\0PSF",4)!=0)return {};
 uint32_t keys=le32(b,8),data=le32(b,12),count=le32(b,16);
 if(keys<20||data<keys||data>b.size()||count>512||count>(keys-20)/16)return {};
 for(uint32_t i=0;i<count;++i){size_t e=20+size_t(i)*16;if(e+16>b.size())break;
  uint16_t ko=le16(b,e);uint32_t len=le32(b,e+4),off=le32(b,e+12);
  size_t kp=size_t(keys)+ko,dp=size_t(data)+off;
  if(kp>=data||dp>b.size()||len>b.size()-dp)continue;
  const char* key=reinterpret_cast<const char*>(b.data()+kp);
  const void* keyEnd=std::memchr(key,0,data-kp);if(!keyEnd)continue;
  if(std::string(key,static_cast<const char*>(keyEnd)-key)==wanted){size_t n=0;while(n<len&&b[dp+n])++n;return std::string(reinterpret_cast<const char*>(b.data()+dp),n);}
 }
 return {};
}
std::string sanitizeTitle(const std::string& raw){
 std::string clean;clean.reserve(std::min<size_t>(raw.size(),120));bool lastSpace=true;
 for(unsigned char ch:raw){
  if(ch>=32&&ch<=126){if(ch==' '){if(!lastSpace&&clean.size()<120){clean.push_back(' ');lastSpace=true;}}else if(clean.size()<120){clean.push_back(char(ch));lastSpace=false;}}
  else if(ch>=128){if(!lastSpace&&clean.size()<120){clean.push_back(' ');lastSpace=true;}}
  if(clean.size()>=120)break;
 }
 while(!clean.empty()&&clean.back()==' ')clean.pop_back();
 return clean;
}
bool validTitleId(const std::string& id){
 if(id.size()!=9)return false;
 for(size_t i=0;i<4;++i)if(id[i]<'A'||id[i]>'Z')return false;
 for(size_t i=4;i<9;++i)if(id[i]<'0'||id[i]>'9')return false;
 return true;
}
bool isFavoriteId(const std::string& id){return std::find(favoriteIds.begin(),favoriteIds.end(),id)!=favoriteIds.end();}
void loadFavorites(){
 favoriteIds.clear();
 FILE* file=std::fopen("/data/launcher_favorites.cfg","rb");
 if(!file)return;
 std::vector<std::string> loaded;
 bool readOk=true;
 char line[16];
 while(std::fgets(line,sizeof(line),file)){
  bool complete=std::strchr(line,'\n')!=nullptr||std::feof(file);
  if(!complete){
   int next=std::fgetc(file);
   if(next!='\n'&&next!=EOF){
    while(next!='\n'&&next!=EOF)next=std::fgetc(file);
    if(std::ferror(file)){readOk=false;break;}
    continue;
   }
   if(next==EOF&&std::ferror(file)){readOk=false;break;}
  }
  std::string id(line);
  while(!id.empty()&&(id.back()=='\n'||id.back()=='\r'))id.pop_back();
  if(validTitleId(id)&&std::find(loaded.begin(),loaded.end(),id)==loaded.end())loaded.push_back(id);
  if(std::ferror(file)){readOk=false;break;}
 }
 if(std::ferror(file))readOk=false;
 if(std::fclose(file)!=0)readOk=false;
 if(readOk)favoriteIds.swap(loaded);
}
bool loadLastSelectedGame(){
 FILE* file=std::fopen("/data/launcher_last_game.cfg","rb");if(!file)return false;
 std::string wanted;bool valid=true;int ch;
 while((ch=std::fgetc(file))!=EOF){
  if(wanted.size()>=11){valid=false;break;}
  wanted.push_back(char(ch));
 }
 if(std::ferror(file))valid=false;
 std::fclose(file);
 if(!valid)return false;
 if(!wanted.empty()&&wanted.back()=='\n')wanted.pop_back();
 if(!wanted.empty()&&wanted.back()=='\r')wanted.pop_back();
 if(!validTitleId(wanted))return false;
 auto it=std::find_if(games.begin(),games.end(),[&](const Game& game){return game.titleId==wanted;});
 if(it==games.end())return false;
 selected=int(it-games.begin());return true;
}
bool saveLastSelectedGame(){
 if(selected<0||size_t(selected)>=games.size()||!validTitleId(games[selected].titleId))return false;
 const char* tempPath="/data/launcher_last_game.cfg.tmp";
 FILE* file=std::fopen(tempPath,"wb");if(!file)return false;
 bool ok=std::fprintf(file,"%s\n",games[selected].titleId.c_str())>0;
 if(std::fflush(file)!=0)ok=false;if(std::fclose(file)!=0)ok=false;
 if(!ok){std::remove(tempPath);return false;}
 if(std::rename(tempPath,"/data/launcher_last_game.cfg")!=0){std::remove(tempPath);return false;}
 return true;
}
bool saveFavorites(){
 const char* tempPath="/data/launcher_favorites.cfg.tmp";
 for(auto it=favoriteIds.begin();it!=favoriteIds.end();++it){
  if(!validTitleId(*it))return false;
  if(std::find(favoriteIds.begin(),it,*it)!=it)return false;
 }
 FILE* file=std::fopen(tempPath,"wb");if(!file)return false;bool ok=true;
 for(const auto& id:favoriteIds){
  if(std::fprintf(file,"%s\n",id.c_str())<0){ok=false;break;}
 }
 if(std::fflush(file)!=0)ok=false;if(std::fclose(file)!=0)ok=false;
 if(!ok){std::remove(tempPath);return false;}
 if(std::rename(tempPath,"/data/launcher_favorites.cfg")!=0){std::remove(tempPath);return false;}
 return true;
}
void sortGames(std::vector<Game>& list){
 std::sort(list.begin(),list.end(),[](const Game& a,const Game& b){
  if(a.favorite!=b.favorite)return a.favorite>b.favorite;
  const auto lessChar=[](unsigned char x,unsigned char y){return std::tolower(x)<std::tolower(y);};
  if(std::lexicographical_compare(a.title.begin(),a.title.end(),b.title.begin(),b.title.end(),lessChar))return true;
  if(std::lexicographical_compare(b.title.begin(),b.title.end(),a.title.begin(),a.title.end(),lessChar))return false;
  return a.titleId<b.titleId;
 });
}
size_t activeLibrarySize(){if(!favoritesOnly)return games.size();return size_t(std::count_if(games.begin(),games.end(),[](const Game& game){return game.favorite;}));}
void toggleSelectedFavorite(){
 const size_t available=activeLibrarySize();
 if(available==0||selected<0||size_t(selected)>=available)return;
 const std::string id=games[selected].titleId;
 const std::vector<std::string> previousFavoriteIds=favoriteIds;
 auto it=std::find(favoriteIds.begin(),favoriteIds.end(),id);
 const bool removing=it!=favoriteIds.end();
 if(removing)favoriteIds.erase(it);
 else favoriteIds.push_back(id);
 if(!saveFavorites()){
  favoriteIds=previousFavoriteIds;
  notice="FAVORITES SAVE FAILED";
  noticeTimer=3.0f;
  return;
 }
 for(auto& game:games)game.favorite=isFavoriteId(game.titleId);sortGames(games);
 auto selectedIt=std::find_if(games.begin(),games.end(),[&](const Game& game){return game.titleId==id;});if(selectedIt!=games.end())selected=int(selectedIt-games.begin());if(favoritesOnly&&removing){detailsPage=false;if(size_t(selected)>=activeLibrarySize())selected=0;}
 notice=removing?"REMOVED FROM FAVORITES":"ADDED TO FAVORITES";noticeTimer=2.0f;
}
bool scanInstalledGames(){
 // Installed-title metadata and artwork are stored in separate PS4 trees.
 // Itemzflow's installed-library implementation uses private appmeta for SFOs
 // and public appmeta for icons; /user/app is retained as a compatibility fallback.
 jailbreak_backup scanBackup{};
 const int scanJailbreakResult=sys_sdk_jailbreak(&scanBackup);
 if(scanJailbreakResult!=0){
  libraryStatus="GOLDHEN ACCESS FAILED - LIBRARY NOT SCANNED";
  notice="GOLDHEN ACCESS FAILED - CHECK PLUGIN";
  noticeTimer=4.0f;
  return false;
 }
 struct ScanPrivilegeGuard { jailbreak_backup* backup; ~ScanPrivilegeGuard(){if(backup)sys_sdk_unjailbreak(backup);} } scanPrivilegeGuard{&scanBackup};
 libraryStatus="SCANNING LIBRARY";
 const std::string previouslySelectedId=(selected>=0&&size_t(selected)<games.size())?games[selected].titleId:std::string();
 std::vector<Game> found;
 bool scanReadError=false;
 size_t scannedEntries=0,appDirectories=0,sfoFiles=0,readableSfos=0,structurallyValidSfos=0,validGameRecords=0;
 auto scanRoot=[&](const std::string& root,bool legacyLayout,const std::string& iconRoot){
  DIR* dir=opendir(root.c_str());
  if(!dir)return;
  dirent* ent;
  while(true){
   errno=0;ent=readdir(dir);
   if(!ent){if(errno!=0)scanReadError=true;break;}
   if(ent->d_name[0]=='.')continue;
   ++scannedEntries;
   const std::string id=ent->d_name;
   if(!validTitleId(id)||id=="LCHR00001")continue;
   const std::string folder=root+"/"+id;
   struct stat st{};if(stat(folder.c_str(),&st)!=0||!S_ISDIR(st.st_mode))continue;
   ++appDirectories;
   const std::string sfoPath=legacyLayout?folder+"/sce_sys/param.sfo":folder+"/param.sfo";
   ++sfoFiles;
   FILE* file=std::fopen(sfoPath.c_str(),"rb");if(!file)continue;
   if(std::fseek(file,0,SEEK_END)!=0){std::fclose(file);continue;}
   long fileSize=std::ftell(file);
   if(fileSize<20||fileSize>1024*1024||std::fseek(file,0,SEEK_SET)!=0){std::fclose(file);continue;}
   std::vector<uint8_t> bytes(static_cast<size_t>(fileSize));
   size_t readBytes=std::fread(bytes.data(),1,bytes.size(),file);
   bool closeOk=std::fclose(file)==0;
   if(readBytes!=bytes.size()||!closeOk)continue;
   ++readableSfos;
   if(!validSfoStructure(bytes))continue;
   ++structurallyValidSfos;
   std::string title=sanitizeTitle(sfoString(bytes,"TITLE"));
   std::string sfoId=sfoString(bytes,"TITLE_ID");
   if(title.empty()||!validTitleId(sfoId)||sfoId!=id)continue;
   ++validGameRecords;
   if(std::any_of(found.begin(),found.end(),[&](const Game& game){return game.titleId==id;}))continue;
   std::string iconPath=iconRoot.empty()?std::string():iconRoot+"/"+id+"/icon0.png";
   struct stat iconStat{};
   if(iconPath.empty()||stat(iconPath.c_str(),&iconStat)!=0||!S_ISREG(iconStat.st_mode)){
    if(legacyLayout)iconPath=folder+"/sce_sys/icon0.png";
    else iconPath="/user/app/"+id+"/sce_sys/icon0.png";
   }
   found.push_back({title,{72,94,112,255},id,iconPath,nullptr,false});
  }
  closedir(dir);
 };
 // Internal and USB metadata roots; public artwork roots are intentionally separate.
 scanRoot("/system_data/priv/appmeta",false,"/user/appmeta");
 scanRoot("/system_data/priv/appmeta/external",false,"/user/appmeta/external");
 // Compatibility scan for homebrew/older layouts with metadata beside the app.
 scanRoot("/user/app",true,"");
 if(scanReadError){libraryStatus="LIBRARY SCAN ERROR - PREVIOUS LIST KEPT";notice="SCAN INTERRUPTED - LIBRARY PRESERVED";noticeTimer=3.0f;return false;}
 if(found.empty()&&!games.empty()&&appDirectories>0&&validGameRecords==0){libraryStatus="METADATA ACCESS FAILED - PREVIOUS LIST KEPT";notice="APP METADATA UNREADABLE - LIBRARY PRESERVED";noticeTimer=4.0f;return false;}
 for(auto& old:games){if(old.icon)SDL_DestroyTexture(old.icon);if(old.background)SDL_DestroyTexture(old.background);old.icon=nullptr;old.background=nullptr;}
 if(previousHeroBackground){SDL_DestroyTexture(previousHeroBackground);previousHeroBackground=nullptr;}bgMix=1.0f;
 for(auto& game:found)game.favorite=isFavoriteId(game.titleId);
 sortGames(found);games.swap(found);selected=0;
 if(!previouslySelectedId.empty()){auto keep=std::find_if(games.begin(),games.end(),[&](const Game& game){return game.titleId==previouslySelectedId;});if(keep!=games.end())selected=int(keep-games.begin());}
 if(favoritesOnly&&size_t(selected)>=activeLibrarySize())selected=0;
 if(!games.empty()){
  char diagnostic[128];
  std::snprintf(diagnostic,sizeof(diagnostic),"GAMES %zu / META %zu / SFO %zu / VALID %zu",games.size(),appDirectories,sfoFiles,validGameRecords);
  libraryStatus=diagnostic;
 }else{
  char diagnostic[128];
  std::snprintf(diagnostic,sizeof(diagnostic),"NO GAMES - META %zu SFO %zu READ %zu VALID %zu",appDirectories,sfoFiles,readableSfos,structurallyValidSfos);
  libraryStatus=diagnostic;
 }
 return true;
}

bool loadSettings(){
 FILE* file=std::fopen("/data/launcher_settings.cfg","rb");
 if(!file)return false;
 char magic[16]{},valuesLine[128]{};
 bool valid=std::fgets(magic,sizeof(magic),file)!=nullptr&&std::strcmp(magic,"MPLS1\n")==0;
 int values[3]={1,1,0},parsed=0;
 if(valid){
  if(!std::fgets(valuesLine,sizeof(valuesLine),file))valid=false;
  else{
   if(!std::strchr(valuesLine,'\n')){
    int extra=std::fgetc(file);
    if(extra!=EOF)valid=false;
    if(std::ferror(file))valid=false;
   }
   char* cursor=valuesLine;
   while(valid&&parsed<3){
    while(*cursor&&std::isspace(static_cast<unsigned char>(*cursor)))++cursor;
    if(!*cursor)break;
    errno=0;char* end=nullptr;long value=std::strtol(cursor,&end,10);
    if(end==cursor||errno==ERANGE||value<0||value>1){valid=false;break;}
    values[parsed++]=static_cast<int>(value);cursor=end;
   }
   while(*cursor&&std::isspace(static_cast<unsigned char>(*cursor)))++cursor;
   if(*cursor||parsed<2)valid=false;
  }
 }
 if(valid){
  int ch;
  while((ch=std::fgetc(file))!=EOF)if(!std::isspace(static_cast<unsigned char>(ch))){valid=false;break;}
 }
 if(std::ferror(file))valid=false;
 std::fclose(file);
 if(!valid)return false;
 particlesEnabled=values[0]!=0;showTitleId=values[1]!=0;reduceMotion=values[2]!=0;return true;
}
bool saveSettings(){
 const char* tempPath="/data/launcher_settings.cfg.tmp";
 FILE* file=std::fopen(tempPath,"wb");
 if(!file)return false;
 int written=std::fprintf(file,"MPLS1\n%d %d %d\n",particlesEnabled?1:0,showTitleId?1:0,reduceMotion?1:0);
 bool ok=written>0&&std::fflush(file)==0;
 if(std::fclose(file)!=0)ok=false;
 if(!ok){std::remove(tempPath);return false;}
 if(std::rename(tempPath,"/data/launcher_settings.cfg")!=0){std::remove(tempPath);return false;}
 return true;
}
void initializeLaunchServices(){
 int32_t systemResult=sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SYSTEM_SERVICE);
 if(systemResult<0){launchServiceStatus="SYSTEM SERVICE LOAD FAILED";return;}
 int32_t userModuleResult=sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_USER_SERVICE);
 if(userModuleResult<0){launchServiceStatus="USER SERVICE LOAD FAILED";return;}
 OrbisUserServiceInitializeParams params{};params.priority=256;
 int32_t userInit=sceUserServiceInitialize(&params);
 int32_t userResult=sceUserServiceGetForegroundUser(&launchUserId);
 if(userResult<0||launchUserId<0){launchServiceStatus="FOREGROUND USER UNAVAILABLE";return;}
 launchServicesReady=true;
 launchServiceStatus=userInit<0?"USER SERVICE WARNING - GOLDHEN LNC REQUIRED":"GOLDHEN LNC READY";
}
void launchSelectedGame(){
 const size_t available=activeLibrarySize();
 if(available==0||selected<0||size_t(selected)>=available){notice="NO GAME SELECTED";noticeTimer=3.0f;return;}
 if(!launchServicesReady){notice=launchServiceStatus;noticeTimer=4.0f;return;}
 const bool selectionSaved=saveLastSelectedGame();

 // GoldHEN privilege escalation must happen before SceLncService IPC.
 jailbreak_backup jailbreakBackup{};
 const int jailbreakResult=sys_sdk_jailbreak(&jailbreakBackup);
 if(jailbreakResult!=0){
  char message[64];std::snprintf(message,sizeof(message),"GOLDHEN JAILBREAK FAILED: %d",jailbreakResult);
  notice=message;noticeTimer=5.0f;return;
 }
 // Restore retail credentials after every launch attempt, including accepted requests.
 struct LaunchPrivilegeGuard {
  jailbreak_backup* backup;
  ~LaunchPrivilegeGuard(){if(backup)sys_sdk_unjailbreak(backup);}
 } launchPrivilegeGuard{&jailbreakBackup};

 const int systemModuleResult=sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SYSTEM_SERVICE);
 const int systemServiceHandle=sceKernelLoadStartModule("/system/common/lib/libSceSystemService.sprx",0,nullptr,0,nullptr,nullptr);
 using LncInitFn=int (*)(void);
 using LncLaunchFn=int (*)(const char*,const char**,void*);
 LncInitFn lncInitialize=nullptr;
 LncLaunchFn lncLaunch=nullptr;
 LncLaunchFn systemLaunch=nullptr;

 if(systemServiceHandle>=0){
  sceKernelDlsym(systemServiceHandle,"sceLncUtilInitialize",reinterpret_cast<void**>(&lncInitialize));
  sceKernelDlsym(systemServiceHandle,"sceLncUtilLaunchApp",reinterpret_cast<void**>(&lncLaunch));
  sceKernelDlsym(systemServiceHandle,"sceSystemServiceLaunchApp",reinterpret_cast<void**>(&systemLaunch));
 }

 // Some firmware builds do not export the two LNC helpers. If the public
 // SystemService launch export is present, inspect its nearby call target
 // rather than relying on a fixed absolute address.
 if(systemLaunch&&(!lncLaunch||!lncInitialize)){
  const uint8_t* code=reinterpret_cast<const uint8_t*>(reinterpret_cast<uintptr_t>(systemLaunch));
  for(int offset=0x28;offset<=0x58;++offset){
   if(code[offset]!=0xE8)continue;
   int32_t relative=0;std::memcpy(&relative,code+offset+1,sizeof(relative));
   if(relative<=0x800||relative>=0x3000)continue;
   const uint8_t* candidate=code+offset+5+relative;
   if(candidate[0]!=0x48||candidate[1]!=0x89||candidate[2]!=0xD1)continue;
   if(!lncLaunch)lncLaunch=reinterpret_cast<LncLaunchFn>(const_cast<uint8_t*>(candidate));
   const uint8_t* initCandidate=candidate-0x20;
   if(!lncInitialize&&initCandidate[0]==0x48&&initCandidate[1]==0x8D&&initCandidate[2]==0x3D)
    lncInitialize=reinterpret_cast<LncInitFn>(const_cast<uint8_t*>(initCandidate));
   break;
  }
 }

 const int initializeResult=lncInitialize?lncInitialize():-1;
 struct __attribute__((packed)) LncParam24 {
  uint32_t size;
  int32_t user_id;
  uint32_t app_opt;
  uint64_t crash_report;
  uint32_t check_flag;
 };
 static_assert(sizeof(LncParam24)==24, "Unexpected compact LNC parameter size");
 static_assert(offsetof(LncParam24, crash_report)==12, "Unexpected compact LNC crash_report offset");
 LncParam24 compactParam{};
 compactParam.size=sizeof(compactParam);
 compactParam.user_id=launchUserId;
 LncAppParam openOrbisParam{};
 openOrbisParam.size=sizeof(openOrbisParam);
 openOrbisParam.user_id=static_cast<uint32_t>(launchUserId);
 openOrbisParam.app_opt=0;
 openOrbisParam.crash_report=0;
 openOrbisParam.LaunchAppCheck_flag=LaunchApp_None;

 const char* titleId=games[selected].titleId.c_str();
 int result=-1;
 if(lncLaunch){
  result=lncLaunch(titleId,nullptr,&compactParam);
  if(result<0)result=lncLaunch(titleId,nullptr,&openOrbisParam);
 }
 if(result<0){
  LncLaunchFn launchViaSystem=systemLaunch?systemLaunch:reinterpret_cast<LncLaunchFn>(&sceSystemServiceLaunchApp);
  result=launchViaSystem(titleId,nullptr,nullptr);
  if(result<0)result=launchViaSystem(titleId,nullptr,&compactParam);
 }

 if(result>=0){
  notice=selectionSaved?"LAUNCH REQUEST ACCEPTED":"LAUNCH ACCEPTED - LAST GAME NOT SAVED";
  noticeTimer=4.0f;
 }else{
  char message[112];
  std::snprintf(message,sizeof(message),"LNC ERROR 0x%08X (init %d / sys %d / mod %d)",static_cast<uint32_t>(result),initializeResult,systemModuleResult,systemServiceHandle);
  notice=message;noticeTimer=6.0f;
 }
}
void update(float dt){if(noticeTimer>0.0f){noticeTimer-=dt;if(noticeTimer<=0.0f)notice.clear();}if(reduceMotion){focus=1.0f;bgMix=1.0f;}else{focus+=(1.0f-focus)*std::min(1.0f,dt*8.0f);bgMix+=(1.0f-bgMix)*std::min(1.0f,dt*2.5f);}if(previousHeroBackground&&bgMix>0.995f){SDL_DestroyTexture(previousHeroBackground);previousHeroBackground=nullptr;}for(auto& p:particles){p.x+=p.vx*dt;p.y+=p.vy*dt;if(p.y<0){p.y=H;p.x=float((int(p.x)+W/3)%W);}if(p.x<0)p.x+=W;if(p.x>=W)p.x-=W;}}
void draw(){
 fill(0,0,W,H,{12,17,27,255});
 const size_t activeCount=activeLibrarySize();
 if(activeCount>0){
  Game& hero=games[selected];
  if(!hero.backgroundAttempted){
   hero.backgroundAttempted=true;
   // Full-size preview artwork lives with the installed app; appmeta may only
   // contain icon0.png. Try internal, both USB mounts, then appmeta fallbacks.
   const std::string id=hero.titleId;
   const std::string candidates[]={
    "/user/app/"+id+"/sce_sys/pic0.png",
    "/user/app/"+id+"/sce_sys/pic1.png",
    "/mnt/ext0/user/app/"+id+"/sce_sys/pic0.png",
    "/mnt/ext0/user/app/"+id+"/sce_sys/pic1.png",
    "/mnt/ext1/user/app/"+id+"/sce_sys/pic0.png",
    "/mnt/ext1/user/app/"+id+"/sce_sys/pic1.png",
    "/user/appmeta/"+id+"/pic0.png",
    "/user/appmeta/external/"+id+"/pic0.png"
   };
   for(const auto& artPath:candidates){hero.background=IMG_LoadTexture(renderer,artPath.c_str());if(hero.background)break;}
  }
  auto drawBackdrop=[&](SDL_Texture* texture,Uint8 alpha){
   if(!texture||alpha==0)return;int iw=0,ih=0;SDL_QueryTexture(texture,nullptr,nullptr,&iw,&ih);if(iw<=0||ih<=0)return;
   SDL_Rect src{0,0,iw,ih};float target=float(W)/float(H),source=float(iw)/float(ih);
   if(source>target){src.w=int(float(ih)*target);src.x=(iw-src.w)/2;}else{src.h=int(float(iw)/target);src.y=(ih-src.h)/2;}
   SDL_Rect dst{0,0,W,H};SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);
   if(SDL_SetTextureAlphaMod(texture,alpha)==0){SDL_RenderCopy(renderer,texture,&src,&dst);SDL_SetTextureAlphaMod(texture,255);}else if(alpha>=128)SDL_RenderCopy(renderer,texture,&src,&dst);
  };
  if(previousHeroBackground)drawBackdrop(previousHeroBackground,Uint8(255.0f*(1.0f-bgMix)));
  drawBackdrop(hero.background,Uint8(255.0f*bgMix));
 }
 fill(0,0,W,H,{3,6,12,155});
 if(particlesEnabled)for(const auto& p:particles){SDL_SetRenderDrawColor(renderer,205,218,232,Uint8(p.a*95));SDL_Rect q{int(p.x),int(p.y),int(p.r),int(p.r)};SDL_RenderFillRect(renderer,&q);}
 // PS5-inspired top navigation: compact labels, restrained contrast and generous spacing.
 textBlock("GAMES",86,48,3,{242,245,249,255});
 textBlock("MEDIA",260,52,2,{139,151,169,255});
 fill(86,91,92,3,{103,220,190,255});
 textBlock("SEARCH",1320,54,1,{178,190,205,255});
 textBlock("SETTINGS",1470,54,1,{178,190,205,255});
 textBlock("PROFILE",1650,54,1,{178,190,205,255});
 // Five-slot horizontal game shelf; the focused tile grows like a console home screen.
 const int baseY=132,cardW=112,cardH=112,gap=15;
 const int visible=(int)std::min<size_t>(11,activeCount);
 const int start=(W-(visible*cardW+(visible-1)*gap))/2;
 for(int slot=0;slot<visible;++slot){
  int idx=(selected-visible/2+slot+(int)activeCount)%(int)activeCount;
  Game& game=games[idx];bool chosen=(idx==selected);
  float active=chosen?focus:0.0f;
  int w=cardW+int(active*42),h=cardH+int(active*42);
  int x=start+slot*(cardW+gap)-int(active*21),y=baseY-int(active*21);
  unsigned hash=2166136261u;for(unsigned char ch:game.titleId){hash^=ch;hash*=16777619u;}
  SDL_Color accent{Uint8(72+(hash&63)),Uint8(82+((hash>>7)&63)),Uint8(105+((hash>>14)&63)),255};
  SDL_Color shade{Uint8(accent.r/3),Uint8(accent.g/3),Uint8(accent.b/3),255};
  fill(x-5,y-5,w+10,h+10,chosen?SDL_Color{220,229,239,255}:SDL_Color{48,61,78,255});
  fill(x,y,w,h,shade);
  if(!game.iconAttempted){game.iconAttempted=true;game.icon=IMG_LoadTexture(renderer,game.iconPath.c_str());}
  if(game.icon){
   int iw=0,ih=0;SDL_QueryTexture(game.icon,nullptr,nullptr,&iw,&ih);
   if(iw>0&&ih>0){float target=float(w)/float(h),source=float(iw)/float(ih);SDL_Rect src{0,0,iw,ih};
    if(source>target){src.w=int(float(ih)*target);src.x=(iw-src.w)/2;}
    else{src.h=int(float(iw)/target);src.y=(ih-src.h)/2;}
    SDL_Rect dst{x,y,w,h};SDL_RenderCopy(renderer,game.icon,&src,&dst);
   }
   fill(x,y,w,h,{5,9,15,95});
  }else{
   fill(x,y,w,7,accent);
   // Deterministic fallback artwork when a game's official icon is inaccessible.
   fill(x+18,y+25,w-36,2,{accent.r,accent.g,accent.b,105});
   fill(x+26,y+43,w-52,72,{Uint8(accent.r/2),Uint8(accent.g/2),Uint8(accent.b/2),255});
   fill(x+34,y+51,w-68,3,{accent.r,accent.g,accent.b,180});
   fill(x+34,y+60,w-68,2,{accent.r,accent.g,accent.b,95});
   fill(x+34,y+101,w-68,2,{accent.r,accent.g,accent.b,130});
  }
  if(chosen)textBlockFit(game.title,x,y+h+12,2,w,{245,248,252,255});
  if(game.favorite)textBlock("FAV",x+w-48,y+10,1,{117,224,190,255});
  if(chosen)fill(x,y+h+7,w,3,{104,210,190,235});
 }
 if(activeCount>0){
  char counter[24];std::snprintf(counter,sizeof(counter),"%02d / %02d",selected+1,(int)activeCount);
  textBlock(counter,W-250,320,2,{158,178,199,255});
 }
 int favoriteCount=0;
 for(const auto& game:games)if(game.favorite)++favoriteCount;
 char favoriteSummary[32];
 std::snprintf(favoriteSummary,sizeof(favoriteSummary),"FAVORITES: %d",favoriteCount);
 textBlock(favoriteSummary,112,320,1,{117,184,180,255});
 // Full-screen game artwork is the dashboard background; no boxed hero panel.
 if(activeCount==0){
  fill(90,390,5,180,{104,210,190,255});
  textBlock(favoritesOnly?"NO FAVORITES YET":"NO GAMES FOUND",130,450,4,{242,245,249,255});
  textBlock(favoritesOnly?"PRESS L3 FOR ALL GAMES":"PRESS TRIANGLE TO RESCAN",130,510,2,{177,194,211,255});
 }
 // Three translucent dashboard widgets, positioned along the bottom edge.
 const int panelY=875,panelH=145,panelW=520;
 for(int i=0;i<3;++i){int px=86+i*584;fill(px,panelY,panelW,panelH,{13,18,29,232});fill(px,panelY,panelW,2,{76,91,111,220});}
 textBlock("TROPHIES",112,900,2,{235,240,247,255});
 textBlock("CONSOLE STORAGE",696,900,2,{235,240,247,255});
 textBlock("CONTROLLER",1280,900,2,{235,240,247,255});
 char trophySummary[48];std::snprintf(trophySummary,sizeof(trophySummary),"GAME LIBRARY: %zu",games.size());
 textBlock(trophySummary,112,950,2,{170,190,211,255});
 textBlockFit(libraryStatus,112,990,1,470,{117,210,190,255});
 textBlock("STORAGE DETAILS UNAVAILABLE",696,950,1,{170,190,211,255});
 textBlock("CHECK PS4 SYSTEM STORAGE",696,980,1,{170,190,211,255});
 textBlock("DUALSHOCK 4 / WIRELESS",1280,950,1,{235,240,247,255});
 textBlock("CROSS LAUNCH  SQUARE FAVORITE",1280,985,1,{170,190,211,255});
 textBlock("D-PAD BROWSE  TRIANGLE RESCAN",1280,1015,1,{170,190,211,255});
 textBlock(launchServiceStatus.c_str(),86,990,1,{117,184,180,255});
 if(!notice.empty()&&!hub){
  fill(105,830,760,64,{20,34,48,235});
  fill(105,830,4,64,{104,210,190,255});
  textBlock(notice.c_str(),128,850,2,{226,235,243,255});
 }
 if(hub){
  fill(0,820,W,260,{10,15,24,248});
  fill(0,820,W,2,{102,145,164,210});
  if(settingsPage){
   textBlock("SETTINGS",110,850,3,{226,235,243,255});
   textBlock("CIRCLE BACK - UP / DOWN SELECT - CROSS CHANGE",110,900,1,{166,184,202,255});
   const char* names[3]={"AMBIENT PARTICLES","SHOW GAME TITLE ID","REDUCE MOTION"};
   const char* values[3]={particlesEnabled?"ON":"OFF",showTitleId?"ON":"OFF",reduceMotion?"ON":"OFF"};
   for(int i=0;i<3;++i){int y=930+i*48;bool active=i==settingsCursor;
    fill(110,y-12,760,48,active?SDL_Color{45,65,82,255}:SDL_Color{24,35,49,255});
    textBlock(names[i],136,y,2,{226,235,243,255});
    textBlock(values[i],760,y,2,active?SDL_Color{117,224,190,255}:SDL_Color{166,184,202,255});
   }
   if(!notice.empty()){
    fill(1050,842,760,48,{20,34,48,245});
    fill(1050,842,4,48,{104,210,190,255});
    textBlockFit(notice,1070,859,1,720,{226,235,243,255});
   }
  }else{
   textBlock("CONTROL CENTER",110,842,2,{204,219,232,255});
   const char* labels[4]={"GAMES","MEDIA","SETTINGS","PROFILE"};
   const char* hints[4]={"YOUR LIBRARY","MUSIC AND VIDEO","SYSTEM OPTIONS","PLAYER ACCOUNT"};
   for(int i=0;i<4;++i){int x=110+i*420;bool active=(i==hubSelected);
    fill(x,890,380,112,active?SDL_Color{222,232,239,255}:SDL_Color{28,39,54,255});
    if(active)fill(x+10,900,360,92,{18,26,37,255});
    SDL_Color tc=active?SDL_Color{238,244,249,255}:SDL_Color{174,190,207,255};
    textBlock(labels[i],x+28,920,2,tc);
    textBlock(hints[i],x+28,956,1,tc);
   }
  }
 }
 if(detailsPage&&activeCount>0){
  Game& detail=games[selected];
  fill(0,0,W,H,{3,6,11,238});
  fill(170,145,1580,790,{19,29,43,255});
  fill(170,145,1580,4,{104,210,190,255});
  textBlock("GAME DETAILS",220,190,3,{226,235,243,255});
  textBlockFit(detail.title,220,270,5,1480,{245,248,251,255});
  fill(220,355,1480,2,{65,83,103,220});
  if(detail.background){
   int iw=0,ih=0;SDL_QueryTexture(detail.background,nullptr,nullptr,&iw,&ih);
   if(iw>0&&ih>0){SDL_Rect src{0,0,iw,ih};float target=680.0f/300.0f,source=float(iw)/float(ih);if(source>target){src.w=int(float(ih)*target);src.x=(iw-src.w)/2;}else{src.h=int(float(iw)/target);src.y=(ih-src.h)/2;}SDL_Rect dst{220,405,680,300};SDL_RenderCopy(renderer,detail.background,&src,&dst);}
  }else{
   fill(220,405,680,300,{27,42,60,255});
   fill(220,405,5,300,{104,210,190,255});
   textBlock("ARTWORK NOT AVAILABLE",260,535,2,{166,184,202,255});
  }
  textBlock("TITLE ID",960,420,2,{117,184,180,255});
  textBlock(detail.titleId.c_str(),960,465,3,{235,241,247,255});
  textBlock("STATUS",960,535,2,{117,184,180,255});
  textBlock(detail.favorite?"FAVORITE":"INSTALLED",960,580,3,{235,241,247,255});
  textBlock("INSTALL DIRECTORY",960,650,2,{117,184,180,255});
  std::string installPath=detail.iconPath;
  const std::string suffix="/sce_sys/icon0.png";
  if(installPath.size()>=suffix.size()&&installPath.compare(installPath.size()-suffix.size(),suffix.size(),suffix)==0)installPath.resize(installPath.size()-suffix.size());
  textBlockFit(installPath,960,695,1,480,{190,204,218,255});
  textBlock("CROSS: LAUNCH    SQUARE: FAVORITE    CIRCLE: CLOSE",220,850,2,{226,235,243,255});
  textBlock("R3: CLOSE",220,900,1,{145,165,187,255});
 }
 SDL_RenderPresent(renderer);
}
void move(int d){
 const size_t activeCount=activeLibrarySize();
 if(activeCount==0)return;
 const int count=static_cast<int>(activeCount);
 if(selected<0||selected>=count)selected=0;
 if(previousHeroBackground){SDL_DestroyTexture(previousHeroBackground);previousHeroBackground=nullptr;}
 previousHeroBackground=games[selected].background;
 games[selected].background=nullptr;
 games[selected].backgroundAttempted=false;
 int next=(selected+d)%count;
 if(next<0)next+=count;
 selected=next;
 // Release icons outside the visible shelf only when the user navigates.
 const size_t visibleCount=std::min<size_t>(11,activeCount);
 for(size_t i=0;i<games.size();++i){
  bool keep=false;
  for(size_t slot=0;slot<visibleCount;++slot){
   int idx=(selected-int(visibleCount/2)+int(slot)+count)%count;
   if(size_t(idx)==i){keep=true;break;}
  }
  if(!keep&&games[i].icon){SDL_DestroyTexture(games[i].icon);games[i].icon=nullptr;games[i].iconAttempted=false;}
 }
 focus=0.0f;
 bgMix=0.0f;
}
}
int main(int,char**){
 if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK)!=0)return 1;
 window=SDL_CreateWindow("PS5-style Homebrew Launcher",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,W,H,SDL_WINDOW_SHOWN);
 if(!window){SDL_Quit();return 2;}
 renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
 if(!renderer)renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
 if(!renderer){SDL_DestroyWindow(window);SDL_Quit();return 3;}
 SDL_JoystickEventState(SDL_ENABLE);SDL_Joystick* pad=SDL_NumJoysticks()>0?SDL_JoystickOpen(0):nullptr;
 float axisRepeat=0.0f,verticalAxisRepeat=0.0f;
 const int imageInit=IMG_Init(IMG_INIT_PNG);
 if((imageInit&IMG_INIT_PNG)==0){notice="PNG SUPPORT UNAVAILABLE - USING FALLBACK ART";noticeTimer=5.0f;}
 SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
 initParticles();loadSettings();loadFavorites();scanInstalledGames();loadLastSelectedGame();initializeLaunchServices();Uint64 prev=SDL_GetPerformanceCounter();
 while(running){Uint64 now=SDL_GetPerformanceCounter();float dt=float(double(now-prev)/double(SDL_GetPerformanceFrequency()));prev=now;dt=std::min(dt,0.05f);SDL_Event e;
  while(SDL_PollEvent(&e)){if(e.type==SDL_QUIT)running=false;
  if(e.type==SDL_JOYDEVICEADDED&&!pad&&SDL_NumJoysticks()>0)pad=SDL_JoystickOpen(e.jdevice.which);
  if(e.type==SDL_JOYDEVICEREMOVED&&pad&&SDL_JoystickInstanceID(pad)==e.jdevice.which){SDL_JoystickClose(pad);pad=nullptr;if(SDL_NumJoysticks()>0)pad=SDL_JoystickOpen(0);}
  if(e.type==SDL_JOYBUTTONDOWN&&pad&&e.jbutton.which==SDL_JoystickInstanceID(pad)){switch(e.jbutton.button){
  case PadLeft:if(detailsPage)break;else if(hub&&!settingsPage)hubSelected=(hubSelected+3)%4;else if(!hub)move(-1);break;
  case PadRight:if(detailsPage)break;else if(hub&&!settingsPage)hubSelected=(hubSelected+1)%4;else if(!hub)move(1);break;
  case PadDown:if(detailsPage)break;else if(settingsPage)settingsCursor=(settingsCursor+1)%3;else hub=true;break;
  case PadUp:if(detailsPage)break;else if(settingsPage)settingsCursor=(settingsCursor+2)%3;else hub=false;break;
  case PadL1:if(!detailsPage&&!hub&&activeLibrarySize()>0)move(-5);break;
  case PadR1:if(!detailsPage&&!hub&&activeLibrarySize()>0)move(5);break;
  case PadCircle:if(detailsPage)detailsPage=false;else if(settingsPage)settingsPage=false;else hub=false;break;
  case PadOptions:if(detailsPage)detailsPage=false;hub=!hub;settingsPage=false;notice.clear();noticeTimer=0.0f;break;
  case PadR3:if(!hub&&activeLibrarySize()>0)detailsPage=!detailsPage;break;
  case PadL3:if(!hub&&!detailsPage){favoritesOnly=!favoritesOnly;selected=0;focus=0.0f;bgMix=1.0f;notice=favoritesOnly?"SHOWING FAVORITES":"SHOWING ALL GAMES";noticeTimer=2.0f;}break;
  case PadTriangle:if(!detailsPage&&!hub){if(scanInstalledGames()){notice="LIBRARY RESCANNED";noticeTimer=2.5f;}else{notice="SCAN FAILED - PREVIOUS LIBRARY KEPT";noticeTimer=3.0f;}}break;
  case PadSquare:if(!hub)toggleSelectedFavorite();break;
  case PadCross:
   if(settingsPage){bool oldParticles=particlesEnabled,oldTitleId=showTitleId,oldReduceMotion=reduceMotion;if(settingsCursor==0)particlesEnabled=!particlesEnabled;else if(settingsCursor==1)showTitleId=!showTitleId;else{reduceMotion=!reduceMotion;if(reduceMotion){focus=1.0f;bgMix=1.0f;if(previousHeroBackground){SDL_DestroyTexture(previousHeroBackground);previousHeroBackground=nullptr;}}}if(!saveSettings()){particlesEnabled=oldParticles;showTitleId=oldTitleId;reduceMotion=oldReduceMotion;notice="SETTINGS SAVE FAILED - CHANGE REVERTED";noticeTimer=3.0f;}else{notice="SETTINGS SAVED";noticeTimer=1.5f;}}
   else if(hub){
    if(hubSelected==0){hub=false;notice.clear();noticeTimer=0.0f;}
    else if(hubSelected==2){settingsPage=true;settingsCursor=0;}
    else{
     static const char* hubMessages[4]={"GAME LIBRARY","MEDIA CENTER NOT AVAILABLE","SETTINGS NOT AVAILABLE","PROFILE NOT AVAILABLE"};
     notice=hubMessages[hubSelected];noticeTimer=3.0f;hub=false;
    }
   }else if(activeLibrarySize()>0){launchSelectedGame();}
   break;default:break;}}}
  if(pad){
   Sint16 axis=SDL_JoystickGetAxis(pad,0);
   if(axis>16000||axis< -16000){axisRepeat-=dt;if(axisRepeat<=0.0f){if(!detailsPage&&!settingsPage){if(hub)hubSelected=(hubSelected+(axis>0?1:3))%4;else move(axis>0?1:-1);}axisRepeat=0.24f;}}else axisRepeat=0.0f;
   Sint16 vertical=SDL_JoystickGetAxis(pad,1);
   if(settingsPage&&(vertical>16000||vertical< -16000)){verticalAxisRepeat-=dt;if(verticalAxisRepeat<=0.0f){settingsCursor=(settingsCursor+(vertical>0?1:2))%3;verticalAxisRepeat=0.24f;}}else verticalAxisRepeat=0.0f;
  }
  update(dt);draw();
 }
 saveLastSelectedGame();
 for(auto& game:games){if(game.icon)SDL_DestroyTexture(game.icon);if(game.background)SDL_DestroyTexture(game.background);}
 if(previousHeroBackground){SDL_DestroyTexture(previousHeroBackground);previousHeroBackground=nullptr;}
 if(pad)SDL_JoystickClose(pad);IMG_Quit();SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return 0;
}
