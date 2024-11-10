#include "precomp.h"
#include "AnimatedMesh.h"

#include "MyShader.h"
#include "Animation.h"
#include "Animator.h"
#include "Material.h"
#include "Camera.h"
#include "AssetManager.h"

Framework::AnimatedMesh::AnimatedMesh(const std::string& filePath) :
	Mesh()
{
	glGenBuffers(1, &mBoneIdsBuffer);
	glGenBuffers(1, &mBoneWeightsBuffer);

    SetShader(AssetManager::Inst().GetAsset<MyShader>("shaders/animated.vert,shaders/standard.frag"));

	Assimp::Importer importer{};
	LoadFrom(filePath, importer.ReadFile(filePath, sReadFileFlags));
}

Framework::AnimatedMesh::~AnimatedMesh()
{
	glDeleteBuffers(1, &mBoneIdsBuffer);
	glDeleteBuffers(1, &mBoneWeightsBuffer);
}

void Framework::AnimatedMesh::Draw(const Camera& camera, const glm::mat4 modelMatrix, const Animator* animator) const
{
    const std::vector<glm::mat4>& transforms = animator->GetFinalBoneMatrices();

    const  MyShader* shader = GetShader();
    shader->Bind();

    for (size_t i = 1; i < transforms.size(); i++)
    {
        shader->SetInputMatrix(std::string{ "finalBonesMatrices[" + std::to_string(i) + "]" }.c_str(), transforms[i]);
    }

    shader->SetInputTexture(0, "sampler", *GetMaterial()->GetDiffuse());
    shader->SetInputMatrix("viewProjection", camera.GetViewProjection());
    shader->SetInputMatrix("model", modelMatrix);

    SimpleDraw();

    shader->Unbind();
}

void Framework::AnimatedMesh::SetBoneIds(std::vector<glm::ivec4> ids)
{
	mBoneIds = std::move(ids);

	glBindVertexArray(GetVertexArrayObject());
	glBindBuffer(GL_ARRAY_BUFFER, mBoneIdsBuffer);

	glBufferData(GL_ARRAY_BUFFER, mBoneIds.size() * sizeof(glm::ivec4), &mBoneIds[0], GL_STATIC_DRAW);
	glVertexAttribIPointer(3, sMaxNumOfBonesPerVertex, GL_INT, sizeof(glm::ivec4), (void*)0);
	glEnableVertexAttribArray(3);

	glBindVertexArray(0);
    CheckGL();
}

void Framework::AnimatedMesh::SetBoneWeights(std::vector<glm::vec4> weights)
{
	mBoneWeights = std::move(weights);

	glBindVertexArray(GetVertexArrayObject());
	glBindBuffer(GL_ARRAY_BUFFER, mBoneWeightsBuffer);

	glBufferData(GL_ARRAY_BUFFER, mBoneWeights.size() * sizeof(glm::vec4), &mBoneWeights[0], GL_STATIC_DRAW);
	glVertexAttribPointer(4, sMaxNumOfBonesPerVertex, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
	glEnableVertexAttribArray(4);

	glBindVertexArray(0);
    CheckGL();
}

void Framework::AnimatedMesh::LoadFrom(const std::string& filePath, const aiScene* scene)
{
	Mesh::LoadFrom(filePath, scene);

    size_t numOfVertices = GetNumOfVertices();

    std::vector<glm::ivec4> tmpIds(numOfVertices);
    
    for (glm::ivec4& boneIndices : tmpIds)
    {
        for (glm::length_t i = 0; i < boneIndices.length(); i++)
        {
            boneIndices[i] = sNullBoneIndex;
        }
    }

    std::vector<glm::vec4> tmpWeights(numOfVertices);

    assert(scene != nullptr
        && scene->mNumMeshes == 1);
    const aiMesh* mesh = scene->mMeshes[0];

    for (uint aiBoneIndex = 0; aiBoneIndex < mesh->mNumBones; aiBoneIndex++)
    {
        const aiBone* aiBone = mesh->mBones[aiBoneIndex];
        const std::string boneName = aiBone->mName.C_Str();
        auto boneIt = mBoneLookUp.find(boneName);

        if (boneIt == mBoneLookUp.end())
        {
            BoneData newBoneInfo{};
            newBoneInfo.mIndex = static_cast<int>(mBoneLookUp.size()) + 1;
            newBoneInfo.mOffset = Math::ToGLM(aiBone->mOffsetMatrix);
            boneIt = mBoneLookUp.insert({ boneName, newBoneInfo }).first;
        }

        aiVertexWeight* weights = aiBone->mWeights;
        uint numWeights = aiBone->mNumWeights;

        for (uint weightIndex = 0; weightIndex < numWeights; ++weightIndex)
        {
            uint vertexId = weights[weightIndex].mVertexId;
            float weight = weights[weightIndex].mWeight;
            assert(vertexId <= GetNumOfVertices());

            glm::ivec4& boneIndex = tmpIds[vertexId];
            glm::vec4& boneWeights = tmpWeights[vertexId];

            for (uint i = 0; i < sMaxNumOfBonesPerVertex; i++)
            {
                if (boneIndex[i] == sNullBoneIndex)
                {
                    boneIndex[i] = boneIt->second.mIndex;
                    boneWeights[i] = weight;
                    break;
                }
            }
        }
    }

    SetBoneIds(std::move(tmpIds));
    SetBoneWeights(std::move(tmpWeights));

    for (uint aiAnimIndex = 0; aiAnimIndex < scene->mNumAnimations; aiAnimIndex++)
    {
        const aiAnimation* aiAnim = scene->mAnimations[aiAnimIndex];

        std::unique_ptr<Animation> animation = std::make_unique<Animation>(aiAnim, scene, *this);

        const std::string name = aiAnim->mName.C_Str();
        assert(mAnimationLookUp.find(name) == mAnimationLookUp.end());

        mAnimationLookUp[name] = animation.get();
        mAnimations.push_back(std::move(animation));
    }
}