#ifndef __ELM_VECTOR_H__
#define __ELM_VECTOR_H__

#include <iostream>
#include <memory>
#include <cmath>
#include <cstring>
#include <cassert>
#include <cstdarg>

typedef unsigned int uint32;
typedef unsigned char byte;

namespace elm {
template <class T>
class vector
{
public:
  /*
   *  Allocates size for two elements on creation
   *
   *  Maybe not allocating anything before insering
   *  an element could be an option.
  */
  vector()
  {
    m_uClassSize = sizeof(T);
    
    m_uCount = 0;
    m_uCapacity = 2;

    m_pVectorBegin = (T*)malloc(m_uCapacity * m_uClassSize);
    assert(m_pVectorBegin != nullptr && "couldn't allocate memory. vector()");
    m_pIterator = nullptr;
  }
  
  ~vector()
  {
    if (m_uCount > 0)
    {
      T* pPtr = m_pVectorBegin;
      for (unsigned int i = 0; i < m_uCount; i++)
      {
        pPtr->~T();
        //m_uCount = 0;
        ++pPtr;
      }
      m_uCount = 0;
    }
    
    if (m_pVectorBegin != nullptr)
    {
      free(m_pVectorBegin);
    }
  }

  T* data() const
  {
    return m_pVectorBegin;
  }

  uint32 classSize() const
  {
    return m_uClassSize;
  }

  uint32 size() const
  {
    return m_uCount;
  }

  uint32 capacity() const
  {
    return m_uCapacity;
  }

  void push_back(const T& element)
  {
    if (m_uCount == m_uCapacity)
    {
      vector::allocate();
    }

    m_pIterator = m_pVectorBegin + m_uCount;
    //memcpy(m_pIterator, &element, m_uClassSize);
    new(m_pIterator)T(element);
    m_pIterator = nullptr;
    
    ++m_uCount;
  }

  /*
   *  Quite costly. Reallocation can happen.
   *
   *  TODO: maybe assert if position >= m_uCount?
  */
  void insert(const T& element, uint32 position)
  {
    if (position < m_uCount)
    {
      if (m_uCount == m_uCapacity)
      {
        vector::allocate();
      }

      vector<T> aux;
      aux.reserve(m_uCapacity);
      for (uint32 i = position; i < m_uCount; i++)
      {
        //T e((*(m_pVectorBegin + i)));
        //aux.push_back(e);
        aux.emplace_back((*(m_pVectorBegin + i)));
      }

      m_pIterator = m_pVectorBegin + position;
      new(m_pIterator)T(element);
      ++m_uCount;
      m_pIterator = nullptr;

      for (uint32 i = position + 1; i < m_uCount; i++)
      {
        T e(aux[i - position - 1]);
        m_pIterator = m_pVectorBegin + i;
        
        m_pIterator->~T();
        new(m_pIterator)T(e);
      }
      m_pIterator = nullptr;
    }
    else
    {
      printf("Index out of range in vector\n");
    }
  }

  /*
   *
  */
  T erase(uint32 uPosition) {
    if (uPosition < m_uCount)
    {
      // COPY the element
      T e((*(m_pVectorBegin + uPosition)));

      vector<T> aux;
      aux.reserve(uPosition + 1); // (m_uCapacity);
      
      for (uint32 i = uPosition + 1; i < m_uCount; ++i)
      {
        // T e = *(m_pVectorBegin + i);
        // aux.push_back(e);
        aux.emplace_back((*(m_pVectorBegin + i)));
      }

      for (uint32 i = uPosition; i < m_uCount; i++)
      {
        T e(aux[i - uPosition]);
        m_pIterator = m_pVectorBegin + i;
        
        m_pIterator->~T();
        new(m_pIterator)T(e);
      }
      m_pIterator = nullptr;

      e.~T();
      --m_uCount;
      return e;
    }
    else
    {
      printf("Index out of range in vector\n");
      return T();
    }
  }

  /*
   *  Returns a copy of the popped element.
  */
  void pop_back()
  {
    if (m_uCount > 0)
    {
      // T e((*(m_pVectorBegin + (m_uCount - 1))));
      // e.~T();
      (m_pVectorBegin + (m_uCount - 1))->~T();
      --m_uCount;
    }
  }

  /*
   *  If amount is lesser than the actual capacity of the vector,
   *  no reallocation will happen and the elements in the difference 
   *  will be lost.
  */
  void reserve(uint32 uAmount)
  {
    if (uAmount > m_uCapacity)
    {
      vector::allocate(uAmount);
    } else
    {
      printf("Requested lesser capacity than the current one\n");

      // TODO: implement shrink_to_fit()

      //m_uCapacity = uAmount;
    }
  }

  /*  
   *  Note: Because it return an object, it has to be destroyed
   *  locally when exiting the function
  */
  T& operator[](uint32 index) const
  {
    return *(m_pVectorBegin + index);
  }

  /*
   *  Should implement exception-throwing system
  */
  T at(uint32 index) const
  {
    if (index < m_uCount)
    {
      return operator[](index);
    } else
    {
      printf("Index out of range in vector\n");
      return T();
    }
  }

  uint32 memorySize()
  {
    return (m_uCapacity * m_uClassSize);
  }

  template <typename... Args> 
  void emplace_back(Args&&... args) {

    if (m_uCount == m_uCapacity)
    {
      vector::allocate();
    }

    m_pIterator = m_pVectorBegin + m_uCount;
    new(m_pIterator)T(std::forward<Args>(args)...);
    m_pIterator = nullptr;
    
    ++m_uCount;
  }

private:
  bool allocate(uint32 amount = 0)
  {
    if (amount == 0)
    {
      printf("Allocating memory...\n");
      /* round(cap + log(cap + 1) */
      //double new_cap = (double)m_uCapacity + log(m_uCapacity + 1);
      //uint32 converted_cap = (uint32)round(new_cap) + 1;

      /* cap * 2 */
      //uint32 converted_cap = m_uCapacity * 2;

      /* cap + cap * 2 */
      //uint32 converted_cap = m_uCapacity + m_uCapacity * 2;

      /* round(cap + (cap / 2) + log(cap + 1)) */
      double new_cap = (double)m_uCapacity + ((double)m_uCapacity / 4.0) + log(m_uCapacity + 1);
      uint32 converted_cap = (uint32)round(new_cap);

      amount = converted_cap;
    }

    T* tmp = m_pVectorBegin;
    m_pVectorBegin = nullptr;
    m_pVectorBegin = (T*)malloc(amount * m_uClassSize);
    if (m_pVectorBegin != nullptr)  // assert(m_pVectorBegin == nullptr && "Failed to allocate new memory");
    {
      // printf("Copying the elements...\n");
      // memset(m_pVectorBegin, 0, amount * m_uClassSize);

      // memcpy(m_pVectorBegin, tmp, m_uClassSize * m_uCount);
      // for (uint32 i = 0; i < m_uCount; i++)
      // {
      //   T* e = tmp + i;
      //   m_pIterator = m_pVectorBegin + i;
      //   *m_pIterator = *e;
      //   e->~T();
      // }
      // m_pIterator = nullptr;

      // // delete all old copies? Not needed I think.

      // free(tmp);
      // tmp = nullptr;

      // m_uCapacity = amount;

      // return true;

      for (uint32 i = 0; i < m_uCount; ++i) {
        new(m_pVectorBegin + i)T((*(tmp + i)));
      }

      free(tmp);
      m_uCapacity = amount;
      return true;
    } else
    {
      m_pVectorBegin = nullptr;
      m_pIterator = nullptr;
      tmp = nullptr;
      return false;
    }
  }

  T* m_pVectorBegin;
  T* m_pIterator;
  uint32 m_uClassSize;
  uint32 m_uCount;
  uint32 m_uCapacity;
};
}

#endif // __ELM_VECTOR_H__