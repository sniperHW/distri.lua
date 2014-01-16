wpacket和rpacket是kendynet中使用的封包处理结构.wpacket用于发送,rpacket用于接收.

wpacket和rpacket和支持两种类型的封包处理，raw模式和非raw模式

###raw模式

raw模式就是原始的字节流模式，包中的所有数据被当作一整个无模式的二进制数据块处理.
对于raw模式的rpacket,只能用`rpk_read_binary`函数将包中的数据作为一个二进制块读取出来.

###非raw模式
非raw模式的数据包，在包头中包含一32位的长度字段，用于表示去除包头以外数据的长度.
当向非raw模式的包中写入string或binary数据时，首先会写入一个32位的长度，之后再写入实际的数据.注意处理数据包时要按写入的顺序来读取.

例如：

发送方

	wpacket_t wpk = NEW_WPK(64);
	wpk_write_uint32(wpk,1);
	wpk_write_uint16(wpk,2);
	wpk_write_uint64(wpk,100);


接收方
	
	rpacket_t rpk = rpk_create_by_other((packet*)wpk);
	rpk_read_uint32(rpk); --->1		
	rpk_read_uint16(rpk); --->2
	rpk_read_uint64(rpk); --->100


wpacket和rpacket的底层使用buffer list来存储数据,这种存储方式可有效的利用内存，
减少内存的拷贝次数.


例如，当创建wpacket的时候，在很多情况下，最开始创建的时候我们并不知道实际要写到wpacket中的数据有多少，所以我们会以一个默认的大小来创建wpacket,例如64。这个时候
会在底层创建一个64字节大小的buffer,当我们往wpacket中写入的数据超过64字节时，wpacket
会再分配一块更大的内存来容纳新的数据，所有这些存储数据的buffer被链接成一个list.

更进一步，基于buffer list的packet在处理数据包广播的时候可以有效的减少内存拷贝.

例如收到一个rpacket,需要将这个rpacket中的数据转发给100个其他的网络连接,那么可以像下面这样做:

	rpacket_t r;
	connection* clients[100];
	int i = 0;
	for(; i < 100; ++i)
		send_packet(clients[i],wpk_create_by_rpacket(r));


因为wpakcet可以共享rpacket的buffer list，所以省掉了100次的内存拷贝.

##reverse_read系列函数

reverse_read系列函数用于从包的尾部读取数据,每次读取都会遍历输入rpacket的buffer list以找到正确的读取下标，所以正确的使用方式如下面介绍的.

首先假设一个场景,有一台内部服务器，需要将一个数据包广播给100个其它客户端：

	wpacket_t wpk;//需要发送的数据包
	uint32_t clients[100];//假设每个元素代表网关服务器中的一个用户索引
	int i = 0;
	for(; i < 100; ++i)
		wpk_write_uint32(wpk,client[i]);//将100个标识写到尾部
	wpk_write_uint16(wpk,client_size);//写入数量
	send_packet(gate,wpk);


在网关中的代码:

	uint16_t size = reverse_read_uint16(rpk);//这个包需要发给多少个客户端
	//用rpk尾部的数据创建一个rpacket_t r,用于读取需要接受数据的客户端
	rpacket_t r = rpk_create_skip(rpk,size*sizeof(uint32_t)+sizeof(size));
	//将rpk尾巴用于广播的信息丢掉
	rpk_dropback(rpk,size*sizeof(uint32_t)+sizeof(size));	
	int i = 0;
	//用rpk创建一个wpk,用于发送
	wpacket_t wpk = wpk_create_by_rpacket(rpk);
	//发送给所有需要接收的客户端
	for( ; i < size; ++i)	
	{
		uint32_t idx = rpk_read_uint32(r);
		send_packet(clients[idx],wpk);
	}
	rpk_destroy(&rpk);
	rpk_destroy(&r);

