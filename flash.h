#ifndef _FLASH_
#define _FLASH_

void flash_init(void);
void save_config(void);
void flash_write_data(u32 addr, void *dat, int len);
void flash_erase(void);

#endif /* _FLASH_ */
