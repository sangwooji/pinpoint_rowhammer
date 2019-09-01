// Copyright 2019, Sangwoo Ji
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Small test program to test Pinpoint Rowhammer.
// Modified double_sided_rowhammer.cc for Pinpoint Rowhammer.i
//
// Compilation instructions:
//   g++ -std=c++11 [filename]
//
// Original author: Thomas Dullien (thomasdullien@google.com)
// Modified author: Sangwoo Ji (sangwooji@postech.edu)

#include <asm/unistd.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/kernel-page-flags.h>
#include <map>
#include <stdlib.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <vector>
#include "pinpoint_module.h"

namespace {

// The fraction of physical memory that should be mapped for testing.
double fraction_of_physical_memory = 0.3;

// The time to hammer before aborting. Defaults to one hour.
uint64_t number_of_seconds_to_hammer = 3600;

// The number of memory reads to try.
uint64_t number_of_reads = 1200000;

// Obtain the size of the physical memory of the system.
uint64_t GetPhysicalMemorySize() {
  struct sysinfo info;
  sysinfo( &info );
  return (size_t)info.totalram * (size_t)info.mem_unit;
}

uint64_t GetPageFrameNumber(int pagemap, uint8_t* virtual_address) {
  // Read the entry in the pagemap.
  uint64_t value;
  int got = pread(pagemap, &value, 8,
                  (reinterpret_cast<uintptr_t>(virtual_address) / 0x1000) * 8);
  assert(got == 8);
  uint64_t page_frame_number = value & ((1ULL << 54)-1);
  return page_frame_number;
}

void SetupMapping(uint64_t* mapping_size, void** mapping) {
  *mapping_size = 
    static_cast<uint64_t>((static_cast<double>(GetPhysicalMemorySize()) * 
          fraction_of_physical_memory));

  *mapping = mmap(NULL, *mapping_size, PROT_READ | PROT_WRITE,
      MAP_POPULATE | MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  assert(*mapping != (void*)-1);

  // Initialize the mapping so that the pages are non-empty.
  printf("[!] Initializing large memory mapping ...");
  for (uint64_t index = 0; index < *mapping_size; index += 0x1000) {
    uint64_t* temporary = reinterpret_cast<uint64_t*>(
        static_cast<uint8_t*>(*mapping) + index);
    temporary[0] = index;
  }
}

uint8_t GetPresumedBankNumber(
    int pagemap,
    uint8_t* address) {
    uint64_t pfn  = GetPageFrameNumber(pagemap, address);
    uint64_t pa = pfn << 12;
    uint8_t presumed_bank_num = ((pa>>13)&7) ^((pa>>16)&7);

    return presumed_bank_num;
}

uint64_t HammerAddressesStandard(
    const std::pair<uint64_t, uint64_t>& first_range,
    const std::pair<uint64_t, uint64_t>& second_range,
    uint64_t number_of_reads) {
  volatile uint64_t* first_pointer =
      reinterpret_cast<uint64_t*>(first_range.first);
  volatile uint64_t* second_pointer =
      reinterpret_cast<uint64_t*>(second_range.first);
  uint64_t sum = 0;

  while (number_of_reads-- > 0) {
    //sum += first_pointer[0];
    //sum += second_pointer[0];
    asm volatile(
        "mov (%0), %%rdx\n\t"
        "mov (%1), %%rdx\n\t"
        "clflush (%0);\n\t"
        "clflush (%1);\n\t"
        : : "r" (first_pointer), "r" (second_pointer) : "memory", "rdx");
  }
  return sum;
}

typedef uint64_t(HammerFunction)(
    const std::pair<uint64_t, uint64_t>& first_range,
    const std::pair<uint64_t, uint64_t>& second_range,
    uint64_t number_of_reads);

// A comprehensive test that attempts to hammer adjacent rows for a given 
// assumed row size (and assumptions of sequential physical addresses for 
// various rows.
uint64_t HammerAllReachablePages(uint64_t presumed_row_size, 
    void* memory_mapping, uint64_t memory_mapping_size, HammerFunction* hammer,
    uint64_t number_of_reads) {
  // This vector will be filled with all the pages we can get access to for a
  // given row size.
  std::vector<std::vector<uint8_t*>> pages_per_row;
  uint64_t total_bitflips = 0;
  uint8_t num_pages_per_row = presumed_row_size/(4*1024);
  uint8_t default_pattern = 2;
  uint64_t results[8][1024];
  uint64_t ppt_results[1024];
  uint64_t first_alter[12][1024];
  uint64_t second_alter[12][1024];

  pages_per_row.resize(memory_mapping_size / presumed_row_size);
  int pagemap = open("/proc/self/pagemap", O_RDONLY);
  assert(pagemap >= 0);

  printf("[!] Identifying rows for accessible pages ... ");
  for (uint64_t offset = 0; offset < memory_mapping_size; offset += 0x1000) {
    uint8_t* virtual_address = static_cast<uint8_t*>(memory_mapping) + offset;
    uint64_t page_frame_number = GetPageFrameNumber(pagemap, virtual_address);
    uint64_t physical_address = page_frame_number * 0x1000;
    uint64_t presumed_row_index = physical_address / presumed_row_size;
    if (presumed_row_index > pages_per_row.size()) {
      pages_per_row.resize(presumed_row_index);
    }
    pages_per_row[presumed_row_index].push_back(virtual_address);
  }
  printf("Done\n");

  // We should have some pages for most rows now.
  for (uint64_t row_index = 0; row_index + 2 < pages_per_row.size(); 
      ++row_index) {
    if ((pages_per_row[row_index].size() != num_pages_per_row) || 
        (pages_per_row[row_index+2].size() != num_pages_per_row)) {
      continue;
    } else if (pages_per_row[row_index+1].size() == 0) {
      continue;
    }
    
    for (uint8_t target_bank=0; target_bank<8; target_bank++) {  
      uint64_t* first_row;
      uint64_t* second_row;
      uint64_t* target_row;
      for (uint8_t* first_row_page : pages_per_row[row_index]) {
        if(GetPresumedBankNumber(pagemap, first_row_page)==target_bank) {
          first_row=reinterpret_cast<uint64_t*>(first_row_page);
          break;
        }
      }
      for (uint8_t* second_row_page : pages_per_row[row_index+2]) {
        if(GetPresumedBankNumber(pagemap, second_row_page)==target_bank) {
          second_row=reinterpret_cast<uint64_t*>(second_row_page);
          break;
        }
      }
      for (uint8_t* target_row_page : pages_per_row[row_index+1]) {
        if(GetPresumedBankNumber(pagemap, target_row_page)==target_bank) {
          target_row=reinterpret_cast<uint64_t*>(target_row_page);
          break;
        }
      }

      printf("[!] Hammering rows (%lx/%lx/%lx)\n", 
          GetPageFrameNumber(pagemap, reinterpret_cast<uint8_t*>(first_row)),
          GetPageFrameNumber(pagemap, reinterpret_cast<uint8_t*>(target_row)),
          GetPageFrameNumber(pagemap, reinterpret_cast<uint8_t*>(second_row)));

      HammerWithPattern(first_row, second_row, target_row,
          first_data[default_pattern], second_data[default_pattern], target_data[default_pattern], 
          number_of_reads, results[default_pattern]);


      uint32_t target_index=-1, target_pattern, target_bit_offset;
      uint64_t target_bit_mask;
      uint32_t count=0;
      for (uint32_t index=0; index<1024; index++) {
        if (results[default_pattern][index] != 0) {
          uint64_t temp = results[default_pattern][index];
          for (uint32_t bit_offset=0; bit_offset<64; bit_offset++) {
            count += (temp>>bit_offset)&1;

            // Choose target bit offset.
            // In this code, pick up the first bit flip for simplicity.
            if (((temp>>bit_offset)&1) && target_index == -1) {
              target_index = index;
              target_bit_mask = 1UL<<bit_offset;
              target_bit_offset = bit_offset;
            }
          }
        }
      }

      if (count > 0) {
        printf ("[!] Double-sided Rowhammer: %d bit flips\n", count);
      } else {
        continue;
      }

      // Scan with eight data patterns
      for (uint8_t pattern=0; pattern<8; pattern++) { 
        HammerWithPattern(first_row, second_row, target_row,
            first_data[pattern], second_data[pattern], target_data[pattern], 
            number_of_reads, results[pattern]);
      }

      // Calculate victim agnostic pattern
      for (uint8_t pattern=0; pattern<4; pattern++) {
        for (uint32_t index=0; index<1024; index++) {
          results[pattern][index] |= results[pattern+4][index];
        }
      }

      // Calculate alternating patttern
      for (uint32_t index=0; index<1024; index++) {
        for (uint32_t bit_offset=0; bit_offset<64; bit_offset++) {
          uint64_t bit_mask = 1UL << bit_offset;
          uint8_t sum=0;
          for (uint8_t pattern=0; pattern<4; pattern++) {
            sum += ((results[pattern][index]&bit_mask)>>bit_offset)<<(3-pattern);
            // If target bit is vulnerable to multiple data pattern,
            // choose one of them empirically.
            if ((index==target_index) && (bit_offset==target_bit_offset))
              switch (sum) {
                case 0b0010:
                case 0b0011:
                case 0b0110:
                case 0b0111:
                case 0b1010:
                case 0b1011:
                case 0b1110:
                case 0b1111:
                  target_pattern=2;
                  break;
                case 0b1000:
                case 0b1001:
                case 0b1100:
                case 0b1101:
                  target_pattern=0;
                  break;
                case 0b0001:
                case 0b0101:
                 target_pattern=3;
                   break;
                case 0b0100:
                  target_pattern=1;
                  break;
                case 0b0000:
                  target_pattern=default_pattern;
                  break;
              }
          }
          ComputePinpointData(first_alter, second_alter, index, bit_mask, sum); 
        }
      }

      asm volatile("mfence ;\n\t":::"memory");
      // Set effective data patter from the target bit offset 
      for (uint8_t i=0; i<12; i++) {
        first_alter[i][target_index] = (first_alter[i][target_index]&(~target_bit_mask))|(first_data[target_pattern]&target_bit_mask);
        second_alter[i][target_index] = (second_alter[i][target_index]&(~target_bit_mask))|(second_data[target_pattern]&target_bit_mask);
      }

      // Perform Pinpoint Rowhammer
      PinpointRowhammer(first_row, second_row, target_row, target_data[default_pattern], 
          first_alter, second_alter, number_of_reads, ppt_results);

      count=0;
      for (uint32_t index=0; index<1024; index++) {
        if (ppt_results[index] != 0) {
          uint64_t temp = ppt_results[index];
          for (uint32_t bit_offset=0; bit_offset<64; bit_offset++) {
            count += (temp>>bit_offset)&1;
          }
        }
      }

      if ((ppt_results[target_index]>>target_bit_offset)&1)
        printf ("[!] Pinpoint Rowhammer: %d bit flips\n\n", count);
      else
        printf ("[!] Pinpoint Rowhammer: %d bit flips (no target bit flip)\n\n", count);
    }
  }
  return total_bitflips;
}

void HammerAllReachableRows(HammerFunction* hammer, uint64_t number_of_reads) {
  uint64_t mapping_size;
  void* mapping;
  SetupMapping(&mapping_size, &mapping);

  HammerAllReachablePages(1024*64, mapping, mapping_size,
                          hammer, number_of_reads);
}

void HammeredEnough(int sig) {
  printf("[!] Spent %ld seconds hammering, exiting now.\n",
      number_of_seconds_to_hammer);
  fflush(stdout);
  fflush(stderr);
  exit(0);
}

}  // namespace

int main(int argc, char** argv) {
  // Turn off stdout buffering when it is a pipe.
  setvbuf(stdout, NULL, _IONBF, 0);

  printf("[!] Starting the testing process...\n");
  HammerAllReachableRows(&HammerAddressesStandard, number_of_reads);
}
