#ifndef __MEMORY_ALLOCATOR_H__
#define __MEMORY_ALLOCATOR_H__

#include <cassert>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <list>
#include <vector>

#include "chrono.h"
#include "vector.h"

#define USE_FIRST_BIG_BLOCK 1
#define ALLOC_POLICY_BEST_FIT 1
#define ALLOC_POLICY_SPLIT_BIG_BLOCK (ALLOC_POLICY_BEST_FIT == 0)

typedef uint8_t byte;

class Allocator {
private:
  class Block {
  private:
    Block();

    Block* m_pPrevBlock;
    Block* m_pNextBlock;
    byte* m_pAddress;
    uint64_t m_uSize;
    uint64_t m_uId;
    bool m_bFree;

  public:
    Block(byte* pAddress, uint64_t uSize, uint64_t uId = 1 << 16, 
      Block* pPrevBlock, Block* pNextBlock, bool bFree = false);
    ~Block();

    inline const byte* getAddress() const;
    inline uint64_t getSize() const;
    inline uint64_t getId() const;
    inline bool isFree() const;

    void print() const;

    void resize(uint64_t uNewSize, byte* pNewAddress = nullptr, 
      bool bIsFree = false);

    void changeAddress(byte* pNewAddress);

    inline void release();
  };

  class BlockPool {
  private:
    elm::vector<Block> m_vBlocks;
    Block* m_pBigFreeBlock;
    Block* m_pFirstFreeBlock;
    byte* m_pMemStart;
    uint64_t m_uUsedMemory;
    uint64_t m_uMaxMemory;
    byte m_byAlignment;

  public:
    BlockPool(uint64_t uMemoryAmount, byte byAlignment = 1);
    BlockPool(const BlockPool& cOther);
    ~BlockPool();

    byte* requestBlock(uint64_t uRequestedSize);
    bool releaseBlock(byte* pAddress);
    void releaseAllBlocks();

    enum class IteratorCheckDirection {
      Backward = 0,
      Forward,
    };
    bool coalesceBlocks(Block* pIt, uint64_t uIndex);
    uint64_t findFirstFreeBlock(uint64_t uSize = 0);
    const Allocator::Block& getBlock(uint64_t uIndex);
    const elm::vector<Allocator::Block>& getBlocks() const;

    uint64_t getFreeMemory() const;
    uint64_t getMaxMemory() const;
    uint64_t getUsedMemory() const;
    uint64_t getNumElements() const;
    void printUsedMemory() const;
    void printNumElements() const;
    void printAllElements() const;
    void* getBlockAddress() const;
    static uint64_t getBlockCount();
  };

  // Allocator variables
  std::vector<BlockPool> m_vBlocksPool;
  BlockPool* m_pCurrentPool = nullptr;
  static uint64_t sm_uBlockCount;
  byte m_byAlignment;

public:
  Allocator(uint64_t uRequestedSize, byte byAlignment = 1);
  ~Allocator();

  byte* requestBlock(uint64_t uRequestedSize);
  void releaseBlock(byte* pAddress);
  void printAllElements() const;
  void printUsedMemory();
};

// [ Managed Pointer ]
template <class T>
class mptr {
private:
  T* m_pPtr;

public:
  mptr();
  mptr(const T& constructor);
  ~mptr();

  T* get() const;
  T* operator ->();
  void release();
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

#endif // __MEMORY_ALLOCATOR_H__