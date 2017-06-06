#include <cassert>
#include <iostream>
#include <iterator>
#include <list>

typedef unsigned char byte;
typedef unsigned int uint64;

class Allocator {
public:
  class Block {
  public:
    Block(byte* pAddress, std::size_t uSize, uint64 uId = 65536, Block* pNextFreeBlock = nullptr) {
      m_pAddress = pAddress;
      m_pNextFreeBlock = pNextFreeBlock;
      m_uSize = uSize;
      m_uId = uId;
      m_bFree = false;
    }

    ~Block() {

    }

    void setNextFreeBlock(Block* pNextFreeBlock) {
      m_pNextFreeBlock = pNextFreeBlock;
    }

    const byte* getAddress() const {
      return m_pAddress;
    }

    std::size_t getSize() const {
      return m_uSize;
    }

    uint64 getId() const {
      return m_uId;
    }

    bool isFree() const {
      return m_bFree;
    }

    void print() const {
      printf("Block address: %p\tid: %u\n", m_pAddress, m_uId);
    }

    void resize(std::size_t uNewSize, byte* pNewAddress = nullptr) {
      m_uSize = uNewSize;
      if (pNewAddress != nullptr) {
        m_pAddress = pNewAddress;
      }
    }

    void changeAddress(byte* pNewAddress) {
      m_pAddress = pNewAddress;
    }

    void release() {
      m_bFree = true;
    }

    // Public variables
    Block* m_pNextFreeBlock;

  private:
    byte* m_pAddress;
    std::size_t m_uSize;
    uint64 m_uId;
    bool m_bFree;
  };

  Allocator(std::size_t uMemoryAmount) {
    m_pMemStart = (byte*)malloc(uMemoryAmount);  // 2MB
    if (m_pMemStart != nullptr) {
      //printf("id: %d\n", sm_uBlockCount);
      m_lBlocks.emplace_back(Block(m_pMemStart, uMemoryAmount, sm_uBlockCount));
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
    if (m_pMemStart != nullptr) {
      free(m_pMemStart);
    }
  }

  byte* requestBlock(std::size_t uRequestedSize) {

    if (m_pBigFreeBlock->getSize() >= uRequestedSize) {
      m_pFirstFreeBlock = m_pBigFreeBlock;
    }
    else {
      m_pFirstFreeBlock = findFirstFreeBlock();
      if (m_pFirstFreeBlock == nullptr) {
        printf("Not enough memory, returning malloc() address\t-> W: Allocator::requestBlock\n");
        return (byte*)malloc(uRequestedSize);
      }
    }

    // This will keep giving blocks until the big block isn't enough
    // TODO: manage freed blocks -> split them as needed 
    //    -> check if prev/next block is free and join them
    if (m_pFirstFreeBlock->getSize() >= uRequestedSize) {
      byte* pNewBlockAddress = (byte*)(m_pFirstFreeBlock->getAddress());

      m_pFirstFreeBlock->resize(m_pFirstFreeBlock->getSize() - uRequestedSize, 
        (byte*)(m_pFirstFreeBlock->getAddress()) + uRequestedSize);

      m_lBlocks.emplace_back(Block(pNewBlockAddress, uRequestedSize, sm_uBlockCount));
      ++sm_uBlockCount;

      m_uUsedMemory += uRequestedSize;

      return pNewBlockAddress;
    }

    return nullptr;
  }

  // TODO: implement a version of everything with a std::vector and compare performance
  void releaseBlock(byte* pAddress) {
    // set next free block to the next free block in the list
    auto it = m_lBlocks.begin();
    while (it != m_lBlocks.end()) {
      if (pAddress == it->getAddress()) {
        it->release();
        m_uUsedMemory -= it->getSize();
        printf("Releasing block %u with address: %p\n", it->getId(), pAddress);

        coalesceBlocks(&it);

        break;
      }
      else {
        ++it;
      }
    }
  }

  // TODO: test all cases;
  // release one sole block, [done]
  // release one block with another free forward, 
  // release one block with another free backward, [done]
  // release one block with free blocks backward and forward
  //
  // @return true if could coalesce at least one block
  bool coalesceBlocks(std::list<Allocator::Block>::iterator* pIt = nullptr) {
    printf("Coalescing...\n");
    bool bReturn = false;
    bool bCoalescePrevBlock = false;

    if (pIt != nullptr) { // if an iterator is supplied, try to coalesce prev and/or next block
      // check if previous block is free
      if ((*pIt) != m_lBlocks.begin()) {
        --(*pIt);
        printf("prev ID: %u\n", (*pIt)->getId());
        if ((*pIt)->isFree() == true) {
          printf("Prev element free\n");
          //(*pIt)->setNextFreeBlock(&(*(*pIt)));
          (*pIt)->resize((*pIt)->getSize() + (++(*pIt))->getSize());
          m_lBlocks.erase(*pIt);  // delete current block
          bReturn = true;
          bCoalescePrevBlock = true;
        }
      }

      // check if next block is free
      if (bCoalescePrevBlock == false) {
        ++(*pIt);
      } 
      ++(*pIt); // if the block was coalesced, ++(*pIt) will be the next block to the current
      if (*pIt != m_lBlocks.end()) {
        printf("next ID: %u\n", (*pIt)->getId());
        if ((*pIt)->isFree() == true) {
          printf("Next element free\n");
          //(*pIt)->setNextFreeBlock(&(*(*pIt)));
          --(*pIt);
          (*pIt)->resize((*pIt)->getSize() + (++(*pIt))->getSize());
          m_lBlocks.erase((*pIt));
          bReturn = true;
        }
      }
    }
    else {  // if no iterator is supplied, coalesce all blocks in the Allocator
      bReturn = false;
    }

    return bReturn;
  }

  //  Engine should call this time by time 
  // to determine if the big block has no more free memory
  // and to find the first released block. Then is when the party start
  Block* findFirstFreeBlock() {
    auto it = m_lBlocks.begin();

    while (it != m_lBlocks.end()) {
      printf("findFirstFreeBlock() ID: %u\n", it->getId());
      if (it->isFree() == true) {
        printf("%u is a free block!\n", it->getId());
        return &(*it);
      }

      ++it;
    }

    return nullptr;
  }

  const Allocator::Block& getBlock(short shIndex) {
    assert(shIndex < m_lBlocks.size() && "Index greater than the list's size\n");

    short shTmp = 0;
    for (std::list<Allocator::Block>::const_iterator it = m_lBlocks.begin();
      it != m_lBlocks.end(); ++it) {
      if (shTmp == shIndex) {
        return *it;
      }
      else {
        ++shTmp;
      }
    }
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
    printf("\nUsed memory: %d bytes\n", m_uUsedMemory);
  }

  void printNumElements() const {
    printf("\nNumber of elements: %u\n", getNumElements());
  }

  static uint64 getBlockCount() {
    return sm_uBlockCount;
  }

private:
  static uint64 sm_uBlockCount;

  std::list<Block> m_lBlocks;
  Block* m_pBigFreeBlock;
  Block* m_pFirstFreeBlock;
  uint64 m_uUsedMemory;
  byte* m_pMemStart;
};

uint64 Allocator::sm_uBlockCount = 0;
Allocator cAlloc(48);

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

  ~mptr() {
    m_pPtr->~T();

    cAlloc.releaseBlock((byte*)m_pPtr);
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

  const std::list<Allocator::Block> pBlocks = cAlloc.getBlocks();

  printf("First block address: ");
  pBlocks.front().print();

  printf("\nRequesting block...\n");
  cAlloc.requestBlock(32);

  printf("First block address (big free block): ");
  cAlloc.getBlock(0).print();
  printf("Second block address: ");
  cAlloc.getBlock(1).print();

  printf("\nRequesting block...\n");
  cAlloc.requestBlock(4);
  printf("First block address (big free block): ");
  cAlloc.getBlock(0).print();
  printf("Second block address: ");
  cAlloc.getBlock(1).print();
  printf("Third block address: ");
  cAlloc.getBlock(2).print();


  cAlloc.printUsedMemory();
  cAlloc.printNumElements();
  printf("Creating Foo...\n");
  {
    mptr<Foo> foo1(Foo(100, 200, 300));
    foo1->print();
    cAlloc.printNumElements();
    printf("Going out of scope\n");
  }
  cAlloc.printNumElements();
  cAlloc.printUsedMemory();

  printf("Creating second Foo()...\n");
  {
    mptr<Foo> foo1;
    foo1->print();
  }
  cAlloc.printNumElements();
  cAlloc.printUsedMemory();

  return 0;
}