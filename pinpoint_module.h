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

#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern uint64_t first_data[8];
extern uint64_t target_data[8];
extern uint64_t second_data[8];

void ComputePinpointData(
    uint64_t first_alter[][1024],
    uint64_t second_alter[][1024],
    uint32_t index,
    uint64_t bit_mask,
    uint8_t flip_results);

void HammerWithPattern(
    uint64_t* first_row,
    uint64_t* second_row,
    uint64_t* target_row,
    uint64_t first_data,
    uint64_t second_data,
    uint64_t target_data,
    uint64_t number_of_reads,
    uint64_t* results);

void PinpointRowhammer(
    uint64_t* first_row,
    uint64_t* second_row,
    uint64_t* target_row,
    uint64_t target_data,
    uint64_t first_alter[][1024],
    uint64_t second_alter[][1024],
    uint32_t number_of_reads,
    uint64_t* results);
