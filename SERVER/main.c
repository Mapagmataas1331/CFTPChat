#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <dirent.h>
#include <ctype.h>
#include "../fonts.h"

#define MAX_CLIENTS 100
#define MAX_USERNAME_LENGTH 12
#define MAX_PASSWORD_LENGTH 16
#define SIZE_BUF 512
#define IP_ADDR "127.0.0.1"
#define PORT 80
#define WWIDTH 96
#define WHEIGHT 48
#define ENG_LOWER "abcdefghijklmnopqrstuvwxyz"
#define ENG_UPPER "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define RUS_LOWER "àáâãäåæçèéêëìíîïğñòóôõö÷øùúûüışÿ"
#define RUS_UPPER "ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏĞÑÒÓÔÕÖ×ØÙÚÛÜİŞß"
#define DIGITS "0123456789"
#define SYMBOLS "~!?@#$%^&-_+=,.;:'\"/\\|(){}[]<>"
#define CMDS HBLU " Commands are:\n" HYEL "  exit" RES " - to leave,\n" HYEL "  shout " HCYN "message" RES " - for chating with over users,\n" HYEL "  file list" RES " - to see all files,\n" HYEL "  file send "  HCYN "file" RES " - to send file,\n" HYEL "  file get "  HCYN "file" RES " - to get file.\n" HBLU " ==-=-----=-==\n" RES

void windowsize(int ww, int wh, int bw, int bh)
{
  char cmd[128];
  snprintf(cmd, sizeof(cmd), "mode %d, %d", ww, wh);
  system(cmd);
  snprintf(cmd, sizeof(cmd), "Powershell \"&{$H=get-host;$W=$H.ui.rawui;$B=$W.buffersize;$B.width=%d;$B.height=%d;$W.buffersize=$B;}\"", bw, bh);
  system(cmd);
}
void skiplines(int number)
{
  for (int i = 0; i < number; i++)
  {
    printf("\n");
  }
}
void skipsymbols(int number)
{
  for (int i = 0; i < number; i++)
  {
    printf(" ");
  }
}
void pause()
{
  printf("\n  " HYEL);
  system("pause");
  printf("\n" RES);
}

typedef struct
{
  SOCKET clientSocket;
  char* clientIp;
  char clientName[MAX_USERNAME_LENGTH + 1];
  char clientPass[MAX_PASSWORD_LENGTH + 1];
}
ClientInfo;

ClientInfo* clientArray[MAX_CLIENTS];
int currentClients = 0;

int checkSymbols(const char* str, const char* symbols)
{
  if (str == NULL || symbols == NULL)
    return 0;

  while (*str != '\0')
  {
    int foundSymbol = 0;
    const char* symbol = symbols;

    while (*symbol != '\0')
    {
        if (*str == *symbol)
        {
          foundSymbol = 1;
          break;
        }
        symbol++;
    }
    if (!foundSymbol)
      return 0;
    str++;
  }

  return 1;
}

int checkDir(char* dir, int ifNotCreate)
{
  if (!mkdir(dir))
  {
    if (ifNotCreate)
    {
      rmdir(dir);
      printf(" There is no " HYEL "\"%s\"\n" RES, dir);
      return 0;
    }
    printf(" Created: " HYEL "\"%s\"\n" RES, dir);
    return 0;
  }
  else
  {
    printf(HYEL " \"%s\"" RES " already exists\n", dir);
    return 1;
  }
}

void clientError(SOCKET client, char* msg)
{
  char buff[SIZE_BUF];
  printf(HRED "  %s\n" RES, msg);
  sprintf(buff, "SKIP" HRED "  %s\n" RES, msg);
  send(client, buff, strlen(buff), 0);
  sprintf(buff, "EXIT");
  send(client, buff, strlen(buff), 0);
}

SOCKET getHost(const char* ip, int port)
{
  struct sockaddr_in serverAddress;
  memset(&serverAddress, 0, sizeof(serverAddress));
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr.s_addr = inet_addr(ip);
  serverAddress.sin_port = htons(port);

  SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (listenSocket == INVALID_SOCKET)
  {
    printf(HRED "  Failed creating socket.\n  Error: %d\n", WSAGetLastError());
    pause();
    exit(0);
  }

  if (bind(listenSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
  {
    printf(HRED "  Failed binding address to the socket.\n  Error: %d\n", WSAGetLastError());
    closesocket(listenSocket);
    pause();
    exit(0);
  }

  if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
  {
    printf(HRED "  Failed listening on the socket.\n  Error: %d\n", WSAGetLastError());
    closesocket(listenSocket);
    pause();
    exit(0);
  }

  return listenSocket;
}

int registerClient(ClientInfo* clientInfo, const char* username, const char* password)
{
  if (strlen(username) < 3)
  {
    clientError(clientInfo->clientSocket, "Username is missing characters. (minimum 3)");
    return 0;
  }
  if (strlen(username) > MAX_USERNAME_LENGTH + 1)
  {
    clientError(clientInfo->clientSocket, "Username exceeds the maximum length.");
    return 0;
  }

  if (strlen(username) < 3)
  {
    clientError(clientInfo->clientSocket, "Username is missing characters. (minimum 3)");
    return 0;
  }
  if (strlen(password) > MAX_PASSWORD_LENGTH + 1)
  {
    clientError(clientInfo->clientSocket, "Password exceeds the maximum length.");
    return 0;
  }
  if (!checkSymbols(username, ENG_LOWER ENG_UPPER DIGITS))
  {
    clientError(clientInfo->clientSocket, "The username must contain only English letters and numbers.");
    return 0;
  }
  if (!checkSymbols(password, ENG_LOWER ENG_UPPER DIGITS SYMBOLS))
  {
    clientError(clientInfo->clientSocket, "The password must contain only English letters, numbers and symbols: \"" SYMBOLS "\".");
    return 0;
  }

  char lowercaseUsername[MAX_USERNAME_LENGTH + 1];
  int i = 0;
  while (username[i])
  {
    lowercaseUsername[i] = tolower(username[i]);
    i++;
  }
  lowercaseUsername[MAX_USERNAME_LENGTH] = '\0';
  username = lowercaseUsername;

  FILE* file = fopen("data/users.txt", "a+");
  if (file == NULL)
  {
    printf(HRED "  Failed opening users file.\n" RES);
    return 0;
  }

  char line[MAX_USERNAME_LENGTH + MAX_PASSWORD_LENGTH + 2];
  char fileUsername[MAX_USERNAME_LENGTH + 1];
  char filePassword[MAX_PASSWORD_LENGTH + 1];
  int userExists = 0;

  while (fgets(line, sizeof(line), file) != NULL)
  {
    sscanf(line, "%s %s", fileUsername, filePassword);
    if (strcmp(username, fileUsername) == 0)
    {
      userExists = 1;
      break;
    }
  }

  if (userExists)
  {
    if (strcmp(password, filePassword) != 0)
    {
      printf(HRED "  Invalid password for existing username.\n" RES);
      fclose(file);
      char buff[SIZE_BUF];
      sprintf(buff, "SKIP" HRED "  Invalid password!\n" RES);
      send(clientInfo->clientSocket, buff, strlen(buff), 0);
      sprintf(buff, "EXIT");
      send(clientInfo->clientSocket, buff, strlen(buff), 0);
      return 0;
    }
    else
    {
      strncpy(clientInfo->clientName, username, MAX_USERNAME_LENGTH);
      strncpy(clientInfo->clientPass, password, MAX_PASSWORD_LENGTH);
      clientInfo->clientName[MAX_USERNAME_LENGTH] = '\0';
      clientInfo->clientPass[MAX_PASSWORD_LENGTH] = '\0';

      printf(HGRN " %s logged in as %s\n" RES, clientInfo->clientIp, username);
      char buff[SIZE_BUF];
      sprintf(buff, "SKIP" HGRN "  You are successfully logged in.\n" HBLU " Welcome, " RES "%s" HBLU "!\n" RES "  10101010101010101010\n  10101010101010101010\n  10101010101010101010\n" CMDS, username);
      send(clientInfo->clientSocket, buff, strlen(buff), 0);
    }
  }
  else
  {
    strncpy(clientInfo->clientName, username, MAX_USERNAME_LENGTH);
    strncpy(clientInfo->clientPass, password, MAX_PASSWORD_LENGTH);
    clientInfo->clientName[MAX_USERNAME_LENGTH] = '\0';
    clientInfo->clientPass[MAX_PASSWORD_LENGTH] = '\0';

    printf(HGRN " %s logged in as %s\n" RES, clientInfo->clientIp, username);
    char buff[SIZE_BUF];
    sprintf(buff, "SKIP" HGRN "  You are successfully registered.\n" HBLU " Welcome, " RES "%s" HBLU "!\n" RES "  10101010101010101010\n  10101010101010101010\n  10101010101010101010\n" CMDS, username);
    send(clientInfo->clientSocket, buff, strlen(buff), 0);

    fprintf(file, "%s %s\n", clientInfo->clientName, clientInfo->clientPass);
  }

  fclose(file);
  return 1;
}

void processCommand(ClientInfo* clientInfo, const char* command)
{
  if (!strncmp(command, "exit", 4))
  {
    char buff[SIZE_BUF];
    sprintf(buff, "EXIT");
    send(clientInfo->clientSocket, buff, strlen(buff), 0);
  }
  else if (!strncmp(command, "shout ", 6))
  {
    char shoutBuff[SIZE_BUF];
    sprintf(shoutBuff, "SKIP - %s: %s\n", clientInfo->clientName, &command[6]);

    if (!checkSymbols(&command[6], ENG_LOWER ENG_UPPER RUS_LOWER RUS_UPPER DIGITS SYMBOLS " "))
    {
      printf(HRED " Message contains invalid characters\n" RES);
      sprintf(shoutBuff, "SKIP" HRED "  Your message: \"%s\" contains invalid characters.\n" RES, &command[6]);
      send(clientInfo->clientSocket, shoutBuff, strlen(shoutBuff), 0);
      return;
    }
    
    for (int i = 0; i < currentClients; i++)
    {
      if (clientArray[i] != NULL && clientArray[i] != clientInfo)
      {
        printf(" Sending " HYEL "\"%s\"" RES " to " HYEL "%s\n" RES, &command[6], clientArray[i]->clientName);
        send(clientArray[i]->clientSocket, shoutBuff, strlen(shoutBuff), 0);
      }
    }
    sprintf(shoutBuff, "SKIP Your message: " HYEL "\"%s\"" RES " was successfully shouted.\n", &command[6]);
    send(clientInfo->clientSocket, shoutBuff, strlen(shoutBuff), 0);
  }
  else if (!strncmp(command, "file list", 9))
  {
    DIR* dir;
    struct dirent* entry;
    char filelistBuff[SIZE_BUF];

    char arg[128];
    snprintf(arg, sizeof(arg), "data/%s", clientInfo->clientName);
    if (!checkDir(arg, 1))
    {
      sprintf(filelistBuff, "SKIP" HRED "  You don't have any files yet.\n" RES);
      send(clientInfo->clientSocket, filelistBuff, strlen(filelistBuff), 0);
      return;
    }

    dir = opendir(arg);
    if (dir == NULL)
    {
      printf(HRED "  Failed to open the folder.\n");
    }

    int isEmpty = 1;
    while ((entry = readdir(dir)) != NULL)
    {
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      {
        continue;
      }

      isEmpty = 0;
      break;
    }

    if (isEmpty)
    {
      sprintf(filelistBuff, "SKIP" HRED "  You don't have any files yet.\n" RES);
      send(clientInfo->clientSocket, filelistBuff, strlen(filelistBuff), 0);
      closedir(dir);
      rmdir(arg);
      printf(" Removed: " HYEL "\"%s\"\n" RES, arg);
      return;
    }

    rewinddir(dir);

    sprintf(filelistBuff, "SKIP Files are:\n");
    send(clientInfo->clientSocket, filelistBuff, strlen(filelistBuff), 0);

    int count = 0;

    while ((entry = readdir(dir)) != NULL)
    {
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      {
        continue;
      }

      sprintf(filelistBuff, "SKIP" HYEL "  %-2d " RES "%s\n", count, entry->d_name);
      send(clientInfo->clientSocket, filelistBuff, strlen(filelistBuff), 0);

      count++;
    }

    sprintf(filelistBuff, "SKIP ==--%2d--==\n", count);
    send(clientInfo->clientSocket, filelistBuff, strlen(filelistBuff), 0);

    closedir(dir);
  }
  else if (!strncmp(command, "file send ", 10))
  {
    char filesendBuff[SIZE_BUF];
    if (!checkSymbols(&command[10], ENG_LOWER ENG_UPPER DIGITS SYMBOLS))
    {
      printf(HRED " Fille name contains invalid characters\n" RES);
      sprintf(filesendBuff, "SKIP" HRED "  Your fille name: \"%s\" contains invalid characters.\n" RES, &command[10]);
      send(clientInfo->clientSocket, filesendBuff, strlen(filesendBuff), 0);
      return;
    }

    sprintf(filesendBuff, "SEND %s\n", &command[10]);
    send(clientInfo->clientSocket, filesendBuff, strlen(filesendBuff), 0);
  }
  else if (strncmp(command, "SKIP", 4))
  {
    char buff[SIZE_BUF];
    sprintf(buff, "SKIP" HRED "  Unknown command: %s\n" RES, command);
    send(clientInfo->clientSocket, buff, strlen(buff), 0);
  }
}

DWORD WINAPI handleClient(LPVOID lpParam)
{
  ClientInfo* clientInfo = (ClientInfo*)lpParam;
  SOCKET clientSocket = clientInfo->clientSocket;
  char* clientIp = clientInfo->clientIp;
  char buff[SIZE_BUF];
  int len;

  printf(HBLU " New connection from %s\n" RES, clientIp);

  sprintf(buff, HBLU " You are in CFTPChat!\n" RES " Enter your name " HYEL "(max %d)" RES ": " HCYN, MAX_USERNAME_LENGTH);
  send(clientSocket, buff, strlen(buff), 0);
  char username[MAX_USERNAME_LENGTH + 1];
  if ((len = recv(clientSocket, (char*)&username, SIZE_BUF, 0)) == SOCKET_ERROR) {
    printf(HRED "  Failed receiving data from client.\n  Error: %d\n", WSAGetLastError());
    return 0;
  }
  username[MAX_USERNAME_LENGTH + 1] = '\0';

  sprintf(buff, RES " Enter your password " HYEL "(max %d)" RES ": " HCYN, MAX_PASSWORD_LENGTH);
  send(clientSocket, buff, strlen(buff), 0);
  char password[MAX_PASSWORD_LENGTH + 1];
  if ((len = recv(clientSocket, (char*)&password, SIZE_BUF, 0)) == SOCKET_ERROR) {
    printf(HRED "  Failed receiving data from client.\n  Error: %d\n", WSAGetLastError());
    return 0;
  }
  password[MAX_PASSWORD_LENGTH + 1] = '\0';

  int registrationResult = registerClient(clientInfo, username, password);
  if (registrationResult == 0) {
    printf(HRED "  Failed registration.\n" RES);
    return 0;
  }

  char* clientName = clientInfo->clientName;

  int clientIndex = -1;
  for (int i = 0; i < MAX_CLIENTS; i++)
  {
    if (clientArray[i] == NULL)
    {
      clientArray[i] = clientInfo;
      clientIndex = i;
      break;
    }
  }

  if (clientIndex == -1)
  {
    printf(HRED "  Maximum number of clients reached.\n");
    closesocket(clientSocket);
    free(clientIp);
    free(clientInfo);
    return 0;
  }

  currentClients++;
  printf(HYEL " %s" RES "'s index: " HCYN "%d\n" RES, username, clientIndex);
  printf(" Current clients: " HCYN "%d\n" RES, currentClients);

  do {
    sprintf(buff, " - " HYEL "(max 80)" RES ": ");
    send(clientSocket, buff, strlen(buff), 0);
    if ((len = recv(clientSocket, (char*)&buff, SIZE_BUF, 0)) == SOCKET_ERROR)
    {
      printf(HRED "  Failed receiving data from client.\n  Error: %d\n", WSAGetLastError());
      break;
    }
    buff[len] = '\0';
    printf(" %s" HYEL " (%s)" RES ": ", clientName, clientIp);
    for (int i = 0; i < len; i++)
    {
      printf("%c", buff[i]);
    }
    printf("\n");

    processCommand(clientInfo, buff);

    Sleep((DWORD)10);
  } while (len != 0 || clientSocket != SOCKET_ERROR);

  printf(HBLU " Lost connection from %s.\n" RES, clientIp);
  closesocket(clientSocket);
  free(clientIp);
  free(clientInfo);
  clientArray[clientIndex] = NULL;
  currentClients--;

  return 0;
}

const char* getSockIp(SOCKET s)
{
  struct sockaddr_in name;
  int lenn = sizeof(name);
  ZeroMemory(&name, sizeof(name));
  if (SOCKET_ERROR == getsockname(s, (struct sockaddr*)&name, &lenn))
  {
    return NULL;
  }
  return inet_ntoa((struct in_addr)name.sin_addr);
}

int main()
{
  system("@echo off");
  system("title CFTPChat - Server");
  system("color 0f");
  system("chcp 1251");
  windowsize(WWIDTH, WHEIGHT, WWIDTH, 9999);
  skipsymbols((WWIDTH - 6) / 2);
  printf("Server\n\n");

  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
  {
    printf(HRED "  Failed to initialize winsock.\nError: %d\n", WSAGetLastError());
    pause();
    return 0;
  }

  SOCKET listenSocket = getHost(IP_ADDR, PORT);
  printf(HGRN "  Server started on %s:%d\n" RES, getSockIp(listenSocket), PORT);

  checkDir("data", 0);

  while (1)
  {
    struct sockaddr_in clientAddress;
    int clientAddressSize = sizeof(clientAddress);
    SOCKET clientSocket = accept(listenSocket, (struct sockaddr*)&clientAddress, &clientAddressSize);

    if (clientSocket == INVALID_SOCKET)
    {
      printf(HRED "  Failed accepting incoming connections.\n");
      closesocket(listenSocket);
      pause();
      exit(0);
    }

    ClientInfo* clientInfo = (ClientInfo*)malloc(sizeof(ClientInfo));
    clientInfo->clientSocket = clientSocket;
    clientInfo->clientIp = strdup(getSockIp(clientSocket));

    DWORD threadId;
    HANDLE hThread = CreateThread(NULL, 0, handleClient, (LPVOID)clientInfo, 0, &threadId);
    if (hThread == NULL)
    {
      printf(HRED "  Error creating thread.\n");
      closesocket(listenSocket);
      pause();
      exit(0);
    }

    CloseHandle(hThread);
  }

  closesocket(listenSocket);

  if (!WSACleanup())
  {
    printf(HRED "  Failed WSACleanup.\nError: %d\n", WSAGetLastError());
    pause();
    return 0;
  }

  pause();
  return 0;
}
