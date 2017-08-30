#include <cassert>
#include <iostream>
#include <iterator>
#include <list>

#include "chrono.h"

#define USE_FIRST_BIG_BLOCK 1
#define ALLOC_POLICY_BEST_FIT 1
#define ALLOC_POLICY_SPLIT_BIG_BLOCK (ALLOC_POLICY_BEST_FIT == 0)

typedef unsigned char byte;
typedef unsigned int uint64;
typedef int int64;

Chrono cChrono;

class Allocator {
public:
  class Block {
  public:
    Block(byte* pAddress, std::size_t uSize, uint64 uId = 65536, 
        Block* pNextFreeBlock = nullptr, bool bFree = false) {
      
      m_pAddress = pAddress;
      m_pNextFreeBlock = pNextFreeBlock;
      m_uSize = uSize;
      m_uId = uId;
      m_bFree = bFree;
    }

    ~Block() {

    }

    void setNextFreeBlock(Block* pNextFreeBlock) {
      m_pNextFreeBlock = pNextFreeBlock;
    }

    const byte* getAddress() const {
      return m_pAddress;
    }

    uint64 getSize() const {
      return m_uSize;
    }

    uint64 getId() const {
      return m_uId;
    }

    bool isFree() const {
      return m_bFree;
    }

    void print() const {
      printf("Block address: %p\tid: %u\tsize: %u \tfree: %d\n", m_pAddress, m_uId, m_uSize, m_bFree);
    }

    void resize(std::size_t uNewSize, byte* pNewAddress = nullptr, bool bIsFree = false) {
      m_uSize = uNewSize;
      m_bFree = bIsFree;
      if (pNewAddress != nullptr) {
        m_pAddress = pNewAddress;
      }
    }

    void changeAddress(byte* pNewAddress) {
      m_pAddress = pNewAddress;
    }

    void release() {
      //if (m_uId != 0) {
        m_bFree = true;
      //}
    }

    // Public variables
    Block* m_pNextFreeBlock;

  private:
    byte* m_pAddress;
    uint64 m_uSize;
    uint64 m_uId;
    bool m_bFree;
  };

  Allocator(std::size_t uMemoryAmount) {
    m_uMaxMemory = uMemoryAmount;
    m_pMemStart = (byte*)malloc(uMemoryAmount);
    printf("Starting address: \t%p\nMax address: \t\t%p\n", m_pMemStart, m_pMemStart + uMemoryAmount);
    if (m_pMemStart != nullptr) {
      m_lBlocks.emplace_back(Block(m_pMemStart, uMemoryAmount, sm_uBlockCount, nullptr, true));
      ++sm_uBlockCount;
      m_pBigFreeBlock = &m_lBlocks.front();
      m_pFirstFreeBlock = &m_lBlocks.front();
      m_lBlocks.front().m_pNextFreeBlock = m_pBigFreeBlock;

      m_uUsedMemory = 0;
    } 
    else {
      m_pMemStart = nullptr;
      printf("ERROR: Not enough memory!\t-> W: Allocator\n");
    }
  }

  ~Allocator() {
    printAllElements();

    while (getNumElements() > 2) {
      auto it = m_lBlocks.begin();
      coalesceBlocks(&it);
    }
    auto it = m_lBlocks.begin();
    printf("sipote\n");
    coalesceBlocks(&it);

    printAllElements();

    if (m_pMemStart != nullptr) {
      free(m_pMemStart);
    }
  }

  byte* requestBlock(std::size_t uRequestedSize) {

    cChrono.start();

    #if (ALLOC_POLICY_BEST_FIT == 1)
      m_pFirstFreeBlock = findFirstFreeBlock(uRequestedSize);
    #elif (ALLOC_POLICY_SPLIT_BIG_BLOCK == 1)
      m_pFirstFreeBlock = m_pBigFreeBlock;
    #endif

    if (m_pFirstFreeBlock == nullptr) {
      // TODO: implement a system to alloc another big block of memory and manage this new big blocks
      printf("Not enough memory, returning malloc() address\t-> W: Allocator::requestBlock\n");
      return (byte*)malloc(uRequestedSize);
    }

    byte* pNewBlockAddress = (byte*)(m_pFirstFreeBlock->getAddress());
    uint64 uFirstFreeBlockSize = m_pFirstFreeBlock->getSize();
    uint64 uResizedMemAmount = uFirstFreeBlockSize;
    bool bUseCurrentBlock = true;

    if (uFirstFreeBlockSize < uRequestedSize || m_pFirstFreeBlock->isFree() == false) {

      uint64 loops = 0;
      do {
        m_pFirstFreeBlock = findFirstFreeBlock(uRequestedSize);
        uFirstFreeBlockSize = m_pFirstFreeBlock->getSize();

        loops++;
        if (loops > 1000) {
          m_pFirstFreeBlock = nullptr;

          break;
        }
      } while (uFirstFreeBlockSize < uRequestedSize);

      if (m_pFirstFreeBlock == nullptr) {
        // TODO: implement a system to alloc another big block of memory and manage this new big blocks
        printf("Not enough memory, returning malloc() address\t-> W: Allocator::requestBlock\n");
        return (byte*)malloc(uRequestedSize);
      }
    }

    if (uFirstFreeBlockSize >= uRequestedSize) {
      if (uFirstFreeBlockSize > uRequestedSize) {
        pNewBlockAddress = (byte*)(m_pFirstFreeBlock->getAddress()) + 
          m_pFirstFreeBlock->getSize() - uRequestedSize;
        uResizedMemAmount -= uRequestedSize;
        bUseCurrentBlock = false;

        printf("new block address: %p\n", pNewBlockAddress);
        m_pFirstFreeBlock->print();

        m_lBlocks.emplace_back(Block(pNewBlockAddress, uRequestedSize, 
          sm_uBlockCount, nullptr, bUseCurrentBlock));
        ++sm_uBlockCount;

        m_uUsedMemory += uRequestedSize;

        bUseCurrentBlock = true;
      }
      else if (uFirstFreeBlockSize == uRequestedSize) {
        // don't create another block, use the first free one without splitting
        printf("Reusing existing block\n");
        bUseCurrentBlock = false;
        m_uUsedMemory += uRequestedSize;
      }
      if (m_pFirstFreeBlock->getId() == 0) {
        printf("Splitting block 0!\n");
      }
      m_pFirstFreeBlock->resize(uResizedMemAmount, nullptr, bUseCurrentBlock);

      cChrono.stop();
      printf("\n\nTime to request block: %.2f\n\n", cChrono.timeAsMilliseconds());

      return pNewBlockAddress;
    }
    else {
      printf("First free block hadn't enough memory available\n");
    }

    return nullptr;
  }

  // TODO: implement a version of everything with a std::vector and compare performance
  void releaseBlock(byte* pAddress) {
    printf("Trying to release block with address %p...\n", pAddress);
    auto it = m_lBlocks.begin();
    while (it != m_lBlocks.end()) {
      if (pAddress == it->getAddress()) {//} && it->getId() != 0) {
        it->release();
        m_uUsedMemory -= it->getSize();
        printf("Releasing block %u with address: %p\n", it->getId(), pAddress);

        coalesceBlocks(&it);
        //printf("&it is pointing to: ");
        //it->print();

        break;
      }
      else {
        ++it;
      }
    }
  }

  // TODO: test with main coalesce function
  // private:
  bool checkAndCoalesce(std::list<Allocator::Block>::iterator* pIt, bool bCheckBackwards) {
    bool bReturn = false;

    Block* pBlockToCoalesce = &(*(*pIt));
    Block* pPrevBlock = nullptr;

    uint64 uCurrentSize = pBlockToCoalesce->getSize();
    uint64 uPrevSize = 0;
    
    if (bCheckBackwards == true) {
      --(*pIt);
    }
    else {
      ++(*pIt);
    }

    pPrevBlock = &(*(*pIt));
    uPrevSize = pPrevBlock->getSize();

    pBlockToCoalesce->print();
    pPrevBlock->print();

    if ((*pIt)->getAddress() < pBlockToCoalesce->getAddress()) {
      pBlockToCoalesce = &(*(*pIt));
      uCurrentSize = pBlockToCoalesce->getSize();
      
      if (bCheckBackwards == true) {
        ++(*pIt);
      }
      else {
        --(*pIt);
      }

      pPrevBlock = &(*(*pIt));
      uPrevSize = pPrevBlock->getSize();
    }

    if (pPrevBlock->isFree() == true) {
      pBlockToCoalesce->resize(pBlockToCoalesce->getSize() + 
        pPrevBlock->getSize(), nullptr, true);
      pBlockToCoalesce->print();

      m_lBlocks.erase(*pIt);
      
      bReturn = true;
    }

    return bReturn;
  }

  // @return true if could coalesce at least one block
  bool coalesceBlocks(std::list<Allocator::Block>::iterator* pIt = nullptr) {
    printf("Coalescing...\n");
    printf("Elements before coalescing: ");
    printNumElements();
    bool bReturn = false;

    if (pIt != nullptr) {
      // check if previous block is free
      if ((*pIt) != m_lBlocks.begin()) {
        Block* pBlockToCoalesce = &(*(*pIt));
        Block* pPrevBlock = nullptr;

        uint64 uCurrentSize = pBlockToCoalesce->getSize();
        uint64 uPrevSize = 0;
        
        // check previous block (by ID)
        --(*pIt);
        pPrevBlock = &(*(*pIt));
        uPrevSize = pPrevBlock->getSize();

        pBlockToCoalesce->print();
        pPrevBlock->print();

        if ((*pIt)->getId() != 0) {
          if ((*pIt)->getAddress() < pBlockToCoalesce->getAddress()) {
            pBlockToCoalesce = &(*(*pIt));
            uCurrentSize = pBlockToCoalesce->getSize();
            
            ++(*pIt);
            pPrevBlock = &(*(*pIt));
            uPrevSize = pPrevBlock->getSize();
          }

          if (pPrevBlock->isFree() == true) {
            printf("Prev element free, coalescing\n");
            pBlockToCoalesce->resize(pBlockToCoalesce->getSize() + 
              pPrevBlock->getSize(), nullptr, true);
            pBlockToCoalesce->print();

            m_lBlocks.erase(*pIt);
            
            bReturn = true;
          }
        }
        else {
          // if the previous block is the first block (ID == 0)
          if ((*pIt)->getAddress() + (*pIt)->getSize() == pBlockToCoalesce->getAddress()) {
            pBlockToCoalesce = &(*(*pIt));
            uCurrentSize = pBlockToCoalesce->getSize();
            
            ++(*pIt);
            pPrevBlock = &(*(*pIt));
            uPrevSize = pPrevBlock->getSize();

            // pBlockToCoalesce->print();
            // pPrevBlock->print();
          //}

          if (pPrevBlock->isFree() == true) {
            printf("FIRST element free, coalescing\n");
            pBlockToCoalesce->resize(pBlockToCoalesce->getSize() + 
              pPrevBlock->getSize(), nullptr, true);
            pBlockToCoalesce->print();

            m_lBlocks.erase(*pIt);
            
            bReturn = true;
          }
          }
        }
      }
      else {
        // pIt points to list::begin

        Block* pBlockToCoalesce = &(*(*pIt));
        Block* pPrevBlock = nullptr;

        uint64 uCurrentSize = pBlockToCoalesce->getSize();
        uint64 uPrevSize = 0;
        
        // check next block (by ID)
        ++(*pIt);
        pPrevBlock = &(*(*pIt));
        uPrevSize = pPrevBlock->getSize();

        pBlockToCoalesce->print();
        pPrevBlock->print();

        if (pPrevBlock->isFree() == true) {
          printf("Next element free (only two remaining), coalescing\n");
          pBlockToCoalesce->resize(pBlockToCoalesce->getSize() + 
            pPrevBlock->getSize(), nullptr, true);
          pBlockToCoalesce->print();

          m_lBlocks.erase(*pIt);

          bReturn = true;
        }
      }

      ++(*pIt);
      if (++(*pIt) != m_lBlocks.end() && --(*pIt) != m_lBlocks.end()) {
        Block* pBlockToCoalesce = &(*(*pIt));
        Block* pPrevBlock = nullptr;

        uint64 uCurrentSize = pBlockToCoalesce->getSize();
        uint64 uPrevSize = 0;
        
        // check next block (by ID)
        ++(*pIt);
        pPrevBlock = &(*(*pIt));
        uPrevSize = pPrevBlock->getSize();

        pBlockToCoalesce->print();
        pPrevBlock->print();

        if ((*pIt)->getAddress() < pBlockToCoalesce->getAddress()) {
          pBlockToCoalesce = &(*(*pIt));
          uCurrentSize = pBlockToCoalesce->getSize();

          --(*pIt);
          pPrevBlock = &(*(*pIt));
          uPrevSize = pPrevBlock->getSize();
        }

        if (pPrevBlock->isFree() == true) {
          printf("Next element free, coalescing\n");
          pBlockToCoalesce->resize(pBlockToCoalesce->getSize() + 
            pPrevBlock->getSize(), nullptr, true);
          pBlockToCoalesce->print();

          m_lBlocks.erase(*pIt);

          bReturn = true;
        }
      }

      printf("Elements after coalescing: ");
      printNumElements();
    }
    else {  // if no iterator is supplied, coalesce all blocks in the Allocator
      bReturn = false;
    }

    return bReturn;
  }

  //  Engine should call this from time to time 
  // to determine if the big block has no more free memory
  // and to find the first released block. Then is when the party start
  //  Do not call this if not trying to find a block because this can return nullptr
  Block* findFirstFreeBlock(uint64 uSize = 0) {
    auto it = m_lBlocks.begin();

    while (it != m_lBlocks.end()) {
      printf("findFirstFreeBlock() ID: %u\n", it->getId());
      if (it->isFree() == true) {
        #if (ALLOC_POLICY_BEST_FIT == 1)
          if (it->getSize() >= uSize) {
            printf("Found free block: ");
            it->print();

            return &(*it);
          }
        #elif (ALLOC_POLICY_SPLIT_BIG_BLOCK == 1)
          if (m_pBigFreeBlock->getSize() < uRequestedSize) {
            printf("WARNING, first big block not big enough");
          }
          return m_pBigFreeBlock;
        #else

          printf("Found free block: ");
          it->print();

          return &(*it);
        #endif
      }

      ++it;
    }

    return nullptr;
  }

  const Allocator::Block& getBlock(short shIndex) {
    assert(shIndex < m_lBlocks.size() && "Index greater than the list's size\n");

    /*short shTmp = 0;
    for (std::list<Allocator::Block>::const_iterator it = m_lBlocks.begin();
      it != m_lBlocks.end(); ++it) {
      if (shTmp == shIndex) {
        return *it;
      }
      else {
        ++shTmp;
      }
    }*/

    auto it = m_lBlocks.begin();
    std::advance(it, shIndex);
    return *it;
  }

  const std::list<Allocator::Block>& getBlocks() const {
    return m_lBlocks;
  }

  uint64 getUsedMemory() const {
    return m_uUsedMemory;
  }

  uint64 getNumElements() const {
    return m_lBlocks.size();
  }

  void printUsedMemory() const {
    printf("Used memory: %d bytes\n", m_uUsedMemory);
  }

  void printNumElements() const {
    printf("Number of elements: %u\n", getNumElements());
  }

  void printAllElements() {
    auto it = m_lBlocks.begin();
    printf("Printing all the elements...\n");
    printNumElements();

    while (it != m_lBlocks.end()) {
      it->print();
      ++it;
      //std::advance(it, 1);
    }

    /*for (std::list<Allocator::Block>::const_iterator it = m_lBlocks.begin();
      it != m_lBlocks.end(); ++it) {
      it->print();
    }*/
  }

  void* getBigBlockAddress() const {
    return (void*)m_pBigFreeBlock->getAddress();
  }

  static uint64 getBlockCount() {
    return sm_uBlockCount;
  }

private:
  static uint64 sm_uBlockCount;

  std::list<Block> m_lBlocks;
  Block* m_pBigFreeBlock;
  Block* m_pFirstFreeBlock;
  byte* m_pMemStart;
  uint64 m_uUsedMemory;
  uint64 m_uMaxMemory;
};

uint64 Allocator::sm_uBlockCount = 0;
Allocator cAlloc(64);

// [ Managed Pointer ]
template <class T>
class mptr {
public:
  mptr(const T& constructor) {
    printf("T Class size: %lu\n", sizeof(T));

    m_pPtr = new (cAlloc.requestBlock(sizeof(T))) T(constructor);
  }

  mptr() {
    printf("T Class size: %lu\n", sizeof(T));
    
    m_pPtr = new (cAlloc.requestBlock(sizeof(T))) T();
  }

  T* get() const {
    return m_pPtr;
  }

  T* operator ->() const {
    return m_pPtr;
  }

  void release() {
    m_pPtr->~T();

    cAlloc.releaseBlock((byte*)m_pPtr);
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


int main() {
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

  return 0;
}