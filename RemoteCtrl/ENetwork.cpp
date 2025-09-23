#include "pch.h"
#include "ENetwork.h"

EServer::EServer(const EServerParameter& param) :m_stop(false), m_args(NULL)
{  // EServer类的构造函数，接收EServerParameter类型的常量引用param
	m_params = param;  // 将传入的param赋值给成员变量m_params，用于存储服务器参数
	m_thread.UpdateWorker(ThreadWorker(this, (FUNCTYPE)&EServer::threadfunc));  // 更新线程的工作函数，设置为当前EServer对象的threadFunc成员函数
}

EServer::~EServer()
{
	Stop();  // 调用Stop方法，停止服务器
}

int EServer::Invoke(void* arg)
{
	m_sock.reset(new ESocket(m_params.m_type));  // 创建UDP类型的ESocket对象并交由智能指针m_sock管理
	if (*m_sock == INVALID_SOCKET) {  // 判断套接字是否无效
		printf("%s(%d):%s ERROR(%d)!!!\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());  // 打印错误信息，包含文件、行号、函数及错误码
		return -1;  // 返回错误码-1
	}
	if (m_params.m_type == ETYPE::ETypeTCP) {  // 判断服务器参数中指定的类型是否为TCP
		if (m_sock->listen() == -1) {  // 调用套接字的listen方法监听连接，判断是否监听失败
			return -2;  // 监听失败返回错误码-2
		}
	}
	ESockaddrIn client;  // 定义ESockaddrIn类型的client对象，用于存储客户端地址信息
	if (-1 == m_sock->bind(m_params.m_ip, m_params.m_port)) {  // 调用套接字的bind方法绑定IP和端口，判断是否绑定失败
		printf("%s(%d):%s ERROR(%d)!!!\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());  // 打印错误信息
		return -3;  // 返回错误码-3
	}
	if (m_thread.Start() == false) return -4;  // 调用线程的Start方法启动线程，若启动失败则返回错误码-4
	m_args = arg;  // 将传入的参数arg赋值给成员变量m_args
	return 0;
}

int EServer::Send(ESOCKET& client, const EBuffer& buffer)
{
	int ret = m_sock->send(buffer); // 调用套接字的send方法发送数据，TODO：待优化，发送虽然成功，但是不完整！！
	if (m_params.m_send) m_params.m_send(m_args, client, ret); // 如果设置了发送回调函数，调用该回调函数
	return ret; // 返回发送操作的返回值
}

int EServer::Sendto(ESockaddrIn& addr, const EBuffer& buffer)
{
	int ret = m_sock->sendto(buffer, addr); // 调用套接字的sendto方法向指定地址发送数据，TODO：待优化，发送虽然成功，但是不完整！！
	if (m_params.m_sendto) m_params.m_sendto(m_args, addr, ret); // 如果设置了发送到指定地址的回调函数，调用该回调函数
	return ret; // 返回发送操作的返回值
}

int EServer::Stop()
{
	if (m_stop == false) {  // 判断服务器是否未停止
		m_sock->close();  // 调用套接字的close方法关闭套接字
		m_stop = true;  // 将停止标识设为true
		m_thread.Stop();  // 调用线程的Stop方法停止线程
	}
	return 0;  // 返回0表示停止操作执行完成
}

int EServer::threadfunc()
{
	if (m_params.m_type == ETYPE::ETypeTCP) {  // 判断服务器参数中的类型是否为TCP
		return threadTCPFunc();  // 如果是TCP类型，调用threadTCPFunc函数并返回其返回值
	}
	else {
		return threadUDPFunc();  // 如果不是TCP类型（即UDP类型等），调用threadUDPFunc函数并返回其返回值
	}
}

int EServer::threadUDPFunc()
{
	EBuffer buf(1024 * 256);  // 创建大小为1024*256的EBuffer对象，用于接收数据
	ESockaddrIn client;  // 定义ESockaddrIn对象，用于存储客户端地址信息
	int ret = 0;  // 用于存储recvfrom的返回值
	while (!m_stop) {  // 当m_stop为false时，持续循环接收数据
		ret = m_sock->recvfrom(buf, client);  // 调用套接字的recvfrom方法接收数据到buf，客户端地址存入client
		if (ret > 0) {  // 如果接收成功（返回值大于0）
			client.update();  // 更新客户端地址相关信息
			if (m_params.m_recvfrom != NULL) {  // 如果设置了接收回调函数
				m_params.m_recvfrom(m_args, buf, client);  // 调用接收回调函数，处理接收到的数据
			}
		}
		else {  // 接收失败或无数据
			printf("%s(%d):%s ERROR(%d)!!! ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);  // 打印错误信息
			break;  // 跳出循环
		}
	}
	if (m_stop == false) m_stop = true;  // 如果m_stop原本为false，将其设为true，停止循环
	m_sock->close();  // 关闭套接字
	printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);  // 打印当前文件、行号、函数信息
	return 0;  // 返回0表示函数执行结束
}

int EServer::threadTCPFunc()
{
	return 0;
}

EServerParameter::EServerParameter(const std::string& ip, short port, ETYPE type, AcceptFunc acceptf, RecvFunc recvf, SendFunc sendf, RecvFromFunc recvfromf, SendToFunc sendtof)
{
	m_ip = ip;  // 将传入的ip赋值给成员变量m_ip，用于存储服务器IP地址
	m_port = port;  // 将传入的port赋值给成员变量m_port，用于存储服务器端口
	m_type = type;  // 将传入的type赋值给成员变量m_type，用于存储服务器类型（如TCP、UDP）
	m_accept = acceptf;  // 将传入的acceptf（接收连接的回调函数指针）赋值给成员变量m_accept
	m_recv = recvf;  // 将传入的recvf（接收数据的回调函数指针）赋值给成员变量m_recv
	m_send = sendf;  // 将传入的sendf（发送数据的回调函数指针）赋值给成员变量m_send
	m_recvfrom = recvfromf;  // 将传入的recvfromf（UDP接收数据的回调函数指针）赋值给成员变量m_recvfrom
	m_sendto = sendtof;  // 将传入的sendtof（UDP发送数据的回调函数指针）赋值给成员变量m_sendto
}

EServerParameter& EServerParameter::operator<<(AcceptFunc func)
{
	m_accept = func;  // 将传入的函数指针func赋值给成员变量m_accept
	return *this;  // 返回当前对象的引用，支持链式调用
}

EServerParameter& EServerParameter::operator<<(RecvFunc func)
{
	m_recv = func;  // 将传入的函数指针func赋值给成员变量m_recv
	return *this;  // 返回当前对象的引用，支持链式调用
}

EServerParameter& EServerParameter::operator<<(SendFunc func)
{
	m_send = func;  // 将传入的函数指针func赋值给成员变量m_send
	return *this;  // 返回当前对象的引用，支持链式调用
}

EServerParameter& EServerParameter::operator<<(RecvFromFunc func)
{
	m_recvfrom = func;  // 将传入的函数指针func赋值给成员变量m_recvfrom
	return *this;  // 返回当前对象的引用，支持链式调用
}

EServerParameter& EServerParameter::operator<<(SendToFunc func)
{
	m_sendto = func;  // 将传入的函数指针func赋值给成员变量m_sendto
	return *this;  // 返回当前对象的引用，支持链式调用
}

EServerParameter& EServerParameter::operator<<(const std::string& ip)
{
	m_ip = ip;  // 将传入的字符串ip赋值给成员变量m_ip
	return *this;  // 返回当前对象的引用，支持链式调用
}

EServerParameter& EServerParameter::operator<<(short port)
{
	m_port = port;  // 将传入的端口号port赋值给成员变量m_port
	return *this;  // 返回当前对象的引用，支持链式调用
}

EServerParameter& EServerParameter::operator<<(ETYPE type)
{
	m_type = type;  // 将传入的类型type赋值给成员变量m_type
	return *this;  // 返回当前对象的引用，支持链式调用
}

EServerParameter& EServerParameter::operator>>(AcceptFunc& func)
{
	func = m_accept;  // 将成员变量m_accept的值赋给传入的函数指针引用func
	return *this;  // 返回当前对象的引用，支持链式调用
}

EServerParameter& EServerParameter::operator>>(RecvFunc& func)
{
	func = m_recv;  // 将成员变量m_recv的值赋给传入的函数指针引用func
	return *this;  // 返回当前对象的引用，支持链式调用
}

EServerParameter& EServerParameter::operator>>(SendFunc& func)
{
	func = m_send;  // 将成员变量m_send的值赋给传入的函数指针引用func
	return *this;  // 返回当前对象的引用，支持链式调用
}

EServerParameter& EServerParameter::operator>>(RecvFromFunc& func)
{
	func = m_recvfrom;  // 将成员变量m_recvfrom的值赋给传入的函数指针引用func
	return *this;  // 返回当前对象的引用，支持链式调用
}

EServerParameter& EServerParameter::operator>>(SendToFunc& func)
{
	func = m_sendto;  // 将成员变量m_sendto的值赋给传入的函数指针引用func
	return *this;  // 返回当前对象的引用，支持链式调用
}

EServerParameter& EServerParameter::operator>>(std::string& ip)
{
	ip = m_ip;  // 将成员变量m_ip的值赋给传入的字符串引用ip
	return *this;  // 返回当前对象的引用，支持链式调用
}

EServerParameter& EServerParameter::operator>>(short& port)
{
	port = m_port;  // 将成员变量m_port的值赋给传入的端口号引用port
	return *this;  // 返回当前对象的引用，支持链式调用
}

EServerParameter& EServerParameter::operator>>(ETYPE& type)
{
	type = m_type;  // 将成员变量m_type的值赋给传入的类型引用type
	return *this;  // 返回当前对象的引用，支持链式调用
}

EServerParameter::EServerParameter(const EServerParameter& param)
{
	m_ip = param.m_ip;  // 复制传入对象param的m_ip成员变量
	m_port = param.m_port;  // 复制传入对象param的m_port成员变量
	m_type = param.m_type;  // 复制传入对象param的m_type成员变量
	m_accept = param.m_accept;  // 复制传入对象param的m_accept成员变量
	m_recv = param.m_recv;  // 复制传入对象param的m_recv成员变量
	m_send = param.m_send;  // 复制传入对象param的m_send成员变量
	m_recvfrom = param.m_recvfrom;  // 复制传入对象param的m_recvfrom成员变量
	m_sendto = param.m_sendto;  // 复制传入对象param的m_sendto成员变量
}

EServerParameter& EServerParameter::operator=(const EServerParameter& param)
{
	if (this != &param) {  // 避免自赋值
		m_ip = param.m_ip;  // 复制传入对象param的m_ip成员变量
		m_port = param.m_port;  // 复制传入对象param的m_port成员变量
		m_type = param.m_type;  // 复制传入对象param的m_type成员变量
		m_accept = param.m_accept;  // 复制传入对象param的m_accept成员变量
		m_recv = param.m_recv;  // 复制传入对象param的m_recv成员变量
		m_send = param.m_send;  // 复制传入对象param的m_send成员变量
		m_recvfrom = param.m_recvfrom;  // 复制传入对象param的m_recvfrom成员变量
		m_sendto = param.m_sendto;  // 复制传入对象param的m_sendto成员变量
	}
	return *this;  // 返回当前对象的引用，支持链式赋值
}



