#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include "Global.h"

// ����ȭ�� ���� �̺�Ʈ ��ü 
HANDLE hRecvEvt;		// Ŭ���̾�Ʈ�κ��� ������ ���� �̺�Ʈ ��ü
HANDLE hUpdateEvt;		// ���� �� ���� �̺�Ʈ ��ü

// ���� ������ ����� ����(���� ���� ����)
Info info;

// ���� �Լ� ���� ��� �� ����
void err_quit(const char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// ���� �Լ� ���� ���
void err_display(const char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

//�� ����� ���� ������ ���� �Լ�(�������� ��ŭ �ݺ� ����)
int recvn(SOCKET s, char *buf, int len, int flags)
{
	int received;
	char *ptr = buf;
	int left = len;

	while (left > 0) {
		received = recv(s, ptr, left, flags);
		if (received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if (received == 0)
			break;
		left -= received;
		ptr += received;
	}

	return (len - left);
}

// ����� ���� ������ ���� �Լ�(Ŭ���̾�Ʈ�κ��� �Է¹��� ������ ����)
//�� recvn�� ��ĥ �� �ִ��� ���� �غ���
void RecvFromClient(LPVOID arg, short PlayerID)
{

}

// ����� ���� ������ �۽� �Լ�(Ŭ���̾�Ʈ���� ���ŵ� ������ �۽�)
void SendToClient(LPVOID arg, short PlayerID)
{

}

// ��� ���� �� ���� ������ �Լ�
DWORD WINAPI CalculateThread(LPVOID ard)
{
	//�� ���� �� ���(�׽�Ʈ��)
	printf("CalculateThread ����\n");

	return 0;
}

// Ŭ���̾�Ʈ�κ��� �����͸� �ְ� �޴� ������ �Լ�
DWORD WINAPI ProccessClient(LPVOID ard)
{
	//�� ���� �� ���(�׽�Ʈ��)
	printf("ProccessClient ����\n");

	return 0;
}

int main(int argc, char *argv[])
{
	int retval;		// ��� return value�� ���� ����

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	// �� ���� �ڵ� ���� �̺�Ʈ ��ü ����(������ ���� �̺�Ʈ, ���� �̺�Ʈ)
	hRecvEvt = CreateEvent(NULL, FALSE, FALSE, NULL);	// ���ȣ ����
	if (hRecvEvt == NULL) return 1;
	hUpdateEvt = CreateEvent(NULL, FALSE, TRUE, NULL);	// ��ȣ ����
	if (hUpdateEvt == NULL) return 1;

	// ������ ��ſ� ����� ����
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;

	HANDLE hCalculateThread;	// ���� ������ �ڵ�
	HANDLE hProccessClient[2];	// Ŭ���̾�Ʈ ó�� ������ �ڵ�

	// hCalculateThread ����
	hCalculateThread = CreateThread(NULL, 0, CalculateThread, NULL, 0, NULL);

	while (info.connectedP < MAX_PLAYERS)		// �� �� ������ �ٽ� ȸ�� �� ����
	{
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR *)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}

		//�� ������ Ŭ���̾�Ʈ ���� ���(�׽�Ʈ��)
		printf("\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
			inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		// hProccessClient ����
		hProccessClient[info.connectedP] = CreateThread(NULL, 0, ProccessClient, (LPVOID)client_sock, 0, NULL);
		if (hProccessClient[info.connectedP] == NULL) { closesocket(client_sock); }
		else { CloseHandle(hProccessClient[info.connectedP]); }		//�� ������ �ڵ鰪�� �ٷ� ���������� ���� �ʿ�

		// ������ �� ����
		info.connectedP++;
	}

	// closesocket()
	closesocket(listen_sock);

	// ���� ����
	WSACleanup();
	return 0;
}