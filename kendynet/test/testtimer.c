#include <stdio.h>
#include "core/timer.h"
#include "core/systime.h"
#include "core/sync.h"

struct ttimer
{
    struct timer_item titem;
    time_t intend_time;
};

void t_callback(struct timer *timer,struct timer_item *item,void *ud)
{
    time_t now = time(NULL);
    struct ttimer *_t = (struct ttimer*)item;
    printf("%ld\n",now - _t->intend_time);
    _t->intend_time = now+1;
    register_timer(timer,item,1);
}

int main()
{
    struct ttimer _t;
    _t.titem.callback = t_callback;
    _t.intend_time = time(NULL);
    init_timer_item((struct timer_item*)&_t);
    struct timer *_timer = new_timer();
    register_timer(_timer,(struct timer_item*)&_t,1);
    while(1)
    {
        update_timer(_timer,time(NULL));
        sleepms(100);
    }
    return 0;
}

