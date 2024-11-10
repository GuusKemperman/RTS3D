#include "precomp.h"
#include "Scope.h"

void Cleanse(std::string& in)
{
	in.erase(std::remove_if(in.begin(), in.end(),
		[](unsigned char x)
		{
			return x == '\t'
				|| x == '\n'
				|| x == '\r';
		}), in.end());
}

Framework::Data::Scope::Scope(const Format format, const std::string& name, Scope* parent) :
	mName(name),
	mParent(parent),
	mFormat(format)
{
	assert(mParent == nullptr
		|| mParent->mFormat == format);
}

Framework::Data::Scope::Scope(std::ifstream& dataFile, const std::string& name, Scope* parent) :
	mName(name),
	mParent(parent),
	mFormat(Format::readable)
{
	std::string line;
	while (getline(dataFile, line))
	{
		// If line contains {, consider it the start of a new location.
		if (line.find('{') != std::string::npos)
		{
			std::string childName = line;

			Cleanse(childName);

			childName.pop_back(); // Remove '{'
			childName.pop_back(); // Remove ' '

			mChildren.emplace_back(dataFile, childName, this);
			continue;
		}

		// If line contains a =, consider it a variable.
		size_t equalSignPos = line.find('=');
		if (equalSignPos != std::string::npos)
		{
			std::string varName = line.substr(0, equalSignPos - 1);

			Cleanse(varName);

			std::string varVal = line.substr(equalSignPos + 2);
			Cleanse(varVal);

			mVariables.emplace_back(varName, varVal, mFormat);
			continue;
		}

		// If a line contains }, consider it the end of this location.
		if (line.find('}') != std::string::npos)
		{
			return;
		}
	}
}

Framework::Data::Scope::Scope(DB::dynamic_bitset::const_iterator& it, const HuffmanTree<std::string, ushort>& huffmanTree, Scope* parent) :
	mName(huffmanTree.Decode(it)),
	mParent(parent),
	mFormat(Format::binary)
{
	DB::bit thereAreVariables = it;
	++it;

	while (thereAreVariables)
	{
		DB::bit isThisTheFinalOne = it;
		++it;

		std::string varName = huffmanTree.Decode(it);
		std::string varValue = it.GetSource()->extract<std::string>(it);

		mVariables.emplace_back(std::move(varName), std::move(varValue), Format::binary);

		if (isThisTheFinalOne)
		{
			break;
		}
	}

	DB::bit thereAreChildren = it;
	++it;

	while (thereAreChildren)
	{
		DB::bit isThisTheFinalOne = it;
		++it;

		mChildren.emplace_back(it, huffmanTree, this);

		if (isThisTheFinalOne)
		{
			break;
		}
	}
}

Framework::Data::Scope::~Scope() = default;

void Framework::Data::Scope::operator=(const Scope& other)
{
	assert(other.mFormat == mFormat);
	mChildren = other.GetChildren();
	mVariables = other.GetVariables();

	for (Scope& child : mChildren)
	{
		child.mParent = this;
	}
}

std::optional<const Framework::Data::Scope*> Framework::Data::Scope::TryGetScope(const std::string& path, bool) const
{
	assert(!path.empty());

	size_t firstPeriod = path.find_first_of('.');
	bool endOfPath = firstPeriod == std::string::npos;
	std::string lookingFor = endOfPath ? path : path.substr(0, firstPeriod);
	
	auto it = find_if(mChildren.begin(), mChildren.end(),
		[lookingFor](const Scope& s)
		{
			return s.mName == lookingFor;
		});

	if (it == mChildren.end())
	{
		return {};
	}

	if (endOfPath)
	{
		return &*it;
	}
	else
	{
		std::string newPath = path;
		newPath.erase(0, firstPeriod + 1);
		return it->TryGetScope(newPath); // Continue searching with the first part of the path cut off.
	}
}

std::optional<Framework::Data::Scope*> Framework::Data::Scope::TryGetScope(const std::string& path)
{
	std::optional<const Framework::Data::Scope*> constScope = TryGetScope(path, false);

	if (!constScope.has_value())
	{
		return {};
	}
	else
	{
		// Should be fine as long as no one goes out of their way to delete the pointer.
		return { const_cast<Framework::Data::Scope*>(constScope.value()) };
	}
}

std::optional<Framework::Data::Variable> Framework::Data::Scope::TryGetVariable(const std::string& path) const
{
	const std::optional<const Variable*> ptr = TryGetVariablePtr(path);
	
	return ptr.has_value() ? std::optional<Variable>{*ptr.value()} : std::optional<Variable>{};
}

void Framework::Data::Scope::Save(std::ofstream& toFile, uint8_t numOfIndentations) const
{
	assert(mFormat == Format::readable);

	std::string indentation = "";

	// Do not add location or brackets for the very first location, the first location is just
	// used to group all the parentless locations in the .txt together and should not actually be
	// written to the file.
	if (mParent != nullptr)
	{
		for (uint8_t i = 0; i < numOfIndentations; i++)
		{
			indentation += '\t';
		}

		toFile << indentation << mName << " {" << std::endl;
		++numOfIndentations;
	}

	for (const Scope& child : mChildren)
	{
		child.Save(toFile, numOfIndentations);
	}

	std::string tmp;
	for (const Variable& var : mVariables)
	{
		var >> tmp;
		toFile << indentation << '\t' << var.GetName() << " = " << tmp << std::endl;
	}

	if (mParent != nullptr)
	{
		toFile << indentation << '}' << std::endl;
	}
}

void Framework::Data::Scope::Save(DB::dynamic_bitset& toBitset, HuffmanTree<std::string, ushort>& huffmanTree) const
{
	assert(mFormat == Format::binary);

	huffmanTree.Encode(toBitset, mName);

	// This scope has variables
	toBitset.push_back(!mVariables.empty());

	for (size_t i = 0; i < mVariables.size(); i++)
	{
		// Is this the final one
		toBitset.push_back(i == mVariables.size() - 1);

		huffmanTree.Encode(toBitset, mVariables[i].GetName());
		toBitset.push_back(mVariables[i].GetValue());
	}

	// This scope has children
	toBitset.push_back(!mChildren.empty());

	for (size_t i = 0; i < mChildren.size(); i++)
	{
		// Is this the final one
		toBitset.push_back(i == mChildren.size() - 1);

		mChildren[i].Save(toBitset, huffmanTree);
	}
}

void Framework::Data::Scope::GatherFrequencies(std::list<HuffmanTree<std::string, ushort>::DataFrequency>& dataFrequencies) const
{
	HuffmanTree<std::string, ushort>::IncrementFrequency(dataFrequencies, mName);

	for (const Variable& var : mVariables)
	{
		HuffmanTree<std::string, ushort>::IncrementFrequency(dataFrequencies, var.GetName());
	}

	for (const Scope& child : mChildren)
	{
		child.GatherFrequencies(dataFrequencies);
	}
}

std::string Framework::Data::Scope::GetPath() const
{
	if (mParent == nullptr)
	{
		return mName;
	}
	else
	{
		return mParent->GetPath() + "." + mName;
	}
}

std::optional<Framework::Data::Variable*> Framework::Data::Scope::TryGetVariablePtr(const std::string& path)
{
	std::optional<const Framework::Data::Variable*> constVar = TryGetVariablePtr(path, false);

	if (!constVar.has_value())
	{
		return {};
	}
	else
	{
		// Should be fine as long as no one goes out of their way to delete the pointer.
		return { const_cast<Framework::Data::Variable*>(constVar.value()) };
	}
}

std::optional<const Framework::Data::Variable*> Framework::Data::Scope::TryGetVariablePtr(const std::string& path, const bool) const
{
	size_t lastPeriod = path.find_last_of('.');

	if (lastPeriod == std::string::npos)
	{
		auto it = find_if(mVariables.begin(), mVariables.end(),
			[path](const Variable& v)
			{
				return v.GetName() == path;
			});

		if (it == mVariables.end())
		{
			return {};
		}
		else
		{
			return &*it;
		}
	}

	// If this location does not contain the variable, find the location that does.
	std::string varName = path.substr(lastPeriod + 1, path.size());
	std::string locationPath = path.substr(0, lastPeriod);

	std::optional<const Scope*> variableLocation = TryGetScope(locationPath);

	if (variableLocation.has_value())
	{
		return variableLocation.value()->TryGetVariablePtr(varName);
	}
	else
	{
		return {};
	}
}

Framework::Data::Variable& Framework::Data::Scope::AddVariable(const std::string& variableName)
{
	mVariables.emplace_back(variableName, "", mFormat);
	return mVariables.back();
}

Framework::Data::Scope& Framework::Data::Scope::AddChild(const std::string& name)
{
	mChildren.emplace_back(mFormat, name, this);
	return mChildren.back();
}

void Framework::Data::Scope::RemoveScope(const std::string& scopeName)
{
	mChildren.erase(std::remove_if(mChildren.begin(), mChildren.end(),
		[scopeName](const Scope& child)
		{
			return child.GetName() == scopeName;
		}), mChildren.end());
}