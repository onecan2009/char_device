#ifndef __DEBUG_H_
#define __DEBUG_H_

#define MAX_DEVICE_NUM 32768

#define DEBUG_FLAG
#ifdef DEBUG_FLAG
    #define debug_print(fmt,arg...) printk(fmt,##arg)
#else
    #define debug_print(fmt,arg...) do{}while(0)
#endif


#endif
