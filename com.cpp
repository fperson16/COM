
#include <windows.h>
#include <iostream>
#include "SynCom.h"
#include "AsynCom.h"
#include <thread>
#include <string>
using namespace std;

//同步接收数据线程
void recThread(SynCom* w)
{
	while (true)
	{
		if (!w->rec())
		{
			break;
		}
	}
}

//异步接收数据线程
void recAsynThread(AsynCom* w)
{
	while (true)
	{
		string str = w->rec(w);
		cout << str<<"\n";

		//当串口状态false时退出
		if (!w->comStatic)
			break;
	}
}

//同步发送数据线程
void sendThread(SynCom* w)
{
	cout << "串口开启成功\n" << "请输入要发送的内容\n";
	while (true)
	{
		string str;
		cin >> str;
		unsigned int dataNum = w->send(str);
		cout << "发送字数：" << dataNum << "\n";
		if (dataNum == 0)
		{
			break;
		}
	}
}

int main()
{
	//同步、异步
	enum comType {Syn, Asyn};
	comType type = Asyn;

	cout << "请输入要打开的串口\n";
	string comPort;
	cin >> comPort;
	//str转为char*
	//char* com = new char[strlen(comPort.c_str()) + 1];
	char* com = new char[comPort.length() + 1];
	strcpy_s(com, comPort.length() + 1, comPort.c_str());

	//strcpy_s(com, strlen(comPort.c_str()) + 1, comPort.c_str());

	switch (type)
	{
	case Syn:
		{
			//同步模式
			SynCom w;
			if (w.init(com))
			{
				thread* rec = new thread(recThread, &w);
				thread* send = new thread(sendThread, &w);
				rec->join();
				send->join();
			}
			else
			{
				cout << "串口开启失败error：\n";
			}
			break;
		}
	case Asyn:
		{
			AsynCom w;
			if (w.init(com))
			{
				cout << "串口开启成功\n" << "请输入要发送的内容\n";
				thread* rec = new thread(recAsynThread, &w);
				while (true)
				{
					string data;
					cin >> data;
					int dataLen = w.send(data);
					cout << "发送字数：" << dataLen << "\n";

					if (dataLen == 0)
						break;
				}
			}	
			else
			{
				cout << "串口开启失败error：\n";
				return 0;
			}
			break;
		}
	default:
		cout << "需要配置同步或异步传输";
		break;
	}
	while (true)
	{

	}
	return 0;
}

