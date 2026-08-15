#include "dev_keyboard.h"
#include <string.h>
#define KBD_FIFO_CAPACITY 64u
typedef struct { uint32_t base; uint8_t fifo[KBD_FIFO_CAPACITY]; unsigned head,tail,count; bool present; } keyboard_dev_t;
static keyboard_dev_t g_kbd;
void dev_keyboard_init(uint32_t base){ memset(&g_kbd,0,sizeof g_kbd); g_kbd.base=base; g_kbd.present=true; }
void dev_keyboard_reset(void){ uint32_t b=g_kbd.base?g_kbd.base:KBD_BASE_ADDR; bool p=g_kbd.present; memset(&g_kbd,0,sizeof g_kbd); g_kbd.base=b; g_kbd.present=p; }
bool dev_keyboard_push(uint8_t ch){ if(!g_kbd.present) dev_keyboard_init(KBD_BASE_ADDR); if(g_kbd.count>=KBD_FIFO_CAPACITY)return false; g_kbd.fifo[g_kbd.tail]=ch; g_kbd.tail=(g_kbd.tail+1u)%KBD_FIFO_CAPACITY; ++g_kbd.count; return true; }
unsigned dev_keyboard_count(void){ return g_kbd.count; }
uint32_t dev_keyboard_read32(uint32_t addr){ uint32_t off; if(!g_kbd.present)dev_keyboard_init(KBD_BASE_ADDR); off=addr-g_kbd.base; if(off==KBD_REG_STATUS)return g_kbd.count?KBD_STATUS_READY:0u; if(off==KBD_REG_DATA){ if(!g_kbd.count)return 0u; uint8_t ch=g_kbd.fifo[g_kbd.head]; g_kbd.head=(g_kbd.head+1u)%KBD_FIFO_CAPACITY; --g_kbd.count; return ch; } return 0u; }
void dev_keyboard_write32(uint32_t addr,uint32_t value){ (void)addr;(void)value; }
