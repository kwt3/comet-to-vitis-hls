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



#ifndef __MEMORY_INTERFACE_H__
#define __MEMORY_INTERFACE_H__

// #include "ac_int.h"
#include "ap_int.h"

typedef enum { BYTE = 0, HALF, WORD, BYTE_U, HALF_U, LONG } memMask;

typedef enum { NONE = 0, LOAD, STORE } memOpType;

template <unsigned int INTERFACE_SIZE> class MemoryInterface {
protected:
  bool wait;

public:
  virtual void process(const ap_uint<32> addr, const memMask mask, const memOpType opType, const ap_uint<INTERFACE_SIZE * 8> dataIn,
                       ap_uint<INTERFACE_SIZE * 8>& dataOut, bool& waitOut) = 0;
};

template <unsigned int INTERFACE_SIZE> class IncompleteMemory : public MemoryInterface<INTERFACE_SIZE> {
public:
  ap_uint<32>* data;
  ap_uint<1> pendingWrite;
  ap_uint<32> valueLoaded;

public:
  IncompleteMemory(ap_uint<32>* arg) { data = arg; }
  void process(const ap_uint<32> addr, const memMask mask, const memOpType opType, const ap_uint<INTERFACE_SIZE * 8> dataIn,
               ap_uint<INTERFACE_SIZE * 8>& dataOut, bool& waitOut)
  {
    // Incomplete memory only works for 32 bits
    assert(INTERFACE_SIZE == 4);

    ap_int<32> temp;
    ap_uint<8> t8;
    ap_int<1> bit;
    ap_uint<16> t16;
    ap_uint<32> mergedAccess;

    if ((!pendingWrite && opType == STORE && mask != WORD) || opType == LOAD) {

      mergedAccess = data[(addr >> 2)];
      // printf("Loading at %x : %x\n", addr >> 2, mergedAccess);
      if (!pendingWrite && opType == STORE && mask != WORD) {
        waitOut      = true;
        valueLoaded  = mergedAccess;
        pendingWrite = 1;
      } else {
        pendingWrite                 = 0;
        waitOut                      = false;
        ap_uint<32> dataOutTmp = mergedAccess;
        switch (mask) {
          case BYTE:
        //   slc<N>(start) is replaced with range(start + N - 1, start)
            // t8  = dataOutTmp.slc<8>(((int)addr.slc<2>(0)) << 3);
        // Extract an 8-bit slice based on addr
            t8 = dataOutTmp.range((((int)addr.range(1, 0)) << 3) + 7, ((int)addr.range(1, 0)) << 3);
            // bit = t8.slc<1>(7);
            // Extract the most significant bit (7th bit) of t8
            bit = t8.range(7, 7);
            // dataOut.set_slc(0, t8);
            // Set the lower 8 bits of dataOut to t8
            dataOut.range(7, 0) = t8;
            // dataOut.set_slc(8, (ap_int<24>)bit);
            // Set the upper 24 bits of dataOut to the sign-extended bit
            dataOut.range(31, 8) = (ap_int<24>)bit;
            break;
          case HALF:
            // t16 = dataOutTmp.slc<16>(addr[1] ? 16 : 0);
            // Extract a 16-bit slice based on addr
            t16 = dataOutTmp.range(addr[1] ? 31 : 15, addr[1] ? 16 : 0);
            // bit = t16.slc<1>(15);
            // Extract the most significant bit (15th bit) of t16
            bit = t16.range(15, 15);
            // dataOut.set_slc(0, t16);
            // Set the lower 16 bits of dataOut to t16
            dataOut.range(15, 0) = t16;
            // dataOut.set_slc(16, (ap_int<16>)bit);
            // Set the upper 16 bits of dataOut to the sign-extended bit
            dataOut.range(31, 16) = (ap_int<16>)bit;
            break;
          case WORD:
            dataOut = dataOutTmp;
            break;
          case BYTE_U:
            // dataOut = dataOutTmp.slc<8>(((int)addr.slc<2>(0)) << 3) & 0xff;
            // Extract an 8-bit slice, mask with 0xFF
            dataOut = dataOutTmp.range((((int)addr.range(1, 0)) << 3) + 7, ((int)addr.range(1, 0)) << 3) & 0xff;
            break;
          case HALF_U:
            // dataOut = dataOutTmp.slc<16>(addr[1] ? 16 : 0) & 0xffff;
            // Extract a 16-bit slice, mask with 0xFFFF
            dataOut = dataOutTmp.range(addr[1] ? 31 : 15, addr[1] ? 16 : 0) & 0xffff;
            break;
          case LONG:
            break;
          }
      }

    } else if (opType == STORE) {
      pendingWrite = 0;
      // no latency, wait is always set to false
      waitOut                      = false;
      ap_uint<32> valToStore = 0;
      switch (mask) {
        case BYTE_U:
        case BYTE:
          valToStore = valueLoaded;
        //   valToStore.set_slc(((int)addr.slc<2>(0)) << 3, dataIn.template slc<8>(0));
        // Replace set_slc with range assignment
        valToStore.range((((int)addr.range(1, 0)) << 3) + 7, ((int)addr.range(1, 0)) << 3) = dataIn.range(7, 0);
          break;
        case HALF:
        case HALF_U:
          valToStore = valueLoaded;
        //   valToStore.set_slc(addr[1] ? 16 : 0, dataIn.template slc<16>(0));
        // Replace set_slc with range assignment
        valToStore.range(addr[1] ? 31 : 15, addr[1] ? 16 : 0) = dataIn.range(15, 0);
          break;
        case WORD:
          valToStore = dataIn;
          break;
        case LONG:
          break;
        }
      // printf("Loading at %x : %x\n", addr >> 2, mergedAccess);
      data[(addr >> 2)] = valToStore;
    }
  }
};

template <unsigned int INTERFACE_SIZE> class SimpleMemory : public MemoryInterface<INTERFACE_SIZE> {
public:
  ap_uint<32>* data;

  SimpleMemory(ap_uint<32>* arg) { data = arg; }
  void process(const ap_uint<32> addr, const memMask mask, const memOpType opType, const ap_uint<INTERFACE_SIZE * 8> dataIn,
               ap_uint<INTERFACE_SIZE * 8>& dataOut, bool& waitOut)
  {
    // no latency, wait is always set to false

    ap_int<32> temp;
    ap_uint<8> t8;
    ap_int<1> bit;
    ap_uint<16> t16;

    switch (opType) {
      case STORE:
        switch (mask) {
          case BYTE_U:
          case BYTE:
            temp = data[addr >> 2];
            // data[addr >> 2].set_slc(((int)addr.slc<2>(0)) << 3, dataIn.template slc<8>(0));
            // Replace set_slc with range
            data[addr >> 2].range((((int)addr.range(1, 0)) << 3) + 7, ((int)addr.range(1, 0)) << 3) = dataIn.range(7, 0);
            break;
          case HALF:
          case HALF_U:
            temp = data[addr >> 2];
            // data[addr >> 2].set_slc(addr[1] ? 16 : 0, dataIn.template slc<16>(0));
            // Replace set_slc with range
            data[addr >> 2].range(addr[1] ? 31 : 15, addr[1] ? 16 : 0) = dataIn.range(15, 0);
            break;
          case WORD:
            temp            = data[addr >> 2];
            data[addr >> 2] = dataIn;
            break;
          case LONG:
            for (int oneWord = 0; oneWord < INTERFACE_SIZE / 4; oneWord++)
              data[(addr >> 2) + oneWord] = dataIn.template slc<32>(32 * oneWord);
            break;
        }
        break;
      case LOAD:

        switch (mask) {
          case BYTE:
            // t8  = data[addr >> 2].template slc<8>(((int)addr.slc<2>(0)) << 3);
            t8 = data[addr >> 2].range((((int)addr.range(1, 0)) << 3) + 7, ((int)addr.range(1, 0)) << 3);
            bit = t8.range(7,7); // Extract the most significant bit (7th bit) of t8
            // dataOut.set_slc(0, t8);
            // Set the lower 8 bits of dataOut to t8
            dataOut.range(7, 0) = t8;
            // dataOut.set_slc(8, (ap_int<24>)bit);
            // Set the upper 24 bits of dataOut to the sign-extended bit
            dataOut.range(31, 8) = (ap_int<24>)bit;
            break;
          case HALF:
            // t16 = data[addr >> 2].template slc<16>(addr[1] ? 16 : 0)
            t16 = data[addr >> 2].range((addr[1] ? 31 : 15), (addr[1] ? 16 : 0));
            bit = t16.range(15,15);
            // dataOut.set_slc(0, t16);
            // Set the lower 16 bits of dataOut to t16
            dataOut.range(15, 0) = t16;
            // dataOut.set_slc(16, (ap_int<16>)bit);
            // Set the upper 16 bits of dataOut to the sign-extended bit
            dataOut.range(31, 16) = (ap_int<16>)bit;
            break;
          case WORD:
            dataOut = data[addr >> 2];
            break;
          case LONG:
            for (int oneWord = 0; oneWord < INTERFACE_SIZE / 4; oneWord++)
            //   dataOut.set_slc(32 * oneWord, data[(addr >> 2) + oneWord]);
            dataOut.range(32 * oneWord + 31, 32 * oneWord) = data[(addr >> 2) + oneWord];
            break;
          case BYTE_U:
            // dataOut = data[addr >> 2].template slc<8>(((int)addr.slc<2>(0)) << 3) & 0xff;
            // Extract an 8-bit slice, then mask with 0xFF
            dataOut = data[addr >> 2].range((((int)addr.range(1, 0)) << 3) + 7, ((int)addr.range(1, 0)) << 3) & 0xff;
            break;
          case HALF_U:
            // dataOut = data[addr >> 2].template slc<16>(addr[1] ? 16 : 0) & 0xffff;
            // Extract a 16-bit slice, then mask with 0xFFFF
            dataOut = data[addr >> 2].range((addr[1] ? 31 : 15), (addr[1] ? 16 : 0)) & 0xffff;
            break;
        }
        break;

      default: // case NONE
        break;
    }
    waitOut = false;
  }
};

#endif //__MEMORY_INTERFACE_H__
