/** Copyright 2021 INRIA, Université de Rennes 1 and ENS Rennes
*
*   Licensed under the Apache License, Version 2.0 (the "License");
*   you may not use this file except in compliance with the License.
*   You may obtain a copy of the License at
*       http://www.apache.org/licenses/LICENSE-2.0
*   Unless required by applicable law or agreed to in writing, software
*   distributed under the License is distributed on an "AS IS" BASIS,
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*   See the License for the specific language governing permissions and
*   limitations under the License.
*/



#ifndef PIPELINE_REGISTERS_H_
#define PIPELINE_REGISTERS_H_
#include "ap_int.h"
/******************************************************************************************
 * Definition of all pipeline registers
 *
 * ****************************************************************************************
 */

struct ForwardReg {
  bool forwardWBtoVal1;
  bool forwardWBtoVal2;
  bool forwardWBtoVal3;

  bool forwardMemtoVal1;
  bool forwardMemtoVal2;
  bool forwardMemtoVal3;

  bool forwardExtoVal1;
  bool forwardExtoVal2;
  bool forwardExtoVal3;
};

struct FtoDC {
  FtoDC() : pc(0), instruction(0x13), we(1) {}
//   ac_int<32, false> pc;          // PC where to fetch
  ap_uint<32> pc;          // PC where to fetch
  ap_uint<32> instruction; // Instruction to execute
  ap_uint<32> nextPCFetch; // Next pc according to fetch
  // Register for all stages
  bool we;
};

struct DCtoEx {
  ap_uint<32> pc; // used for branch
  ap_uint<32> instruction;

  ap_uint<7> opCode; // opCode = instruction[6:0]
  ap_uint<7> funct7; // funct7 = instruction[31:25]
  ap_uint<3> funct3; // funct3 = instruction[14:12]

  ap_int<32> lhs;   //  left hand side : operand 1
  ap_int<32> rhs;   // right hand side : operand 2
  ap_int<32> datac; // ST, BR, JAL/R,

  // For branch unit
  ap_uint<32> nextPCDC;
  bool isBranch;

  // Information for forward/stall unit
  bool useRs1;
  bool useRs2;
  bool useRs3;
  bool useRd;
  ap_uint<5> rs1; // rs1    = instruction[19:15]
  ap_uint<5> rs2; // rs2    = instruction[24:20]
  ap_uint<5> rs3;
  ap_uint<5> rd; // rd     = instruction[11:7]

  // Register for all stages
  bool we;
};

struct ExtoMem {
// ac_int<32, false> → ap_uint<32>
// ac_int<32, true> → ap_int<32>
  ap_uint<32> pc;
  ap_uint<32> instruction;

  ap_int<32> result; // result of the EX stage
  ap_uint<5> rd;     // destination register
  bool useRd;
  bool isLongInstruction;
  ap_uint<7> opCode; // LD or ST (can be reduced to 2 bits)
  ap_uint<3> funct3; // datasize and sign extension bit

  ap_int<32> datac; // data to be stored in memory or csr result

  // For branch unit
  ap_uint<32> nextPC;
  bool isBranch;

  // Register for all stages
  bool we;
};

struct MemtoWB {
  ap_uint<32> result; // Result to be written back
  ap_uint<5> rd;      // destination register
  bool useRd;

  ap_int<32> address;
  ap_uint<32> valueToWrite;
  ap_uint<4> byteEnable;
  bool isStore;
  bool isLoad;

  // Register for all stages
  bool we;
};

struct WBOut {
  ap_uint<32> value;
  ap_uint<5> rd;
  bool useRd;
  bool we;
};

#endif /* PIPELINE_REGISTERS_H_ */
