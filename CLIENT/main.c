#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include "../fonts.h"

#define SIZE_BUF 512
#define IP_ADDR "127.0.0.1"
#define PORT 80
#define WWIDTH 96
#define WHEIGHT 48

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

int processCommand(char *buff)
{
  if (!strncmp(buff, "SKIP", 4))
    return 0;
  if (!strncmp(buff, "EXIT", 4))
    return 1;
  if (!strncmp(buff, "SEND", 4))
    return 2;
  return INT_MAX;
}

SOCKET getConn(char * ipAddr, int port)
{
  SOCKET s;
  if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
  {
    printf(HRED "  Failed to create connection socket.\n  Error: %d\n", WSAGetLastError());
    pause();
    exit(0);
  }

  struct sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr.s_addr = inet_addr(ipAddr);
  serverAddress.sin_port = htons(port);

  if (connect(s, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
  {
    printf(HRED "  Failed connection.\n  Error: %d\n", WSAGetLastError());
    closesocket(s);
    pause();
    exit(0);
  }
  printf(HGRN "  Connected successfully.\n" RES);
  return s;
}

int scanInput(char *scanBuffer, int bufferLength)
{
  if (fgets(scanBuffer, bufferLength, stdin) == NULL)
  {
    if (feof(stdin))
    {
      printf(HRED "  End of input encountered.\n" RES);
    }
    else
    {
      printf(HRED "  Error reading input.\n" RES);
    }
    return 0;
  }

  size_t inputLength = strlen(scanBuffer);
  if (inputLength > 0 && scanBuffer[inputLength - 1] == '\n')
  {
    scanBuffer[inputLength - 1] = '\0';
    return 1;
  }

  int dropped = 0;
  int c;
  while ((c = fgetc(stdin)) != '\n' && c != EOF)
  {
    dropped++;
  }

  if (dropped == 0)
  {
    return 1;
  }

  printf(HRED "  Your input exceeded the limit by %d characters!\n" RES, dropped);

  return 0;
}

void sendToSock(SOCKET s, char *msg, int len)
{
  if (len > SIZE_BUF)
  {
    printf(HRED "  Message not sent! Exceeding the max message size...\n");
    return;
  }
  if (send(s, msg, len, 0) == SOCKET_ERROR)
  {
    printf(HRED "  Failed sending.\n  Error: %d\n", WSAGetLastError());
    closesocket(s);
    pause();
    exit(0);
  }
}

int main()
{
  system("@echo off");
  system("title CFTPChat - Client");
  system("color 0f");
  system("chcp 1251");
  windowsize(WWIDTH, WHEIGHT, WWIDTH, 999);
  skipsymbols((WWIDTH-6)/2);
  printf("Client\n\n");

  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
  {
    printf(HRED "  Failed to initializate winsock.\n  Error: %d\n", WSAGetLastError());
    pause();
    exit(0);
  }

  SOCKET connectionSocket = getConn(IP_ADDR, PORT);

  while (1)
  {
    char msgbuff[SIZE_BUF];
    int len;
    do
    {
      if ((len = recv(connectionSocket, (char*)&msgbuff, SIZE_BUF, 0)) == SOCKET_ERROR)
      {
        printf(HRED "  Failed to recive a message.\n  Error: %d\n", WSAGetLastError());
        pause();
        exit(0);
      }

      switch(processCommand((char*)&msgbuff)){
      case 0:
        for (int i = 4; i < len; i++)
        {
          if (msgbuff[i] == 'S' && msgbuff[i+1] == 'K' && msgbuff[i+2] == 'I' && msgbuff[i+3] == 'P')
            i += 3;
          else
            printf ("%c", msgbuff[i]);
        }
        continue;
      case 1:
        printf(HRED "  Disconnected from the server.\n" RES);
        pause();
        return 1;
      default:
        break;
      }
      for (int i = 0; i < len; i++)
      {
        printf ("%c", msgbuff[i]);
      }
      break;
    }
    while (len != 1);

    char *resp;
    resp = (char*)malloc(81*sizeof(char));

    if (scanInput(resp, 81))
    {
      sendToSock(connectionSocket, resp, strlen(resp));
    }
    else
    {
      sendToSock(connectionSocket, "SKIP" HRED " Error on scanInput" RES, 34);
    }

    free(resp);
  }

  printf(HYEL "  Connection lost.\n" RES);
  closesocket(connectionSocket);

  int closeResult = WSACleanup();
  if (closeResult == SOCKET_ERROR)
  {
    printf(HRED "  Failed WSACleanup.\n  Error: %d\n", WSAGetLastError());
    pause();
    return 0;
  }

  pause();
  return 0;
}
