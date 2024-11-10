#pragma once
#include "Variable.h"
#include "HuffmanTree.h"

namespace Framework::Data
{
	// A container for the variables, stored with the following syntax:
	// "mName" {
	// var1 = value
	// }
	class Scope
	{
	public:
		Scope(const Format format, const std::string& name, Scope* parent);
		Scope(std::ifstream& dataFile, const std::string& name, Scope* parent);
		Scope(DB::dynamic_bitset::const_iterator& it, const HuffmanTree<std::string, ushort>& huffmanTree, Scope* parent);
		~Scope();

		// The original name is kept
		void operator=(const Scope& other);

		std::optional<Scope*> TryGetScope(const std::string& path);
		inline Scope& GetScope(const std::string& path) { return *TryGetScope(path).value(); }
		inline const Scope& GetScope(const std::string& path) const { return *TryGetScope(path).value(); }

		std::optional<Variable> TryGetVariable(const std::string& path) const;
		std::optional<Variable*> TryGetVariablePtr(const std::string& path);
		inline Variable& GetVariable(const std::string& path) { return *TryGetVariablePtr(path).value(); }
		inline const Variable& GetVariable(const std::string& path) const { return *TryGetVariablePtr(path).value(); }

		// Dummy default value can be used to specify the const version.
		std::optional<const Scope*> TryGetScope(const std::string& path, const bool = false) const;
		// Dummy default value can be used to specify the const version.
		std::optional<const Variable*> TryGetVariablePtr(const std::string& path, const bool = false) const;

		inline const std::vector<Scope>& GetChildren() const { return mChildren; }
		inline std::vector<Scope>& GetChildren() { return mChildren; }

		inline const std::vector<Variable>& GetVariables() const { return mVariables; }
		inline std::vector<Variable>& GetVariables() { return mVariables; }

		Variable& AddVariable(const std::string& variableName);
		Scope& AddChild(const std::string& scopeName);

		void RemoveScope(const std::string& scopeName);

		inline const std::string& GetName() const { return mName; }
		inline void SetName(const std::string& name) { mName = name; }

		inline Scope* GetParent() const { return mParent; }
		std::string GetPath() const;
		
		inline void Clear() { mChildren.clear(); mVariables.clear(); }

	private:
		friend class SavedData;
		void Save(std::ofstream& toFile, uint8_t numOfIndentations = 0) const;
		void Save(DB::dynamic_bitset& toBitset, HuffmanTree<std::string, ushort>& huffmanTree) const;
		
		// Needed for constructing the Huffman tree
		void GatherFrequencies(std::list<HuffmanTree<std::string, ushort>::DataFrequency>& dataFrequencies) const;

		std::string mName{};
		Scope* mParent{};
		const Format mFormat{};


		std::vector<Scope> mChildren{};
		std::vector<Variable> mVariables{};

	};
}