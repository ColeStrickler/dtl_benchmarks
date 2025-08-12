#ifndef UTIL_HPP
#define UTIL_HPP
#include <stdint.h>
#include <cstdint>
#include <stdio.h>
#include <fcntl.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <cstring> // for memset
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unistd.h>
#define RME_CONFIG                  0x3000000
#define RME_CONFIG_SIZE             0xfff
#define RME_EN(base)				((uint64_t)base)
#define RME_ROWSIZE(base)			((uint64_t)base + 0x10)
#define RME_EN_COL(base)			((uint64_t)base + 0x30)
#define RME_COL_WIDTH(base)		    ((uint64_t)base + 0x40)
#define RME_COL_OFFSET(base, i)	    ((uint64_t)base + i * 0x10 + 0x48)
#define RME_RESET(base)             ((uint64_t)base + 16 * 0x10 + 0x48)
#define RELCACHE_ADDR  0x110000000UL
#define RELCACHE_SIZE  0x00fffffffUL
#define WRITE_BOOL(addr, value)(*(bool*)(addr) = value)
#define WRITE_UINT8(addr, value)(*(uint8_t*)(addr) = value)
#define WRITE_UINT16(addr, value)(*(uint16_t*)(addr) = value)
#define WRITE_UINT32(addr, value)(*(uint32_t*)(addr) = value)
#define WRITE_UINT64(addr, value)(*(uint64_t*)(addr) = value)

#define READ_BOOL(addr)(*(bool*)(addr))
#define READ_UINT8(addr)(*(uint8_t*)(addr))
#define READ_UINT16(addr)(*(uint16_t*)(addr))
#define READ_UINT32(addr)(*(uint32_t*)(addr))
#define READ_UINT64(addr)(*(uint64_t*)(addr))


int open_fd();
uint64_t read_cycle();
uint64_t read_instret();
volatile void FlushAndDisable(int fd);
int EnableRelCache(int fd);
int reset_relcache(unsigned int frame_offset);

std::string FileToString(const std::string &file_);

int configure_relcache();
volatile void flush_cache();

#endif

