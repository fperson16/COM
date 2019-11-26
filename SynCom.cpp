#include "SynCom.h"
#include <iostream>
#include <windows.h>
#include <stdint.h>

using namespace std;
 SynCom::SynCom()
{

}
 SynCom::~SynCom()
 {
	 close();
 }


//初始化
// 打开串口,成功返回true，失败返回false
bool SynCom::init(const char* com)
{
	HANDLE hCom = CreateFileA(
		(LPCSTR)com,									    // file name
		GENERIC_READ | GENERIC_WRITE,					    //允许读和写                    
		0,													// share mode
		NULL,												// 安全描述符
		OPEN_EXISTING,										// how to create
		0,													//NULL为同步发送，OVERLAPPED*为异步发送
		NULL												
	);

	if (INVALID_HANDLE_VALUE == hCom)
	{
		//记录日志..
		return false;
	}
	if (!SetupComm(hCom, 1024, 1024))
	{
		//记录日志..
		return false;
	}
	
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
	//设置读取超时时间，及ReadFlie最长等待时间
	ct.ReadIntervalTimeout = 0;
	ct.ReadTotalTimeoutConstant = 5000;
	ct.ReadTotalTimeoutMultiplier = 500;

	ct.WriteTotalTimeoutMultiplier = 500;
	ct.WriteTotalTimeoutConstant = 5000;

	SetCommTimeouts(hCom, &ct);//设置超时
	memcpy(pHandle, &hCom, sizeof(hCom));
	return true;
}

//串口发送消息 成功返回：成功发送的字节数。不成功返回0
unsigned int SynCom::send(string data)
{
	HANDLE hCom = *(HANDLE*)pHandle;
	uint32_t dwBytesWrite = data.length();
	BOOL bWriteStat = WriteFile(
			hCom, //串口句柄
			(char*)data.c_str(), //数据首地址
			dwBytesWrite, //要发送的数据字节数
			(LPDWORD)&dwBytesWrite, //DWORD*，用来接收返回成功发送的数据字节数
			NULL); //NULL为同步发送，OVERLAPPED*为异步发送

	if (!bWriteStat)
	{
		cout << "false";
		//记录日志..
		return 0;
	}
	return dwBytesWrite;
}

//读取缓存区数据，以string返回
bool SynCom::rec()
{
	string recStr = "";
	HANDLE hCom = *(HANDLE*)pHandle;
	char buf[1024];
	DWORD wCount = 1024; //成功读取的数据字节数
	BOOL bReadStat = ReadFile(hCom, //串口句柄
		buf, //数据首地址
		wCount, //要读取的数据最大字节数
		&wCount, //DWORD*,用来接收返回成功读取的数据字节数 
		NULL  //NULL为同步发送，OVERLAPPED*为异步
	);		
	if (!bReadStat)
	{
		//及日志..
		return bReadStat;
	}
	//将缓冲区字符拼成str
	for (int i = 0; i < wCount; i++)
	{
		if (buf[i] != -2)
			recStr += buf[i];
		else
			break;
	}

	cout << recStr.c_str();
	return true;
}

//关闭串口
void SynCom::close()
{
	HANDLE hCom = *(HANDLE*)this->pHandle;
	if (INVALID_HANDLE_VALUE != hCom)
	{
		CloseHandle(hCom);
		hCom = INVALID_HANDLE_VALUE;
	}
}

