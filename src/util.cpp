#include "util.hpp"


#define SIZE 1024*1024

std::string FileToString(const std::string& file_)
{
  std::ifstream file(file_);
    if (!file) {
        std::cerr << "Failed to open file\n";
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();  // Read entire file into the buffer
    std::string contents = buffer.str();
    return contents;
}




int open_fd() {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd == -1) {
        printf("Can't open /dev/mem.\n");
        exit(0);
    }
    return fd;
}


volatile void flush_cache() {
    char *array = (char*)malloc(32*SIZE);

    if (array == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
    }
    memset(array, 0, 8*SIZE);

    for (int i = 0; i <8*SIZE; ++i) {
        char value = array[i];
    }
    free(array);
}

int configure_relcache() {

  flush_cache();
  // added this for testing


  printf("configure_relcache()\n");
    int lpd_fd  = open_fd();
    printf("got lpd_fd %d\n", lpd_fd);
    uint64_t config = (uint64_t)mmap(NULL, RME_CONFIG_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, lpd_fd, RME_CONFIG);
    if (!config)
    {
        printf("could not map relcache config\n");
        return -1;
    }
    printf("Attempting to configure RME\n");

    WRITE_UINT16(RME_COL_WIDTH(config), sizeof(float));
    WRITE_BOOL(RME_EN(config), 1);
    close(lpd_fd);
    int unmap_result = munmap((void*)config, RME_CONFIG_SIZE);
    return unmap_result;
}





int reset_relcache() {  
    printf("Resetting RME\n");
    int lpd_fd  = open_fd();
    printf("got lpd_fd %d\n", lpd_fd);
    uint64_t config = (uint64_t)mmap(NULL, RME_CONFIG_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, lpd_fd, RME_CONFIG);
    //__dsb();
    // reset
    
    WRITE_BOOL(RME_RESET(config), 1);

    while (READ_BOOL(RME_RESET(config)) != 1)
    {

    }
    WRITE_BOOL(RME_RESET(config), 0);

    int unmap_result = munmap((void*)config, RME_CONFIG_SIZE);
    close(lpd_fd);
    return unmap_result;
}

int EnableRelCache(int fd)
{
    return 0;
    printf("EnableRelCache()\n");
    int lpd_fd  = open_fd();

    void* config = (void*)mmap(NULL, RME_CONFIG_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, lpd_fd, RME_CONFIG);
    printf("EnableRelCache() Got config 0x%x\n", (uint64_t)config);
    WRITE_BOOL(RME_EN(config), 1);
    printf("Wrote config!\n");
    while(READ_BOOL(RME_EN(config)) != 1)
    {
      printf("Wrote config 0x%x\n", READ_BOOL(RME_EN(config)));
    }



  int unmap_result = munmap(config, RME_CONFIG_SIZE);
  printf("EnableRelCache() done\n");
  return unmap_result;
}


volatile void FlushAndDisable(int fd)
{
   printf("FlushAndDisable()\n");
    //int lpd_fd  = open_fd();
    struct _config* config = (struct _config *)mmap(NULL, RME_CONFIG_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, RME_CONFIG);
    WRITE_BOOL(RME_EN(config), 0);
    while(READ_BOOL(RME_EN(config)) != 0)
    {
      
    }
  int unmap_result = munmap(config, RME_CONFIG_SIZE);
  
  flush_cache();
  printf("FlushAndDisable() done\n");
}



uint64_t read_cycle() {
    uint64_t cycle_count;
    asm volatile ("csrr %0, cycle" : "=r"(cycle_count));
    return cycle_count;
}

uint64_t read_instret() {
    uint64_t instret;
    asm volatile ("csrr %0, instret" : "=r"(instret));
    return instret;
}

