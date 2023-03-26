#include <cstdio>
#include <iostream>
#include <Windows.h>
#include <ctime>
#pragma comment (lib, "ws2_32.lib")
#define KEYDOWN(VK_NONAME) ((GetAsyncKeyState((int)VK_NONAME) & 0x8000)?1:0)
using namespace std;
DWORD r;
HANDLE hOutBuf1, hOutBuf2;
COORD coord = {0, 0};
int cnt;
DWORD bytes;
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
} gamemap[20][41];
char tmp[83];
void initbuf() {
  hOutBuf1 = CreateConsoleScreenBuffer(
    GENERIC_WRITE,
    FILE_SHARE_WRITE,
    NULL,
    CONSOLE_TEXTMODE_BUFFER,
    NULL
  );
  hOutBuf2 = CreateConsoleScreenBuffer(
    GENERIC_WRITE,
    FILE_SHARE_WRITE,
    NULL,
    CONSOLE_TEXTMODE_BUFFER,
    NULL
  );
  SetConsoleActiveScreenBuffer(hOutBuf2);
  CONSOLE_CURSOR_INFO cci;
  cci.bVisible = 0;
  cci.dwSize = 1;
  SetConsoleCursorInfo(hOutBuf1, &cci);
  SetConsoleCursorInfo(hOutBuf2, &cci);
}
void SwapBuf() {
  HANDLE tmp;
  tmp = hOutBuf1;
  hOutBuf1 = hOutBuf2;
  hOutBuf2 = tmp;
}
SOCKET SocketClient;
char ip[256];
int port;
char name[20];
char killnumber[10][50], mesg[50];
inline void MyWait(int time) {
  int st = clock();
  while (clock() - st < time);
}
void send(int op, int op1, int op2, int op3) {
  send(SocketClient, (char *)&op, sizeof(op), 0);
  send(SocketClient, (char *)&op1, sizeof(op1), 0);
  send(SocketClient, (char *)&op2, sizeof(op2), 0);
  send(SocketClient, (char *)&op3, sizeof(op3), 0);
  if (op) {
    coord.X = 0;
    for (int i = 0, j, k; i < 20; ++i) {
      if( recv(SocketClient, (char*)gamemap[i], sizeof(gamemap[i]), 0) <= 0 && name[0] == '#') {
        exit(114514);
      }
      memset(tmp, 0, sizeof(tmp));
      for (j = 0, k = 0; j < 41; ++j) {
        if (gamemap[i][j].a >= 0) tmp[k++] = gamemap[i][j].a;
        else {
          tmp[k++] = gamemap[i][j].a;
          tmp[k++] = gamemap[i][j].b;
        }
      }
      coord.Y = i;
      WriteConsoleOutputCharacterA(hOutBuf1, tmp, 41, coord, &r);
    }
    coord.X = 45;
    for (int i = 0; i < 10; ++i) {
      if(recv(SocketClient, killnumber[i], sizeof(killnumber[i]), 0) <= 0) {
        exit(114514);
      }
      coord.Y = i;
      WriteConsoleOutputCharacterA(hOutBuf1, killnumber[i], 50, coord, &r);
    }
    coord.X = 0;
    coord.Y = 20;
    recv(SocketClient, mesg, sizeof(mesg), 0);
    WriteConsoleOutputCharacterA(hOutBuf1, mesg, 50, coord, &r);
    SwapBuf();
  }
}
int shouldattack() {
  if (KEYDOWN('J')) return 1;
  if (KEYDOWN('K')) return -1;
  return 0;
}
void run() {
  int op1 = 0;
  char kdown;
  send(SocketClient,(char*)name,sizeof(name),0);
  while(1) {
    if (KEYDOWN('A')) {
      op1 = 3;
      send(1, op1, 1, shouldattack());
    }
    if (KEYDOWN('S')) {
      op1 = 2;
      send(1, op1, 1, shouldattack());
    }
    if (KEYDOWN('D')) {
      op1 = 1;
      send(1, op1, 1, shouldattack());
    }
    if (KEYDOWN('W')) {
      op1 = 0;
      send(1, op1, 1, shouldattack());
    }
    send(1, op1, 0, shouldattack());
  }
}
int main() {
  bg:;
  cout << "请输入服务端ip: ";
  cin >> ip;
  cout << "请输入服务端端口: ";
  cin >> port;
  cout << "请输入您的名字(长度不超过15): ";
  cin >> name;
  if (name[0] == '#') {
    cout << "你个挂逼， 给我重新输入" <<endl;
    goto bg;
  }
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2, 2), &wsaData);
  SocketClient=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
  SOCKADDR_IN  ClientAddr;

  ClientAddr.sin_family= AF_INET;
  ClientAddr.sin_addr.S_un.S_addr= inet_addr(ip);
  ClientAddr.sin_port = htons(port);

  int status = 0;
  status = connect(SocketClient,(struct sockaddr*)&ClientAddr,sizeof(ClientAddr));
  if (status == SOCKET_ERROR) {
    cout << "连接失败，请重新输入！\n";
    goto bg;
  }
  cout << "连接成功！正在加入游戏……\n";
  Sleep(1000);
  initbuf();
  run();
  WSACleanup();
  return 0;
}
