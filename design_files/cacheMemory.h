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



#ifndef INCLUDE_CACHEMEMORY_H_
#define INCLUDE_CACHEMEMORY_H_

#include "logarithm.h"
#include "memoryInterface.h"
// #include "ac_int.h"
#include "ap_int.h"

/************************************************************************
 * 	Following values are templates:
 * 		- OFFSET_SIZE
 * 		- TAG_SIZE
 * 		- SET_SIZE
 * 		- ASSOCIATIVITY
 ************************************************************************/
template <unsigned int INTERFACE_SIZE, int LINE_SIZE, int SET_SIZE>
class CacheMemory : public MemoryInterface<INTERFACE_SIZE> {

  static const int LOG_SET_SIZE           = log2const<SET_SIZE>::value;
  static const int LOG_LINE_SIZE          = log2const<LINE_SIZE>::value;
  static const int TAG_SIZE               = (32 - LOG_LINE_SIZE - LOG_SET_SIZE);
  static const int ASSOCIATIVITY          = 4;
  static const int LOG_ASSOCIATIVITY      = 2;
  static const int STATE_CACHE_MISS       = ((LINE_SIZE / INTERFACE_SIZE) * 2 + 2);
  static const int STATE_CACHE_LAST_STORE = ((LINE_SIZE / INTERFACE_SIZE) + 3);
  static const int STATE_CACHE_FIRST_LOAD = ((LINE_SIZE / INTERFACE_SIZE) + 2);
  static const int STATE_CACHE_LAST_LOAD  = 2;
  static const int LOG_INTERFACE_SIZE     = log2const<INTERFACE_SIZE>::value;

public:
  IncompleteMemory<INTERFACE_SIZE>* nextLevel;

  ap_uint<TAG_SIZE + LINE_SIZE * 8> cacheMemory[SET_SIZE][ASSOCIATIVITY];
  ap_uint<40> age[SET_SIZE][ASSOCIATIVITY];
  ap_uint<1> dataValid[SET_SIZE][ASSOCIATIVITY];
  ap_uint<1> dirtyBit[SET_SIZE][ASSOCIATIVITY];

  ap_uint<6> cacheState;                // Used for the internal state machine
  ap_uint<LOG_ASSOCIATIVITY> older = 0; // Set where the miss occurs

  // Variables for next level access
  ap_uint<LINE_SIZE * 8 + TAG_SIZE> newVal, oldVal;
  ap_uint<32> nextLevelAddr;
  memOpType nextLevelOpType;
  ap_uint<INTERFACE_SIZE * 8> nextLevelDataIn;
  ap_uint<INTERFACE_SIZE * 8> nextLevelDataOut;
  ap_uint<40> cycle;
  ap_uint<LOG_ASSOCIATIVITY> setMiss;
  bool isValid;
  bool isDirty;

  bool wasStore = false;
  ap_uint<LOG_ASSOCIATIVITY> setStore;
  ap_uint<LOG_SET_SIZE> placeStore;
  ap_uint<LINE_SIZE * 8 + TAG_SIZE> valStore;
  ap_uint<INTERFACE_SIZE * 8> dataOutStore;
  ap_uint<1> valDirty = 0;

  bool nextLevelWaitOut;

  bool VERBOSE = false;

  // Stats
  unsigned long numberAccess, numberMiss;

  CacheMemory(IncompleteMemory<INTERFACE_SIZE>* nextLevel, bool v)
  {
    this->nextLevel = nextLevel;
    for (int oneSetElement = 0; oneSetElement < SET_SIZE; oneSetElement++) {
      for (int oneSet = 0; oneSet < ASSOCIATIVITY; oneSet++) {
        cacheMemory[oneSetElement][oneSet] = 0;
        age[oneSetElement][oneSet]         = 0;
        dataValid[oneSetElement][oneSet]   = 0;
        dirtyBit[oneSetElement][oneSet]    = 0;
      }
    }
    VERBOSE          = v;
    numberAccess     = 0;
    numberMiss       = 0;
    nextLevelWaitOut = false;
    wasStore         = false;
    cacheState       = 0;
    nextLevelOpType  = NONE;
  }

  void process(ap_uint<32> addr, memMask mask, memOpType opType, ap_uint<INTERFACE_SIZE * 8> dataIn,
               ap_uint<INTERFACE_SIZE * 8>& dataOut, bool& waitOut)
  {

    // bit size is the log(setSize)
    // ap_uint<LOG_SET_SIZE> place = addr.slc<LOG_SET_SIZE>(LOG_LINE_SIZE);
    ap_uint<LOG_SET_SIZE> place = addr.range(LOG_LINE_SIZE + LOG_SET_SIZE - 1, LOG_LINE_SIZE);
    // startAddress is log(lineSize) + log(setSize) + 2
    // ap_uint<TAG_SIZE> tag = addr.slc<TAG_SIZE>(LOG_LINE_SIZE + LOG_SET_SIZE);
    ap_uint<TAG_SIZE> tag = addr.range(LOG_LINE_SIZE + LOG_SET_SIZE + TAG_SIZE - 1, LOG_LINE_SIZE + LOG_SET_SIZE);
    // bitSize is log(lineSize), start address is 2(because of #bytes in a word)
    // ap_uint<LOG_LINE_SIZE> offset = addr.slc<LOG_LINE_SIZE - 2>(2);
    ap_uint<LOG_LINE_SIZE> offset = addr.range(LOG_LINE_SIZE - 1, 2);
    if (!nextLevelWaitOut) {
      cycle++;

      if (wasStore || cacheState == 1) {
        cacheMemory[placeStore][setStore] = valStore;
        age[placeStore][setStore]         = cycle;
        dataValid[placeStore][setStore]   = 1;
        dirtyBit[placeStore][setStore]    = valDirty;
        dataOut                           = dataOutStore;
        wasStore                          = false;
        cacheState                        = 0;
        waitOut                           = 0;
        return;
      } else if (opType != NONE) {

        ap_uint<LINE_SIZE * 8 + TAG_SIZE> val1 = cacheMemory[place][0];
        ap_uint<LINE_SIZE * 8 + TAG_SIZE> val2 = cacheMemory[place][1];
        ap_uint<LINE_SIZE * 8 + TAG_SIZE> val3 = cacheMemory[place][2];
        ap_uint<LINE_SIZE * 8 + TAG_SIZE> val4 = cacheMemory[place][3];

        ap_uint<1> valid1 = dataValid[place][0];
        ap_uint<1> valid2 = dataValid[place][1];
        ap_uint<1> valid3 = dataValid[place][2];
        ap_uint<1> valid4 = dataValid[place][3];


        ap_uint<16> age1 = age[place][0];
        ap_uint<16> age2 = age[place][1];
        ap_uint<16> age3 = age[place][2];
        ap_uint<16> age4 = age[place][3];

        ap_uint<1> dirty1 = dirtyBit[place][0];
        ap_uint<1> dirty2 = dirtyBit[place][1];
        ap_uint<1> dirty3 = dirtyBit[place][2];
        ap_uint<1> dirty4 = dirtyBit[place][3];

        if (cacheState == 0) {
          numberAccess++;

        //   ap_uint<TAG_SIZE> tag1 = val1.template slc<TAG_SIZE>(0);
          ap_uint<TAG_SIZE> tag1 = val1.range(TAG_SIZE - 1, 0);
          ap_uint<TAG_SIZE> tag2 = val2.range(TAG_SIZE - 1, 0);
          ap_uint<TAG_SIZE> tag3 = val3.range(TAG_SIZE - 1, 0);
          ap_uint<TAG_SIZE> tag4 = val4.range(TAG_SIZE - 1, 0);

          bool hit1 = (tag1 == tag) && valid1;
          bool hit2 = (tag2 == tag) && valid2;
          bool hit3 = (tag3 == tag) && valid3;
          bool hit4 = (tag4 == tag) && valid4;
          bool hit  = hit1 | hit2 | hit3 | hit4;

          ap_uint<LOG_ASSOCIATIVITY> set = 0;
          ap_uint<LINE_SIZE * 8> selectedValue;
          ap_uint<TAG_SIZE> tag;

          if (hit1) {
            // selectedValue = val1.template slc<LINE_SIZE * 8>(TAG_SIZE);
            // slc<N>(start) is replaced by range(upper, lower)
            // upper = TAG_SIZE + (LINE_SIZE * 8) - 1 (most significant bit of the slice)
            // lower = TAG_SIZE (least significant bit of the slice)
            selectedValue = val1.range(TAG_SIZE + LINE_SIZE * 8 - 1, TAG_SIZE);
            tag           = tag1;
            set           = 0;
          }

          if (hit2) {
            selectedValue = val2.range(TAG_SIZE + LINE_SIZE * 8 - 1, TAG_SIZE);
            tag           = tag2;
            set           = 1;
          }

          if (hit3) {
            selectedValue = val3.range(TAG_SIZE + LINE_SIZE * 8 - 1, TAG_SIZE);
            tag           = tag3;
            set           = 2;
          }

          if (hit4) {
            selectedValue = val4.range(TAG_SIZE + LINE_SIZE * 8 - 1, TAG_SIZE);
            tag           = tag4;
            set           = 3;
          }

          ap_int<8> signedByte;
          ap_int<16> signedHalf;
          ap_int<32> signedWord;

          if (hit) {
            ap_uint<LINE_SIZE * 8 + TAG_SIZE> localValStore = 0;
            // localValStore.set_slc(TAG_SIZE, selectedValue);
            // range(upper, lower) replaces set_slc.
            // upper = TAG_SIZE + selectedValue.length() - 1.
            // lower = TAG_SIZE
            localValStore.range(TAG_SIZE + selectedValue.length() - 1, TAG_SIZE) = selectedValue;
            // localValStore.set_slc(0, tag);
            localValStore.range(tag.length() - 1, 0) = tag;
            // First we handle the store
            if (opType == STORE) {
              switch (mask) {
                case BYTE:
                case BYTE_U:
                //   localValStore.set_slc((((int)addr.slc<2>(0)) << 3) + TAG_SIZE + 4 * 8 * offset,
                //                         dataIn.template slc<8>(0));
                    // Extract an 8-bit slice from dataIn and store it in the calculated position of localValStore
                    localValStore.range(
                        (((int)addr.range(1, 0)) << 3) + TAG_SIZE + 4 * 8 * offset + 7,
                        (((int)addr.range(1, 0)) << 3) + TAG_SIZE + 4 * 8 * offset
                    ) = dataIn.range(7, 0); // Extract the lower 8 bits of dataIn
                  break;
                case HALF:
                case HALF_U:
                //   localValStore.set_slc((addr[1] ? 16 : 0) + TAG_SIZE + 4 * 8 * offset, dataIn.template slc<16>(0));
                    // Extract a 16-bit slice from dataIn and store it in the calculated position of localValStore
                    localValStore.range(
                        (addr[1] ? 16 : 0) + TAG_SIZE + 4 * 8 * offset + 15,
                        (addr[1] ? 16 : 0) + TAG_SIZE + 4 * 8 * offset
                    ) = dataIn.range(15, 0); // Extract the lower 16 bits of dataIn
                  break;
                case WORD:
                //   localValStore.set_slc(TAG_SIZE + 4 * 8 * offset, dataIn.template slc<32>(0));
                    // Extract a 32-bit slice from dataIn and store it in the calculated position of localValStore
                    localValStore.range(
                        TAG_SIZE + 4 * 8 * offset + 31,
                        TAG_SIZE + 4 * 8 * offset
                    ) = dataIn.range(31, 0); // Extract the lower 32 bits of dataIn
                  break;
                case LONG:
                //   localValStore.set_slc(TAG_SIZE + 4 * 8 * offset, dataIn);
                    // Assign the entire value of dataIn to the calculated position in localValStore
                    localValStore.range(
                        TAG_SIZE + 4 * 8 * offset + dataIn.length() - 1,
                        TAG_SIZE + 4 * 8 * offset
                    ) = dataIn; // Assign the entire dataIn to localValStore
                  break;
              }

              placeStore = place;
              setStore   = set;
              valStore   = localValStore;
              valDirty   = 1;
              wasStore   = true;

            } else {
              switch (mask) {
                case BYTE:
                //   signedByte = selectedValue.template slc<8>((((int)addr.slc<2>(0)) << 3) + 4 * 8 * offset);
                // Extract an 8-bit signed slice and assign it to signedWord
                  signedByte = selectedValue.range(
                      (((int)addr.range(1, 0)) << 3) + 4 * 8 * offset + 7,
                      (((int)addr.range(1, 0)) << 3) + 4 * 8 * offset
                  );
                  signedWord = signedByte;
                //   dataOut.set_slc(0, signedWord);
                  dataOut.range(31, 0) = signedWord; // Ensure `dataOut` receives the 32-bit signed value
                  break;
                case HALF:
                //   signedHalf = selectedValue.template slc<16>((addr[1] ? 16 : 0) + 4 * 8 * offset);
                  // Extract a 16-bit signed slice and assign it to signedWord
                  signedHalf = selectedValue.range(
                      (addr[1] ? 16 : 0) + 4 * 8 * offset + 15,
                      (addr[1] ? 16 : 0) + 4 * 8 * offset
                  );
                  signedWord = signedHalf;
                  dataOut.range(31, 0) = signedWord; // Ensure `dataOut` receives the 32-bit signed value
                  break;
                case WORD:
                //   dataOut = selectedValue.template slc<32>(4 * 8 * offset);
                  // Extract a 32-bit slice
                  dataOut.range(31, 0) = selectedValue.range(4 * 8 * offset + 31, 4 * 8 * offset);
                  break;
                case BYTE_U:
                //   dataOut = selectedValue.template slc<8>((((int)addr.slc<2>(0)) << 3) + 4 * 8 * offset) & 0xff;
                  // Extract an 8-bit unsigned slice and mask it with 0xFF
                  dataOut.range(7, 0) = selectedValue.range(
                      (((int)addr.range(1, 0)) << 3) + 4 * 8 * offset + 7,
                      (((int)addr.range(1, 0)) << 3) + 4 * 8 * offset
                  ) & 0xFF;
                  dataOut.range(31, 8) = 0; // Zero out remaining bits for unsigned BYTE
                  break;
                case HALF_U:
                //   dataOut = selectedValue.template slc<16>((addr[1] ? 16 : 0) + 4 * 8 * offset) & 0xffff;
                  // Extract a 16-bit unsigned slice and mask it with 0xFFFF
                  dataOut.range(15, 0) = selectedValue.range(
                      (addr[1] ? 16 : 0) + 4 * 8 * offset + 15,
                      (addr[1] ? 16 : 0) + 4 * 8 * offset
                  ) & 0xFFFF;
                  dataOut.range(31, 16) = 0; // Zero out remaining bits for unsigned HALF
                  break;
                case LONG:
                //   dataOut = selectedValue.template slc<INTERFACE_SIZE * 8>(4 * 8 * offset);
                  // Extract a slice of `INTERFACE_SIZE * 8` bits
                  for (int i = 0; i < INTERFACE_SIZE; i++) {
                      dataOut.range((i + 1) * 8 - 1, i * 8) = selectedValue.range(
                          4 * 8 * offset + (i + 1) * 8 - 1,
                          4 * 8 * offset + i * 8
                      );
                  }
                    break;
              }

              // printf("Hit read %x at %x\n", (unsigned int)dataOut.slc<32>(0), (unsigned int)addr);
            }
            // age[place][set] = cycle;

          } else {
            numberMiss++;
            cacheState = STATE_CACHE_MISS;
          }
        } else {
          // printf("Miss %d\n", (unsigned int)cacheState);

          if (cacheState == STATE_CACHE_MISS) {
            newVal  = tag;
            setMiss = (age1 < age2 && age1 < age3 && age1 < age4)
                          ? 0
                          : ((age2 < age1 && age2 < age3 && age2 < age4)
                                 ? 1
                                 : ((age3 < age2 && age3 < age1 && age3 < age4) ? 2 : 3));
            oldVal = (age1 < age2 && age1 < age3 && age1 < age4)
                         ? val1
                         : ((age2 < age1 && age2 < age3 && age2 < age4)
                                ? val2
                                : ((age3 < age2 && age3 < age1 && age3 < age4) ? val3 : val4));
            isValid = (age1 < age2 && age1 < age3 && age1 < age4)
                          ? valid1
                          : ((age2 < age1 && age2 < age3 && age2 < age4)
                                 ? valid2
                                 : ((age3 < age2 && age3 < age1 && age3 < age4) ? valid3 : valid4));
            isDirty = (age1 < age2 && age1 < age3 && age1 < age4)
                          ? dirty1
                          : ((age2 < age1 && age2 < age3 && age2 < age4)
                                 ? dirty2
                                 : ((age3 < age2 && age3 < age1 && age3 < age4) ? dirty3 : dirty4));
            if(isDirty == 0){
             cacheState = STATE_CACHE_LAST_STORE - 1;
            }
            // printf("TAG is %x\n", oldVal.slc<TAG_SIZE>(0));
          }

        //   ap_uint<32> oldAddress = (((int)oldVal.template slc<TAG_SIZE>(0)) << (LOG_LINE_SIZE + LOG_SET_SIZE)) |
        //                                  (((int)place) << LOG_LINE_SIZE);
          ap_uint<32> oldAddress = ((int)(oldVal.range(TAG_SIZE - 1, 0)) << (LOG_LINE_SIZE + LOG_SET_SIZE)) |
                                   ((int)(place) << LOG_LINE_SIZE);

          // First we write back the four memory values in upper level

          if (cacheState >= STATE_CACHE_LAST_STORE) {
            // We store all values into next memory interface
            nextLevelAddr   = oldAddress + (((int)(cacheState - STATE_CACHE_LAST_STORE)) << LOG_INTERFACE_SIZE);
            // nextLevelDataIn = oldVal.template slc<INTERFACE_SIZE * 8>(
            //     (cacheState - STATE_CACHE_LAST_STORE) * INTERFACE_SIZE * 8 + TAG_SIZE);
            nextLevelDataIn = oldVal.range(
                (cacheState - STATE_CACHE_LAST_STORE) * INTERFACE_SIZE * 8 + TAG_SIZE + INTERFACE_SIZE * 8 - 1,
                (cacheState - STATE_CACHE_LAST_STORE) * INTERFACE_SIZE * 8 + TAG_SIZE
            );
            nextLevelOpType = (isValid) ? STORE : NONE;

            // printf("Writing back %x %x at %x\n", (unsigned int)nextLevelDataIn.slc<32>(0),
            //        (unsigned int)nextLevelDataIn.slc<32>(32), (unsigned int)nextLevelAddr);

          } else if (cacheState >= STATE_CACHE_LAST_LOAD) {
            // Then we read values from next memory level
            if (cacheState != STATE_CACHE_FIRST_LOAD) {
            //   newVal.set_slc(((unsigned int)(cacheState - STATE_CACHE_LAST_LOAD)) * INTERFACE_SIZE * 8 + TAG_SIZE,
            //                  nextLevelDataOut); // at addr +1
              newVal.range(
                  ((unsigned int)(cacheState - STATE_CACHE_LAST_LOAD)) * INTERFACE_SIZE * 8 + TAG_SIZE + nextLevelDataOut.length() - 1,
                  ((unsigned int)(cacheState - STATE_CACHE_LAST_LOAD)) * INTERFACE_SIZE * 8 + TAG_SIZE
              ) = nextLevelDataOut;
            }

            if (cacheState != STATE_CACHE_LAST_LOAD) {
              // We initiate the load at the address determined by next cache state
            //   nextLevelAddr = (((int)addr.slc<32 - LOG_LINE_SIZE>(LOG_LINE_SIZE)) << LOG_LINE_SIZE) +
            //                   ((cacheState - STATE_CACHE_LAST_LOAD - 1) << LOG_INTERFACE_SIZE);
              nextLevelAddr = (((int)addr.range(31, LOG_LINE_SIZE)) << LOG_LINE_SIZE) +
                              ((cacheState - STATE_CACHE_LAST_LOAD - 1) << LOG_INTERFACE_SIZE);
              nextLevelOpType = LOAD;
            }
          }

          cacheState--;

          if (cacheState == 1) {
            valDirty = 0;
            if (opType == STORE) {
              switch (mask) {
                case BYTE:
                case BYTE_U:
                //   newVal.set_slc((((int)addr.slc<2>(0)) << 3) + TAG_SIZE + 4 * 8 * offset, dataIn.template slc<8>(0));
                  // Assign an 8-bit slice from dataIn to newVal
                  newVal.range(
                      (((int)addr.range(1, 0)) << 3) + TAG_SIZE + 4 * 8 * offset + 7,
                      (((int)addr.range(1, 0)) << 3) + TAG_SIZE + 4 * 8 * offset
                  ) = dataIn.range(7, 0); // Extract the lower 8 bits from dataIn
                  break;
                case HALF:
                case HALF_U:
                //   newVal.set_slc((addr[1] ? 16 : 0) + TAG_SIZE + 4 * 8 * offset, dataIn.template slc<16>(0));
                  // Assign a 16-bit slice from dataIn to newVal
                  newVal.range(
                      (addr[1] ? 16 : 0) + TAG_SIZE + 4 * 8 * offset + 15,
                      (addr[1] ? 16 : 0) + TAG_SIZE + 4 * 8 * offset
                  ) = dataIn.range(15, 0); // Extract the lower 16 bits from dataIn
                  break;
                case WORD:
                //   newVal.set_slc(TAG_SIZE + 4 * 8 * offset, dataIn.template slc<32>(0));
                  // Assign a 32-bit slice from dataIn to newVal
                  newVal.range(
                      TAG_SIZE + 4 * 8 * offset + 31,
                      TAG_SIZE + 4 * 8 * offset
                  ) = dataIn.range(31, 0); // Extract the lower 32 bits from dataIn
                  break;
                case LONG:
                //   newVal.set_slc(TAG_SIZE + 4 * 8 * offset, dataIn);
                  // Assign the entire dataIn to newVal starting from TAG_SIZE + 4 * 8 * offset
                  for (int i = 0; i < INTERFACE_SIZE; ++i) {
                      newVal.range(
                          TAG_SIZE + 4 * 8 * offset + (i + 1) * 8 - 1,
                          TAG_SIZE + 4 * 8 * offset + i * 8
                      ) = dataIn.range((i + 1) * 8 - 1, i * 8); // Assign 8 bits at a time
                  }
                  break;
              }
              valDirty = 1;
            }

            placeStore = place;
            setStore   = setMiss;
            valStore   = newVal;

            // cacheMemory[place][setMiss] = newVal;
            // dataValid[place][setMiss] = 1;
            // age[place][setMiss] = cycle;
            nextLevelOpType = NONE;

            ap_int<8> signedByte;
            ap_int<16> signedHalf;
            ap_int<32> signedWord;

            switch (mask) {
              case BYTE:
                // signedByte = newVal.template slc<8>((((int)addr.slc<2>(0)) << 3) + 4 * 8 * offset + TAG_SIZE);
                // Extract an 8-bit slice from newVal
                signedByte = newVal.range(
                    (((int)addr.range(1, 0)) << 3) + 4 * 8 * offset + TAG_SIZE + 7,
                    (((int)addr.range(1, 0)) << 3) + 4 * 8 * offset + TAG_SIZE
                );
                signedWord = signedByte;
                // dataOut.set_slc(0, signedWord);
                dataOut.range(31, 0) = signedWord; // Assign to the lower 32 bits of dataOut
                break;
              case HALF:
                // signedHalf = newVal.template slc<16>((addr[1] ? 16 : 0) + 4 * 8 * offset + TAG_SIZE);
                // Extract a 16-bit slice from newVal
                signedHalf = newVal.range(
                    (addr[1] ? 16 : 0) + 4 * 8 * offset + TAG_SIZE + 15,
                    (addr[1] ? 16 : 0) + 4 * 8 * offset + TAG_SIZE
                );
                signedWord = signedHalf;
                // dataOut.set_slc(0, signedWord);
                dataOut.range(31, 0) = signedWord; // Assign to the lower 32 bits of dataOut
                break;
              case WORD:
                // dataOut = newVal.template slc<32>(4 * 8 * offset + TAG_SIZE);
                // Extract a 32-bit slice from newVal
                dataOut.range(31, 0) = newVal.range(4 * 8 * offset + TAG_SIZE + 31, 4 * 8 * offset + TAG_SIZE);
                break;
              case BYTE_U:
                // dataOut = newVal.template slc<8>((((int)addr.slc<2>(0)) << 3) + 4 * 8 * offset + TAG_SIZE) & 0xff;
                // Extract an 8-bit slice from newVal and mask with 0xff
                dataOut.range(7, 0) = newVal.range(
                    (((int)addr.range(1, 0)) << 3) + 4 * 8 * offset + TAG_SIZE + 7,
                    (((int)addr.range(1, 0)) << 3) + 4 * 8 * offset + TAG_SIZE
                ) & 0xff;
                break;
              case HALF_U:
                // dataOut = newVal.template slc<16>((addr[1] ? 16 : 0) + 4 * 8 * offset + TAG_SIZE) & 0xffff;
                // Extract a 16-bit slice from newVal and mask with 0xffff
                dataOut.range(15, 0) = newVal.range(
                    (addr[1] ? 16 : 0) + 4 * 8 * offset + TAG_SIZE + 15,
                    (addr[1] ? 16 : 0) + 4 * 8 * offset + TAG_SIZE
                ) & 0xffff;
                break;
              case LONG:
                // dataOut = newVal.template slc<INTERFACE_SIZE * 8>(4 * 8 * offset);
                // Extract INTERFACE_SIZE * 8 bits from newVal
                for (int i = 0; i < INTERFACE_SIZE; ++i) {
                    dataOut.range(
                        (i + 1) * 8 - 1,
                        i * 8
                    ) = newVal.range(
                        4 * 8 * offset + (i + 1) * 8 - 1,
                        4 * 8 * offset + i * 8
                    );
                }
                break;
            }
            // printf("After Miss read %x at %x\n", (unsigned int)dataOut.slc<32>(0), (unsigned int)addr);

            dataOutStore = dataOut;
          }
        }
      }
    }

    this->nextLevel->process(nextLevelAddr, LONG, nextLevelOpType, nextLevelDataIn, nextLevelDataOut, nextLevelWaitOut);
    waitOut = nextLevelWaitOut || cacheState || wasStore;
  }
};

#endif /* INCLUDE_CACHEMEMORY_H_ */
