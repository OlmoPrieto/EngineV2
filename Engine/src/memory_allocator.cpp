#include <iostream>
#include <list>

typedef unsigned char byte;

class Allocator {
public:
  class Block {
  public:
    Block(byte* pAddress, std::size_t uSize) {
      m_pAddress = pAddress;
      m_uSize = uSize;
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
      printf("Block address: %p\n", m_pAddress);
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

  private:
    byte* m_pAddress;
    std::size_t m_uSize;
    bool m_bFree;
  };

  Allocator() {
    byte* pMemStart = (byte*)malloc(2000000);  // 2MB
    if (pMemStart != nullptr) {
      Block cBlock(pMemStart, 2000000);
      m_lBlocks.push_back(cBlock);
      //m_lBlocks.emplace_back(Block(pMemStart, 2000000));
    } 
    else {
      printf("ERROR: Not enough memory!\nW: Allocator");
    }
  }

  ~Allocator() {

  }

  byte* requestBlock(std::size_t uRequestedSize) {
    Block cBlock = m_lBlocks.front();

    // TODO: checks checks checks
    byte* pNewBlockAddress = (byte*)(cBlock.address());
    m_lBlocks.front().resize(cBlock.size() - uRequestedSize, 
      (byte*)(cBlock.address()) + uRequestedSize);
    
    //const byte* pConstAddress = m_lBlocks.front().address();
    //byte* pNextAddress = (byte*)(pConstAddress) + uRequestedSize;
    //printf("Old: %p\nNew: %p\n", pConstAddress, pNextAddress);

    Block cNewBlock(pNewBlockAddress, uRequestedSize);
    m_lBlocks.push_back(cNewBlock);
  }

  const Allocator::Block& getBlock(short shIndex) {
    short shTmp = 0;
    for (std::list<Allocator::Block>::iterator it = m_lBlocks.begin();
      it != m_lBlocks.end(); it++) {
      if (shTmp == shIndex) {
        return *it;
      }
      else {
        shTmp++;
      }
    }
  }

  const std::list<Allocator::Block>& getBlocks() const {
    return m_lBlocks;
  }

private:
  std::list<Block> m_lBlocks;
};

Allocator cAlloc;

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

  void* operator new(std::size_t uSize) {

  }

  int iX;
  int iY;
  int iZ;
};

int main() {
  printf("\n=======================\n");
  printf("\nmain()\n\n");
  printf("Stats: \n"),
  printf("sizeof int\t: %d\n", sizeof(int));
  printf("sizeof float\t: %d\n", sizeof(float));
  printf("sizeof double\t: %d\n", sizeof(double));
  printf("sizeof byte*\t: %d\n", sizeof(byte*));
  printf("=======================\n");

  cAlloc = Allocator();
  printf("Created allocator\n");
  const std::list<Allocator::Block> pBlocks = cAlloc.getBlocks();
  //printf("Address: %p\n", pBlocks.front().address());
  printf("First block address: ");
  pBlocks.front().print();

  printf("\nRequesting block...\n");
  cAlloc.requestBlock(4);
  //const Allocator::Block cBlock1 = cAlloc.getBlock(1);
  //cBlock1.print();
  printf("First block address (big free block): ");
  cAlloc.getBlock(0).print();
  printf("Second block address: ");
  cAlloc.getBlock(1).print();

  Foo foo1;
  printf("sizeof Foo = %d\n", sizeof(foo1));

  return 0;
}