#pragma once
//#include "GameFramework/Actor.h"
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <direct.h>
//#include "AllowWindowsPlatformTypes.h" 
#include <d3dx9.h>
//#include "HideWindowsPlatformTypes.h"
//#include "ue

#include <fbxsdk.h>

#include "../Common/Common.h"

struct FVector
{
	float X;

	float Y;

	float Z;
};

struct FVector2D
{
	float X;

	float Y;

};
class TueXFile
{
private:
	std::wstring XFileName;
public:
	TueXFile();
	~TueXFile();
	bool LoadFromFile(
		const std::string& FileName,
		HWND hWnd, std::string ExportPath
	);
};

struct TueVertex
{
	FVector position;
	FVector normal;
	unsigned long  color;
	FVector2D uv;
};


struct TueTriVertex
{
	int subsetIndex;
	TueVertex v1;
	TueVertex v2;
	TueVertex v3;
};


struct MyTriVertex
{
	int subsetIndex;
	TueVertex v1;
	TueVertex v2;
	TueVertex v3;
	D3DCOLORVALUE diffuse;
	D3DCOLORVALUE ambient;
	D3DCOLORVALUE emissive;
	D3DCOLORVALUE specular;

	int id;
};


struct MyVertex
{
	int subsetIndex;
	TueVertex v;
	D3DCOLORVALUE diffuse;
	D3DCOLORVALUE ambient;
	D3DCOLORVALUE emissive;
	D3DCOLORVALUE specular;
};




struct Material
{
	D3DCOLORVALUE diffuse;
	D3DCOLORVALUE ambient;
	D3DCOLORVALUE emissive;
	D3DCOLORVALUE specular;
};


struct FLinearColor
{
	float R;

	float G;

	float B;

	float A;

};
struct ThreePoints
{
	FVector p1;
	//FVector2D uv1;
	FVector p2;
	//FLinearColor c1;
	FVector p3;
};

struct TueMeshTri
{
	FVector p1;
	ThreePoints points;
	//FVector2D uv1;
	FVector n1;
	//FLinearColor c1;
	FVector c1;



	float CompareValue;

	D3DCOLORVALUE diffuse;
	D3DCOLORVALUE ambient;
	D3DCOLORVALUE emissive;
	D3DCOLORVALUE specular;

	int id;

	bool operator<(const TueMeshTri& Right)const
	{

		return CompareValue<Right.CompareValue;
	}

};


void CreateMaterials(FbxScene* pScene, FbxMesh* pMesh, int face_num);

FbxSurfacePhong* CreateMaterial(FbxScene* pScene, FbxDouble3 pColor);
FbxSurfacePhong* CreateMaterial2(FbxScene* pScene, FbxDouble3 pColor);

bool getXInfo(const std::string& lxoFilename, HWND hwnd, std::string ExportPath);
bool CreateScene(FbxScene* pScene, char* pSampleFileName);



FbxNode* CreatePyramidWithMaterials(FbxScene* pScene, const char* pName, std::vector<TueMeshTri> PtAry);

struct  FMeshInfo
{
	std::string Name;
	std::vector<MyVertex>VecPtAry;
	std::vector<TueMeshTri>UnequeVecPtAry;
	
	std::set<TueMeshTri>SetPtAry;
	std::map<TueMeshTri, int> MapPtAry;
};
using FMeshInfoMap = std::map<int, FMeshInfo>;

void CreateTexture(const FMeshInfoMap FMeshInfoMap, FbxScene* pScene, FbxMesh* pMesh);

void CreateCube();
