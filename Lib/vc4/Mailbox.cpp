/*
Copyright (c) 2012, Broadcom Europe Ltd.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "Mailbox.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include "Support/basics.h"  // fatal()


#define PAGE_SIZE (4*1024)

namespace V3DLib {

void *mapmem(unsigned base, unsigned size)
{
   int mem_fd;
   unsigned offset = base % PAGE_SIZE;
   base = base - offset;
   /* open /dev/mem */
   if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
      fatal("can't open /dev/mem\nThis program should be run as root. Try prefixing command with: sudo");
   }
   void *mem = mmap(
      0,
      size,
      PROT_READ|PROT_WRITE,
      MAP_SHARED/*|MAP_FIXED*/,
      mem_fd,
      base);

//#ifdef DEBUG
//   printf("base=0x%x, mem=%p\n", base, mem);
//#endif  // DEBUG

   if (mem == MAP_FAILED) {
      char buf[64];
      sprintf(buf, "mmap error %p\n", mem);
      fatal(buf);
   }
   close(mem_fd);
   return (char *)mem + offset;
}

void unmapmem(void *addr, unsigned size)
{
   int s = munmap(addr, size);
   if (s != 0) {
      char buf[64];
      sprintf(buf, "munmap error %d\n", s);
      fatal(buf);
   }
}

/*
 * use ioctl to send mbox property message
 */
static int mbox_property(int file_desc, void *buf)
{
   int ret_val = ioctl(file_desc, IOCTL_MBOX_PROPERTY, buf);

   if (ret_val < 0) {
     printf("ioctl_set_msg failed: %d\n", ret_val);
   }

   return ret_val;
}


/**
 * @brief Get the hardware revision code.
 *
 * For mapping the code to a Pi mode, see: https://www.raspberrypi-spy.co.uk/2012/09/checking-your-raspberry-pi-board-version/
 */
unsigned get_version(int file_desc)
{
   unsigned i=0;
   unsigned p[32];
   p[i++] = 0; // size
   p[i++] = 0x00000000; // process request

   p[i++] = 0x10002; // (the tag id)
   p[i++] = 4; // (size of the buffer)
   p[i++] = 0; // (size of the data)
   p[i++] = 0; // place for response data
   p[i++] = 0x00000000; // end tag
   p[0] = i * (unsigned) sizeof(*p); // actual size

   mbox_property(file_desc, p);
   return p[5];
}

unsigned mem_alloc(int file_desc, unsigned size, unsigned align, unsigned flags)
{
   unsigned i=0;
   unsigned p[32];
   p[i++] = 0; // size
   p[i++] = 0x00000000; // process request

   p[i++] = 0x3000c; // (the tag id)
   p[i++] = 12; // (size of the buffer)
   p[i++] = 12; // (size of the data)
   p[i++] = size; // (num bytes? or pages?)
   p[i++] = align; // (alignment)
   p[i++] = flags; // (MEM_FLAG_L1_NONALLOCATING)

   p[i++] = 0x00000000; // end tag
   p[0] = i * (unsigned) sizeof(*p); // actual size

   mbox_property(file_desc, p);
   //printf("mem_alloc returns %d\n", p[5]);
   return p[5];
}

unsigned mem_free(int file_desc, unsigned handle)
{
   unsigned i=0;
   unsigned p[32];
   p[i++] = 0; // size
   p[i++] = 0x00000000; // process request

   p[i++] = 0x3000f; // (the tag id)
   p[i++] = 4; // (size of the buffer)
   p[i++] = 4; // (size of the data)
   p[i++] = handle;

   p[i++] = 0x00000000; // end tag
   p[0] = i * (unsigned) sizeof(*p); // actual size

   int ret = mbox_property(file_desc, p);
   if (ret < 0) {
     return (uint32_t) -1;  // Failure
   } else {
     // printf("mem_free returns %d\n", p[5]);
     return p[5];  // On failure, this would return the passed in handle
   }
}

unsigned mem_lock(int file_desc, unsigned handle)
{
   unsigned i=0;
   unsigned p[32];
   p[i++] = 0; // size
   p[i++] = 0x00000000; // process request

   p[i++] = 0x3000d; // (the tag id)
   p[i++] = 4; // (size of the buffer)
   p[i++] = 4; // (size of the data)
   p[i++] = handle;

   p[i++] = 0x00000000; // end tag
   p[0] = i * (unsigned) sizeof(*p); // actual size

   mbox_property(file_desc, p);
   //printf("mem_lock returns %d\n", p[5]);
   return p[5];
}


/**
 * @return ioctl return code on success, -1 on iotctl error
 */
unsigned mem_unlock(int file_desc, unsigned handle)
{
   unsigned i=0;
   unsigned p[32];
   p[i++] = 0; // size
   p[i++] = 0x00000000; // process request

   p[i++] = 0x3000e; // (the tag id)
   p[i++] = 4; // (size of the buffer)
   p[i++] = 4; // (size of the data)
   p[i++] = handle;

   p[i++] = 0x00000000; // end tag
   p[0] = i * (unsigned) sizeof(*p); // actual size

   int ret = mbox_property(file_desc, p);
   if (ret < 0) {
     return (uint32_t) -1;  // Failure
   } else {
     //printf("mem_unlock returns %d\n", p[5]);
     return p[5];  // On failure, this would return the passed in handle
   }
}


/**
 * TODO What does this do??
 *
 * Apparently, a single instruction is passed in with values for all acc's.
 * Is this a kind a debug statement, to check working of an instruction??
 *
 */
unsigned execute_code(int file_desc, unsigned code, unsigned r0, unsigned r1, unsigned r2, unsigned r3, unsigned r4, unsigned r5)
{
   unsigned int i=0;
   unsigned p[32];
   p[i++] = 0; // size
   p[i++] = 0x00000000; // process request

   p[i++] = 0x30010; // (the tag id)
   p[i++] = 28; // (size of the buffer)
   p[i++] = 28; // (size of the data)
   p[i++] = code;
   p[i++] = r0;
   p[i++] = r1;
   p[i++] = r2;
   p[i++] = r3;
   p[i++] = r4;
   p[i++] = r5;

   p[i++] = 0x00000000; // end tag
   p[0] = i * (unsigned) sizeof(*p); // actual size

   mbox_property(file_desc, p);
   //printf("execute_code returns %d\n", p[5]);
   return p[5];
}

unsigned qpu_enable(int file_desc, unsigned enable)
{
   unsigned i=0;
   unsigned p[32];

   p[i++] = 0; // size
   p[i++] = 0x00000000; // process request

   p[i++] = 0x30012; // (the tag id)
   p[i++] = 4; // (size of the buffer)
   p[i++] = 4; // (size of the data)
   p[i++] = enable;

   p[i++] = 0x00000000; // end tag
   p[0] = i * (unsigned) sizeof(*p); // actual size

   mbox_property(file_desc, p);

   //printf("qpu_enable returns %d\n", p[5]);
   return p[5];
}

unsigned execute_qpu(int file_desc, unsigned num_qpus, unsigned control, unsigned noflush, unsigned timeout) {
   unsigned i=0;
   unsigned p[32];

   p[i++] = 0; // size
   p[i++] = 0x00000000; // process request
   p[i++] = 0x30011; // (the tag id)
   p[i++] = 16; // (size of the buffer)
   p[i++] = 16; // (size of the data)
   p[i++] = num_qpus;
   p[i++] = control;
   p[i++] = noflush;
   p[i++] = timeout; // ms

   p[i++] = 0x00000000; // end tag
   p[0] = i * (unsigned) sizeof(*p); // actual size

   mbox_property(file_desc, p);
   //printf("execute_qpu returns %d\n", p[5]);
   return p[5];
}

int mbox_open() {
   int file_desc;

   // open a char device file used for communicating with kernel mbox driver
   file_desc = open(DEVICE_FILE_NAME, 0);
   if (file_desc < 0) {
      char buf[128];
      sprintf(buf,
          "Can't open device file: %s\n"
          "Try creating a device file with: sudo mknod %s c %d 0\n",
          DEVICE_FILE_NAME, DEVICE_FILE_NAME, MAJOR_NUM
      );
      fatal(buf);
   }
   return file_desc;
}

void mbox_close(int file_desc) {
  close(file_desc);
}

}  // namespace V3DLib
