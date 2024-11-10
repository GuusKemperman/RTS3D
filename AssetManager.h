#pragma once
#include "Singleton.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <typeindex>
#include <functional>

namespace Framework
{
	class AssetManager :
		public Singleton<AssetManager>
	{
	public:
		inline void Clear()
		{
			mAssets.clear();
		}
		
		template<typename T>
		std::shared_ptr<T> GetAsset(const std::string& filePath);

		// Not necesarry, but we're assuming now that everything has been loaded in.
		// If we're trying to load something else in, this would probably mean that we 
		// specified the wrong name/type/arguments and an additional asset will be loaded
		// instead of being shared.
		void LockFurtherLoading() { mLoadingAllowed = false; }

	private:
		template<typename T>
		size_t GenerateKey(const std::string& filePath);

		std::unordered_map<size_t, std::shared_ptr<void>> mAssets{};
		bool mLoadingAllowed = true;
		std::hash<std::string> mHasher{};
	};

	template<typename T>
	inline std::shared_ptr<T> AssetManager::GetAsset(const std::string& filePath)
	{
		size_t key = GenerateKey<T>(filePath);
		std::unordered_map<size_t, std::shared_ptr<void>>::const_iterator it = mAssets.find(key);

		if (it == mAssets.end())
		{
			LOGMESSAGE("Loading asset " << filePath);
			assert(mLoadingAllowed);
			std::shared_ptr<void> asset = std::static_pointer_cast<void>(std::make_shared<T>(sAssetsRoot + filePath));
			std::pair assetPair = { key, std::move(asset) };
			it = mAssets.insert(assetPair).first;
		}

		assert(it != mAssets.end());
		std::shared_ptr<T> asset = std::static_pointer_cast<T>(it->second);
		return asset;
	}

	template<typename T>
	inline size_t AssetManager::GenerateKey(const std::string& filePath)
	{
		std::type_index typeIndex = typeid(T);
		size_t typeHash = typeIndex.hash_code();
		size_t type = (typeHash ^ mHasher(filePath)) + 0x9e3779b9 + (typeHash << 6) + (typeHash >> 2);
		return type;
	}
}