#include "precomp.h"
#include "SavedData.h"

#include "DynamicBitset.h"
#include "Variable.h"
#include "Scope.h"

std::unordered_map<std::string, std::weak_ptr<Framework::Data::Scope>> sInMemory{};

Framework::Data::SavedData::SavedData(const std::string& filePath, const std::string& scopePath) :
	mFilePath(sDataRoot + filePath),
	mScopePath(scopePath),
	mFormat(DetermineFormat(filePath))
{
	auto it = sInMemory.find(mFilePath);

	if (it == sInMemory.end()
		|| (*it).second.expired())
	{
		Load();
		sInMemory[mFilePath] = mGlobalScope;
	}
	else
	{
		mGlobalScope = static_cast<std::shared_ptr<Scope>>(sInMemory[mFilePath]);
	}
}

Framework::Data::SavedData::~SavedData() = default;

Framework::Data::Scope& Framework::Data::SavedData::GetScope() const
{
	if (mScopePath.empty())
	{
		return *mGlobalScope.get();
	}
	else
	{
		return mGlobalScope->GetScope(mScopePath);
	}
}

Framework::Data::Scope& Framework::Data::SavedData::GetScope(const std::string& path) const
{
	return GetScope().GetScope(path);
}

Framework::Data::Variable Framework::Data::SavedData::GetVariable(const std::string& path) const
{
	return GetScope().GetVariable(path);
}

std::optional<Framework::Data::Scope*> Framework::Data::SavedData::TryGetScope(const std::string& path) const
{
	return GetScope().TryGetScope(path);
}

std::optional<Framework::Data::Variable> Framework::Data::SavedData::TryGetVariable(const std::string& path) const
{
	return GetScope().TryGetVariable(path);
}

void Framework::Data::SavedData::LoadFromReadableFormat()
{
	assert(mFormat == Format::readable);

	assert(mGlobalScope == nullptr
		&& "Has already been loaded in");

	std::ifstream dataFile(mFilePath);
	assert(dataFile.is_open());

	mGlobalScope = std::make_shared<Scope>(dataFile, "GlobalScope", nullptr);

	dataFile.close();
}

void Framework::Data::SavedData::LoadFromBinaryFormat()
{
	assert(mGlobalScope == nullptr
		&& "Has already been loaded in");

	assert(mFormat == Format::binary);

	const DB::dynamic_bitset bitset = DB::dynamic_bitset::deserialize(mFilePath);

	if (bitset.empty())
	{
		mGlobalScope = std::make_shared<Scope>(Format::binary, "GlobalScope", nullptr);
		return;
	}

	DB::dynamic_bitset::const_iterator it = bitset.begin();
	HuffmanTree<std::string, ushort> tree = HuffmanTree<std::string, ushort>::Deserialize(it);
	mGlobalScope = std::make_shared<Scope>(it, tree, nullptr);
}

Framework::Data::Format Framework::Data::SavedData::DetermineFormat(const std::string& filePath)
{
	const std::string extension = filePath.substr(filePath.size() - 3);

	if (extension == "txt")
	{
		return Format::readable;
	}
	else if (extension == "dat")
	{
		return Format::binary;
	}
	else
	{
		assert(false
			&& "Invalid extension");
		return {};
	}
}

void Framework::Data::SavedData::MakeEmpty(const std::string& filePath)
{
	std::string root = { sDataRoot };
	std::string fullPath = filePath.substr(0, root.size()) == root ? filePath : root + filePath;

	std::ofstream outFile(fullPath);

	assert(outFile.is_open()
		&& "Could not create/open file. Make sure that the folder you're trying to create a file in exists. If the file already exists, it may be read only.");
	outFile.clear();
	outFile.close();

	if (auto it = sInMemory.find(fullPath); it != sInMemory.end()
		&& !it->second.expired())
	{
		static_cast<std::shared_ptr<Scope>>(it->second)->Clear();
	}
}

void Framework::Data::SavedData::Delete(const std::string& filePath)
{
	std::string root = { sDataRoot };
	std::string fullPath = filePath.substr(0, root.size()) == root ? filePath : root + filePath;

	if (std::remove(fullPath.c_str()))
	{
		LOGMESSAGE("Could not delete " << filePath);
		assert(false);
	}

	if (auto it = sInMemory.find(fullPath); it != sInMemory.end()
		&& !it->second.expired())
	{
		static_cast<std::shared_ptr<Scope>>(it->second)->Clear();
	}
}

void Framework::Data::SavedData::SaveInReadableFormat()
{
	assert(mFormat == Format::readable);

	std::ofstream dataFile(mFilePath);
	if (!dataFile.is_open())
	{
		LOGWARNING("Could not save to " << mFilePath << ", might be read only.");
		return;
	}

	mGlobalScope->Save(dataFile);

	dataFile.close();
}

void Framework::Data::SavedData::SaveInBinaryFormat()
{
	assert(mFormat == Format::binary);

	std::list<HuffmanTree<std::string, ushort>::DataFrequency> frequencies{};
	mGlobalScope->GatherFrequencies(frequencies);
	HuffmanTree<std::string, ushort> tree = { frequencies };

	// First save everything to this bitset
	DB::dynamic_bitset bitset{};

	tree.Serialize(bitset);
	mGlobalScope->Save(bitset, tree);

	bitset.serialize(mFilePath);
}
