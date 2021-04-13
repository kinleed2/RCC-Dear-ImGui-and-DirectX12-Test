#pragma once
#include "../Common/d3dUtil.h"

struct PMDHeader
{
	char			m_magic[3];
	float			m_version;
	char			m_modelName[20];
	char			m_comment[256];
};

struct PMDVertex
{
	Vector3		m_position;
	Vector3		m_normal;
	Vector2		m_uv;
	uint16_t	m_bone[2];
	uint8_t		m_boneWeight;
	uint8_t		m_edge;
};

struct PMDFace
{
	uint16_t  m_vertices[3];
};

#pragma pack(1)
struct PMDMaterial
{
	Vector3			m_diffuse;
	float			m_alpha;
	float			m_specularPower;
	Vector3			m_specular;
	Vector3			m_ambient;
	uint8_t			m_toonIndex;
	uint8_t			m_edgeFlag;
	uint32_t		m_faceVertexCount;
	char			m_textureName[20];
};
#pragma pack()

#pragma pack(1)
struct PMDBone
{
	char			m_boneName[20];
	uint16_t		m_parent;
	uint16_t		m_tail;
	uint8_t			m_boneType;
	uint16_t		m_ikParent;
	Vector3			m_position;

};
#pragma pack()

struct PMDIk
{
	using ChainList = std::vector<uint16_t>;

	uint16_t	m_ikNode;
	uint16_t	m_ikTarget;
	uint8_t		m_numChain;
	uint16_t	m_numIteration;
	float		m_rotateLimit;
	ChainList	m_chanins;
};

struct PMDMorph
{
	struct Vertex
	{
		uint32_t	m_vertexIndex;
		Vector3		m_position;
	};
	using VertexList = std::vector<Vertex>;

	enum MorphType : uint8_t
	{
		Base,
		Eyebrow,
		Eye,
		Rip,
		Other
	};

	char			m_morphName[20];
	MorphType		m_morphType;
	VertexList		m_vertices;
	char			m_englishShapeNameExt[20];

};

struct PMDMorphDisplayList
{
	typedef std::vector<uint16_t> DisplayList;

	DisplayList	m_displayList;
};

struct PMDBoneDisplayList
{
	typedef std::vector<uint16_t> DisplayList;

	char			m_name[50];
	DisplayList		m_displayList;
	char			m_englishNameExt[50];
};

enum class PMDRigidBodyShape : uint8_t
{
	Sphere,		
	Box,		
	Capsule,	
};

enum class PMDRigidBodyOperation : uint8_t
{
	Static,				
	Dynamic,			
	DynamicAdjustBone	
};

struct PMDRigidBodyExt
{
	char			m_rigidBodyName[20];
	uint16_t		m_boneIndex;
	uint8_t			m_groupIndex;
	uint16_t		m_groupTarget;		
	PMDRigidBodyShape	m_shapeType;
	float			m_shapeWidth;
	float			m_shapeHeight;
	float			m_shapeDepth;
	Vector3			m_pos;
	Vector3			m_rot;
	float			m_rigidBodyWeight;
	float			m_rigidBodyPosDimmer;
	float			m_rigidBodyRotDimmer;
	float			m_rigidBodyRecoil;
	float			m_rigidBodyFriction;
	PMDRigidBodyOperation	m_rigidBodyType;
};

struct PMDJointExt
{
	enum { NumJointName = 20 };

	char			m_jointName[20];
	uint32_t		m_rigidBodyA;
	uint32_t		m_rigidBodyB;
	Vector3			m_jointPos;
	Vector3			m_jointRot;
	Vector3			m_constrainPos1;
	Vector3			m_constrainPos2;
	Vector3			m_constrainRot1;
	Vector3			m_constrainRot2;
	Vector3			m_springPos;
	Vector3			m_springRot;
};

struct PMDFile
{
	PMDHeader						m_header;
	std::vector<PMDVertex>			m_vertices;
	std::vector<PMDFace>			m_faces;
	std::vector<PMDMaterial>		m_materials;
	std::vector<PMDBone>			m_bones;
	std::vector<PMDIk>				m_iks;
	std::vector<PMDMorph>			m_morphs;
	PMDMorphDisplayList				m_morphDisplayList;
	std::vector<PMDBoneDisplayList>	m_boneDisplayLists;
	std::array<unsigned char, 10>	m_toonTextureNames[100];
	std::vector<PMDRigidBodyExt>	m_rigidBodies;
	std::vector<PMDJointExt>		m_joints;
};

struct PMDMaterialData
{
	string materialname;
	string meshname;
	string diffuse;
	string normal;
};

class PMDLoader
{
public:
	PMDLoader() {};
	~PMDLoader() {};
	void Load(const std::string& filename);
	std::vector<std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::vector<PMDMaterialData> mMaterialData;

private:
	
	Matrix m_global_transform;
};
