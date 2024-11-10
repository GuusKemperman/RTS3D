#pragma once
#include <cassert>
#include <vector>
#include <sstream>
#include <string>

/*
MIT License

Copyright(c)[2022][Guus Kemperman]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this softwareand associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright noticeand this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

namespace DB
{
	using byte_index = size_t;
	using bit_index = unsigned char;
	using bit = bool;

	constexpr bit_index sNumOfBitsInByte = 8;

	class byte;
	class dynamic_bitset;

	// Changing the value of a bitref also updates the value in the byte that it's from.
	// The value will always match the one from the byte that it's originally from.
	// Will lead to undefined behaviour if the byte gets destroyed or moved (e.g. when 
	// the underlying vector of a dynamic_bitset resizes.).
	class bit_ref
	{
	public:
		bit_ref(byte& owner, bit_index indexAtOwner) :
			mOwner(owner),
			mIndexAtOwner(indexAtOwner)
		{}

		inline operator bit() const;
		inline void operator=(bit value);

	private:
		byte& mOwner;
		bit_index mIndexAtOwner{};
	};

	class byte
	{
	public:
		byte() = default;
		byte(char data) : mData(data) {}

		inline void set(const bit_index index, bit bit)
		{
#if _CONTAINER_DEBUG_LEVEL > 0
			assert(index < sNumOfBitsInByte);
#endif // _CONTAINER_DEBUG_LEVEL > 0

			char shiftAmount = sNumOfBitsInByte - index - 1;
			mData = static_cast<char>((mData & ~(1 << shiftAmount)) + (bit << shiftAmount));
		}

		inline bit get(const bit_index index) const
		{
#if _CONTAINER_DEBUG_LEVEL > 0
			assert(index < sNumOfBitsInByte);
#endif // _CONTAINER_DEBUG_LEVEL > 0

			const char shiftAmount = sNumOfBitsInByte - index - 1;
			return (mData & (1 << shiftAmount)) >> shiftAmount;
		}

		inline bit_ref getBitRef(const bit_index index)
		{
			return { *this, index };
		}

		inline operator char& () { return mData; }
		inline operator const char() const { return mData; }

	private:
		char mData{};
	};

	static_assert(sizeof(byte) == 1);

	inline bit_ref::operator bit() const
	{
		return mOwner.get(mIndexAtOwner);
	}

	inline void bit_ref::operator=(bit value)
	{
		mOwner.set(mIndexAtOwner, value);
	}

	// The bits here are stored as part of chars, which in turn are stored inside a vector. This
	// ensures that 1 bit is actually taking up the space of 1 bit, as opposed to std::bitset.
	// This also meanst that getting/retrieving values is going to be slower than std::bitset. If 
	// you know at compile time what size the bitset is going to be, it is highly recommended to 
	// use std::bitset. If you don't need to store/retrieve triviably copyable types in binary 
	// format, it is highly recommned to use std::vector<bool>.
	class dynamic_bitset
	{
		template<typename DerivedType>
		class IteratorBase
		{
		public:
			IteratorBase() = default;
			IteratorBase(byte_index byteIndex, bit_index bitIndex) : mByteIndex(byteIndex), mBitIndex(bitIndex) {}

			using value_type = bit;
			using difference_type = std::ptrdiff_t;
			using iterator_category = std::forward_iterator_tag;

			// Prefix increment
			DerivedType& operator++()
			{
				if (++mBitIndex == 8)
				{
					mBitIndex = 0;
					++mByteIndex;
				}
				return *static_cast<DerivedType*>(this);
			}

			// Postfix increment
			DerivedType operator++(int)
			{
				DerivedType tmp = *static_cast<DerivedType*>(this);
				++(*this);
				return tmp;
			}

			friend bool operator== (const IteratorBase& a, const IteratorBase& b)
			{
				return a.mByteIndex == b.mByteIndex
					&& a.mBitIndex == b.mBitIndex;
			};
			friend bool operator!= (const IteratorBase& a, const IteratorBase& b)
			{
				return a.mByteIndex != b.mByteIndex
					|| a.mBitIndex != b.mBitIndex;
			};

			inline bool operator<(const IteratorBase& other)
			{
				return mByteIndex < other.mByteIndex
					|| (mByteIndex == other.mByteIndex && mBitIndex < other.mBitIndex);
			}

		protected:
			byte_index mByteIndex{};
			bit_index mBitIndex{};
		};

	public:
		class iterator :
			public IteratorBase<iterator>
		{
		public:
			iterator() = default;
			iterator(dynamic_bitset* source, byte_index byteIndex, bit_index bitIndex) : IteratorBase(byteIndex, bitIndex), mSource(source) {}

			using pointer = bit_ref;
			using reference = bit_ref;

			reference operator*() const
			{
#if _ITERATOR_DEBUG_LEVEL > 0
				assert(mSource != nullptr);
#endif // _ITERATOR_DEBUG_LEVEL > 0
				return mSource->getBitRef(mByteIndex, mBitIndex);
			}
			pointer operator->() const
			{
				return *(*this);
			}

			// Prefer this over getting a const reference for performance reasons.
			inline operator bit() const
			{
#if _ITERATOR_DEBUG_LEVEL > 0
				assert(mSource != nullptr);
#endif // _ITERATOR_DEBUG_LEVEL > 0
				return mSource->get(mByteIndex, mBitIndex);
			}

			inline dynamic_bitset* GetSource() const { return mSource; }

		private:
			friend dynamic_bitset;
			dynamic_bitset* mSource{};
		};

		class const_iterator :
			public IteratorBase<const_iterator>
		{
		public:
			const_iterator() = default;
			const_iterator(const dynamic_bitset* source, byte_index byteIndex, bit_index bitIndex) : IteratorBase(byteIndex, bitIndex), mSource(source) {}

			using pointer = bit;
			using reference = bit; 

			reference operator*() const
			{ 
#if _ITERATOR_DEBUG_LEVEL > 0
				assert(mSource != nullptr);
#endif // _ITERATOR_DEBUG_LEVEL > 0
				return mSource->get(mByteIndex, mBitIndex);
			}
			pointer operator->() const 
			{ 
				return *(*this);
			}

			inline operator bit() const
			{
#if _ITERATOR_DEBUG_LEVEL > 0
				assert(mSource != nullptr);
#endif // _ITERATOR_DEBUG_LEVEL > 0
				return mSource->get(mByteIndex, mBitIndex);
			}

			inline const dynamic_bitset* GetSource() const { return mSource; }

		private:
			friend dynamic_bitset;
			const dynamic_bitset* mSource{};
		};

		template<typename IteratorType>
		inline IteratorType begin()
		{
			return begin<IteratorType>(this);
		}

		template<typename IteratorType>
		inline IteratorType end()
		{
			return end<IteratorType>(this);
		}

		inline iterator begin()
		{
			return begin<iterator>(this);
		}

		inline iterator end()
		{
			return end<iterator>(this);
		};

		inline const_iterator begin() const
		{
			return begin<const_iterator>(this);
		}

		inline const_iterator end() const
		{
			return end<const_iterator>(this);
		}

		inline bit get(byte_index byteIndex, bit_index bitIndex) const
		{
#if _ITERATOR_DEBUG_LEVEL > 0
			assert(byteIndex < mData.size()
				|| (byteIndex == mData.size() && bitIndex < mIncompleteByte.mNumOfBits));
#endif // _ITERATOR_DEBUG_LEVEL

			const byte& byte = byteIndex < mData.size() ? mData[byteIndex] : mIncompleteByte.mByte;
			return byte.get(bitIndex);
		}

		// Returns the bit the iterator is pointing too and increments the iterator
		inline bit get(iterator& it) const
		{
			bit returnBit = it;
			++it;
			return returnBit;
		}

		inline bit_ref getBitRef(byte_index byteIndex, bit_index bitIndex)
		{
#if _ITERATOR_DEBUG_LEVEL > 0
			assert(byteIndex < mData.size()
				|| (byteIndex == mData.size() && bitIndex < mIncompleteByte.mNumOfBits));
#endif // _ITERATOR_DEBUG_LEVEL

			byte& returnByte = byteIndex < mData.size() ? mData[byteIndex] : mIncompleteByte.mByte;
			return returnByte.getBitRef(bitIndex);
		}

		// Returns the bitref the iterator is pointing too and increments the iterator
		inline bit_ref getBitRef(iterator& it)
		{
			bit_ref returnBitref = *it;
			++it;
			return returnBitref;
		}

		template<typename TriviablyCopyableType>
		inline void push_back(const TriviablyCopyableType& value)
		{
			static_assert(std::is_trivially_copyable<TriviablyCopyableType>::value);
			push_back(reinterpret_cast<const char*>(&value), sizeof(value));
		}

		inline void push_back(const char* dataStart, const size_t numOfBytes)
		{
			for (size_t i = 0; i < numOfBytes; i++)
			{
				push_back(byte{ dataStart[i] });
			}
		}

		inline void push_back(const std::string& string)
		{
			// We can't use the terminating character, since the string might represent binary data with coincidental terminating characters inside.
			if (string.size() > std::numeric_limits<unsigned char>::max())
			{
				push_back(false);
				push_back(string.size());
			}
			else
			{
				push_back(true);
				push_back(static_cast<unsigned char>(string.size()));
			}
			push_back(string.c_str(), string.size());
		}

		inline void push_back(byte byte)
		{
			for (bit_index i = 0; i < sNumOfBitsInByte; i++)
			{
				push_back(byte.get(i));
			}
		}

		inline void push_back(bit bit)
		{
			mIncompleteByte.mByte.set(mIncompleteByte.mNumOfBits, bit);
			mIncompleteByte.mNumOfBits++;

			if (mIncompleteByte.isFull())
			{
				mData.push_back(mIncompleteByte.mByte);
				mIncompleteByte.mNumOfBits = 0;
			}
		}

		// Removes the last bit.
		inline void pop_back()
		{
			if (isThereAnIncompleteByte())
			{
				mIncompleteByte.mNumOfBits--;
				return;
			}

			assert(!mData.empty());

			mIncompleteByte.mByte = mData.back();
			mIncompleteByte.mNumOfBits = sNumOfBitsInByte - 1;

			mData.pop_back();
		}

		inline void clear()
		{
			mData.clear();
			mIncompleteByte.mNumOfBits = 0;
		}

		// Returns the amount of COMPLETE BYTES
		inline size_t size_in_bytes() const
		{
			return mData.size();
		}

		// Beware of overflows
		inline size_t size_in_bits() const
		{
			return size_in_bytes() * sNumOfBitsInByte + mIncompleteByte.mNumOfBits;
		}

		inline bool empty() const
		{
			return size_in_bytes() == 0 && !isThereAnIncompleteByte();
		}

		// Returns the byte the iterator is pointing too and increments the iterator
		template<typename IteratorType>
		static byte getByte(IteratorType& it)
		{
			byte returnByte{};

			// This is much faster, it's worth it for us to check to see if this is a possibility.
//			if (it.mBitIndex == 0)
//			{
//#if _ITERATOR_DEBUG_LEVEL > 0
//				assert(it.mByteIndex < it.mSource->mData.size());
//#endif // _ITERATOR_DEBUG_LEVEL
//				returnByte = it.mSource->mData[it.mByteIndex];
//				++it.mByteIndex;
//				return returnByte;
//			}

			for (bit_index i = 0; i < sNumOfBitsInByte; i++, ++it)
			{
				bit returnBit = it.mSource->get(it.mByteIndex, it.mBitIndex);
				returnByte.set(i, returnBit);
			}

			return returnByte;
		}

		// Creates and returns an instance of the type by using the next sizeof(type) bytes.
		template <typename TriviablyCopyableType>
		inline TriviablyCopyableType extract(byte_index byteIndex, bit_index bitIndex) const
		{
			const_iterator it = { this, byteIndex, bitIndex };
			return extract<TriviablyCopyableType>(it);
		}

		template <typename TriviablyCopyableType>
		static inline TriviablyCopyableType extract(const_iterator& it)
		{
			TriviablyCopyableType tmp{};
			extract<const_iterator>(it, tmp);
			return tmp;
		}

		template <typename TriviablyCopyableType>
		static inline TriviablyCopyableType extract(iterator& it)
		{
			TriviablyCopyableType tmp{};
			extract<iterator>(it, tmp);
			return tmp;
		}

		// Fills the destination with the bytes the iterator is pointing too and increments the iterator
		template<typename IteratorType>
		static inline void extract(char* destination, size_t amountOfBytesToExtract, IteratorType& it)
		{
			for (size_t i = 0; i < amountOfBytesToExtract; i++)
			{
				// Use operator&, because conversion sequence is better.
				destination[i] = static_cast<char&>(getByte(it));
			}
		}

		// Fills the destination with the bytes specified using the byteIndex and bitIndex
		inline void extract(char* destination, size_t amountOfBytesToExtract, byte_index byteIndex, bit_index bitIndex) const
		{
			const_iterator it = { this, byteIndex, bitIndex };
			extract(destination, amountOfBytesToExtract, it);
		}

		bool isThereAnIncompleteByte() const 
		{ 
			return mIncompleteByte.mNumOfBits > 0;
		}

		void serialize(const std::string& filePath) const
		{
			std::ofstream destination(filePath, std::ostream::binary);
			assert(destination.is_open());

			destination.put(mIncompleteByte.mNumOfBits);

			if (isThereAnIncompleteByte())
			{
				destination.put(mIncompleteByte.mByte);
			}

			for (char ch : mData)
			{
				destination.put(ch);
			}

			destination.close();
		}

		static dynamic_bitset deserialize(const std::string& filePath)
		{
			std::ifstream source(filePath, std::ostream::binary);
			assert(source.is_open());

			int read;
			read = source.get();

			dynamic_bitset bitset{};

			if (read == EOF)
			{
				return bitset;
			}

			bitset.mIncompleteByte.mNumOfBits = static_cast<bit_index>(read);

			if (bitset.isThereAnIncompleteByte())
			{
				read = source.get();
				assert(read != EOF);
				bitset.mIncompleteByte.mByte = static_cast<char>(read);
			}

			bitset.mData.reserve(2500);
			while ((read = source.get()), read != EOF)
			{
				bitset.mData.emplace_back(static_cast<char>(read));
			}
			bitset.mData.shrink_to_fit();

			source.close();

			return bitset;
		}

	private:
		template<typename IteratorType, typename From>
		static IteratorType begin(From* fromBitset)
		{
			return IteratorType{ fromBitset, 0, 0 };
		}

		template<typename IteratorType, typename From>
		static IteratorType end(From* fromBitset)
		{
			byte_index byteIndex = fromBitset->mData.size();
			bit_index bitIndex = 0;

			if (fromBitset->isThereAnIncompleteByte())
			{
				bitIndex = fromBitset->mIncompleteByte.mNumOfBits;
			}

			return IteratorType{ fromBitset, byteIndex, bitIndex };
		}

		// Creates and returns an instance of the type by using the next sizeof(type) bytes. Increments the iterator by the size of the type.
		template <typename IteratorType, typename TriviablyCopyableType>
		static inline void extract(IteratorType& it, TriviablyCopyableType& out)
		{
			static_assert(std::is_trivially_copyable<TriviablyCopyableType>::value);
			extract(reinterpret_cast<char*>(&out), sizeof(TriviablyCopyableType), it);
		}

		template <typename IteratorType>
		static inline void extract(IteratorType& it, std::string& out)
		{
			DB::bit sizeAsChar = it++;

			const size_t numOfBytes = sizeAsChar ? static_cast<size_t>(extract<unsigned char>(it)) : extract<size_t>(it);
			char* data = new char[numOfBytes];
			extract(data, numOfBytes, it);
			out = { data, numOfBytes };
			delete[] data;
		}

		std::vector<byte> mData{};

		struct IncompleteByte
		{
			byte mByte{};
			bit_index mNumOfBits = 0;

			bool isFull() const { return mNumOfBits == sNumOfBitsInByte; }
		};
		IncompleteByte mIncompleteByte{};

		bit_index mBitIndex = sNumOfBitsInByte - 1;
	};
}