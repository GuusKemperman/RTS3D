#pragma once
#include <string>

namespace Framework
{
	class Random
	{
	public:
		static inline uint Uint() { return Uint(sSeed); }
		static inline float Float() { return Float(sSeed); }
		static inline float Range(float range) { return Range(range, sSeed); }
		static inline float Range(float min, float max) { return Range(min, max, sSeed); }
		static inline std::string GenerateRandomPronounceableString(const size_t numOfSylables) { return GenerateRandomPronounceableString(numOfSylables, sSeed); }
		static inline std::string RandomAdjective(uint& seed) { return RandomAdjective(seed); }

		static inline void Seed(uint seed) { sSeed = seed; }

		static inline uint Uint(uint& seed)
		{
			seed ^= seed << 13;
			seed ^= seed >> 17;
			seed ^= seed << 5;
			return seed;
		}

		static inline uint Range(uint max)
		{
			return Uint(sSeed) % max;
		}

		static inline uint Range(uint min, uint max)
		{
			if (min == max)
			{
				return min;
			}
			assert(min < max);
			return Range(max - min) + min;
		}

		static inline float Float(uint& seed)
		{
			return static_cast<float>(Uint(seed)) * 2.3283064365387e-10f;
		}

		static inline float Range(float max, uint& seed)
		{
			return Float(seed) * max;;
		}

		static inline float Range(float min, float max, uint& seed)
		{
			assert(min <= max);
			return Range(max - min, seed) + min;
		}

		static inline std::string GenerateRandomPronounceableString(const size_t numOfSylables, uint& seed)
		{
			std::string output{};

			constexpr uint numOfVowels = 7;
			constexpr const char* vowelishSyllables[numOfVowels] = { "a", "e", "u", "i", "o", "oo", "ea" };

			constexpr uint numOfConsonants = 19;
			constexpr const char* consonantishSyllables[numOfConsonants] = { "w", "r", "t", "y", "p", "s", "d", "f", "g", "j", "k", "br", "ch", "rt", "tr", "pr", "dr", "sh", "ng" };

			bool lastSylableWasVowel = Random::Uint(seed) & 1;
			for (size_t i = 0; i < numOfSylables; i++)
			{
				const char* whatToAppendThisCycle{};
				if (lastSylableWasVowel)
				{
					whatToAppendThisCycle = consonantishSyllables[Random::Range(numOfConsonants)];
				}
				else
				{
					whatToAppendThisCycle = vowelishSyllables[Random::Range(numOfVowels)];
				}
				output.append(whatToAppendThisCycle);
				lastSylableWasVowel = !lastSylableWasVowel;
			}

			return output;
		}

		static inline std::string RandomAdjective()
		{
			constexpr uint numOfAdjectives = 12;
			constexpr const char* adjectives[numOfAdjectives] =
			{
				"great", "amazing", "tragic", "horrible", "gruesome", "deadly", "beautiful", "historic", "unnecessary", "necessary", "unforeseen", "long awaited"
			};

			return adjectives[Random::Range(numOfAdjectives)];
		}
	private:
		static inline uint sSeed = 0x12345678;
	};
}