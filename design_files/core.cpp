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



// #include "ac_int.h"
#include "ap_int.h" 
#include "cacheMemory.h"
#include "core.h"


void fetch(const ap_uint<32> pc, struct FtoDC& ftoDC, const ap_uint<32> instruction)
{
  ftoDC.instruction = instruction;
  ftoDC.pc          = pc;
  ftoDC.nextPCFetch = pc + 4;
  ftoDC.we          = 1;
}

void decode(const struct FtoDC ftoDC, struct DCtoEx& dctoEx, const ap_int<32> registerFile[32])
{
  const ap_uint<32> pc          = ftoDC.pc;
  const ap_uint<32> instruction = ftoDC.instruction;

  // R-type instruction
//   const ap_uint<7> funct7 = instruction.slc<7>(25);
//   const ap_uint<5> rs2    = instruction.slc<5>(20);
//   const ap_uint<5> rs1    = instruction.slc<5>(15);
//   const ap_uint<3> funct3 = instruction.slc<3>(12);
//   const ap_uint<5> rd     = instruction.slc<5>(7);
//   const ap_uint<7> opCode = instruction.slc<7>(0); // could be reduced to 5 bits because 1:0 is always 11
  const ap_uint<7> funct7 = instruction.range(31, 25); // Extract bits [31:25]
  const ap_uint<5> rs2    = instruction.range(24, 20); // Extract bits [24:20]
  const ap_uint<5> rs1    = instruction.range(19, 15); // Extract bits [19:15]
  const ap_uint<3> funct3 = instruction.range(14, 12); // Extract bits [14:12]
  const ap_uint<5> rd     = instruction.range(11, 7);  // Extract bits [11:7]
  const ap_uint<7> opCode = instruction.range(6, 0);   // Extract bits [6:0] (opCode)
  
  // Construction of different immediate values
  ap_uint<12> imm12_S = 0;
//   imm12_S.set_slc(5, instruction.slc<7>(25));
//   imm12_S.set_slc(0, instruction.slc<5>(7));
  imm12_S.range(11, 5) = instruction.range(31, 25); // Extract bits [31:25] and assign to imm12_S[11:5]
  imm12_S.range(4, 0) = instruction.range(11, 7);  // Extract bits [11:7] and assign to imm12_S[4:0]

//   ap_int<12> imm12_I_signed = instruction.slc<12>(20);
  ap_int<12> imm12_I_signed = instruction.range(31, 20); // Extract bits [31:20] as signed value
  ap_int<12> imm12_S_signed = 0;
//   imm12_S_signed.set_slc(0, imm12_S.slc<12>(0));
  imm12_S_signed.range(11, 0) = imm12_S.range(11, 0); // Copy all bits from imm12_S to imm12_S_signed

  ap_uint<13> imm13 = 0;
  imm13[12]= instruction[31]; // Assign bit 31 (sign bit) to imm13[12]
//   imm13.set_slc(5, instruction.slc<6>(25));
  imm13.range(11, 5) = instruction.range(30, 25);   // Extract bits [30:25] and assign to imm13[11:5]
//   imm13.set_slc(1, instruction.slc<4>(8));
  imm13.range(4, 1) = instruction.range(11, 8);     // Extract bits [11:8] and assign to imm13[4:1]
  imm13[11] = instruction[7]; // Assign bit 7 to imm13[0]

  ap_int<13> imm13_signed = 0;
//   imm13_signed.set_slc(0, imm13);
  imm13_signed.range(12, 0) = imm13.range(12, 0); // Copy all 13 bits from imm13 to imm13_signed

  ap_int<32> imm31_12 = 0;
//   imm31_12.set_slc(12, instruction.slc<20>(12));
  imm31_12.range(31, 12) = instruction.range(31, 12); // Copy bits [31:12] from instruction to imm31_12

  ap_uint<21> imm21_1 = 0;
//   imm21_1.set_slc(12, instruction.slc<8>(12));
  imm21_1.range(20, 12) = instruction.range(31, 24); // Copy bits [31:24] to imm21_1[20:12]
  imm21_1[11] = instruction[20];
//   imm21_1.set_slc(1, instruction.slc<10>(21));
  imm21_1.range(10, 1) = instruction.range(30, 21);  // Copy bits [30:21] to imm21_1[10:1]
  imm21_1[20] = instruction[31];

  ap_int<21> imm21_1_signed = 0;
//   imm21_1_signed.set_slc(0, imm21_1);
  imm21_1_signed.range(20, 0) = imm21_1.range(20, 0); // Copy all 21 bits from imm21_1 to imm21_1_signed

  // Register access
  const ap_uint<32> valueReg1 = registerFile[rs1];
  const ap_uint<32> valueReg2 = registerFile[rs2];

  dctoEx.rs1         = rs1;
  dctoEx.rs2         = rs2;
  dctoEx.rs3         = rs2;
  dctoEx.rd          = rd;
  dctoEx.opCode      = opCode;
  dctoEx.funct3      = funct3;
  dctoEx.funct7      = funct7;
  dctoEx.instruction = instruction;
  dctoEx.pc          = pc;

  // Initialization of control bits
  dctoEx.useRs1   = 0;
  dctoEx.useRs2   = 0;
  dctoEx.useRd    = 0;
  dctoEx.we       = ftoDC.we;
  dctoEx.isBranch = 0;

  switch (opCode) {
    case RISCV_LUI:
      dctoEx.lhs    = imm31_12;
      dctoEx.useRs1 = 0;
      dctoEx.useRs2 = 0;
      dctoEx.useRs3 = 0;
      dctoEx.useRd  = 1;

      break;
    case RISCV_AUIPC:
      dctoEx.lhs    = ftoDC.pc;
      dctoEx.rhs    = imm31_12;
      dctoEx.useRs1 = 0;
      dctoEx.useRs2 = 0;
      dctoEx.useRs3 = 0;
      dctoEx.useRd  = 1;
      break;
    case RISCV_JAL:
      dctoEx.lhs      = ftoDC.pc + 4;
      dctoEx.rhs      = 0;
      dctoEx.nextPCDC = ftoDC.pc + imm21_1_signed;
      dctoEx.useRs1   = 0;
      dctoEx.useRs2   = 0;
      dctoEx.useRs3   = 0;
      dctoEx.useRd    = 1;
      dctoEx.isBranch = 1;

      break;
    case RISCV_JALR:
      dctoEx.lhs    = valueReg1;
      dctoEx.rhs    = imm12_I_signed;
      dctoEx.useRs1 = 1;
      dctoEx.useRs2 = 0;
      dctoEx.useRs3 = 0;
      dctoEx.useRd  = 1;
      break;
    case RISCV_BR:

      dctoEx.lhs    = valueReg1;
      dctoEx.rhs    = valueReg2;
      dctoEx.datac  = imm13_signed;
      dctoEx.useRs1 = 1;
      dctoEx.useRs2 = 1;
      dctoEx.useRs3 = 0;
      dctoEx.useRd  = 0;

      break;
    case RISCV_LD:

      dctoEx.lhs    = valueReg1;
      dctoEx.rhs    = imm12_I_signed;
      dctoEx.useRs1 = 1;
      dctoEx.useRs2 = 0;
      dctoEx.useRs3 = 0;
      dctoEx.useRd  = 1;

      break;

      //******************************************************************************************
      // Treatment for: STORE INSTRUCTIONS
    case RISCV_ST:
      dctoEx.lhs    = valueReg1;
      dctoEx.rhs    = imm12_S_signed;
      dctoEx.datac  = valueReg2; // Value to store in memory
      dctoEx.useRs1 = 1;
      dctoEx.useRs2 = 0;
      dctoEx.useRs3 = 1;
      dctoEx.useRd  = 0;
      dctoEx.rd     = 0;
      break;
    case RISCV_OPI:
      dctoEx.lhs    = valueReg1;
      dctoEx.rhs    = imm12_I_signed;
      dctoEx.useRs1 = 1;
      dctoEx.useRs2 = 0;
      dctoEx.useRs3 = 0;
      dctoEx.useRd  = 1;
      break;

    case RISCV_OP:

      dctoEx.lhs    = valueReg1;
      dctoEx.rhs    = valueReg2;
      dctoEx.useRs1 = 1;
      dctoEx.useRs2 = 1;
      dctoEx.useRs3 = 0;
      dctoEx.useRd  = 1;

      break;
    case RISCV_SYSTEM:


      break;
    default:

      break;
  }

  // If dest is zero, useRd should be at zero
  if (rd == 0) {
    dctoEx.useRd = 0;
  }

  // If the instruction was dropped, we ensure that isBranch is at zero
  if (!ftoDC.we) {
    dctoEx.isBranch = 0;
    dctoEx.useRd    = 0;
    dctoEx.useRs1   = 0;
    dctoEx.useRs2   = 0;
    dctoEx.useRs3   = 0;
  }
}

void execute(const struct DCtoEx dctoEx, struct ExtoMem& extoMem)
{
  extoMem.pc                = dctoEx.pc;
  extoMem.opCode            = dctoEx.opCode;
  extoMem.rd                = dctoEx.rd;
  extoMem.funct3            = dctoEx.funct3;
  extoMem.we                = dctoEx.we;
  extoMem.isBranch          = 0;
  extoMem.useRd             = dctoEx.useRd;
  extoMem.isLongInstruction = 0;
  extoMem.instruction       = dctoEx.instruction;

  // switch must be in the else, otherwise external op may trigger default
  // case
  switch (dctoEx.opCode) {
    case RISCV_LUI:
      extoMem.result = dctoEx.lhs;
      break;
    case RISCV_AUIPC:
      extoMem.result = dctoEx.lhs + dctoEx.rhs;
      break;
    case RISCV_JAL:
      // Note: in current version, the addition is made in the decode stage
      // The value to store in rd (pc+4) is stored in lhs
      extoMem.result = dctoEx.lhs;
      break;
    case RISCV_JALR:
      // Note: in current version, the addition is made in the decode stage
      // The value to store in rd (pc+4) is stored in lhs
      extoMem.nextPC   = dctoEx.rhs + dctoEx.lhs;
      extoMem.isBranch = 1;

      extoMem.result = dctoEx.pc + 4;
      break;
    case RISCV_BR:
      extoMem.nextPC = dctoEx.pc + dctoEx.datac;

      switch (dctoEx.funct3) {
        case RISCV_BR_BEQ:
          extoMem.isBranch = (dctoEx.lhs == dctoEx.rhs);
          break;
        case RISCV_BR_BNE:
          extoMem.isBranch = (dctoEx.lhs != dctoEx.rhs);
          break;
        case RISCV_BR_BLT:
          extoMem.isBranch = (dctoEx.lhs < dctoEx.rhs);
          break;
        case RISCV_BR_BGE:
          extoMem.isBranch = (dctoEx.lhs >= dctoEx.rhs);
          break;
        case RISCV_BR_BLTU:
          extoMem.isBranch = ((ap_uint<32>)dctoEx.lhs < (ap_uint<32>)dctoEx.rhs);
          break;
        case RISCV_BR_BGEU:
          extoMem.isBranch = ((ap_uint<32>)dctoEx.lhs >= (ap_uint<32>)dctoEx.rhs);
          break;
      }
      break;
    case RISCV_LD:
      extoMem.isLongInstruction = 1;
      extoMem.result            = dctoEx.lhs + dctoEx.rhs;
      break;
    case RISCV_ST:
      extoMem.datac  = dctoEx.datac;
      extoMem.result = dctoEx.lhs + dctoEx.rhs;
      break;
    case RISCV_OPI:
      switch (dctoEx.funct3) {
        case RISCV_OPI_ADDI:
          extoMem.result = dctoEx.lhs + dctoEx.rhs;
          break;
        case RISCV_OPI_SLTI:
          extoMem.result = dctoEx.lhs < dctoEx.rhs;
          break;
        case RISCV_OPI_SLTIU:
          extoMem.result = (ap_uint<32>)dctoEx.lhs < (ap_uint<32>)dctoEx.rhs;
          break;
        case RISCV_OPI_XORI:
          extoMem.result = dctoEx.lhs ^ dctoEx.rhs;
          break;
        case RISCV_OPI_ORI:
          extoMem.result = dctoEx.lhs | dctoEx.rhs;
          break;
        case RISCV_OPI_ANDI:
          extoMem.result = dctoEx.lhs & dctoEx.rhs;
          break;
        case RISCV_OPI_SLLI: // cast rhs as 5 bits, otherwise generated hardware
                             // is 32 bits
          // & shift amount held in the lower 5 bits (riscv spec)
          extoMem.result = dctoEx.lhs << (ap_uint<5>)dctoEx.rhs;
          break;
        case RISCV_OPI_SRI:
          if (dctoEx.funct7[5]) // SRAI
            // extoMem.result = dctoEx.lhs >> (ap_uint<5>)dctoEx.rhs.slc<5>(0);
            extoMem.result = dctoEx.lhs >> (ap_uint<5>)dctoEx.rhs.range(4, 0);
          else // SRLI
            // extoMem.result = (ap_uint<32>)dctoEx.lhs >> (ap_uint<5>)dctoEx.rhs.slc<5>(0);
            extoMem.result = (ap_uint<32>)dctoEx.lhs >> (ap_uint<5>)dctoEx.rhs.range(4, 0);
          break;
      }
      break;
    case RISCV_OP:
      if (dctoEx.funct7[0]) // M Extension
      {

      } else {
        switch (dctoEx.funct3) {
          case RISCV_OP_ADD:
            if (dctoEx.funct7[5]) // SUB
              extoMem.result = dctoEx.lhs - dctoEx.rhs;
            else // ADD
              extoMem.result = dctoEx.lhs + dctoEx.rhs;
            break;
          case RISCV_OP_SLL:
            extoMem.result = dctoEx.lhs << (ap_uint<5>)dctoEx.rhs;
            break;
          case RISCV_OP_SLT:
            extoMem.result = dctoEx.lhs < dctoEx.rhs;
            break;
          case RISCV_OP_SLTU:
            extoMem.result = (ap_uint<32>)dctoEx.lhs < (ap_uint<32>)dctoEx.rhs;
            break;
          case RISCV_OP_XOR:
            extoMem.result = dctoEx.lhs ^ dctoEx.rhs;
            break;
          case RISCV_OP_SR:
            if (dctoEx.funct7[5]) // SRA
              extoMem.result = dctoEx.lhs >> (ap_uint<5>)dctoEx.rhs;
            else // SRL
              extoMem.result = (ap_uint<32>)dctoEx.lhs >> (ap_uint<5>)dctoEx.rhs;
            break;
          case RISCV_OP_OR:
            extoMem.result = dctoEx.lhs | dctoEx.rhs;
            break;
          case RISCV_OP_AND:
            extoMem.result = dctoEx.lhs & dctoEx.rhs;
            break;
        }
      }
      break;
    case RISCV_MISC_MEM: // this does nothing because all memory accesses are
                         // ordered, and we have only one core
      break;

    case RISCV_SYSTEM:
      switch (dctoEx.funct3) { // case 0: mret instruction, dctoEx.memValue
                               // should be 0x302
        case RISCV_SYSTEM_ENV:
#ifndef __HLS__
          // TODO handling syscall correctly
          // extoMem.result = sim->solveSyscall(dctoEx.lhs, dctoEx.rhs,
          // dctoEx.datac, dctoEx.datad, dctoEx.datae, exit);
#endif
          break;
        case RISCV_SYSTEM_CSRRW:       // lhs is from csr, rhs is from reg[rs1]
          extoMem.datac  = dctoEx.rhs; // written back to csr
          extoMem.result = dctoEx.lhs; // written back to rd
          break;
        case RISCV_SYSTEM_CSRRS:
          extoMem.datac  = dctoEx.lhs | dctoEx.rhs;
          extoMem.result = dctoEx.lhs;
          break;
        case RISCV_SYSTEM_CSRRC:
          extoMem.datac  = dctoEx.lhs & ((ap_uint<32>)~dctoEx.rhs);
          extoMem.result = dctoEx.lhs;
          break;
        case RISCV_SYSTEM_CSRRWI:
          extoMem.datac  = dctoEx.rhs;
          extoMem.result = dctoEx.lhs;
          break;
        case RISCV_SYSTEM_CSRRSI:
          extoMem.datac  = dctoEx.lhs | dctoEx.rhs;
          extoMem.result = dctoEx.lhs;
          break;
        case RISCV_SYSTEM_CSRRCI:
          extoMem.datac  = dctoEx.lhs & ((ap_uint<32>)~dctoEx.rhs);
          extoMem.result = dctoEx.lhs;
          break;
      }
      break;
  }

  // If the instruction was dropped, we ensure that isBranch is at zero
  if (!dctoEx.we) {
    extoMem.isBranch = 0;
    extoMem.useRd    = 0;
  }
}

void memory(const struct ExtoMem extoMem, struct MemtoWB& memtoWB)
{
  memtoWB.we                = extoMem.we;
  memtoWB.useRd             = extoMem.useRd;
  memtoWB.result            = extoMem.result;
  memtoWB.rd                = extoMem.rd;

  ap_uint<32> mem_read;

  switch (extoMem.opCode) {
    case RISCV_LD:
      memtoWB.rd = extoMem.rd;

      memtoWB.address = extoMem.result;
      memtoWB.isLoad  = 1;
      //    formatread(extoMem.result, datasize, signenable, mem_read); //TODO
      break;
    case RISCV_ST:
      //        mem_read = dataMemory[extoMem.result >> 2];
      // if(extoMem.we) //TODO0: We do not handle non 32bit writes
      //        	dataMemory[extoMem.result >> 2] = extoMem.datac;
      memtoWB.isStore      = 1;
      memtoWB.address      = extoMem.result;
      memtoWB.valueToWrite = extoMem.datac;
      memtoWB.byteEnable   = 0xf;

      break;
  }
}

void writeback(const struct MemtoWB memtoWB, struct WBOut& wbOut)
{
  wbOut.we = memtoWB.we;
  if ((memtoWB.rd != 0) && (memtoWB.we) && memtoWB.useRd) {
    wbOut.rd    = memtoWB.rd;
    wbOut.value = memtoWB.result;
    wbOut.useRd = 1;
  }
}

void branchUnit(const ap_uint<32> nextPC_fetch, const ap_uint<32> nextPC_decode, const bool isBranch_decode,
                const ap_uint<32> nextPC_execute, const bool isBranch_execute, ap_uint<32>& pc, bool& we_fetch,
                bool& we_decode, const bool stall_fetch)
{

  if (!stall_fetch) {
    if (isBranch_execute) {
      we_fetch  = 0;
      we_decode = 0;
      pc        = nextPC_execute;
    } else if (isBranch_decode) {
      we_fetch = 0;
      pc       = nextPC_decode;
    } else {
      pc = nextPC_fetch;
    }
  }
}

void forwardUnit(const ap_uint<5> decodeRs1, const bool decodeUseRs1,
                 const ap_uint<5> decodeRs2, const bool decodeUseRs2,
                 const ap_uint<5> decodeRs3, const bool decodeUseRs3,
                 const ap_uint<5> executeRd, const bool executeUseRd,
                 const bool executeIsLongComputation,
                 const ap_uint<5> memoryRd, const bool memoryUseRd,
                 const ap_uint<5> writebackRd, const bool writebackUseRd,
                 bool stall[5], struct ForwardReg& forwardRegisters)
{

  if (decodeUseRs1) {
    if (executeUseRd && decodeRs1 == executeRd) {
      if (executeIsLongComputation) {
        stall[0] = 1;
        stall[1] = 1;
      } else {
        forwardRegisters.forwardExtoVal1 = 1;
      }
    } else if (memoryUseRd && decodeRs1 == memoryRd) {
      forwardRegisters.forwardMemtoVal1 = 1;
    } else if (writebackUseRd && decodeRs1 == writebackRd) {
      forwardRegisters.forwardWBtoVal1 = 1;
    }
  }

  if (decodeUseRs2) {
    if (executeUseRd && decodeRs2 == executeRd) {
      if (executeIsLongComputation) {
        stall[0] = 1;
        stall[1] = 1;
      } else {
        forwardRegisters.forwardExtoVal2 = 1;
      }
    } else if (memoryUseRd && decodeRs2 == memoryRd)
      forwardRegisters.forwardMemtoVal2 = 1;
    else if (writebackUseRd && decodeRs2 == writebackRd)
      forwardRegisters.forwardWBtoVal2 = 1;
  }

  if (decodeUseRs3) {
    if (executeUseRd && decodeRs3 == executeRd) {
      if (executeIsLongComputation) {
        stall[0] = 1;
        stall[1] = 1;
      } else {
        forwardRegisters.forwardExtoVal3 = 1;
      }
    } else if (memoryUseRd && decodeRs3 == memoryRd)
      forwardRegisters.forwardMemtoVal3 = 1;
    else if (writebackUseRd && decodeRs3 == writebackRd)
      forwardRegisters.forwardWBtoVal3 = 1;
  }
}

void doCycle(struct Core& core, // Core containing all values
             bool globalStall)
{
  // printf("PC : %x\n", core.pc);
  bool localStall = globalStall;

  core.stallSignals[0] = 0;
  core.stallSignals[1] = 0;
  core.stallSignals[2] = 0;
  core.stallSignals[3] = 0;
  core.stallSignals[4] = 0;
  core.stallIm         = false;
  core.stallDm         = false;

  // declare temporary structs
  struct FtoDC ftoDC_temp;
  ftoDC_temp.pc          = 0;
  ftoDC_temp.instruction = 0;
  ftoDC_temp.nextPCFetch = 0;
  ftoDC_temp.we          = 0;
  struct DCtoEx dctoEx_temp;
  dctoEx_temp.isBranch = 0;
  dctoEx_temp.useRs1   = 0;
  dctoEx_temp.useRs2   = 0;
  dctoEx_temp.useRs3   = 0;
  dctoEx_temp.useRd    = 0;
  dctoEx_temp.we       = 0;
  struct ExtoMem extoMem_temp;
  extoMem_temp.useRd    = 0;
  extoMem_temp.isBranch = 0;
  extoMem_temp.we       = 0;
  struct MemtoWB memtoWB_temp;
  memtoWB_temp.useRd   = 0;
  memtoWB_temp.isStore = 0;
  memtoWB_temp.we      = 0;
  memtoWB_temp.isLoad  = 0;
  struct WBOut wbOut_temp;
  wbOut_temp.useRd = 0;
  wbOut_temp.we    = 0;
  wbOut_temp.rd    = 0;
  struct ForwardReg forwardRegisters;
  forwardRegisters.forwardExtoVal1  = 0;
  forwardRegisters.forwardExtoVal2  = 0;
  forwardRegisters.forwardExtoVal3  = 0;
  forwardRegisters.forwardMemtoVal1 = 0;
  forwardRegisters.forwardMemtoVal2 = 0;
  forwardRegisters.forwardMemtoVal3 = 0;
  forwardRegisters.forwardWBtoVal1  = 0;
  forwardRegisters.forwardWBtoVal2  = 0;
  forwardRegisters.forwardWBtoVal3  = 0;

  // declare temporary register file
  ap_uint<32> nextInst;

  core.im->process(core.pc, WORD, (!localStall && !core.stallDm) ? LOAD : NONE, 0, nextInst, core.stallIm);

  fetch(core.pc, ftoDC_temp, nextInst);
  decode(core.ftoDC, dctoEx_temp, core.regFile);
  execute(core.dctoEx, extoMem_temp);
  memory(core.extoMem, memtoWB_temp);
  writeback(core.memtoWB, wbOut_temp);

  // resolve stalls, forwards
  if (!localStall)
    forwardUnit(dctoEx_temp.rs1, dctoEx_temp.useRs1, dctoEx_temp.rs2, dctoEx_temp.useRs2, dctoEx_temp.rs3,
                dctoEx_temp.useRs3, extoMem_temp.rd, extoMem_temp.useRd, extoMem_temp.isLongInstruction,
                memtoWB_temp.rd, memtoWB_temp.useRd, wbOut_temp.rd, wbOut_temp.useRd, core.stallSignals,
                forwardRegisters);

  memMask mask;
  // TODO: carry the data size to memToWb
  switch (core.extoMem.funct3) {
    case 0:
      mask = BYTE;
      break;
    case 1:
      mask = HALF;
      break;
    case 2:
      mask = WORD;
      break;
    case 4:
      mask = BYTE_U;
      break;
    case 5:
      mask = HALF_U;
      break;
    // Should NEVER happen
    default:
      mask = WORD;
      break;
  }

  memOpType opType = (!core.stallSignals[STALL_MEMORY] && !localStall && memtoWB_temp.we && !core.stallIm && memtoWB_temp.isLoad) ? LOAD
    : (!core.stallSignals[STALL_MEMORY] && !localStall && memtoWB_temp.we && !core.stallIm && memtoWB_temp.isStore ? STORE : NONE);

  core.dm->process(memtoWB_temp.address, mask, opType, memtoWB_temp.valueToWrite, memtoWB_temp.result, core.stallDm);

  // commit the changes to the pipeline register
  if (!core.stallSignals[STALL_FETCH] && !localStall && !core.stallIm && !core.stallDm) {
    core.ftoDC = ftoDC_temp;
  }

  if (!core.stallSignals[STALL_DECODE] && !localStall && !core.stallIm && !core.stallDm) {
    core.dctoEx = dctoEx_temp;

    if (forwardRegisters.forwardExtoVal1 && extoMem_temp.we)
      core.dctoEx.lhs = extoMem_temp.result;
    else if (forwardRegisters.forwardMemtoVal1 && memtoWB_temp.we)
      core.dctoEx.lhs = memtoWB_temp.result;
    else if (forwardRegisters.forwardWBtoVal1 && wbOut_temp.we)
      core.dctoEx.lhs = wbOut_temp.value;

    if (forwardRegisters.forwardExtoVal2 && extoMem_temp.we)
      core.dctoEx.rhs = extoMem_temp.result;
    else if (forwardRegisters.forwardMemtoVal2 && memtoWB_temp.we)
      core.dctoEx.rhs = memtoWB_temp.result;
    else if (forwardRegisters.forwardWBtoVal2 && wbOut_temp.we)
      core.dctoEx.rhs = wbOut_temp.value;

    if (forwardRegisters.forwardExtoVal3 && extoMem_temp.we)
      core.dctoEx.datac = extoMem_temp.result;
    else if (forwardRegisters.forwardMemtoVal3 && memtoWB_temp.we)
      core.dctoEx.datac = memtoWB_temp.result;
    else if (forwardRegisters.forwardWBtoVal3 && wbOut_temp.we)
      core.dctoEx.datac = wbOut_temp.value;
  }

  if (core.stallSignals[STALL_DECODE] && !core.stallSignals[STALL_EXECUTE] && !core.stallIm && !core.stallDm &&
      !localStall) {
    core.dctoEx.we          = 0;
    core.dctoEx.useRd       = 0;
    core.dctoEx.isBranch    = 0;
    core.dctoEx.instruction = 0;
    core.dctoEx.pc          = 0;
  }

  if (!core.stallSignals[STALL_EXECUTE] && !localStall && !core.stallIm && !core.stallDm) {
    core.extoMem = extoMem_temp;
  }

  if (!core.stallSignals[STALL_MEMORY] && !localStall && !core.stallIm && !core.stallDm) {
    core.memtoWB = memtoWB_temp;
  }

  if (wbOut_temp.we && wbOut_temp.useRd && !localStall && !core.stallIm && !core.stallDm) {
    core.regFile[wbOut_temp.rd] = wbOut_temp.value;
  }

  branchUnit(ftoDC_temp.nextPCFetch, dctoEx_temp.nextPCDC, dctoEx_temp.isBranch, extoMem_temp.nextPC,
             extoMem_temp.isBranch, core.pc, core.ftoDC.we, core.dctoEx.we,
             core.stallSignals[STALL_FETCH] || core.stallIm || core.stallDm || localStall);

  core.cycle++;
}

// void doCore(IncompleteMemory im, IncompleteMemory dm, bool globalStall)
void doCore(bool globalStall, ap_uint<32> imData[1 << 24],
            ap_uint<32> dmData[1 << 24])
{
  Core core;

  IncompleteMemory<4> imInterface = IncompleteMemory<4>(imData);
  IncompleteMemory<4> dmInterface = IncompleteMemory<4>(dmData);

  // CacheMemory<4, 16, 64> dmCache = CacheMemory<4, 16, 64>(&dmInterface, false);
  // CacheMemory<4, 16, 64> imCache = CacheMemory<4, 16, 64>(&imInterface, false);

  core.im         = &imInterface;
  core.dm         = &dmInterface;
  core.pc         = 0;

  while (1) {
    doCycle(core, globalStall);
  }
  return;
}
