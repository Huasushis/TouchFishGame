#include <cstdio>
#include <iostream>
#include <Windows.h>
#include <thread>
#include <vector>
#include <list>
#include <ctime>
#include <atomic>
#pragma comment (lib, "ws2_32.lib")
#define PORT 11451
using namespace std;
char playericon[7] = "@&*%$+";
char killnumber[10][50];
char mesg[50];
#define iconamount (6)
struct wch {
  char a, b;
  wch() {a = b = 0;}
  wch(char a): a(a) {b = 0;}
  wch(char a, char b): a(a), b(b) {}
  wch& operator = (const wch& tmp) {
    a = tmp.a;
    b = tmp.b;
    return *this;
  }
  wch& operator = (const char& c) {
    a = c;
    b = 0;
    return *this;
  }
  bool operator == (const char& c) {
    return a == c;
  }
  bool operator != (const char& c) {
    return a != c;
  }
};
atomic_int playernumber(0), alivenumber(0);
char gamemap[20][41];
wch scenebuf[20][41];
const int f[4][2]={-1,0,0,1,1,0,0,-1};
int dura;
SOCKET ListenSocket;
inline void MyWait(int time) {
  int st = clock();
  while (clock() - st < time);
}
class Player {
 public:
  int direction, movement, shoot;
 public:
  int x, y, direc;
  bool alive;
  wch icon;
  char name[20];
  int rank;
  int lastshoot;
  int killed;
  void setoperation(int d, int m, int s) {
    direction = d;
    movement = m;
    shoot = s;
  }
  Player(int x = 0, int  y = 0, int direc = 0): x(x), y(y), direc(direc) {
    alive = true;
    icon = playericon[(playernumber - 1) % iconamount];
    direction = 0;
    movement = 0;
    shoot = 0;
    rank = 0;
    lastshoot = 0;
    killed = 0;
    memset(name, 0, sizeof(name));
  }
  bool operator<(const Player cmper) const {
    return rank < cmper.rank;
  }
};
class Bullet {
 public:
  int x, y, direc;
  list <Player>::iterator it;
  Bullet(int x = 0, int  y = 0, int direc = 0): x(x), y(y), direc(direc) {}
};
void ListenNewPlayerAdd();
list <Player> PlayerList;
vector <Bullet> BulletList;
DWORD WINAPI PlayerContactThread(LPVOID lpParameter);
//DWORD WINAPI PlayerSendThread(LPVOID lpParameter);
void run();
void dfs(int step, int x, int y) {
  if (step >= 3 && (step > 6 || rand()%5 == 0)) return;
  gamemap[x][y] = '#';
  for (int i = 0; i < 4; ++i) {
    int tx = x + f[i][0], ty = y + f[i][1];
    if (tx <= 1 || ty <= 1 || tx >= 18 || ty >= 38) continue;
    if(gamemap[tx][ty]=='#') continue;
    if (rand() % 3 == 0) return dfs(step + 1, tx, ty);
  }
}
void mapinit() {
  for (int i = 0;i < 20; ++i) {
    for (int j = 0;j < 40; ++j) {
      if(i == 0 || j == 0 || i == 19 || j == 39) {
        gamemap[i][j] = '#';
      }
      else gamemap[i][j] = ' ';
    }
    gamemap[i][40] = '\0';
  }
  for (int i = 2;i < 18; ++i) {
    for (int j = 2;j < 38; ++j) {
      if(gamemap[i][j] == '#') continue;
      if(rand() % 30  == 0) dfs(0, i, j);
    }
  }
}
Player NewPlayer() {
  Player res;
  while (1) {
    res.x = rand() % 18 + 2;
    res.y = rand() % 38 + 1;
    if(gamemap[res.x][res.y] == ' '&& gamemap[res.x - 1][res.y] == ' ') break;
  }
  return res;
}
int main() {
  /*cout << "设置更新速度(ms)";
  cin >> dura;*/
  srand(time(NULL));
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		cout << "加载winsock.dll失败" << endl;
		return -1;
	}
  ListenSocket = socket(AF_INET,SOCK_STREAM,0);
  SOCKADDR_IN ListenAddr;
  ListenAddr.sin_family=AF_INET;
  ListenAddr.sin_addr.S_un.S_addr=INADDR_ANY;
  ListenAddr.sin_port=htons(PORT);
  int status;
  status = bind(ListenSocket, (LPSOCKADDR)& ListenAddr, sizeof(ListenAddr));
  if (status == SOCKET_ERROR) {
    cout << "端口绑定失败" << endl;
    return -1;
  }
  status = listen(ListenSocket, 10);
  thread thListenNewPlayerAdd(ListenNewPlayerAdd);
  thListenNewPlayerAdd.detach();
  run();
  closesocket(ListenSocket);
  WSACleanup();
  return 0;
}
void run() {
  while (true) {
    alivenumber = playernumber.load();
    mapinit();
    for (auto it = PlayerList.begin(); it != PlayerList.end(); ++it) {
      it -> alive = true;
      it -> rank = 0;
      Player playertmp = NewPlayer();
      it -> x = playertmp.x;
      it -> y = playertmp.y;
      it -> direc = 0;
      it -> direction = 0;
    }
    int bulletupdate = clock() - 100;
    while (alivenumber > 1) {
      for (int i = 0; i < 20; ++i) {
        for (int j =0; j < 41; ++j) {
          scenebuf[i][j] = gamemap[i][j];
        }
      }
      for (auto it = PlayerList.begin(); it != PlayerList.end(); ++it) {
        if (! it -> alive) continue;
        if (it -> movement) {
          if (scenebuf[it -> x + f[it -> direction][0]][it -> y + f[it -> direction][1]] != ' ') goto nxt;
          if (scenebuf[it -> x][it -> y] != ' ') goto nxt;
          it -> direc = it -> direction;
          it -> x += f[it -> direc][0];
          it -> y += f[it -> direc][1];
          it -> movement = 0;
        }
        else {
          if (scenebuf[it -> x - f[it -> direction][0]][it -> y - f[it -> direction][1]] != ' ') goto nxt;
          it -> direc = it -> direction;
        }
        nxt:;
        if (it -> shoot && clock() - (it -> lastshoot) > 3000) {
          it -> lastshoot = clock();
          int sig = it -> shoot > 0 ? 1 : -1;
          Bullet tmp(it -> x + sig * f[it -> direc][0], it -> y + sig * f[it -> direc][1], it -> direc);
          if (sig == -1) tmp.direc = (tmp.direc + 2) % 4;
          tmp.it = it;
          BulletList.emplace_back(tmp);
          it -> shoot = 0;
        }
        scenebuf[it -> x][it -> y] = it -> icon;
        scenebuf[it -> x - f[it -> direc][0]][it -> y - f[it -> direc][1]] = (it -> direc % 2 == 1? '-' : '|');
      }
      bool p = 0;
      for (auto it = BulletList.begin(); it != BulletList.end();) {
        if (clock() - bulletupdate > 100) {
          p = 1;
          it -> x += f[it -> direc][0];
          it -> y += f[it -> direc][1];
          if (scenebuf[it -> x][it -> y] == '#') {
            BulletList.erase(it);
            continue;
          }
        }
        if (scenebuf[it -> x][it -> y] != ' ' && scenebuf[it -> x][it -> y] != '|' && scenebuf[it -> x][it -> y] != '-') {
          scenebuf[it -> x][it -> y] = (it -> direc % 2 == 1? '-' : '|');
          //++ (it -> it -> killed);
          snprintf(mesg, sizeof(mesg), "%s", it -> it -> name);
          BulletList.erase(it);
          continue;
        }
        scenebuf[it -> x][it -> y] = (it -> direc % 2 == 1? '-' : '|');
        ++it;
      }
      if (p) bulletupdate = clock();
      {
      int i = 0;
      for (auto it = PlayerList.begin(); it != PlayerList.end(); ++it, ++i) {
        if (scenebuf[it -> x][it -> y] == '|' || scenebuf[it -> x][it -> y] == '-') {
          it -> rank = alivenumber;
          it -> alive = false;
          --alivenumber;
          char tmp[50];
          snprintf(tmp, sizeof(tmp), "%s", mesg);
          snprintf(mesg, sizeof(mesg), "%s被%s杀死了!", it -> name, tmp);
          for (int j = strlen(mesg); j < 49; ++j) mesg[j] = ' ';
          mesg[49] = '\0';
        }
        if (i < 10) {
          snprintf(killnumber[i], sizeof(killnumber[i]), "%s的分数是: %d", it -> name, it -> killed);
          for (int j = strlen(killnumber[i]); j < 49; ++j) killnumber[i][j] = ' ';
          killnumber[i][49] = '\0';
        }
      }
      }
      MyWait(20);
    }
    PlayerList.sort();
    int i = 1;
    //memset(scenebuf, (int)' ', sizeof(scenebuf));
    for (int i = 0; i < 20; ++i) {
      for (int j = 0; j < 40; ++j) {
        scenebuf[i][j] = ' ';
      }
    }
    char tmps[41];
    for (auto it = PlayerList.begin(); it != PlayerList.end(); ++it, ++i) {
      if (it -> rank == 0) it -> rank = 1;
      if (playernumber > 1)
        it -> killed += playernumber - it -> rank;
      snprintf(tmps, sizeof(tmps), "第%d名: %s", i, it -> name);
      int len = strlen(tmps);
      for (int j = 0, k = 0; j < len; ++j, ++k) {
        scenebuf[i - 1][k] = tmps[j];
        if (tmps[j] < 0) {
          scenebuf[i - 1][k].b = tmps[++j];
        }
      }
      scenebuf[i - 1][40] = '\0';
    }
    MyWait(5000);
  }
}
void ListenNewPlayerAdd() {
  while (true) {
    SOCKET *ClientSocket = new SOCKET;
    ClientSocket = (SOCKET*) malloc(sizeof(SOCKET));
    int SockAddrlen = sizeof(sockaddr);
    *ClientSocket = accept(ListenSocket, 0, 0);
    cout << "一个客户端已连接到服务器, socket是: " << *ClientSocket << endl;
    ++playernumber;
    ++alivenumber;
    PlayerList.push_back(NewPlayer());
    CreateThread(NULL, 0, &PlayerContactThread, ClientSocket, 0, NULL);
    //CreateThread(NULL, 0, &PlayerSendThread, ClientSocket, 0, NULL);
  }
}
DWORD WINAPI PlayerContactThread(LPVOID lpParameter) {
  SOCKET *ClientSocket = (SOCKET*) lpParameter;
  auto ClientPlayer = prev(PlayerList.end());
  int status, issend;
  status = recv(*ClientSocket, (char*)(ClientPlayer -> name), sizeof(ClientPlayer -> name), 0);
  if (status == SOCKET_ERROR || status == 0) {
    --alivenumber;
    --playernumber;
    PlayerList.erase(ClientPlayer);
    closesocket(*ClientSocket);
    free(ClientSocket);
    return 0;
  }
  cout << ClientPlayer -> name << ' ' << "已加入！\n";
  ClientPlayer -> icon = ClientPlayer -> name[0];
  if (ClientPlayer -> name[0] < 0) {
    ClientPlayer -> icon.a = rand() % 26 + 'A' + rand() % 2 * 32;
  }
  if (ClientPlayer -> name[0] == '#') {
    goto abaaba;
  }
  int st, ed;
  while (1) {
    st = clock();
    status = recv(*ClientSocket, (char*) &issend, sizeof(issend), 0);
    if (status <= 0) break;
    status = recv(*ClientSocket, (char *)(&(ClientPlayer -> direction)), sizeof (ClientPlayer -> direction), 0);
    if (status <= 0) break;
    status = recv(*ClientSocket, (char *)(&(ClientPlayer -> movement)), sizeof (ClientPlayer -> movement), 0);
    if (status <= 0) break;
    status = recv(*ClientSocket, (char *)(&(ClientPlayer -> shoot)), sizeof (ClientPlayer -> shoot), 0);
    if (status <= 0) break;
    //cout<<ClientPlayer->direction<<' '<<ClientPlayer->movement<<' '<<ClientPlayer -> shoot<<endl;
    if (issend) {
      for (int i = 0; i < 20; ++i) {
        send(*ClientSocket, (char*)scenebuf[i], sizeof(scenebuf[i]), 0);
      }
      for (int i = 0; i < 10; ++i) {
        send(*ClientSocket, (char*)killnumber[i], sizeof(killnumber[i]), 0);
      }
      send(*ClientSocket, mesg, sizeof(mesg), 0);
    }
    ed = clock();
    MyWait(max(0, 30 - ed + st));
  }
  abaaba:;
  --playernumber;
  if (ClientPlayer -> alive) {
    --alivenumber;
  }
  cout << ClientPlayer -> name << ' ' << "退出了！\n";
  PlayerList.erase(ClientPlayer);
  closesocket(*ClientSocket);
  free(ClientSocket);
  return 0;
}
