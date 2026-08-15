#include <stdio.h>
#include "dev_keyboard.h"
static int ck(int x,const char*n){printf("  %s %s\n",x?"PASS":"FAIL",n);return x?0:1;}
int main(void){int f=0;uint32_t v;puts("Running ARM7 keyboard MMIO layer-1 validation...");
dev_keyboard_init(KBD_BASE_ADDR);
f+=ck(dev_keyboard_count()==0,"FIFO initially empty");
v=dev_keyboard_read32(KBD_BASE_ADDR+KBD_REG_STATUS);f+=ck(v==0,"STATUS clear when empty");
f+=ck(dev_keyboard_push('A'),"push A");f+=ck(dev_keyboard_push('B'),"push B");
v=dev_keyboard_read32(KBD_BASE_ADDR+KBD_REG_STATUS);f+=ck((v&KBD_STATUS_READY)!=0,"STATUS READY set");
v=dev_keyboard_read32(KBD_BASE_ADDR+KBD_REG_DATA);f+=ck(v=='A',"DATA returns A first");
v=dev_keyboard_read32(KBD_BASE_ADDR+KBD_REG_DATA);f+=ck(v=='B',"DATA returns B second");
v=dev_keyboard_read32(KBD_BASE_ADDR+KBD_REG_STATUS);f+=ck(v==0,"STATUS clears when drained");
puts(f?"ARM7 keyboard MMIO layer-1: FAIL":"ARM7 keyboard MMIO layer-1: PASS");return f?1:0;}
