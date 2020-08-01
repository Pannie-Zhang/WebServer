/*********************************************************************************************
1.WSADATA，一种数据结构。这个结构被用来存储被WSAStartup函数调用后返回的Windows Sockets数据。
它包含Winsock.dll执行的数据。
结构原型：　　
struct WSAData {
　　WORD wVersion;//Windows Sockets DLL期望调用者使用的Windows Sockets规范的版本
  　WORD wHighVersion;//这个DLL能够支持的Windows Sockets规范的最高版本。通常它与wVersion相同。
	char szDescription[WSADESCRIPTION_LEN+1];//以null结尾的ASCII字符串，Windows Sockets DLL将对Windows Sockets实现的描述拷贝到这个字符串中，包括制造商标识
	char szSystemStatus[WSASYSSTATUS_LEN+1];//以null结尾的ASCII字符串，Windows Sockets DLL把有关的状态或配置信息拷贝到该字符串中
	unsigned short iMaxSockets;//单个进程能够打开的socket的最大数目
	unsigned short iMaxUdpDg;//iMaxUdpDg Windows Sockets应用程序能够发送或接收的最大的用户数据包协议（UDP）的数据包大小，以字节为单位
	char *lpVendorInfo;//lpVendorInfo 指向销售商的数据结构的指针
};

WSAStartup 给WSADATA 结构的成员分配些列值：
　　wVersion wVersionRequested 数值
  　wHighVersion wVersionRequested 数值
	szDescription NULL 字符串
	szSystemStatus NULL 字符串
	iMaxSockets 100
	iMaxUdpDg 0
	lpVendorInfo NULL  

当一个应用程序调用WSAStartup函数时，操作系统根据请求的Socket版本来搜索相应的Socket库，然后绑定找到的Socket库到该应用程序中。
以后应用程序就可以调用所请求的Socket库中的其它Socket函数了

argc：命令行总的参数的个数,即argv中元素的格式。
*argv[]: 字符串数组,用来存放指向你的字符串参数的指针数组,每一个元素指向一个参数
argv[0]:指向程序的全路径名
argv[1]:指向在DOS命令行中执行程序名后的第一个字符串。
argv[2]:指向第二个字符串

网络字节顺序与本地字节顺序之间的转换函数：
htonl()--"Host to Network Long"
ntohl()--"Network to Host Long"
htons()--"Host to Network Short"
ntohs()--"Network to Host Short"
网络字节顺序NBO（Network Byte Order）：
按从高到低的顺序存储，在网络上使用统一的网络字节顺序，可以避免兼容性问题。
主机字节顺序（HBO，Host Byte Order）：
不同的机器HBO不相同，与CPU设计有关，数据的顺序是由cpu决定的,而与操作系统无关。

recv()是编程语言函数。
函数原型int recv( _In_ SOCKET s, _Out_ char *buf, _In_ int len, _In_ int flags);
第二个参数指明一个缓冲区，该缓冲区用来存放recv函数接收到的数据；
第三个参数指明buf的长度；
第四个参数一般置0。
（1）recv先等待s的发送缓冲中的数据被协议传送完毕，如果协议在传送s的发送缓冲中的数据时出现网络错误，那么recv函数返回SOCKET_ERROR，
（2）如果s的发送缓冲中没有数据或者数据被协议成功发送完毕后，recv先检查套接字s的接收缓冲区，如果s接收缓冲区中没有数据或者协议正在接收数据，
那么recv就一直等待，直到协议把数据接收完毕。当协议把数据接收完毕，recv函数就把s的接收缓冲中的数据copy到buf中
（注意协议接收到的数据可能大于buf的长度，所以在这种情况下要调用几次recv函数才能把s的接收缓冲中的数据copy完。
recv函数仅仅是copy数据，真正的接收数据是协议来完成的）。
recv函数返回其实际copy的字节数。如果recv在copy时出错，那么它返回SOCKET_ERROR；如果recv函数在等待协议接收数据时网络中断了，那么它返回0
*********************************************************************************************/
#define  _CRT_SECURE_NO_WARNINGS 
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <winsock2.h>


#pragma comment(lib, "ws2_32.lib")  /* WinSock使用的库函数  winsock2.h是套接字接口， ws2_32.lib是套接字实现*/
/* 定义常量 */
#define HTTP_DEF_PORT        80     /* 连接的缺省端口 */
#define HTTP_BUF_SIZE      1024     /* 缓冲区的大小 */
#define HTTP_FILENAME_LEN   256     /* 文件名长度 */

/* 定义文件类型对应的 Content-Type */
struct doc_type
{
	char *suffix; /* 文件后缀 */
	char *type;   /* Content-Type */
};

struct doc_type file_type[] =
{
	{ "html",    "text/html" },
	{ "gif",     "image/gif" },
	{ "jpeg",    "image/jpeg" },
	{ "png",    "image/png" },
	{ "jpg",    "image/jpg" },
	{ NULL,      NULL }
};
char *http_res_hdr_tmpl = "HTTP/1.1 200 OK\r\nServer: Huiyong's Server <0.1>\r\n"
"Accept-Ranges: bytes\r\nContent-Length: %d\r\nConnection: close\r\n"
"Content-Type: %s\r\n\r\n";
/**************************************************************************
*
* 函数功能: 根据文件后缀查找对应的 Content-Type.
*
* 参数说明: [IN] suffix, 文件名后缀;
*
* 返 回 值: 成功返回文件对应的 Content-Type, 失败返回 NULL.
*
**************************************************************************/



char *http_get_type_by_suffix(const char *suffix)
{
	struct doc_type *type;

	for (type = file_type; type->suffix; type++)
	{
		if (strcmp(type->suffix, suffix) == 0)
			return type->type;
	}

	return NULL;
}
/**************************************************************************
*
* 函数功能: 解析请求行, 得到文件名及其后缀. 请求行格式:
*           [GET http://www.baidu.com:8080/index.html HTTP/1.1]
*
* 参数说明: [IN]  buf, 字符串指针数组;
*           [IN]  buflen, buf 的长度;
*           [OUT] file_name, 文件名;
*           [OUT] suffix, 文件名后缀;
*
* 返 回 值: void.
*
**************************************************************************/
void http_parse_request_cmd(char *buf, int buflen, char *file_name, char *suffix){
	int length = 0;
	char *begin, *end, *bias;
	/*char *temp ;
	temp = buf;*/
	//查找URL的开始位置
	//strchr函数功能为在一个串中查找给定字符的第一个匹配之处。函数原型为：char *strchr(const char *str, int c)，即在参数 str 所指向的字符串中搜索第一次出现字符 c（一个无符号字符）的位置。strchr函数包含在C 标准库 <string.h>中。
	/*printf("buf=");
	for (size_t i = 0; i < buflen; i++)
	{
		printf(" %c", *temp);
		temp++;
	}
	printf("\n");*/

	begin = strchr(buf,' ');
	begin += 1;

	//查找URL的结束位置
	end = strchr(begin,' ');//表示buf从begin开始的后面的字符串
	*end = 0;
	//与 strchr 函数一样，它同样表示在字符串 s 中查找字符 c，返回字符 c 第一次在字符串 s 中出现的位置，如果未找到字符 c，则返回 NULL。但两者唯一不同的是，strrchr 函数在字符串 s 中是从后到前（或者称为从右向左）查找字符 c，找到字符 c 第一次出现的位置就返回，返回值指向这个位置
	bias = strrchr(begin, '/');
	length = end - bias;

	//找到文件名的开始位置
	if ((*bias == '/')||(*bias=='\\')) {
		bias++;
		length--;
	}
	/* 得到文件名 */
	if (length > 0) {
		//memcpy指的是C和C++使用的内存拷贝函数，函数原型为void *memcpy(void *destin, void *source, unsigned n)；函数的功能是从源内存地址的起始位置开始拷贝若干个字节到目标内存地址中，即从源source中拷贝n个字节到目标destin中。
		memcpy(file_name, bias, length);//从bias中拷贝length个字节到file_name里面
		file_name[length] = 0;

		begin = strchr(file_name, '.');
		//strcpy是一种C语言的标准库函数，strcpy把含有'\0'结束符的字符串复制到另一个地址空间，返回值的类型为char*。
		if (begin)
			strcpy(suffix,begin+1);//得到后缀
	}
}



/**************************************************************************
*
* 函数功能: 向客户端发送 HTTP 响应.
*
* 参数说明: [IN]  buf, 字符串指针数组;
*           [IN]  buf_len, buf 的长度;
*
* 返 回 值: 成功返回非0, 失败返回0.
*
**************************************************************************/


int http_send_response(SOCKET soc, char *buf, int buf_len) {
	int read_len, file_len, hdr_len, send_len;
	char *type;
	char read_buf[HTTP_BUF_SIZE];
	char http_header[HTTP_BUF_SIZE];
	char file_name[HTTP_FILENAME_LEN] = "index.html", suffix[16] = "html";
	FILE *res_file;

	/*得到文件名和后缀*/
	http_parse_request_cmd(buf, buf_len, file_name, suffix);

	res_file = fopen(file_name, "rb+");/* 用二进制格式打开文件 */
	if (res_file == NULL) {
		printf("[Web] The file [%s] is not existed\n", file_name);
		return 0;
	}
	fseek(res_file,0,SEEK_END);//文件指针定位到文件末尾，偏移0个字节
	file_len = ftell(res_file);//ftell返回当前文件指针,首先将文件的当前位置移到文件的末尾，然后调用函数ftell()获得当前位置相对于文件首的位移，该位移值等于文件所含字节数。
	fseek(res_file, 0, SEEK_SET);//文件指针定位到文件开头，偏移0个字节

	type = http_get_type_by_suffix(suffix);/* 文件对应的 Content-Type */
	if (type == NULL) {
		printf("[Web] There is not the related content type\n");
		return 0;
	}

	/* 构造 HTTP 首部，并发送 */
	hdr_len = sprintf(http_header, http_res_hdr_tmpl, file_len, type);
	send_len = send(soc,http_header,hdr_len,0);

	if (send_len == SOCKET_ERROR) {
		fclose(res_file);
		printf("[Web] Fail to send,error=%d\n", WSAGetLastError());
		return 0;
	}
	do/* 发送文件, HTTP 的消息体 */
	{
		read_len = fread(read_buf, sizeof(char), HTTP_BUF_SIZE, res_file);
		if (read_len > 0) {
			send_len = send(soc, read_buf, read_len, 0);
			file_len -= read_len;
		   }
		}while ((read_len > 0) && (file_len > 0));

		fclose(res_file);
		return 1;   
}

int main(int argc,char **argv) {
	WSADATA wsa_data;
	SOCKET srv_soc = 0, acpt_soc;/* socket 句柄 */
	struct sockaddr_in serv_addr;/* 服务器地址  */
	struct sockaddr_in from_addr;/* 客户端地址  */

	unsigned short port = HTTP_DEF_PORT;
	char recv_buf[HTTP_BUF_SIZE];//从客户端接收的信息
	unsigned long from_len = sizeof(from_addr);
	int result = 0;
	int recv_len;

	if (argc == 2) {/* 端口号 */
		port = atoi(argv[1]);
	}

	WSAStartup(MAKEWORD(2,0),&wsa_data);/* 程序要使用2.0版本的Socket, 初始化 WinSock 资源 */
	
	srv_soc = socket(AF_INET,SOCK_STREAM,0);  //创建TCP套接字,AF_INET 表示 IPv4 地址(IP地址类型)， type 为数据传输方式/套接字类型，常用的有 SOCK_STREAM（流格式套接字/面向连接的套接字） 和 SOCK_DGRAM（数据报套接字/无连接的套接字），将 protocol 的值设为 0，系统会自动推演出应该使用什么协议
	
	
   //如果不出错，socket函数将返回socket的描述符（句柄），否则，将返回INVALID_SOCKET。WSAGetLastError()返回上次发生的网络错误
	if (srv_soc==INVALID_SOCKET) {
		printf("[Web] socket() Fails,error=%d\n",WSAGetLastError());
		return -1;
	}

	//服务器地址
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //INADDR_ANY就是指定地址为0.0.0.0的地址，这个地址事实上表示不确定地址，或“所有地址”、“任意地址”。 一般来说，在各个系统中均定义成为0值。

	result = bind(srv_soc,(struct sockaddr*) &serv_addr,sizeof(serv_addr));

	if (result==SOCKET_ERROR) {/* 绑定失败 */
		closesocket(srv_soc);
		printf("[Web] Fail to bind, error = %d\n", WSAGetLastError());
		return -1;
	}

	result = listen(srv_soc, SOMAXCONN); //SOMAXCONN 侦听队列的长度
	printf("[Web] The server is running... ...\n");

	//接收请求
	while (1) {
		acpt_soc = accept(srv_soc, (struct sockaddr *) &from_addr, &from_len);

		if (acpt_soc == INVALID_SOCKET) {
			printf("[Web] Fail to accept,error=%d\n", WSAGetLastError());
			break;
		}
		printf("[Web] Accepted address:[%s],port:[%d]\n", 
			inet_ntoa(from_addr.sin_addr), ntohs(from_addr.sin_port));//inet_ntoa把地址转成点分十进制的

		recv_len = recv(acpt_soc, recv_buf, HTTP_BUF_SIZE, 0);
		if (recv_len == SOCKET_ERROR) {/* 接收失败 */
			closesocket(acpt_soc);
			printf("[Web] Fail to recv,error=%d\n", WSAGetLastError());
			break;
		}

		recv_buf[recv_len] = 0;//清空

		/*向客户端发送数据响应*/
		result = http_send_response(acpt_soc,recv_buf,recv_len);
		closesocket(acpt_soc);
	}

	closesocket(srv_soc);
	WSACleanup();
	printf("[Web] The server is stopped.\n");
	return 0;
}