#include "core/wpacket.h"
#include "core/rpacket.h"



int main()
{
    {
        printf("--------------------test1--------------------\n");
        wpacket_t wpk = wpk_create(32,0);
        wpk_write_uint8(wpk,1);
        wpk_write_uint16(wpk,2);
        wpk_write_uint32(wpk,3);
        wpk_write_uint32(wpk,4);
        wpk_write_string(wpk,"hellokenny2");
        wpk_write_string(wpk,"hellokenny2");
        wpk_write_string(wpk,"hellokenny3");
        wpk_write_string(wpk,"hellokenny4");
        wpk_write_uint8(wpk,5);
        wpk_write_uint16(wpk,6);
        wpk_write_uint32(wpk,7);
        wpk_write_uint32(wpk,8);

        rpacket_t rpk = rpk_create_by_other((struct packet*)wpk);
        printf("%d\n",rpk_len(rpk));
        printf("%d\n",rpk_read_uint8(rpk));
        printf("%d\n",rpk_read_uint16(rpk));
        printf("%d\n",rpk_read_uint32(rpk));
        printf("%d\n",rpk_read_uint32(rpk));
        printf("%s\n",rpk_read_string(rpk));
        printf("%s\n",rpk_read_string(rpk));
        printf("%s\n",rpk_read_string(rpk));
        printf("%s\n",rpk_read_string(rpk));
        printf("%d\n",rpk_read_uint8(rpk));
        printf("%d\n",rpk_read_uint16(rpk));
        printf("%d\n",rpk_read_uint32(rpk));
        printf("%d\n",rpk_read_uint32(rpk));

        //读取尾部的4个字节
        printf("%d\n",reverse_read_uint32(rpk));
        //丢掉尾部4个子节
        rpk_dropback(rpk,4);
        printf("%d\n",rpk_len(rpk));
        printf("%d\n",reverse_read_uint32(rpk));

        //丢弃hellokenny3之前的数据创建rpk2
        rpacket_t rpk2 = rpk_create_skip(rpk,1+2+4+4+2*(strlen("hellokenny2")+1+4));

        printf("%d\n",rpk_len(rpk2));
        //printf("%s\n",rpk_read_string(rpk2));
        //printf("%s\n",rpk_read_string(rpk2));
        printf("%s\n",rpk_read_string(rpk2));
        printf("%s\n",rpk_read_string(rpk2));
        printf("%d\n",rpk_read_uint8(rpk2));
        printf("%d\n",rpk_read_uint16(rpk2));
        printf("%d\n",rpk_read_uint32(rpk2));
        //printf("%d\n",rpk_read_uint32(rpk2));
    }

    {
        printf("--------------------test2--------------------\n");
        wpacket_t wpk = wpk_create(32,0);
        wpk_write_uint8(wpk,1);
        wpk_write_uint16(wpk,2);
        wpk_write_uint32(wpk,3);
        wpk_write_uint32(wpk,4);
        wpk_write_string(wpk,"hellokenny2");
        wpk_write_string(wpk,"hellokenny2");
        wpk_write_string(wpk,"hellokenny3");
        wpk_write_string(wpk,"hellokenny4");
        wpk_write_uint8(wpk,5);
        wpk_write_uint16(wpk,6);
        wpk_write_uint32(wpk,7);
        wpk_write_uint32(wpk,8);

        rpacket_t rpk = rpk_create_by_other((struct packet*)wpk);
        //丢弃hellokenny3之前的数据创建rpk2
        rpacket_t rpk2 = rpk_create_skip(rpk,1+2+4+4+2*(strlen("hellokenny2")+1+4));

        printf("%d\n",rpk_len(rpk2));
        //printf("%s\n",rpk_read_string(rpk2));
        //printf("%s\n",rpk_read_string(rpk2));
        printf("%s\n",rpk_read_string(rpk2));
        printf("%s\n",rpk_read_string(rpk2));
        printf("%d\n",rpk_read_uint8(rpk2));
        printf("%d\n",rpk_read_uint16(rpk2));
        printf("%d\n",rpk_read_uint32(rpk2));
        //printf("%d\n",rpk_read_uint32(rpk2));

        //原rpk不受rpk2的影响
        printf("%d\n",rpk_len(rpk));
        printf("%d\n",rpk_read_uint8(rpk));
        printf("%d\n",rpk_read_uint16(rpk));
        printf("%d\n",rpk_read_uint32(rpk));
        printf("%d\n",rpk_read_uint32(rpk));
        printf("%s\n",rpk_read_string(rpk));
        printf("%s\n",rpk_read_string(rpk));
        printf("%s\n",rpk_read_string(rpk));
        printf("%s\n",rpk_read_string(rpk));
        printf("%d\n",rpk_read_uint8(rpk));
        printf("%d\n",rpk_read_uint16(rpk));
        printf("%d\n",rpk_read_uint32(rpk));
        printf("%d\n",rpk_read_uint32(rpk));

    }

    return 0;
}
