#pragma once
#include <string>
#include <vector>
#include <sstream>

namespace Framework
{
	class StringFunctions
	{
	public:
		static inline std::string HexToBinary(const std::string& source)
		{
			static int nibbles[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15 };
			std::string retval{};
			for (std::string::const_iterator it = source.begin(); it < source.end(); it += 2) {
				char v = 0;
				if (std::isxdigit(*it))
					v = static_cast<char>(nibbles[std::toupper(*it) - '0'] << 4);
				if (it + 1 < source.end() && std::isxdigit(*(it + 1)))
					v += static_cast<char>(nibbles[std::toupper(*(it + 1)) - '0']);
				retval.push_back(v);
			}
			return retval;
		}

		static inline std::string BinaryToHex(const std::string& binary)
		{
			static char syms[] = "0123456789ABCDEF";
			std::stringstream ss{};

			for (const char& ch : binary)
			{
				ss << syms[((ch >> 4) & 0xf)] << syms[ch & 0xf];
			}

			return ss.str();
		}

		static inline std::vector<std::string> SplitString(const std::string& stringToSplit, const std::string& splitOn)
		{
			std::string copy = stringToSplit;
			size_t pos = 0;
			std::vector<std::string> returnValue{};

			while ((pos = copy.find(splitOn)) != std::string::npos)
			{
				returnValue.emplace_back(copy.substr(0, pos));
				copy.erase(0, pos + splitOn.length());
			}
			returnValue.push_back(std::move(copy));
			return returnValue;
		}

		static inline std::string ReadFile(const char* filePath)
		{
			std::string content;
			std::ifstream fileStream(filePath, std::ios::in);

			if (!fileStream.is_open()) {
				std::cerr << "Could not read file " << filePath << ". File does not exist." << std::endl;
				return "";
			}

			std::string line = "";
			while (!fileStream.eof()) {
				std::getline(fileStream, line);
				content.append(line + "\n");
			}

			fileStream.close();
			return content;
		}

		static inline void RemoveLinuxLineEndings(std::string& string)
		{
			string.erase(std::remove_if(string.begin(), string.end(),
				[](unsigned char x)
				{
					return x == '\n'
					|| x == '\r';
				}), string.end());
		}
	};
}