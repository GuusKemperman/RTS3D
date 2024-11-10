#pragma once
#include "DynamicBitset.h"

namespace Framework::Data
{
	template<typename T, typename FrequencyType = size_t>
	class HuffmanTree
	{
	public:
		struct DataFrequency
		{
			T mData{};
			FrequencyType mFrequency{};
		};

		HuffmanTree(const std::list<DataFrequency>& dataFrequencies);

		static void IncrementFrequency(std::list<DataFrequency>& existingFrequencies, const T& dataToIncrement, const FrequencyType incrementBy = 1);

		// Not const, this way we can cache results to speed things up.
		void Encode(DB::dynamic_bitset& toBitset, const T& data);
		const T& Decode(DB::dynamic_bitset::const_iterator& iterator) const;

		void Serialize(DB::dynamic_bitset& toBitset);
		static HuffmanTree Deserialize(DB::dynamic_bitset::const_iterator& iterator);

	private:
		std::vector<DB::bit> CalculatePathTo(const T& data) const;

		class Node
		{
		public:
			Node() = default;
			Node(const size_t index, const T& data, const FrequencyType frequency) :
				mIndex(index),
				mIsLeaf(true),
				mFrequency(frequency),
				mData(data)
			{}

			Node(const size_t index, const size_t leftChildIndex, const size_t rightChildIndex, const FrequencyType combinedFrequency) :
				mIndex(index),
				mIsLeaf(false),
				mFrequency(combinedFrequency)
			{
				mChildren[0] = leftChildIndex;
				mChildren[1] = rightChildIndex;
			}

			size_t GetLeftChildIndex() const { return GetChildIndex(0); }
			size_t GetRightChildIndex() const { return GetChildIndex(1); }

			size_t GetChildIndex(bool rightChild) const
			{
				assert(!mIsLeaf);
				return mChildren[rightChild];
			}

			const T& GetData() const
			{
				assert(mIsLeaf);
				return mData;
			}

			const size_t mIndex{};
			const bool mIsLeaf{};
			const FrequencyType mFrequency{};

		private:
			T mData{};
			size_t mChildren[2]{};
		};

		std::vector<Node> mNodes{};
		std::unordered_map<T, std::vector<DB::bit>> mPaths{};
		Node* mRoot{};
	};

	template<typename T, typename FrequencyType>
	inline HuffmanTree<T, FrequencyType>::HuffmanTree(const std::list<DataFrequency>& dataFrequencies)
	{
		std::list<size_t> open{};

		for (const DataFrequency& data : dataFrequencies)
		{
			size_t nodeIndex = mNodes.size();
			mNodes.push_back({ nodeIndex, data.mData, data.mFrequency });
			open.push_back(nodeIndex);
		}

		{
			const std::vector<Node>& nodes = mNodes;
			open.sort(
				[nodes](size_t index1, size_t index2)
				{
					return nodes[index1].mFrequency < nodes[index2].mFrequency;
				});
		}

		while (open.size() > 1)
		{
			size_t leftNodeIndex = open.front();
			open.pop_front();

			size_t rightNodeIndex = open.front();
			open.pop_front();

			size_t parentNodeIndex = mNodes.size();

			// Save from overflows
			size_t combinedFrequencyAsSizeT = static_cast<size_t>(mNodes[leftNodeIndex].mFrequency) + static_cast<size_t>(mNodes[rightNodeIndex].mFrequency);
			FrequencyType combinedFrequency = combinedFrequencyAsSizeT <= std::numeric_limits<FrequencyType>::max() ? static_cast<FrequencyType>(combinedFrequencyAsSizeT) : std::numeric_limits<FrequencyType>::max();

			Node parentNode = { parentNodeIndex, leftNodeIndex, rightNodeIndex, combinedFrequency };

			bool hasBeenInserted = false;
			// Insert it in the correct place, so we don't have to sort again
			for (auto it = open.begin(); it != open.end(); ++it)
			{
				if (mNodes[*it].mFrequency >= parentNode.mFrequency)
				{
					open.insert(it, parentNodeIndex);
					hasBeenInserted = true;
					break;
				}
			}

			if (!hasBeenInserted)
			{
				open.push_back(parentNodeIndex);
			}

			mNodes.push_back(std::move(parentNode));
		}
		mNodes.shrink_to_fit();
		mRoot = &mNodes[open.front()];

		// Now let's calculate the paths to hopefully save some time later on.
		for (const Node& node : mNodes)
		{
			if (node.mIsLeaf)
			{
				mPaths[node.GetData()] = CalculatePathTo(node.GetData());
			}
		}
	}

	template<typename T, typename FrequencyType>
	inline void HuffmanTree<T, FrequencyType>::IncrementFrequency(std::list<DataFrequency>& existingFrequencies, const T& dataToIncrement, const FrequencyType incrementBy)
	{
		auto it = std::find_if(existingFrequencies.begin(), existingFrequencies.end(),
			[dataToIncrement](const DataFrequency& dataFrequency)
			{
				return dataToIncrement == dataFrequency.mData;
			});

		if (it == existingFrequencies.end())
		{
			existingFrequencies.push_back({ dataToIncrement, incrementBy });
		}
		else
		{
			if (std::numeric_limits<FrequencyType>::max() - incrementBy < it->mFrequency)
			{
				LOGWARNING("Frequency has reached it's limit and cannot be incremented further.");
				return;
			}

			it->mFrequency += incrementBy;
		}
	}

	template<typename T, typename FrequencyType>
	inline void HuffmanTree<T, FrequencyType>::Encode(DB::dynamic_bitset& toBitset, const T& data)
	{
		auto it = mPaths.find(data);

		if (it == mPaths.end())
		{
			it = mPaths.insert({ data, CalculatePathTo(data) }).first;
		}

		for (const DB::bit bit : it->second)
		{
			toBitset.push_back(bit);
		}
	}

	template<typename T, typename FrequencyType>
	inline const T& HuffmanTree<T, FrequencyType>::Decode(DB::dynamic_bitset::const_iterator& startFrom) const
	{
		const Node* currentNode = mRoot;
		while (true)
		{
			if (currentNode->mIsLeaf)
			{
				return currentNode->GetData();
			}

			DB::bit bit = startFrom;
			++startFrom;
			currentNode = &mNodes[currentNode->GetChildIndex(bit)];
		}
	}

	template<typename T, typename FrequencyType>
	inline void HuffmanTree<T, FrequencyType>::Serialize(DB::dynamic_bitset& toBitset)
	{
		// It's cheapest and easiest to just save the frequencies and the data, since that's all the information we need to construct a huffman tree.
		size_t numOfLeaves{};

		// First do this to count the frequencies
		for (const Node& node : mNodes)
		{
			if (node.mIsLeaf)
			{
				++numOfLeaves;
			}
		}

		toBitset.push_back(numOfLeaves);

		// Then actually store the leaves
		for (const Node& node : mNodes)
		{
			if (node.mIsLeaf)
			{
				toBitset.push_back(node.mFrequency);
				toBitset.push_back(node.GetData());
			}
		}
	}

	template<typename T, typename FrequencyType>
	inline HuffmanTree<T, FrequencyType> HuffmanTree<T, FrequencyType>::Deserialize(DB::dynamic_bitset::const_iterator& iterator)
	{
		size_t numOfLeaves = DB::dynamic_bitset::extract<size_t>(iterator);

		std::list<DataFrequency> frequencies{};

		for (size_t i = 0; i < numOfLeaves; i++)
		{
			FrequencyType frequency = DB::dynamic_bitset::extract<FrequencyType>(iterator);
			T data = DB::dynamic_bitset::extract<T>(iterator);
			frequencies.push_back({ std::move(data), frequency });
		}

		return HuffmanTree<T, FrequencyType>(frequencies);
	}

	template<typename T, typename FrequencyType>
	inline std::vector<DB::bit> HuffmanTree<T, FrequencyType>::CalculatePathTo(const T& data) const
	{
		struct NodeWithParent
		{
			const Node* mNode{};
			NodeWithParent* mParent{};
		};

		std::queue<NodeWithParent> open{};
		std::vector<NodeWithParent> closed{};
		closed.reserve(mNodes.size());

		open.push({ mRoot, nullptr });

		while (!open.empty())
		{
			NodeWithParent current = open.front();
			open.pop();

			if (current.mNode->mIsLeaf)
			{
				if (current.mNode->GetData() == data)
				{
					std::vector<DB::bit> path{};

					NodeWithParent* pathCurrent = &current;
					NodeWithParent* parent = pathCurrent->mParent;

					while (parent != nullptr)
					{
						const Node* parentNode = parent->mNode;
						const size_t pathCurrentIndex = pathCurrent->mNode->mIndex;

						path.emplace_back(parentNode->GetChildIndex(1) == pathCurrentIndex);

						pathCurrent = parent;
						parent = pathCurrent->mParent;
					}
					std::reverse(path.begin(), path.end());

					return path;
				}
			}
			else
			{
				closed.push_back(std::move(current));
				NodeWithParent& closedBack = closed.back();

				open.push({ &mNodes[closedBack.mNode->GetChildIndex(0)], &closedBack });
				open.push({ &mNodes[closedBack.mNode->GetChildIndex(1)], &closedBack });
			}
		}

		assert(false
			&& "Could not find a path");
		return std::vector<DB::bit>();
	}
}