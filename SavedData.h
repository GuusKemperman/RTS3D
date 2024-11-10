#pragma once
#include "Scope.h"
#include "Variable.h"

namespace Framework::Data
{
	// A class intended to help you save and load settings stored in a human-readable way, sectioned of like namespaces. 
	// Loading the Data happens the first time an instance is created, after which the Data will be stored in memory.
	// Any changes to that Data will not be saved unless the Save() function is called.
	class SavedData
	{
	public:
		SavedData(const std::string& filePath, const std::string& scopePath = "");
		~SavedData();

		SavedData(const SavedData&) = delete;

		// Saves the global scope to file. (Not just the this instance's scope.
		inline void Save()
		{
			switch (mFormat)
			{
			case Format::binary:
				SaveInBinaryFormat();
				return;
			case Format::readable:
				SaveInReadableFormat();
				return;
			}
		}

		// Loads ALL the Data from file, and updates each instance.
		//static void ReloadAll();

		Scope& GetScope() const;
		Scope& GetScope(const std::string& path) const;
		Variable GetVariable(const std::string& path) const;

		std::optional<Scope*> TryGetScope(const std::string& path) const;
		std::optional<Variable> TryGetVariable(const std::string& path) const;

		inline void Clear() { MakeEmpty(mFilePath); }

		// Will create a new file if the given filepath does not exist yet, otherwise clears it (and the data in memory as well).
		static void MakeEmpty(const std::string& filePath);
		static void Delete(const std::string& filePath);

	private:
		void SaveInReadableFormat();
		void SaveInBinaryFormat();

		inline void Load()
		{
			switch (mFormat)
			{
			case Format::binary:
				LoadFromBinaryFormat();
				return;
			case Format::readable:
				LoadFromReadableFormat();
				return;
			}
		}
		void LoadFromReadableFormat();
		void LoadFromBinaryFormat();

		static Format DetermineFormat(const std::string& filepath);

		std::shared_ptr<Scope> mGlobalScope{};

		const std::string mFilePath{};

		// Every instance of playerdata will only be able to find and access the Data on their focused location when looking for variables.
		// An inputmanager for example could be focused on "playerdata/input", and will only be able to access the variables of that location and it's children.
		const std::string mScopePath{};

		const Format mFormat{};
	};
}