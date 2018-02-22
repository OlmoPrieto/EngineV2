#include <cassert>
#include <iostream>
#include <iterator>
#include <list>
#include <vector>

#include "chrono.h"
#include "vector.h"

#define USE_FIRST_BIG_BLOCK 1
#define ALLOC_POLICY_BEST_FIT 1
#define ALLOC_POLICY_SPLIT_BIG_BLOCK (ALLOC_POLICY_BEST_FIT == 0)

typedef unsigned char byte;
typedef unsigned int uint64;
typedef int int64;

Chrono cChrono;

float average_request = 0.0f;
uint64 times_request = 0;
float average_free = 0.0f;
uint64 times_free = 0;

static inline bool isAddressAligned(void* pAddress, byte byAlignment) {
  return (((unsigned long)(pAddress) % (byAlignment)) == 0);
}

class Allocator {
private:
  class Block;
  class BlockPool;
public:

  Allocator(uint64 uRequestedSize, byte byAlignment = 1) {
    m_byAlignment = byAlignment;

    printf("Creating allocator...\n");
    printf("Requested: %u bytes\n", uRequestedSize);

    // TODO: do this in a separate and private function!!
    if (uRequestedSize % 2 != 0) {
      ++uRequestedSize;
    }
    while (uRequestedSize % m_byAlignment != 0) {
      uRequestedSize += 2;
    }

    printf("Given %u bytes\n", uRequestedSize);

    m_vBlocksPool.emplace_back(uRequestedSize, m_byAlignment);
    m_pCurrentPool = &m_vBlocksPool[0];
  }

  ~Allocator() {
    printf("Num pools: %lu\n", m_vBlocksPool.size());

    while (m_vBlocksPool.size() > 0) {
      m_vBlocksPool[m_vBlocksPool.size() - 1].releaseAllBlocks();
      m_vBlocksPool.pop_back();
    }

    printf("Destroyed allocator\n");
  }

  byte* requestBlock(uint64 uRequestedSize) {
    bool bFoundPool = false;
    if (m_pCurrentPool->getFreeMemory() < uRequestedSize) {
      for (uint64 i = 0; i < m_vBlocksPool.size(); ++i) {
        if (m_vBlocksPool[i].getFreeMemory() >= uRequestedSize) {
          m_pCurrentPool = &m_vBlocksPool[i];
          bFoundPool = true;

          break;
        }
      }

      if (bFoundPool == false) {
        printf("\nAdded another block pool\n");

        uint64 uSize = m_vBlocksPool[m_vBlocksPool.size() - 1].getMaxMemory() * 2;

        printf("Requested %u bytes\n", uSize);

        // TODO: do this in a separate and private function!!
        if (uSize % 2 != 0) {
          ++uSize;
        }
        while (uSize % m_byAlignment != 0) {
          uSize += 2;
        }

        printf("Given %u bytes\n", uSize);
        for (uint64 i = 0; i < m_vBlocksPool.size(); ++i) {
          m_vBlocksPool[i].m_pBigFreeBlock->print();
        }
        m_vBlocksPool.emplace_back(uSize, m_byAlignment);
        printf("after emplace_back\n");
        // for if emplace_back reallocates the elements and pointers gets invalidated
        for (uint64 i = 0; i < m_vBlocksPool.size(); ++i) {
          m_vBlocksPool[i].m_pBigFreeBlock->print();
          //m_vBlocksPool[i].m_vBlocks[0].print();
          m_vBlocksPool[i].m_pBigFreeBlock = &(m_vBlocksPool[i].m_vBlocks[0]);
        }
        m_pCurrentPool = &m_vBlocksPool[m_vBlocksPool.size() - 1];
      }
    }
    // else -> there is enough memory, so go on

    return m_pCurrentPool->requestBlock(uRequestedSize);
  }

  // Try to optimize the function by limting the search sorted by pools
  void releaseBlock(byte* pAddress) {
    const byte* pPoolAddress = nullptr;
    uint64 uPoolSize = 0;
    bool bBlockFound = false;
    printf("Given address: %p\n", pAddress);
    for (uint64 i = 0; i < m_vBlocksPool.size(); ++i) {
      pPoolAddress = m_vBlocksPool[i].m_pBigFreeBlock->getAddress();
      printf("m_pBigFreeBlock address: %p\n", pPoolAddress);
      uPoolSize = m_vBlocksPool[i].m_uMaxMemory;
      if (pPoolAddress <= pAddress && pAddress < pPoolAddress + uPoolSize) {
        printf("Pool #%u\n", i);
        bBlockFound = m_vBlocksPool[i].releaseBlock(pAddress);
        printf("This printf is allowed\n");
        //bBlockFound = true;
        // CAREFUL: will this always succeed?
        break;
      }
    }
    
    if (bBlockFound == false) {
      printf("\n\nTHIS PRINTF ISNT ALLOWED\n\n");

      for (uint64 i = 0; i < m_vBlocksPool.size(); ++i) {
        if (m_vBlocksPool[i].releaseBlock(pAddress) == true) {
          printf("FOUND!!\n");

          break;
        }
      }
    }
  }

  void printAllElements() const {
    for (uint64 i = 0; i < m_vBlocksPool.size(); ++i) {
      m_vBlocksPool[i].printAllElements();
    }
    // for (const auto& e : m_vBlocksPool) {
    //   e.printAllElements();
    // }
  }

  void printUsedMemory() {
    uint64 uUsedMemory = 0;

    // for (const auto& e : m_vBlocksPool) {
    //   uUsedMemory += e.getUsedMemory();
    // }
    for (uint64 i = 0; i < m_vBlocksPool.size(); ++i) {
      uUsedMemory += m_vBlocksPool[i].getUsedMemory();
      m_vBlocksPool[i].printUsedMemory();
    }

    printf("TOTAL used memory: %u bytes\n", uUsedMemory);
  }

  void printNumElements() {

  }

private:
  class Block {
  public:
    Block(byte* pAddress, uint64 uSize, uint64 uId = 65535, 
        Block* pNextFreeBlock = nullptr, bool bFree = false) {

      printf("Address given: %p\n", pAddress);
      printf("ID: %u\n", uId);

      m_pAddress = pAddress;
      m_pNextFreeBlock = pNextFreeBlock;
      m_uSize = uSize;
      m_uId = uId;
      m_bFree = bFree;
    }

    Block() {}

    ~Block() {

    }

    void setNextFreeBlock(Block* pNextFreeBlock) {
      m_pNextFreeBlock = pNextFreeBlock;
    }

    const byte* getAddress() const {
      return m_pAddress;
    }

    inline uint64 getSize() const {
      return m_uSize;
    }

    uint64 getId() const {
      return m_uId;
    }

    inline bool isFree() const {
      return m_bFree;
    }

    void print() const {
      printf("Block address: %p\tid: %u\tsize: %u \tfree: %d\n", m_pAddress, m_uId, m_uSize, m_bFree);
    }

    void resize(uint64 uNewSize, byte* pNewAddress = nullptr, bool bIsFree = false) {
      m_uSize = uNewSize;
      m_bFree = bIsFree;
      if (pNewAddress != nullptr) {
        m_pAddress = pNewAddress;
      }
    }

    void changeAddress(byte* pNewAddress) {
      m_pAddress = pNewAddress;
    }

    inline void release() {
      if (!m_bFree) {
        m_bFree = true;
      }
    }

    // Public variables
    Block* m_pNextFreeBlock;

  //private:
    byte* m_pAddress;
    uint64 m_uSize;
    uint64 m_uId;
    bool m_bFree;
  };

  class BlockPool {
  public:
    BlockPool(uint64 uMemoryAmount, byte byAlignment = 1) {
      m_uMaxMemory = uMemoryAmount;
      m_byAlignment = byAlignment;
      m_pMemStart = (byte*)malloc(uMemoryAmount);
      printf("Starting address: \t%p\nMax address: \t\t%p\n", m_pMemStart, m_pMemStart + uMemoryAmount);
      if (m_pMemStart != nullptr) {
        if (!isAddressAligned(m_pMemStart, m_byAlignment)) {
          printf("WARNING, MEMORY ISN'T ALIGNED TO %u bytes\n", m_byAlignment);
        }
        
        // This calls copy constructor: m_lBlocks.emplace_back(Block(m_pMemStart, uMemoryAmount, sm_uBlockCount, nullptr, true));
        // m_lBlocks.emplace_back(m_pMemStart, uMemoryAmount, sm_uBlockCount, nullptr, true);
        m_vBlocks.emplace_back(m_pMemStart, uMemoryAmount, sm_uBlockCount, nullptr, true);
        ++sm_uBlockCount;
        // m_pBigFreeBlock = &m_vBlocks.front();
        // m_pFirstFreeBlock = &m_vBlocks.front();
        // m_vBlocks.front().m_pNextFreeBlock = m_pBigFreeBlock;
        m_pBigFreeBlock = &m_vBlocks[0];
        m_pFirstFreeBlock = &m_vBlocks[0];
        m_vBlocks[0].m_pNextFreeBlock = m_pBigFreeBlock;

        m_uUsedMemory = 0;
      } 
      else {
        m_pMemStart = nullptr;
        printf("ERROR: Not enough memory!\t-> W: Allocator\n");
      }
    }

    ~BlockPool() {
      printf("Destroying block pool\n");

      //releaseAllBlocks();

      if (m_uUsedMemory == 0 && m_vBlocks.size() <= 1) { // && m_pFirstFreeBlock == nullptr) { // && m_pMemStart != nullptr
        printf("Address to free: %p\n", m_pMemStart);
        free(m_pMemStart);

        printf("Memory correctly released\n");
      }
    }

    BlockPool(const BlockPool& cOther) {
      printf("\nCopy constructor\n");

      printf("m_pMemStart this:  %p\n", m_pMemStart);
      printf("m_pMemStart other: %p\n", cOther.m_pMemStart);

      for (uint64 i = 0; i < cOther.m_vBlocks.size(); ++i) {
        m_vBlocks.push_back(cOther.m_vBlocks[i]);
      }
      m_pBigFreeBlock = cOther.m_pBigFreeBlock;
      m_pFirstFreeBlock = cOther.m_pFirstFreeBlock;
      m_uUsedMemory = cOther.m_uUsedMemory;
      m_uMaxMemory = cOther.m_uMaxMemory;
      m_pMemStart = cOther.m_pMemStart;
      m_byAlignment = cOther.m_byAlignment;
    }

    byte* requestBlock(uint64 uRequestedSize) {

      cChrono.start();

      byte* pNewBlockAddress = nullptr;
      uint64 uFirstFreeBlockSize = 0;
      uint64 uResizedMemAmount = uFirstFreeBlockSize;
      uint64 uFirstFreeBlockIndex = (1 << 32) - 1;
      bool bUseCurrentBlock = true;

      uint64 loops = 0;
      do {
        #if (ALLOC_POLICY_BEST_FIT == 1)
          //m_pFirstFreeBlock = findFirstFreeBlock(uRequestedSize);
          uFirstFreeBlockIndex = findFirstFreeBlock(uRequestedSize);
          printf("index retrieved by findFirstFreeBlock(): %u\n", uFirstFreeBlockIndex);
          m_pFirstFreeBlock = &m_vBlocks[uFirstFreeBlockIndex];
        #elif (ALLOC_POLICY_SPLIT_BIG_BLOCK == 1)
          m_pFirstFreeBlock = m_pBigFreeBlock;
        #endif

        pNewBlockAddress = (byte*)(m_pFirstFreeBlock->getAddress());
        uFirstFreeBlockSize = m_pFirstFreeBlock->getSize();
        uResizedMemAmount = uFirstFreeBlockSize;

        ++loops;
        if (loops > 1000) {
          m_pFirstFreeBlock = nullptr;

          break;
        }
      } while (uFirstFreeBlockSize < uRequestedSize || m_pFirstFreeBlock->isFree() == false);

      if (m_pFirstFreeBlock == nullptr) {
        // RESOLVED? -> TODO: implement a system to alloc another big block of memory and manage this new big blocks
        printf("Not enough memory, returning malloc() address\t-> W: Allocator::requestBlock\n");
        return (byte*)malloc(uRequestedSize);
      }

      // if (m_byAlignment > uRequestedSize) {
      //   uRequestedSize = (uint64)(m_byAlignment);
      // }

      /* Above and below are equivalent */
      
      // Source: http://graphics.stanford.edu/~seander/bithacks.html#IntegerMinOrMax
      uRequestedSize = uRequestedSize ^ ((uRequestedSize ^ m_byAlignment) & -(uRequestedSize < m_byAlignment));

      if (uFirstFreeBlockSize >= uRequestedSize) {
        if (uFirstFreeBlockSize > uRequestedSize) {
          // TODO: problem -> MAYBE, the calculation for pNewBlockAddress gives
          //   an address already given, so what to do?
          // The problem is in coalesce(), because is trying to unite two blocks
          //   physically separated (id 2 and 3)
          pNewBlockAddress = (byte*)(m_pFirstFreeBlock->getAddress()) + 
            m_pFirstFreeBlock->getSize() - uRequestedSize;
          uResizedMemAmount -= uRequestedSize;
          bUseCurrentBlock = false;

          if (!isAddressAligned(pNewBlockAddress, m_byAlignment)) {
            printf("WARNING, MEMORY ISN'T ALIGNED TO %u bytes\n", m_byAlignment);
          }

          m_vBlocks[0].print();

          // -- IF a reallocation happens, the pointers get invalidated, 
          //   so m_pBigFreeBlock which was pointing to an address gets invalidated.
          m_vBlocks.emplace_back(pNewBlockAddress, uRequestedSize, 
            sm_uBlockCount, nullptr, bUseCurrentBlock);

          m_vBlocks[0].print();
          
          // for if reallocation happens
          m_pBigFreeBlock = &m_vBlocks[0];
          m_pFirstFreeBlock = &m_vBlocks[uFirstFreeBlockIndex];
          
          ++sm_uBlockCount;

          bUseCurrentBlock = true;
        }
        else if (uFirstFreeBlockSize == uRequestedSize) {
          // don't create another block, use the first free one without splitting
          printf("Reusing existing block\n");
          bUseCurrentBlock = false;
          //m_uUsedMemory += uRequestedSize;
        }
        if (m_pFirstFreeBlock->getAddress() == m_pBigFreeBlock->getAddress()) {
          printf("Splitting FIRST block!\n");
        }
        m_pFirstFreeBlock->resize(uResizedMemAmount, nullptr, bUseCurrentBlock);
        m_uUsedMemory += uRequestedSize;

        cChrono.stop();
        printf("\n\nTime to request block: %.3fms\n\n", cChrono.timeAsMilliseconds());
        average_request += cChrono.timeAsMilliseconds();
        ++times_request;

        return pNewBlockAddress;
      }
      else {
        printf("First free block hadn't enough memory available\n");
      }

      return nullptr;
    }

    // bool releaseBlock(byte* pAddress) {
    //   printf("Trying to release block with address %p...\n", pAddress);

    //   Chrono cChrono;
    //   cChrono.start();

    //   auto it = m_vBlocks.begin();
    //   while (it != m_vBlocks.end()) {
    //     if (pAddress == it->getAddress()) {//} && it->getId() != 0) {
    //       it->release();
    //       m_uUsedMemory -= it->getSize();
    //       printf("Releasing block %u with address: %p\n", it->getId(), pAddress);

    //       coalesceBlocks(&it);
    //       // printf("&it is pointing to: ");
    //       // it->print();

    //       //break;
    //       cChrono.stop();
    //       printf("Time to release block: %.3fms\n", cChrono.timeAsMilliseconds());
    //       average_free += cChrono.timeAsMilliseconds();
    //       ++times_free;
    //       return true;
    //     }
    //     else {
    //       ++it;
    //     }
    //   }

    //   cChrono.stop();
    //   printf("Time to release block: %.3fms\n", cChrono.timeAsMilliseconds());
    //   average_free += cChrono.timeAsMilliseconds();
    //   ++times_free;

    //   return false;
    // }
    
    //TODO: this function cannot be uncommented until coalesceBlocks() has been changed 
    //      (to receive) a pointer a pointer to Block instead a pointer to iterator
    bool releaseBlock(byte* pAddress) {
      printf("Trying to release block with address %p...\n", pAddress);

      Chrono cChrono;
      cChrono.start();

      Block* pBlock = nullptr;
      for (uint64 i = 0; i < m_vBlocks.size(); ++i) {
        pBlock = &m_vBlocks[i];
        if (pAddress == pBlock->getAddress()) {
          printf("\nused memory: %u bytes\n", m_uUsedMemory);
          m_uUsedMemory -= pBlock->getSize();
          printf("\nused memory: %u bytes\n", m_uUsedMemory);
          printf("Releasing block %u with address: %p\n", pBlock->getId(), pAddress);
          pBlock->release();

          coalesceBlocks(pBlock, i);

          cChrono.stop();
          printf("Time to release block: %.3fms\n", cChrono.timeAsMilliseconds());
          average_free += cChrono.timeAsMilliseconds();
          ++times_free;
          return true;
        }
      }

      cChrono.stop();
      printf("Time to release block: %.3fms\n", cChrono.timeAsMilliseconds());
      average_free += cChrono.timeAsMilliseconds();
      ++times_free;

      return false;
    }

    void releaseAllBlocks() {
      printf("\nused memory: %u bytes\n", m_uUsedMemory);
      Block* pBlock = nullptr;
      uint64 i = 1; // CAREFUL: not zero! (the first one)
      while (m_vBlocks.size() > 1 || i < m_vBlocks.size()) {
        pBlock = &m_vBlocks[i];
        if (!pBlock->isFree()) {
          //m_uUsedMemory -= pBlock->getSize();
        }
        pBlock->release();
        if (!coalesceBlocks(pBlock, i)) {
          ++i;
        }
      }
      // m_lBlocks.size() == 1 -> true

      pBlock = &m_vBlocks[0];
      pBlock->release();
      //m_uUsedMemory -= pBlock->getSize();
      m_pFirstFreeBlock = nullptr;

      printAllElements();
      printf("Released all blocks. Current memory: %u\n", m_uUsedMemory);
    }

    enum IteratorCheckDirection {
      Backward,
      Forward
    };

    // TODO: test with main coalescing function
    // private:
    // bool checkAndCoalesce(std::vector<Allocator::Block>::iterator* pIt, 
    //   IteratorCheckDirection eCheckDirection) {
      
    //   bool bReturn = false;

    //   Block* pBlockToCoalesce = &(*(*pIt));
    //   Block* pPrevBlock = nullptr;

    //   uint64 uCurrentSize = pBlockToCoalesce->getSize();
    //   uint64 uPrevSize = 0;
      
    //   if (eCheckDirection == IteratorCheckDirection::Backward) {
    //     --(*pIt);
    //   }
    //   else {
    //     ++(*pIt);
    //   }

    //   pPrevBlock = &(*(*pIt));
    //   uPrevSize = pPrevBlock->getSize();

    //   pBlockToCoalesce->print();
    //   pPrevBlock->print();

    //   if ((*pIt)->getAddress() < pBlockToCoalesce->getAddress()) {
    //     pBlockToCoalesce = &(*(*pIt));
    //     uCurrentSize = pBlockToCoalesce->getSize();
        
    //     if (eCheckDirection == IteratorCheckDirection::Backward) {
    //       ++(*pIt);
    //     }
    //     else {
    //       --(*pIt);
    //     }

    //     pPrevBlock = &(*(*pIt));
    //     uPrevSize = pPrevBlock->getSize();
    //   }

    //   if (pPrevBlock->isFree() == true) {
    //     pBlockToCoalesce->resize(pBlockToCoalesce->getSize() + 
    //       pPrevBlock->getSize(), nullptr, true);
    //     pBlockToCoalesce->print();

    //     m_vBlocks.erase(*pIt);
        
    //     bReturn = true;
    //   }

    //   return bReturn;
    // }

    // // @return true if could coalesce at least one block
    // bool coalesceBlocks(std::vector<Allocator::Block>::iterator* pIt = nullptr) {
    //   printf("Coalescing...\n");
    //   printf("Elements before coalescing: ");
    //   printNumElements();
    //   bool bReturn = false;

    //   if (pIt != nullptr) {
    //     // check if previous block is free
    //     if ((*pIt) != m_vBlocks.begin()) {
    //       Block* pBlockToCoalesce = &(*(*pIt));
    //       Block* pPrevBlock = nullptr;

    //       uint64 uCurrentSize = pBlockToCoalesce->getSize();
    //       uint64 uPrevSize = 0;
          
    //       // check previous block (by ID)
    //       --(*pIt);
    //       pPrevBlock = &(*(*pIt));
    //       uPrevSize = pPrevBlock->getSize();

    //       pBlockToCoalesce->print();
    //       pPrevBlock->print();

    //       // (*pIt) points to the previous element, pPrevBlock
          
    //       // TODO: look at this for optimization, maybe this distinction is not needed
    //       if ((*pIt)->getId() != 0) {
    //         // normal case, a block in between the pool
    //         if ((*pIt)->getAddress() < pBlockToCoalesce->getAddress()) {
    //           pBlockToCoalesce = &(*(*pIt));
    //           uCurrentSize = pBlockToCoalesce->getSize();
              
    //           ++(*pIt);
    //           pPrevBlock = &(*(*pIt));
    //           uPrevSize = pPrevBlock->getSize();
    //         }
    //       }
    //       else {
    //         // if the previous block is the first block (ID == 0)
    //         // is this check needed? maybe always change the block to coalesce
    //         // to the first one (ID == 0)
    //         if ((*pIt)->getAddress() + (*pIt)->getSize() == pBlockToCoalesce->getAddress()) {
    //           pBlockToCoalesce = &(*(*pIt));
    //           uCurrentSize = pBlockToCoalesce->getSize();
              
    //           ++(*pIt);
    //           pPrevBlock = &(*(*pIt));
    //           uPrevSize = pPrevBlock->getSize();
    //         }
    //       }

    //       if (pPrevBlock->isFree() == true) {
    //         printf("FIRST element free, coalescing\n");
    //         pBlockToCoalesce->resize(pBlockToCoalesce->getSize() + 
    //           pPrevBlock->getSize(), nullptr, true);
    //         pBlockToCoalesce->print();

    //         auto aux = --(*pIt);
    //         ++(*pIt);
    //         m_vBlocks.erase(*pIt);
    //         *pIt = aux;
            
    //         bReturn = true;
    //       }
    //     }
    //     else {
    //       // pIt points to list::begin

    //       if (m_vBlocks.size() > 1) {
    //         Block* pBlockToCoalesce = &(*(*pIt));
    //         Block* pPrevBlock = nullptr;

    //         uint64 uCurrentSize = pBlockToCoalesce->getSize();
    //         uint64 uPrevSize = 0;
            
    //         // check next block (by ID)
    //         ++(*pIt);
    //         pPrevBlock = &(*(*pIt));
    //         uPrevSize = pPrevBlock->getSize();

    //         pBlockToCoalesce->print();
    //         pPrevBlock->print();

    //         // most probably, pBlockToCoalesce will point to the first block (ID == 0)
    //         // (*pIt) and pPrevBlock will point to the next element
    //         if (pPrevBlock->isFree() == true) {
    //           printf("Next element free (only two remaining), coalescing\n");
    //           pBlockToCoalesce->resize(pBlockToCoalesce->getSize() + 
    //             pPrevBlock->getSize(), nullptr, true);
    //           pBlockToCoalesce->print();

    //           auto aux = --(*pIt);
    //           ++(*pIt);
    //           m_vBlocks.erase(*pIt);
    //           *pIt = aux;

    //           bReturn = true;
    //         }
    //       }
    //     }

    //     // CAREFUL: first thing to check if the program starts to crash again
    //     // ++(*pIt) shouldn't crash because it will always point to at least the last element

    //     // check the NEXT block            // maybe not needed, delete!
    //     if (++(*pIt) != m_vBlocks.end() && --(*pIt) != m_vBlocks.end()) {
    //       Block* pBlockToCoalesce = &(*(*pIt));
    //       Block* pPrevBlock = nullptr;

    //       uint64 uCurrentSize = pBlockToCoalesce->getSize();
    //       uint64 uPrevSize = 0;
          
    //       // check next block (by ID)
    //       ++(*pIt);
    //       pPrevBlock = &(*(*pIt));
    //       uPrevSize = pPrevBlock->getSize();

    //       printf("\nThis is the check for the next block:\n");
    //       pBlockToCoalesce->print();
    //       pPrevBlock->print();

    //       if ((*pIt)->getAddress() < pBlockToCoalesce->getAddress()) {
    //         pBlockToCoalesce = &(*(*pIt));
    //         uCurrentSize = pBlockToCoalesce->getSize();

    //         --(*pIt);
    //         pPrevBlock = &(*(*pIt));
    //         uPrevSize = pPrevBlock->getSize();
    //       }

    //       if (pPrevBlock->isFree() == true) {
    //         printf("Next element free, coalescing\n");
    //         pBlockToCoalesce->resize(pBlockToCoalesce->getSize() + 
    //           pPrevBlock->getSize(), nullptr, true);
    //         pBlockToCoalesce->print();

    //         auto aux = --(*pIt);
    //         ++(*pIt);
    //         m_vBlocks.erase(*pIt);
    //         *pIt = aux;

    //         bReturn = true;
    //       }
    //     }

    //     printf("Elements after coalescing: ");
    //     printNumElements();
    //   }
    //   else {  // TODO: if no iterator is supplied, coalesce all blocks in the Allocator
    //     bReturn = false;
    //   }

    //   return bReturn;
    // }

    // @return true if could coalesce at least one block
    bool coalesceBlocks(Block* pIt, uint64 uIndex) {
      printf("Coalescing...\n");
      printf("Elements before coalescing: ");
      printNumElements();
      uint64 uIndexCopy = uIndex;
      bool bReturn = false;

      if (pIt != nullptr) {
        // check if previous block is free
        if (pIt != &m_vBlocks[0]) {
          Block* pBlockToCoalesce = pIt;
          Block* pPrevBlock = nullptr;

          uint64 uCurrentSize = pBlockToCoalesce->getSize();
          uint64 uPrevSize = 0;
          
          // check previous block (by ID)
          --uIndexCopy;
          pPrevBlock = &m_vBlocks[uIndexCopy];
          uPrevSize = pPrevBlock->getSize();

          pBlockToCoalesce->print();
          pPrevBlock->print();
          
          // TODO: look at this for optimization, maybe this distinction is not needed
          if (pPrevBlock->getId() != 0) {
            // STANDARD case, a block in between the pool
            if (pPrevBlock->getAddress() < pBlockToCoalesce->getAddress()) {
            //if (pPrevBlock->getAddress() >= pBlockToCoalesce->getAddress() - pBlockToCoalesce->getSize()) {
              // swap blocks references
              Block* pBlock = pBlockToCoalesce;

              pBlockToCoalesce = pPrevBlock;
              uCurrentSize = pBlockToCoalesce->getSize();
              
              pPrevBlock = pBlock;
              uPrevSize = pPrevBlock->getSize();

              // uIndexCopy == uIndex;
              ++uIndexCopy;
            }
          }
          else {
            // if the previous block is the first block (ID == 0)
            // is this check needed? maybe always change the block to coalesce
            // to the first one (ID == 0)
            //if (pPrevBlock->getAddress() + pPrevBlock->getSize() == pBlockToCoalesce->getAddress()) {
            if (pPrevBlock->getAddress() >= pBlockToCoalesce->getAddress() - pBlockToCoalesce->getSize()) {
              printf("SUPPA\n");
              // swap blocks references
              Block* pBlock = pBlockToCoalesce;

              pBlockToCoalesce = pPrevBlock;
              uCurrentSize = pBlockToCoalesce->getSize();
              
              pPrevBlock = pBlock;
              uPrevSize = pPrevBlock->getSize();

              // uIndexCopy == uIndex;
              ++uIndexCopy;
            }
          }

          if (pPrevBlock->isFree() == true) {
            printf("FIRST element free, coalescing\n");
            pBlockToCoalesce->resize(pBlockToCoalesce->getSize() + 
              pPrevBlock->getSize(), nullptr, true);
            pBlockToCoalesce->print();

            m_vBlocks.erase(uIndexCopy);
            
            bReturn = true;
          }
        }
        else {
          // pIt points to list::begin

          if (m_vBlocks.size() > 1) {
            Block* pBlockToCoalesce = &m_vBlocks[0];
            Block* pPrevBlock = nullptr;

            uint64 uCurrentSize = pBlockToCoalesce->getSize();
            uint64 uPrevSize = 0;
            
            // check next block (by ID)
            //++uIndexCopy;
            pPrevBlock = &m_vBlocks[uIndexCopy];
            uPrevSize = pPrevBlock->getSize();

            pBlockToCoalesce->print();
            pPrevBlock->print();

            // most probably, pBlockToCoalesce will point to the first block (ID == 0)
            // (*pIt) and pPrevBlock will point to the next element
            if (pPrevBlock->isFree() == true) {
              printf("Next element free (only two remaining), coalescing\n");
              pBlockToCoalesce->resize(pBlockToCoalesce->getSize() + 
                pPrevBlock->getSize(), nullptr, true);
              pBlockToCoalesce->print();

              m_vBlocks.erase(uIndexCopy);

              bReturn = true;
            }
          }
        }

        // check the next block (next from the original)
        if (uIndexCopy + 1 < m_vBlocks.size()) {
          Block* pBlockToCoalesce = &m_vBlocks[uIndexCopy];
          Block* pPrevBlock = nullptr;

          uint64 uCurrentSize = pBlockToCoalesce->getSize();
          uint64 uPrevSize = 0;
          
          // check next block (by ID)
          ++uIndexCopy;
          pPrevBlock = &m_vBlocks[uIndexCopy];
          uPrevSize = pPrevBlock->getSize();

          printf("\nThis is the check for the next block:\n");
          pBlockToCoalesce->print();
          pPrevBlock->print();

          if (pPrevBlock->getAddress() < pBlockToCoalesce->getAddress()) {
            // swap with previous block
            Block* pBlock = pBlockToCoalesce;

            pBlockToCoalesce = pPrevBlock;
            uCurrentSize = pBlockToCoalesce->getSize();

            pPrevBlock = pBlock;
            uPrevSize = pPrevBlock->getSize();

            --uIndexCopy;
          }

          if (pPrevBlock->isFree() == true) {
            printf("Next element free, coalescing\n");
            pBlockToCoalesce->resize(pBlockToCoalesce->getSize() + 
              pPrevBlock->getSize(), nullptr, true);
            pBlockToCoalesce->print();

            m_vBlocks.erase(uIndexCopy);
            
            bReturn = true;
          }
        }

        printf("Elements after coalescing: ");
        printNumElements();
      }
      else {  // TODO: if no iterator is supplied, coalesce all blocks in the Allocator
        bReturn = false;
      }

      return bReturn;
    }

    //  Engine should call this from time to time 
    // to determine if the big block has no more free memory
    // and to find the first released block. Then is when the party start
    //  Do not call this if not trying to find a block because this can return nullptr
    // Block* findFirstFreeBlock(uint64 uSize = 0) {
    //   auto it = m_vBlocks.begin();

    //   while (it != m_vBlocks.end()) {
    //     printf("findFirstFreeBlock() ID: %u\n", it->getId());
    //     if (it->isFree() == true) {
    //       #if (ALLOC_POLICY_BEST_FIT == 1)
    //         if (it->getSize() >= uSize) {
    //           printf("Found free block: ");
    //           it->print();

    //           return &(*it);
    //         }
    //       #elif (ALLOC_POLICY_SPLIT_BIG_BLOCK == 1)
    //         if (m_pBigFreeBlock->getSize() < uRequestedSize) {
    //           printf("WARNING, first big block not big enough");
    //         }
    //         return m_pBigFreeBlock;
    //       #else

    //         printf("Found free block: ");
    //         it->print();

    //         return &(*it);
    //       #endif
    //     }

    //     ++it;
    //   }

    //   return nullptr;
    // }
    uint64 findFirstFreeBlock(uint64 uSize = 0) {
      Block* pBlock = nullptr;
      for (uint64 i = 0; i < m_vBlocks.size(); ++i) {
        pBlock = &m_vBlocks[i];
        //printf("findFirstFreeBlock() ID: %u\n", pBlock->getId());
        if (pBlock->isFree() == true) {
          #if (ALLOC_POLICY_BEST_FIT == 1)
            if (pBlock->getSize() >= uSize) {
              printf("Found free block: ");
              pBlock->print();

              return i;
            }
          #elif (ALLOC_POLICY_SPLIT_BIG_BLOCK == 1)
            if (m_pBigFreeBlock->getSize() < uRequestedSize) {
              printf("WARNING, first big block not big enough");
            }
            //i = 0;
            return 0; // index zero (0) in the container is always going to be the first big block
          #else

            printf("Found free block: ");
            pBlock->print();

            return i;
          #endif

          //return i;
        }
      }

      return (1 << 32) - 1; // (2^32) in case the program is running on 32 bits
    }

    const Allocator::Block& getBlock(short shIndex) {
      assert(shIndex < m_vBlocks.size() && "Index greater than the list's size\n");

      return m_vBlocks[shIndex];

      // auto it = m_vBlocks.begin();
      // std::advance(it, shIndex);
      // return *it;
    }

    // const std::vector<Allocator::Block>& getBlocks() const {
    //   return m_vBlocks;
    // }
    const elm::vector<Allocator::Block>& getBlocks() const {
      return m_vBlocks;
    }

    uint64 getFreeMemory() const {
      return  m_uMaxMemory - m_uUsedMemory;
    }

    uint64 getMaxMemory() const {
      return m_uMaxMemory;
    }

    uint64 getUsedMemory() const {
      return m_uUsedMemory;
    }

    uint64 getNumElements() const {
      return m_vBlocks.size();
    }

    void printUsedMemory() const {
      printf("Used memory: %d bytes\n", m_uUsedMemory);
    }

    void printNumElements() const {
      printf("Number of elements: %u\n", getNumElements());
    }

    void printAllElements() const {
      //auto it = m_vBlocks.begin();
      printf("Printing all the elements...\n");
      printNumElements();

      // while (it != m_vBlocks.end()) {
      //   it->print();
      //   ++it;
      // }

      for (uint64 i = 0; i < m_vBlocks.size(); ++i) {
        m_vBlocks[i].print();
      }

      printf("m_pBigFreeBlock address: %p\n", m_pBigFreeBlock->getAddress());
    }

    void* getBigBlockAddress() const {
      return (void*)m_pBigFreeBlock->getAddress();
    }

    static uint64 getBlockCount() {
      return sm_uBlockCount;
    }

  //private:
    //static uint64 sm_uBlockCount;

    //std::list<Block> m_lBlocks;
    elm::vector<Block> m_vBlocks;
    Block* m_pBigFreeBlock;
    Block* m_pFirstFreeBlock;
    uint64 m_uUsedMemory;
    uint64 m_uMaxMemory;
    byte* m_pMemStart;
    byte m_byAlignment;
  };  // BlockPool

  // Allocator variables
  std::vector<BlockPool> m_vBlocksPool;
  BlockPool* m_pCurrentPool = nullptr;
  static uint64 sm_uBlockCount;
  byte m_byAlignment;

};  // Allocator

uint64 Allocator::sm_uBlockCount = 0;
Allocator cAlloc(16);

// [ Managed Pointer ]
template <class T>
class mptr {
public:
  mptr(const T& constructor) {
    printf("T Class size: %lu\n", sizeof(T));

    m_pPtr = new (cAlloc.requestBlock(sizeof(T))) T(constructor);
    printf("mptr address: %p\n", (byte*)m_pPtr);
  }

  mptr() {
    printf("T Class size: %lu\n", sizeof(T));
    
    m_pPtr = new (cAlloc.requestBlock(sizeof(T))) T();
    printf("mptr address: %p\n", (byte*)m_pPtr);
  }

  T* get() const {
    return m_pPtr;
  }

  T* operator ->() const {
    return m_pPtr;
  }

  void release() {
    if (m_pPtr != nullptr) {
      m_pPtr->~T();

      cAlloc.releaseBlock((byte*)m_pPtr);

      m_pPtr = nullptr;
    }
  }

  ~mptr() {
    release();
  }

private:
  T* m_pPtr;
};

class Foo {
public:
  Foo() {
    iX = 0;
    iY = 0;
    iZ = 0;
  }

  Foo (int x, int y, int z) {
    iX = x;
    iY = y;
    iZ = z;
  }

  Foo (const Foo& rOther) {
    printf("Foo copy constructor\n");
    iX = rOther.iX;
    iY = rOther.iY;
    iZ = rOther.iZ;
  }

  void print() const {
    printf("x: %d\ty: %d\tz: %d\n", iX, iY, iZ);
  }

  void fooing() {
    iY = 123123;
  }

  int iX;
  int iY;
  int iZ;
};


int _main_() {
  /*printf("\n=======================\n");
  printf("\nmain()\n\n");
  printf("Stats: \n"),
  printf("sizeof int\t: %d bytes\n", sizeof(int));
  printf("sizeof float\t: %d bytes\n", sizeof(float));
  printf("sizeof double\t: %d bytes\n", sizeof(double));
  printf("sizeof byte*\t: %d bytes\n", sizeof(byte*));
  printf("=======================\n");*/

  printf("\n\nAll elements:\n\n");
  cAlloc.printAllElements();
  printf("\n\n");

  //printf("Creating Foo 1...\n");
  mptr<Foo> foo1;
  cAlloc.printUsedMemory();
  //printf("Creating Foo 2...\n");
  mptr<Foo> foo2;
  cAlloc.printUsedMemory();
  //printf("Creating Foo 3...\n");
  mptr<Foo> foo3;
  cAlloc.printUsedMemory();
  printf("\n\nAll elements:\n\n");
  cAlloc.printAllElements();
  printf("\n\n");

  cAlloc.printUsedMemory();

  printf("Releasing Foo 3...\n");
  foo3.release();

  printf("\n\nAll elements:\n\n");
  cAlloc.printAllElements();
  printf("\n\n");

  cAlloc.printUsedMemory();

  {
    //printf("Creating Foo...\n");
    mptr<Foo> foo1(Foo(100, 200, 300));
    foo1->print();
    printf("\n\nAll elements:\n\n");
    cAlloc.printAllElements();
    printf("\n\n");
    cAlloc.printUsedMemory();
  }

  // after here, the big block is exhausted
  printf("\n\nAll elements:\n\n");
  cAlloc.printAllElements();
  printf("\n\n");

  {
    //printf("Creating second Foo()...\n");
    mptr<Foo> foo1;
    foo1->print();
    printf("\n\nAll elements:\n\n");
    cAlloc.printAllElements();
    printf("\n\n");
  }

  printf("\n\nAll elements:\n\n");
  cAlloc.printAllElements();
  printf("\n\n");
  cAlloc.printUsedMemory();
  
  printf("Releasing Foo 2...\n");
  foo2.release();

  printf("\n\nAll elements:\n\n");
  cAlloc.printAllElements();
  printf("\n\n");

  cAlloc.printUsedMemory();

  printf("\n\nInit test...\n");
  for (int i = 0; i < 3; ++i) {
    printf("\n-- Loop %d --\n", i);
    printf("\n\nAll elements:\n\n");
    cAlloc.printAllElements();
    printf("\n\n");
    cAlloc.printUsedMemory();
    mptr<Foo> foo1;
    cAlloc.printNumElements();
    cAlloc.printUsedMemory();
    printf("\n");
    cAlloc.printAllElements();
    printf("\n\n");
  }
  
  printf("\n\nCompleted execution!\n\n");

  cAlloc.printAllElements();

  printf("\n");
  printf("Average time for request block: %.3f\n", (average_request / (float)times_request));
  printf("Average time for free    block: %.3f\n", (average_free / (float)times_free));
  printf("\n");

  return 0;
}
