#pragma once

namespace Framework
{
	class Texture;

	class Material
	{
	public:
		Material(const std::string& materialName);
		~Material();

		void LoadFrom(const aiMaterial* aiMaterial, const std::string& texturesDirectory);
		void LoadWithoutAssimp();

		Texture* GetDiffuse() const { assert(mHasBeenLoaded); return mDiffuse.get(); }
		Texture* GetAlpha() const { assert(mHasBeenLoaded); return mAlpha.get(); }

		bool HasBeenLoaded() const { return mHasBeenLoaded; }
	private:
		std::shared_ptr<Texture> LoadTexture(const aiMaterial* aiMaterial, aiTextureType textureType, const std::string& texturesDirectory);

		std::string mName{};
		std::shared_ptr<Texture> mDiffuse{};
		std::shared_ptr<Texture> mAlpha{};
		bool mHasBeenLoaded{};
	};
}