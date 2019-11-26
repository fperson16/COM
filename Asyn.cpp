#pragma once
#include "AsynCom.h"
#include <iostream>
#include <windows.h>
#include <thread>
#include <string>
using namespace std;

AsynCom::AsynCom()
{
	memset(&ovSend, 0, sizeof(ovSend));
	memset(&ovRec, 0, sizeof(ovRec));
	memset(&ovWait, 0, sizeof(ovWait));
}
AsynCom::~AsynCom()
{
	AsynCom::close();
}

//初始化，配置串口
bool AsynCom::init(const char* com)
{
	HANDLE hCom = CreateFileA(
		(LPCSTR)com,									    // file name
		GENERIC_READ | GENERIC_WRITE,					    //允许读和写                    
		0,													// share mode
		NULL,												// 安全描述符
		OPEN_EXISTING,										// how to create
		FILE_FLAG_OVERLAPPED,								//指向接收读取的字节数的变量的指针
		NULL												//NULL为同步发送，OVERLAPPED*为异步发送
	);

	if (INVALID_HANDLE_VALUE == hCom)
	{
		//记录日志..
		return false;
	}
	SetupComm(hCom, 1024, 1024);
	//配置dcb
	DCB dcb;
	GetCommState(hCom, &dcb);
	dcb.DCBlength = sizeof(dcb);
	dcb.Parity = NOPARITY;
	dcb.BaudRate = CBR_9600;
	dcb.ByteSize = 8; // 数据位
	dcb.StopBits = ONESTOPBIT;
	SetCommState(hCom, &dcb);

	PurgeComm(hCom, PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR | PURGE_TXABORT);//清空缓刚刚申请的缓冲区

	COMMTIMEOUTS ct;
	//异步抄的网上配置，有待考究
	ct.ReadIntervalTimeout = MAXDWORD;
	ct.ReadTotalTimeoutConstant = 0;
	ct.ReadTotalTimeoutMultiplier = 0;
	ct.WriteTotalTimeoutMultiplier = 500;
	ct.WriteTotalTimeoutConstant = 5000;

	SetCommTimeouts(hCom, &ct);//设置超时
	memcpy(pHandle, &hCom, sizeof(hCom));

	//创建事件对象、安全描述符、复位方式（false为自动）、初始状态、对象名称
	ovRec.hEvent = CreateEvent(NULL, false, false, NULL);//接收事件
	ovSend.hEvent = CreateEvent(NULL, false, false, NULL);//发送事件
	ovWait.hEvent = CreateEvent(NULL, false, false, NULL);//等待事件

	SetCommMask(hCom, EV_ERR | EV_RXCHAR);//设置接受事件

	//创建读取线程
	//thread* recThread = new thread(&AsynCom::rec,this);
	//recThread->detach();
	comStatic = true;
	return true;
}

int AsynCom::send(string data)
{
	HANDLE hCom = *(HANDLE*)pHandle;
	DWORD dwBytesWrite = data.length();
	if( dwBytesWrite <= 0 )
		return 0;
	PurgeComm(hCom, PURGE_TXCLEAR | PURGE_TXABORT);
	ovSend.Offset = 0;
	BOOL bWriteStat = WriteFile(
		hCom, //串口句柄
		(char*)data.c_str(), //数据首地址
		dwBytesWrite, //要发送的数据字节数
		(LPDWORD)&dwBytesWrite, //DWORD*，用来接收返回成功发送的数据字节数
		&ovSend); //NULL为同步发送，OVERLAPPED*为异步发送

	if (FALSE == bWriteStat && GetLastError() == ERROR_IO_PENDING)//后台读取
	{
		//写入事件临时没有用异步效果
		if (FALSE == GetOverlappedResult(hCom, &ovSend, &dwBytesWrite, TRUE))
		{
			return 0;
		}
	}
	return dwBytesWrite;
}


// map<"false", string>map<"true", string>
string AsynCom::rec( AsynCom *w)
{
	string str;
	DWORD waitEvent = 0, bytes = 0;
	bool status = FALSE;
	uint8_t readBuf[1024];
	DWORD error;
	COMSTAT cs;//通信事件实时状态的结构体，由clearCommError填充
	HANDLE hCom = *(HANDLE*)w->pHandle;

	waitEvent = 0;
	w->ovWait.Offset = 0;
	status = WaitCommEvent(hCom, &waitEvent, &w->ovWait);
	if (FALSE == status && GetLastError() == ERROR_IO_PENDING)//
	{
		//如果缓存中无数据线程会停在此，如果hCom关闭会立即返回false
		status = GetOverlappedResult(hCom, &w->ovWait, &bytes, TRUE);
	}
	ClearCommError(hCom, &error, &cs);
	if (TRUE == status //等待事件成功
		&& waitEvent & EV_RXCHAR//缓存中有数据到达
		&& cs.cbInQue > 0)//有数据
	{
		bytes = 0;
		w->ovRec.Offset = 0;
		memset(readBuf, 0, sizeof(readBuf));
		//数据已经到达缓存区，立即开始读，从Offset开始读，返回true
		status = ReadFile(hCom, readBuf, sizeof(readBuf), &bytes, &w->ovRec);
		if (status != FALSE)
		{
			//cout << "Read:" << readBuf << "   Len:" << bytes << endl;
			str = string((char*)readBuf, bytes);
			
		}
		PurgeComm(hCom, PURGE_RXCLEAR | PURGE_RXABORT);//结束清空缓冲区
	}
	return str;
}

void AsynCom::close()
{
	HANDLE hCom = *(HANDLE*)this->pHandle;
	CloseHandle(hCom);
	comStatic = false;
	//关闭串口
	if (INVALID_HANDLE_VALUE != hCom)
	{
		CloseHandle(hCom);
		hCom = INVALID_HANDLE_VALUE;
	}
	//关闭事件
	if (NULL != ovRec.hEvent)
	{
		CloseHandle(ovRec.hEvent);
		ovRec.hEvent = NULL;
	}
	if (NULL != ovSend.hEvent)
	{
		CloseHandle(ovSend.hEvent);
		ovSend.hEvent = NULL;
	}
	if (NULL != ovWait.hEvent)
	{
		CloseHandle(ovWait.hEvent);
		ovWait.hEvent = NULL;
	}
}


