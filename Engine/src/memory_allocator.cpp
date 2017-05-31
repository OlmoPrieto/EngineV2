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

    const byte* address() const {
      return m_pAddress;
    }

    std::size_t size() const {
      return m_uSize;
    }

    bool free() const {
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

    // Public variables
    Block* m_pNextFreeBlock;

  private:
    byte* m_pAddress;
    std::size_t m_uSize;
    uint64 m_uId;
    bool m_bFree;
  };

  Allocator() {
    m_pMemStart = (byte*)malloc(2000000);  // 2MB
    if (m_pMemStart != nullptr) {
      //printf("id: %d\n", sm_uBlockCount);
      m_lBlocks.emplace_back(Block(m_pMemStart, 2000000, sm_uBlockCount));
      sm_uBlockCount++;
      m_pBigFreeBlock = &m_lBlocks.front();
      m_pFirstFreeBlock = &m_lBlocks.front();
      m_lBlocks.front().m_pNextFreeBlock = m_pBigFreeBlock;

      m_uUsedMemory = 0;
    } 
    else {
      m_pMemStart = nullptr;
      printf("ERROR: Not enough memory!\nW: Allocator");
    }
  }

  ~Allocator() {
    if (m_pMemStart != nullptr) {
      free(m_pMemStart);
    }
  }

  byte* requestBlock(std::size_t uRequestedSize) {

    if (m_pBigFreeBlock->size() >= uRequestedSize) {
      m_pFirstFreeBlock = m_pBigFreeBlock;
    }

    // This will keep giving blocks until the big block isn't enough
    // TODO: manage freed blocks -> split them as needed 
    //    -> check if prev/next block is free and join them
    if (m_pFirstFreeBlock->size() >= uRequestedSize) {
      byte* pNewBlockAddress = (byte*)(m_pFirstFreeBlock->address());

      m_pFirstFreeBlock->resize(m_pFirstFreeBlock->size() - uRequestedSize, 
        (byte*)(m_pFirstFreeBlock->address()) + uRequestedSize);

      m_lBlocks.emplace_back(Block(pNewBlockAddress, uRequestedSize, sm_uBlockCount));
      sm_uBlockCount++;

      m_uUsedMemory += uRequestedSize;

      return pNewBlockAddress;
    }

    return nullptr;

  }

  void releaseBlock(byte* pAddress) {
    // set next free block to the next free block in the list
  }

  //  Engine should call this time by time 
  // to determine if the big block has no more free memory
  // and to find the first released block. Then is when the party start
  void findFirstFreeBlock() {

  }

  const Allocator::Block& getBlock(short shIndex) {
    short shTmp = 0;
    for (std::list<Allocator::Block>::const_iterator it = m_lBlocks.begin();
      it != m_lBlocks.end(); it++) {
      if (shTmp == shIndex) {
        return *it;
      }
      else {
        shTmp++;
      }
    }
    //std::list<Allocator::Block>::iterator it = m_lBlocks.begin();
    //std::advance(it, 0);
    //return *it;
  }

  const std::list<Allocator::Block>& getBlocks() const {
    return m_lBlocks;
  }

  uint64 getUsedMemory() const {
    return m_uUsedMemory;
  }

  void printUsedMemory() const {
    printf("\nUsed memory: %d bytes\n", m_uUsedMemory);
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
Allocator cAlloc;

// [ Managed Pointer ]
template <class T>
class mptr {
public:
  mptr() {
    printf("T Class size: %d\n", sizeof(T));

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
  printf("\n=======================\n");
  printf("\nmain()\n\n");
  printf("Stats: \n"),
  printf("sizeof int\t: %d bytes\n", sizeof(int));
  printf("sizeof float\t: %d bytes\n", sizeof(float));
  printf("sizeof double\t: %d bytes\n", sizeof(double));
  printf("sizeof byte*\t: %d bytes\n", sizeof(byte*));
  printf("=======================\n");

  printf("Created allocator\n");

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

  //printf("Used memory: %d bytes\n", cAlloc.getUsedMemory());
  cAlloc.printUsedMemory();
  printf("Creating Foo...\n");
  mptr<Foo> foo1;
  foo1->print();
  //printf("Used memory: %d bytes\n", cAlloc.getUsedMemory());
  cAlloc.printUsedMemory();

  return 0;
}