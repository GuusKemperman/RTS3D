#include "precomp.h"
#include "Material.h"

#include "AssetManager.h"
#include "Texture.h"

Framework::Material::Material(const std::string& materialName) :
	mName(materialName)
{
}

Framework::Material::~Material() = default;

void Framework::Material::LoadFrom(const aiMaterial* aiMaterial, const std::string& texturesDirectory)
{
	mDiffuse = LoadTexture(aiMaterial, aiTextureType::aiTextureType_DIFFUSE, texturesDirectory);
	mHasBeenLoaded = true;
}

void Framework::Material::LoadWithoutAssimp()
{
	objl::Loader loader{};

	bool succes = loader.LoadMaterials(mName);

	if (!succes)
	{
		LOGERROR("Failed to load material " << mName);
		assert(false);
	}

	const objl::Material& material = loader.LoadedMaterials[0];

	AssetManager& assetManager = AssetManager::Inst();

	//if (!material.map_Ka.empty())
	//{
	//	mAmbient = assetManager.GetAsset<Texture>(material.map_Ka);
	//}
	if (!material.map_Kd.empty())
	{
		mDiffuse = assetManager.GetAsset<Texture>(material.map_Kd);
	}
	//if (!material.map_Ks.empty())
	//{
	//	mSpecular = assetManager.GetAsset<Texture>(material.map_Ks);
	//}
	//if (!material.map_bump.empty())
	//{
	//	mNormal = assetManager.GetAsset<Texture>(material.map_bump);
	//}
	if (!material.map_d.empty())
	{
		mAlpha = assetManager.GetAsset<Texture>(material.map_d);
	}
	mHasBeenLoaded = true;
}

std::shared_ptr<Framework::Texture> Framework::Material::LoadTexture(const aiMaterial* aiMaterial, aiTextureType textureType, const std::string& texturesDirectory)
{
	size_t numOfTextures = aiMaterial->GetTextureCount(textureType);

	if (numOfTextures == 0)
	{
		return nullptr;
	}
	assert(numOfTextures == 1);

	aiString relativePath;
	aiMaterial->GetTexture(textureType, 0, &relativePath);
	return AssetManager::Inst().GetAsset<Texture>(texturesDirectory + relativePath.C_Str());

}