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
char gamemap[20][41];
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
    for (int i = 0; i < 20; ++i) {
      recv(SocketClient, gamemap[i], sizeof(gamemap[i]), 0);
      coord.Y = i;
      WriteConsoleOutputCharacterA(hOutBuf1, gamemap[i], 40, coord, &r);
    }
    SwapBuf();
  }
}
void yd(int op1, int preop) {
  if ((op1 + preop) % 2 == 0) {
    send(0, (op1 + 2) % 4, -1, KEYDOWN('K'));
    if (KEYDOWN('J')) {
      send(0, op1, 0, 1);
      send(0, (op1 + 2) % 4, 0, 0);
    }
  } else {
    send(0, op1, 1, KEYDOWN('J'));
    send(0, (op1 + 2) % 4, 0, KEYDOWN('K'));
  }
}
void run() {
  int op1 = 0, preop = 0;
  char kdown;
  send(SocketClient,name,sizeof(name),0);
  while(1) {
    if (KEYDOWN('A')) {
      op1 = 3;
      yd(op1, preop);
      preop = op1;
    }
    if (KEYDOWN('S')) {
      op1 = 2;
      yd(op1, preop);
      preop = op1;
    }
    if (KEYDOWN('D')) {
      op1 = 1;
      yd(op1, preop);
      preop = op1;
    }
    if (KEYDOWN('W')) {
      op1 = 0;
      yd(op1, preop);
      preop = op1;
    }
    if (KEYDOWN('J')) {
      send(0, op1, 0, 1);
      send(0, (op1 + 2) % 4, 0, 0);
    }
    if (KEYDOWN('K')) {
      send(0, (op1 + 2) % 4, 0, 1);
    }
    send(1, (op1 + 2) % 4, 0, 0);
    MyWait(50);
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