#pragma once  // 防止头文件重复包含
#include <WinSock2.h>  // 包含 Winsock2 头文件，用于网络编程
#include <memory>  // 包含内存管理头文件

class ESockaddrIn {  // 定义 ESockaddrIn 类，用于封装网络地址相关操作
public:
	ESockaddrIn() {  // ESockaddrIn类的默认构造函数
		memset(&m_addr, 0, sizeof(m_addr));  // 将m_addr成员（sockaddr_in类型）的内存区域全部置0，初始化地址结构体
		m_port = -1;  // 将端口号成员m_port初始化为-1，表示未设置有效端口
	}

	ESockaddrIn(sockaddr_in addr) {  // 构造函数，参数为sockaddr_in类型的地址
		memcpy(&m_addr, &addr, sizeof(addr));  // 将传入的addr内容拷贝到成员变量m_addr中
		m_ip = inet_ntoa(m_addr.sin_addr);  // 将网络字节序的IP地址转换为字符串形式，赋值给m_ip
		m_port = ntohs(m_addr.sin_port);  // 将网络字节序的端口号转换为主机字节序，赋值给m_port
	}

	ESockaddrIn(UINT nIP, short nPort) {// 构造函数，参数为 UINT 类型 IP 和 short 类型端口
		m_addr.sin_family = AF_INET;  // 设置地址族为IPv4
		m_addr.sin_port = htons(nPort);  // 将主机字节序的端口号转换为网络字节序后，赋值给地址结构体的端口成员
		m_addr.sin_addr.s_addr = htonl(nIP);  // 将主机字节序的IP地址（整数形式）转换为网络字节序后，赋值给地址结构体的IP成员
		m_ip = inet_ntoa(m_addr.sin_addr);  // 将网络字节序的IP地址转换为字符串形式，赋值给成员变量m_ip
		m_port = nPort;  // 将主机字节序的端口号赋值给成员变量m_port
	}

	ESockaddrIn(const std::string& strIP, short nPort) { // 构造函数，参数为字符串类型 IP 和 short 类型端口
		m_ip = strIP;  // 将传入的字符串形式IP赋值给成员变量m_ip
		m_port = nPort;  // 将传入的端口号赋值给成员变量m_port
		m_addr.sin_family = AF_INET;  // 设置地址族为IPv4
		m_addr.sin_port = htons(nPort);  // 将主机字节序的端口号转换为网络字节序后，赋值给地址结构体的端口成员
		m_addr.sin_addr.s_addr = inet_addr(strIP.c_str());  // 将字符串形式的IP转换为网络字节序的整数后，赋值给地址结构体的IP成员
	}

	ESockaddrIn(const ESockaddrIn& addr) {  // 拷贝构造函数，参数为另一个 ESockaddrIn对象
		memcpy(&m_addr, &addr.m_addr, sizeof(addr.m_addr));  // 将传入对象的地址结构体内容拷贝到当前对象的成员变量m_addr中
		m_ip = addr.m_ip;  // 复制传入对象的IP字符串到当前对象的m_ip
		m_port = addr.m_port;  // 复制传入对象的端口号到当前对象的m_port
	}

	ESockaddrIn& operator=(const ESockaddrIn& addr) {  // 重载赋值运算符，实现ESockaddrIn对象的赋值
		if (this != &addr) {  // 避免自赋值
			memcpy(&m_addr, &addr.m_addr, sizeof(m_addr));  // 拷贝地址结构体内容
			m_ip = addr.m_ip;  // 拷贝IP字符串
			m_port = addr.m_port;  // 拷贝端口号
		}
		return *this;  // 返回当前对象的引用，支持链式赋值
	}


	operator sockaddr* () const {  // 类型转换运算符重载，将对象转换为 sockaddr* 类型（常量成员函数）
		return (sockaddr*)&m_addr;  // 返回地址结构体 m_addr 的地址，转换为 sockaddr* 类型
	}

	operator void* () const {  // 重载类型转换运算符，将对象转换为 void* 类型（常量成员函数）
		return (void*)&m_addr;  // 返回成员变量 m_addr 的地址，转换为 void* 类型
	}

	void update() {  // 定义update成员函数，用于更新IP和端口相关成员变量
		m_ip = inet_ntoa(m_addr.sin_addr);  // 将网络字节序的IP地址转换为字符串形式并赋值给m_ip
		m_port = ntohs(m_addr.sin_port);  // 将网络字节序的端口号转换为主机字节序并赋值给m_port
	}

	std::string GetIP() const {  // 获取 IP 地址的成员函数（常量成员函数）
		return m_ip;  // 返回成员变量 m_ip，表示 IP 地址的字符串形式
	}

	short GetPort() const {  // 获取端口号的成员函数（常量成员函数）
		return m_port;  // 返回成员变量 m_port，表示端口号
	}
	inline int size() const { return sizeof(sockaddr_in); }  // 内联成员函数size，常量成员函数，返回sockaddr_in结构体的大小
private:
	sockaddr_in m_addr;  // 封装的 sockaddr_in 类型地址结构体
	std::string m_ip;  // 存储 IP 地址的字符串
	short m_port;  // 存储端口号的 short 类型变量
};

class EBuffer : public std::string {  // 定义EBuffer类，公有继承自std::string
public:
	EBuffer(const char* str) {  // EBuffer类的构造函数，接收const char*类型的字符串
		this->resize(strlen(str));  // 调整EBuffer（继承自std::string）的大小为字符串str的长度
		memcpy((void*)this->c_str(), str, this->size());  // 将str指向的字符串内容拷贝到EBuffer的内存空间中，拷贝长度为size()（即str的长度）
	}
	EBuffer(size_t size = 0) : std::string() {  // 构造函数，参数为size，默认0
		if (size > 0) {  // 判断size是否大于0
			resize(size);  // 调整当前对象（假设为字符串或类似可调整大小的容器）的大小为size
			memset(*this, 0, this->size());  // 将当前对象的内存空间全部置0，长度为this->size()
		}
	}

	EBuffer(void* buffer, size_t size) : std::string() {  // EBuffer类的构造函数，接收void*类型的缓冲区和大小
		resize(size);  // 调整字符串大小为size
		memcpy((void*)c_str(), buffer, size);  // 将buffer指向的内存数据拷贝到当前字符串的内存中，拷贝大小为size
	}

	~EBuffer() {  // 析构函数
		std::string::~basic_string();  // 调用基类std::string的析构函数
	}
	operator char* () const { return (char*)this->c_str(); }  // 重载类型转换运算符，转为char*（常量成员函数）
	operator const char* () const { return this->c_str(); }  // 重载类型转换运算符，转为const char*（常量成员函数）
	operator BYTE* () const { return (BYTE*)this->c_str(); }  // 重载类型转换运算符，转为BYTE*（常量成员函数）
	operator void* () const { return (void*)this->c_str(); }  // 重载类型转换运算符，转为void*（常量成员函数）
	void Update(void* buffer, size_t size) {  // 定义Update方法，接收void*类型的缓冲区和大小
		resize(size);  // 调整当前对象（继承自std::string的类对象，如EBuffer）的大小为size
		memcpy((void*)c_str(), buffer, size);  // 将buffer指向的内存数据拷贝到当前对象的内存空间，拷贝大小为size
	}
};

enum class ETYPE {  // 定义枚举类 ETYPE，用于区分套接字类型
	ETypeTCP = 1,  // TCP 类型，值为 1
	ETypeUDP  // UDP 类型
};

class ESocket {  // 定义 ESocket 类，封装套接字相关操作
public:
	// 构造函数，参数为套接字类型（默认 TCP）和协议（默认 0），内部创建对应类型的套接字并初始化成员变量
	ESocket(ETYPE nType = ETYPE::ETypeTCP, int nProtocol = 0) {
		m_socket = socket(PF_INET, (int)nType, nProtocol);
		m_type = nType;
		m_protocol = nProtocol;
	}

	ESocket(const ESocket& sock) {  // 拷贝构造函数
		m_socket = socket(PF_INET, (int)sock.m_type, m_protocol);  // 根据源对象的套接字类型和当前对象的协议，创建新的套接字并赋值给 m_socket
		m_type = sock.m_type;  // 将源对象的套接字类型赋值给当前对象的 m_type
		m_protocol = sock.m_protocol;  // 将源对象的协议类型赋值给当前对象的 m_protocol
		m_addr = sock.m_addr; // 拷贝源对象的地址结构体到当前对象的 m_addr
	}

	~ESocket() {  // 析构函数，关闭套接字，释放资源
		close();
	}

	ESocket& operator=(const ESocket& sock) {  // 重载赋值运算符，实现 ESocket 对象的赋值操作
		if (this != &sock) {  // 避免自赋值
			closesocket(m_socket);  // 关闭当前对象的套接字
			m_socket = socket(PF_INET, (int)sock.m_type, sock.m_protocol);  // 创建与源对象同类型、同协议的新套接字
			m_type = sock.m_type;  // 复制套接字类型
			m_protocol = sock.m_protocol;  // 复制协议类型
			m_addr = sock.m_addr; // 拷贝源对象的地址结构体到当前对象的 m_addr
		}
		return *this;  // 返回当前对象的引用，支持链式赋值
	}

	// 类型转换运算符重载，将 ESocket 对象转换为 SOCKET 类型（常量版本和普通版本）
	operator SOCKET() const { return m_socket; }
	operator SOCKET() { return m_socket; }
	bool operator==(SOCKET sock) const {  // 重载==运算符（常量成员函数，不修改对象状态），判断当前套接字描述符是否等于传入的整数sock
		return m_socket == sock;  // 返回m_socket与sock的比较结果
	}

	int listen(int backlog = 5) {  // 封装系统的listen函数，默认backlog为5
		if (m_type != ETYPE::ETypeTCP) return -1;  // 若套接字类型不是TCP，返回-1
		return ::listen(m_socket, backlog);  // 调用系统listen函数，监听套接字m_socket，等待连接的队列长度为backlog
	}

	int bind(const std::string& ip, short port) {  // 封装系统的bind函数，绑定IP和端口
		m_addr = ESockaddrIn(ip, port);  // 初始化地址结构体m_addr，参数为ip和port
		return ::bind(m_socket, m_addr, m_addr.size());  // 调用系统bind函数，将套接字m_socket绑定到m_addr地址，地址长度为m_addr.size()
	}

	int send(const EBuffer& buffer) {  // 封装系统send函数，发送EBuffer类型的缓冲区数据
		return ::send(m_socket, buffer, buffer.size(), 0);  // 调用系统send函数，向m_socket对应的套接字发送buffer中的数据，数据长度为buffer.size()，标志为0
	}

	int recv(EBuffer& buffer) {  // 封装系统recv函数，接收数据到EBuffer对象
		return ::recv(m_socket, buffer, buffer.size(), 0);  // 调用系统recv，从m_socket接收数据到buffer，长度为buffer.size()，标志0
	}

	int sendto(const EBuffer& buffer, const ESockaddrIn& to) {  // 封装系统sendto，向指定地址发送EBuffer数据
		return ::sendto(m_socket, buffer, buffer.size(), 0, to, to.size());  // 调用系统sendto，发送buffer数据到to地址，标志0
	}

	int recvfrom(EBuffer& buffer, ESockaddrIn& from) {  // 封装系统recvfrom，从指定地址接收数据到EBuffer
		int len = from.size();  // 获取地址结构体大小
		int ret = ::recvfrom(m_socket, buffer, buffer.size(), 0, from, &len);  // 调用系统recvfrom函数，从m_socket接收数据到buffer，来源地址存入from，地址长度存入len
		if (ret > 0) {  // 如果接收成功（ret大于0）
			from.update();  // 调用from对象的update方法，更新地址相关信息
		}
		return ret;  // 返回接收的字节数ret	
	}

	void close() {  // 定义close方法，用于关闭套接字
		if (m_socket != INVALID_SOCKET) {  // 判断套接字m_socket是否有效（不是INVALID_SOCKET）
			closesocket(m_socket);  // 调用closesocket函数关闭套接字
			m_socket = INVALID_SOCKET;  // 将套接字标识设为INVALID_SOCKET，表示套接字已无效
		}
	}

private:
	SOCKET m_socket;  // 套接字描述符
	ETYPE m_type;  // 套接字类型（TCP 或 UDP）
	int m_protocol;  // 协议类型
	ESockaddrIn m_addr;  // 封装的地址结构体
};

typedef std::shared_ptr<ESocket> ESOCKET;  // 定义类型别名 ESOCKET，代表 std::shared_ptr<ESocket>，用于简化智能指针的使用


