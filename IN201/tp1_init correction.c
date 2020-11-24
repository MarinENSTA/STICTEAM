#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>

#include <string.h>
#include <arpa/inet.h>

struct fs_header { 
 
  char romfs[8];
  uint32_t full_size;
  uint32_t checksum;
  char volume_name[];
};

struct file_header {
  uint32_t next_filehdr;
  uint32_t spec_info;
  uint32_t size; 
  uint32_t checksum;
  char file_name[];

} 

uint32_t read32(char *ptr){
  uint32_t res;                           // 0000<#3>00 | 000000<#2> = 0000<#3><#2>
  res = ptr[3]; res <<= 8;              // res = 000000<#3>; res = 0000<#3>00
  res |= ptr[2]; res <<= 8;             // res = 0000<#3><#2>; res = 00<#3><#2>00;
  res |= ptr[1]; res <<= 8;             //  res = 00<#3><#2><#1>; res = <#3><#2><#1>00
  res |= ptr[0];                        //  res = <#3><#2><#1><#0>
  return res;
}

int round_up16(int value){
  //int res = ((value + 15) / 16) * 16; // 22 + 15  = 37 / 16 2 * 2 32
  //int res = (value + 15) >> 4 << 4;
  int res = (value + 15) & (~0xf);          // 0xf = 0b00000.....0001111
	                                    // ~0xf = 0b11111.....110000
  return res;

}

void ls(char *start, struct file_header *fh){
   printf("%s\n", fh->file_name);
   int next_offset = ntohl(fh->next_filehdr) & (~0xf);
   if(next_offset != 0) {
     struct file_header *next = &start[next_offset];
     ls(start, next);
   }
}

void find(char *start, struct file_header *fh){
   printf("%s\n", fh->file_name);
   uint32_t next_filehdr = ntohl(fh->next_filehdr)
   int next_offset = next_filehdr & (~0xf);
   if((next_filehdr & 7) == 1) {
     printf("This is a repertory\n");
     struct file_header *first_in_dir = ...;
     find(start, first_in_dir);
   
   }
   
   if(next_offset != 0) {
     struct file_header *next = &start[next_offset];
     ls(start, next);
   }
}

void decode(struct fs_header *p, size_t size){
  // char *volname = (char *) p + sizeof(struct fs_header);
  char *volname = &p->volume_name[0];

  assert(p->romfs[0] == '-');
  assert(p->romfs[1] == 'r');

  // man strncmp
  assert(strncmp(p->romfs,"-rom1fs-",8) == 0);
  // ne pas faire: uint32_t x = p -> full_size;

//  uint32_t real_size = p->full_size;
  uint32_t real_size = read32((char *) &p->full_size);
  uint32_t real_size  = ntohl(p->full_size);
  printf("%x %x %x\n", real_size, p->full_size, size);
  assert(real_size < size);

  int i = 0;
  while(p->volume_name[i] != 0) { putchar(p->volume_name[i]); i++; }
  i = round_up16(i+1);

  struct file_header *root_dir = &p->volume_name[i];

  ls((char *) p, root_dir);



}




int main(void){

  int fd = open("tp1fs.romfs",O_RDONLY);
  assert(fd != -1);
  off_t fsize;
  fsize = lseek(fd,0,SEEK_END);

  //  printf("size is %d", fsize);
  
  void *addr = mmap(addr, fsize, PROT_READ, MAP_SHARED, fd, 0);
  assert(addr != MAP_FAILED);
  decode(addr, fsize);
  return 0;
}
