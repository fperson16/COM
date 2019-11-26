#pragma once
#include <iostream>

//同步串口
class SynCom
{
private:

	uint8_t pHandle[4];
public:
	SynCom();
	~SynCom();

public:
	//初始化
	bool init(const char* com);
	//发送
	unsigned int send(std::string data);
	//接收
	bool rec();
	//关闭
	void close();
};


