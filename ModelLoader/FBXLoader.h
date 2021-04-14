#pragma once

#include "../Include/FBXSDK/fbxsdk.h"
#include "../Common/d3dUtil.h"

using namespace fbxsdk;

struct FBXMaterialData
{
	string materialname;
	string meshname;
	string diffuse;
	string normal;
};

class FBXLoader
{
public:
	FBXLoader() {};
	~FBXLoader() {};
	void Load(const std::string& filename);
	std::vector<std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::vector<FBXMaterialData> mMaterialData;


private:
	Matrix m_global_transform;
};
