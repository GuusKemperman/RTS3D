#pragma once

namespace Framework::Data
{
	enum class Format : bool { readable, binary };

	class Scope;

	// Stored as a string on a single line: "mName" = "value".
	class Variable
	{
	public:
		Variable() = default;
		Variable(const std::string& name, const std::string& value, const Format format);

		inline const std::string& GetName() const { return mName; }
		inline const std::string& GetValue() const { return mValue; }

		bool operator==(const Variable& other) const { return mValue == other.mValue; }
		bool operator!=(const Variable& other) const { return mValue != other.mValue; }

		// Basic types
		template<typename T>
		inline void operator<<(const T& value)
		{
			if (mFormat == Format::readable)
			{
				// Borrow stringstream write operations
				std::stringstream tmpStream{};
				tmpStream << value;
				mValue = tmpStream.str();
			}
			else
			{
				mValue = ConvertToBinary(value);
			}
		}

		template<typename T>
		inline void operator>>(T& value) const
		{
			if (mFormat == Format::readable)
			{
				std::stringstream tmpStream = std::stringstream{ mValue };
				tmpStream >> value;
			}
			else
			{
				ConvertFromBinary(mValue, value);
			}
		}

		inline void operator>>(std::string& value) const
		{
			value = mValue;
		}

		inline void operator<<(const std::string& value)
		{
			mValue = value;
		}

		inline void operator<<(const bool& value)
		{
			if (mFormat == Format::readable)
			{
				if (value)
				{
					mValue = "true";
				}
				else
				{
					mValue = "false";
				}
			}
			else
			{
				operator<<(static_cast<char>(value));
			}
		}

		inline void operator>>(bool& value) const
		{
			if (mFormat == Format::readable)
			{
				if (mValue == "true")
				{
					value = true;
					return;
				}
				else if (mValue == "false")
				{
					value = false;
					return;
				}
			}
			char tmp;
			operator>>(tmp);
			value = static_cast<bool>(tmp);
		}

		inline Format GetFormat() const 
		{
			return mFormat; 
		}

		template<typename T>
		static std::string ConvertToBinary(const T& value);
		
		template<typename T>
		static void ConvertFromBinary(const std::string& binaryStr, T& out);

	private:
		std::string mName{};
		// The value stored on file for this variable will be set to this value at the time of saving.
		std::string mValue{};

		Format mFormat{};
	};

	template<typename T>
	inline std::string Variable::ConvertToBinary(const T& value)
	{
		static_assert(std::is_trivially_copyable<T>::value);

		constexpr size_t size = sizeof(T);
		const char* data = reinterpret_cast<const char*>(&value);

		return std::string{ data, size };
	}


	template<typename T>
	inline void Variable::ConvertFromBinary(const std::string& binaryStr, T& out)
	{
		static_assert(std::is_trivially_copyable<T>::value);

		constexpr size_t numOfBytes = sizeof(T);
		char* dest = reinterpret_cast<char*>(&out);
		memcpy(dest, binaryStr.c_str(), numOfBytes);
	}
}

template<typename T>
void operator<<(Framework::Data::Variable& destination, const std::vector<T>& value)
{
	const char* binaryData = reinterpret_cast<const char*>(value.data());
	const size_t numOfBytes = value.size() * sizeof(T);

	const std::string binaryString = { binaryData, numOfBytes };

	switch (destination.GetFormat())
	{
	case Framework::Data::Format::readable:
	{
		destination << Framework::StringFunctions::BinaryToHex(binaryString);
		break;
	}
	case Framework::Data::Format::binary:
	{
		destination << binaryString;
		break;
	}
	}
}

template<typename T>
void operator>>(const Framework::Data::Variable& source, std::vector<T>& out)
{
	std::string binaryString = source.GetValue();

	switch (source.GetFormat())
	{
	case Framework::Data::Format::readable:
	{
		binaryString = Framework::StringFunctions::HexToBinary(binaryString);
		break;
	}
	case Framework::Data::Format::binary:
	{
		// Already in binary format
		break;
	}
	}

	const size_t vectorSize = binaryString.size() / sizeof(T);
	out.resize(vectorSize);
	memcpy(out.data(), binaryString.c_str(), binaryString.size());
}

template<glm::length_t N, typename T>
void operator<<(Framework::Data::Variable& destination, const glm::vec<N, T>& value)
{
	switch (destination.GetFormat())
	{
	case Framework::Data::Format::readable:
	{
		std::string tmpString{};
		for (glm::length_t i = 0; i < N; i++)
		{
			tmpString.append(std::to_string(value[i]));

			if (i != N - 1)
			{
				tmpString.append(", ");
			}
		}
		destination << tmpString;
		break;
	}
	case Framework::Data::Format::binary:
	{
		destination << Framework::Data::Variable::ConvertToBinary(value);
		break;
	}
	}
}

template<glm::length_t N, typename T>
void operator>>(const Framework::Data::Variable& source, glm::vec<N, T>& out)
{
	const std::string& sourceValue = source.GetValue();
	switch (source.GetFormat())
	{
	case Framework::Data::Format::readable:
	{
		std::stringstream stringStream = std::stringstream{ sourceValue };

		for (glm::length_t i = 0; i < N; i++)
		{
			stringStream >> out[i];

			const char nextChar = static_cast<char>(stringStream.peek());
			if (nextChar == ',')
			{
				// Ignore two characters, since theyre seperated by both a comma and a space
				stringStream.ignore(2);
			}
		}
		break;
	}
	case Framework::Data::Format::binary:
	{
		Framework::Data::Variable::ConvertFromBinary(sourceValue, out);
		break;
	}
	}
}

template<typename T>
void operator<<(Framework::Data::Variable& destination, const glm::qua<T>& value)
{
	destination << glm::vec<4, T>{ value.w, value.x, value.y, value.z };
}

template<typename T>
void operator>>(const Framework::Data::Variable& source, glm::qua<T>& out)
{
	glm::vec<4, T> tmp{};
	source >> tmp;
	out = { tmp.x, tmp.y, tmp.z, tmp.w };
}