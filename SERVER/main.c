#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <dirent.h>
#include <ctype.h>
#include "fonts.h"

#define MAX_CLIENTS 100
#define MAX_USERNAME_LENGTH 12
#define MAX_PASSWORD_LENGTH 16
#define SIZE_BUF 512
#define IP_ADDR "127.0.0.1"
#define PORT 80
#define WWIDTH 96
#define WHEIGHT 48
#define CMDS HBLU " Commands are:\n" HYEL "  shout " RES "- for chating with over users.\n" HBLU " ==-=-----=-==\n" RES

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
} ClientInfo;

ClientInfo* clientArray[MAX_CLIENTS];
int currentClients = 0;

void checkDir(char* dir)
{
  if (!mkdir(dir))
  {
    printf(" Created: " HYEL "\"%s\".\n" RES, dir);
  }
  else
  {
    printf(HYEL " \"%s\"" RES " already exists.\n", dir);
  }

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
  if (strlen(username) > MAX_USERNAME_LENGTH + 1)
  {
    printf(HRED "  Username exceeds the maximum length.\n" RES);
    char buff[SIZE_BUF];
    sprintf(buff, "SKIP" HRED "  Username exceeds the maximum length.\n" RES);
    send(clientInfo->clientSocket, buff, strlen(buff), 0);
    sprintf(buff, "EXIT");
    send(clientInfo->clientSocket, buff, strlen(buff), 0);
    return 0;
  }

  if (strlen(password) > MAX_PASSWORD_LENGTH + 1)
  {
    printf(HRED "  Password exceeds the maximum length.\n" RES);
    char buff[SIZE_BUF];
    sprintf(buff, "SKIP" HRED "  Password exceeds the maximum length.\n" RES);
    send(clientInfo->clientSocket, buff, strlen(buff), 0);
    sprintf(buff, "EXIT");
    send(clientInfo->clientSocket, buff, strlen(buff), 0);
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
  if (!strncmp(command, "shout ", 6))
  {
    char shoutBuff[SIZE_BUF];
    sprintf(shoutBuff, "SKIP - %s: %s\n", clientInfo->clientName, &command[6]);

    for (int i = 0; i < currentClients; i++)
    {
      if (clientArray[i] != NULL && clientArray[i] != clientInfo)
      {
        printf(" Sending %s to %s.\n", &command[6], clientArray[i]->clientName);
        send(clientArray[i]->clientSocket, shoutBuff, strlen(shoutBuff), 0);
      }
    }
    sprintf(shoutBuff, "SKIP Your message: " HYEL "\"%s\"" RES " was successfully shouted.\n", &command[6]);
    send(clientInfo->clientSocket, shoutBuff, strlen(shoutBuff), 0);
  }
  else if (!strncmp(command, "file list ", 10))
  {

  }
  else
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

  printf(HBLU " New connection from %s.\n" RES, clientIp);

  sprintf(buff, HBLU " You are in CFTPChat!\n" RES " Enter your name " HYEL "(max %d)" RES ": ", MAX_USERNAME_LENGTH);
  send(clientSocket, buff, strlen(buff), 0);
  char username[MAX_USERNAME_LENGTH + 1];
  if ((len = recv(clientSocket, (char*)&username, SIZE_BUF, 0)) == SOCKET_ERROR) {
    printf(HRED "  Failed receiving data from client.\n  Error: %d\n", WSAGetLastError());
    return 0;
  }
  username[MAX_USERNAME_LENGTH + 1] = '\0';

  sprintf(buff, " Enter your password " HYEL "(max %d)" RES ": ", MAX_PASSWORD_LENGTH);
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
  printf(" %s's index: %d\n", username, clientIndex);
  printf(" Current clients: %d\n", currentClients);


  char arg[128];
  snprintf(arg, sizeof(arg), "data/%s", clientName);
  checkDir(arg);

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
  printf(HYEL "Server\n\n" RES);

  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
  {
    printf(HRED "  Failed to initialize winsock.\nError: %d\n", WSAGetLastError());
    pause();
    return 0;
  }

  SOCKET listenSocket = getHost(IP_ADDR, PORT);
  printf(HGRN "  Server started on %s.\n" RES, getSockIp(listenSocket));

  checkDir("data");

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
