typedef struct						//通信实体
{
	char				Role;		//角　　色  C/S

	unsigned char		IPvN;		//版　　本  4/6

	union
	{
		unsigned int	IPv4;		//网　　址

		unsigned char	IPv6[16];	//网　　址
	};

	unsigned short		Port;		//端　　口

} ENTITY;

typedef struct						//交互任务
{
	//指 令 区

	union
	{
		unsigned char	Inform;		//通告指令         0x12 = 数据传输 0x13 = 关闭

		unsigned char	Action;		//动作指令         0x22 = 转发 0x21 = 知晓 
	};

	unsigned char		Option;		//标志选项

	//标 识 区

	unsigned short		Thread;		//线程编号

	unsigned short		Number;		//链路标识

	//通 信 区

	ENTITY				Source;		//源　　端

	ENTITY				Target;		//宿　　端

	//承 载 区

	unsigned char	   *Buffer;		//数　　据

	unsigned int		Length;		//长　　度

	unsigned int		Volume;		//容　　限

} TASK;