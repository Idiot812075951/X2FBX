#include "ueXReader.h"

#include <map>
#include <list>
#include <set>
#include<iostream>

#include "D3DX9Mesh.h"
#include <algorithm>

#include <fbxsdk.h>
#include "Common.h"
#include "MyKFbxMesh.h"
#include "Common.cxx"


FbxManager* gSdkManager = NULL;
#ifdef IOS_REF
#undef  IOS_REF
#define IOS_REF (*(gSdkManager->GetIOSettings()))
#endif


static const char* gDiffuseElementName = "DiffuseUV";

static const char* gAmbientElementName = "AmbientUV";

static const char* gEmissiveElementName = "EmissiveUV";

char* CutPostfix(char* Target)
{
	int len = strlen(Target);
	int DotPosition = 0;
	for (int i = 0; i < len; i++)
	{
		if (Target[i] == '.')
			DotPosition = i;
	}
	if (DotPosition > 0)
	{
		Target[DotPosition] = 0;
	}
	return Target;
}


bool getXInfo(const std::string& lxoFilename, HWND hwnd, std::string ExportPath)
{

	//D3DCOLOR myColor = D3DCOLOR_XRGB(0, 0, 255);//绿色
	//D3DCOLOR myColor = D3DCOLOR_XRGB(0, 0, 255);//绿色

	IDirect3D9* g_d3d;
	IDirect3DDevice9* g_device;
	ID3DXMesh* g_mesh;
	ID3DXBuffer* material_buffer;

	unsigned long g_num_materials;
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));

	d3dpp.Windowed = true;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	d3dpp.EnableAutoDepthStencil = true;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

	g_d3d = Direct3DCreate9(D3D_SDK_VERSION);
	if (g_d3d == NULL)
	{
		return false;
	}

	g_d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &g_device);
	if (FAILED(D3DXLoadMeshFromXA(lxoFilename.data(), D3DXMESH_MANAGED, g_device, NULL, &material_buffer, NULL, &g_num_materials, &g_mesh)))
	{
		return false;
	}

	ID3DXMesh* g_mesh_temp;
	unsigned long oldFVF;
	PDWORD rgdwAdjacency;
	oldFVF = g_mesh->GetFVF();
	if (oldFVF != (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1))
	{
		g_mesh->CloneMeshFVF(g_mesh->GetOptions(), D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1, g_device, &g_mesh_temp);
		g_mesh->Release();
		g_mesh = g_mesh_temp;
	}
	if ((oldFVF && D3DFVF_NORMAL) == 0)
		D3DXComputeNormals(g_mesh, NULL);
	rgdwAdjacency = (PDWORD)malloc(sizeof(unsigned long) * g_mesh->GetNumFaces() * 3);
	g_mesh->GenerateAdjacency(1E-6, rgdwAdjacency);
	g_mesh->OptimizeInplace(D3DXMESHOPT_VERTEXCACHE, rgdwAdjacency, NULL, NULL, NULL);
	free(rgdwAdjacency);

	auto NumFaces = g_mesh->GetNumFaces();
	auto NumVertices = g_mesh->GetNumVertices();

	IDirect3DIndexBuffer9* ppIB;
	g_mesh->GetIndexBuffer(&ppIB);

	D3DINDEXBUFFER_DESC pDesc;
	ppIB->GetDesc(&pDesc);
	int step = pDesc.Size / (NumFaces * 3);

	PDWORD AttributeBuffer = 0;
	g_mesh->LockAttributeBuffer(0, &AttributeBuffer);

	PWORD IndexBufferSize2 = nullptr;
	PDWORD IndexBufferSize4 = nullptr;
	if (step == 2)
	{
		g_mesh->LockIndexBuffer(0, (void**)&IndexBufferSize2);
	}
	else
	{
		g_mesh->LockIndexBuffer(0, (void**)&IndexBufferSize4);
	}
	TueVertex* pv = nullptr;
	g_mesh->LockVertexBuffer(0, (void**)&pv);

	std::vector<TueVertex> VerAry;
	for (int i = 0; i < NumVertices; i++)
	{
		VerAry.emplace_back(*pv);
		pv++;
	}

	std::map<int, std::pair<std::string, std::vector<long long>>> SubsetIndexMap;
	if (step == 2)
	{
		for (int i = 0; i < NumFaces; i++)
		{
			auto& SebsetIndexRef = SubsetIndexMap[*(AttributeBuffer + i)];
			for (int j = 0; j < 3; j++)
			{
				SebsetIndexRef.second.emplace_back(*(IndexBufferSize2 + (i * 3 + j)));
			}
			//材质索引
		}
	}
	else
	{
		for (int i = 0; i < NumFaces; i++)
		{
			auto& SebsetIndexRef = SubsetIndexMap[*(AttributeBuffer + i)];
			for (int j = 0; j < 3; j++)
			{
				SebsetIndexRef.second.emplace_back(*(IndexBufferSize4 + (i * 3 + j)));
			}
			//材质索引
		}
	}

	g_mesh->UnlockAttributeBuffer();
	g_mesh->UnlockIndexBuffer();
	g_mesh->UnlockVertexBuffer();

	D3DXMATERIAL* MaterialsPtr = (D3DXMATERIAL*)(material_buffer)->GetBufferPointer();
	for (unsigned int j = 0; j < g_num_materials; j++)
	{
		if (MaterialsPtr[j].pTextureFilename)
		{
			SubsetIndexMap[j].first = MaterialsPtr[j].pTextureFilename;
		}
	}

	FbxManager* pManager = NULL;
	FbxScene* pScene = NULL;
	InitializeSdkObjects(pManager, pScene);
	char* cc = NULL;
	bool PResult = CreateScene(pScene, cc);
	const char* pName = "test";
	FbxMesh* lMesh = FbxMesh::Create(pScene, pName);

	lMesh->InitControlPoints(VerAry.size());
	FbxVector4* lControlPoints = lMesh->GetControlPoints();

	for (int i = 0; i < VerAry.size(); i++)
	{
		lControlPoints[i].mData[0] = VerAry[i].position.Z / 10;
		lControlPoints[i].mData[1] = VerAry[i].position.Y / 10;
		lControlPoints[i].mData[2] = VerAry[i].position.X / 10;
	}

	FbxGeometryElementNormal* lGeometryElementNormal = lMesh->CreateElementNormal();
	lGeometryElementNormal->SetMappingMode(FbxGeometryElement::eByControlPoint);
	lGeometryElementNormal->SetReferenceMode(FbxGeometryElement::eIndexToDirect);
	lGeometryElementNormal->GetDirectArray().Add(FbxVector4(
		1,
		0,
		0
	));

	// 以前的normal位置
	for (auto Iter : SubsetIndexMap)
	{
		for (int i = 0; i < Iter.second.second.size() / 3; i++)
		{
			lMesh->BeginPolygon(Iter.first);
			for (int j = 0; j < 3; j++)
			{
				lGeometryElementNormal->GetIndexArray().Add(0);
				lMesh->AddPolygon(Iter.second.second[i * 3 + j]);
				//				lUVAmbientElement->GetIndexArray().SetAt(i * 3 + j, i * 3 + j);
			}
			lMesh->EndPolygon();
		}
	}

	FbxNode* lNode = FbxNode::Create(pScene, pName);
	lNode->SetNodeAttribute(lMesh);
	pScene->GetRootNode()->AddChild(lNode);

	std::cout << ExportPath.data();
	SaveScene(pManager, pScene, ExportPath.data());

	return true;
}

TueXFile::TueXFile()
{
}

TueXFile::~TueXFile()
{

}

bool TueXFile::LoadFromFile(
	const std::string& FileName,
	HWND hWnd, std::string ExportPath
)
{
	if (!getXInfo(FileName.data(), hWnd, ExportPath))
	{
		return false;
	}

	return true;
}

bool CreateScene(FbxScene* pScene, char* pSampleFileName)
{
	return true;
}
//没有使用
void CreateTexture(const FMeshInfoMap MeshInfoMap, FbxScene* pScene, FbxMesh* pMesh)
{
	FbxString lMaterialName = "material";
	FbxString lShadingName = "Phong";

	FbxDouble3 lBlack(1.0, 1.0, 1.0);
	FbxDouble3 lRed(1.0, 1.0, 1.0);
	FbxDouble3 lDiffuseColor(1.0, 1.0, 1.0);

	for (auto Iter : MeshInfoMap)
	{
		//get the node of mesh, add material for it.

		auto lMaterial = FbxSurfacePhong::Create(pScene, lMaterialName.Buffer());

		lMaterial->Emissive.Set(lBlack);
		lMaterial->Ambient.Set(lRed);
		lMaterial->AmbientFactor.Set(1.);
		// Add texture for diffuse channel
		lMaterial->Diffuse.Set(lDiffuseColor);
		lMaterial->DiffuseFactor.Set(1.);
		lMaterial->TransparencyFactor.Set(0.4);
		lMaterial->ShadingModel.Set(lShadingName);
		lMaterial->Shininess.Set(0.5);
		lMaterial->Specular.Set(lBlack);
		lMaterial->SpecularFactor.Set(0.3);

		FbxFileTexture* lTexture = FbxFileTexture::Create(pScene, "Diffuse Texture");
		if (Iter.second.Name.size() > 0)
		{
			bool isTexture = lTexture->SetFileName(Iter.second.Name.data());
		}
		else
		{
			bool isTexture = lTexture->SetFileName("");
		}

		lTexture->SetTextureUse(FbxTexture::eStandard);
		lTexture->SetMappingType(FbxTexture::eUV);
		lTexture->SetMaterialUse(FbxFileTexture::eModelMaterial);
		lTexture->SetSwapUV(false);
		lTexture->SetTranslation(0.0, 0.0);
		lTexture->SetScale(1.0, 1.0);
		lTexture->SetRotation(0.0, 0.0);
		lTexture->UVSet.Set(FbxString(gDiffuseElementName)); // Connect texture to the proper UV


		if (lMaterial)
		{
			lMaterial->Diffuse.ConnectSrcObject(lTexture);
		}
		FbxNode* lNode = pMesh->GetNode();
		if (lNode)
		{
			lNode->AddMaterial(lMaterial);
		}
	}
}

void CreateCube()
{



	FbxManager* pManager = NULL;
	FbxScene* pScene = NULL;
	InitializeSdkObjects(pManager, pScene);
	char* cc = NULL;
	bool PResult = CreateScene(pScene, cc);
	const char* pName = "test";
	FbxMesh* lMesh = FbxMesh::Create(pScene, pName);

	int PtCount = 0;

	//FbxVector4 lNormalXPos(1, 0, 0);
	//FbxVector4 lNormalXNeg(-1, 0, 0);
	//FbxVector4 lNormalYPos(0, 1, 0);
	//FbxVector4 lNormalYNeg(0, -1, 0);
	//FbxVector4 lNormalZPos(0, 0, 1);
	//FbxVector4 lNormalZNeg(0, 0, -1);

	FbxVector4 lNormalXPos(1, 0, 0);
	FbxVector4 lNormalXNeg(1, 0, 0);
	FbxVector4 lNormalYPos(1, 0, 0);
	FbxVector4 lNormalYNeg(1, 0, 0);
	FbxVector4 lNormalZPos(0, 0, 1);
	FbxVector4 lNormalZNeg(1, 0, 0);


	FbxGeometryElementNormal* lGeometryElementNormal = lMesh->CreateElementNormal();
	lGeometryElementNormal->SetMappingMode(FbxGeometryElement::eByControlPoint);

	lGeometryElementNormal->SetReferenceMode(FbxGeometryElement::eIndexToDirect);


	lGeometryElementNormal->GetDirectArray().Add(lNormalZPos);
	//lGeometryElementNormal->GetDirectArray().Add(lNormalXPos);
	//lGeometryElementNormal->GetDirectArray().Add(lNormalZNeg);
	//lGeometryElementNormal->GetDirectArray().Add(lNormalXNeg);
	//lGeometryElementNormal->GetDirectArray().Add(lNormalYPos);
	//lGeometryElementNormal->GetDirectArray().Add(lNormalYNeg);

	//lGeometryElementNormal->GetIndexArray().Add(0); // index of lNormalZPos in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(0); // index of lNormalZPos in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(0); // index of lNormalZPos in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(0); // index of lNormalZPos in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(1); // index of lNormalXPos in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(1); // index of lNormalXPos in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(1); // index of lNormalXPos in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(1); // index of lNormalXPos in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(2); // index of lNormalZNeg in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(2); // index of lNormalZNeg in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(2); // index of lNormalZNeg in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(2); // index of lNormalZNeg in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(3); // index of lNormalXNeg in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(3); // index of lNormalXNeg in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(3); // index of lNormalXNeg in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(3); // index of lNormalXNeg in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(4); // index of lNormalYPos in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(4); // index of lNormalYPos in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(4); // index of lNormalYPos in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(4); // index of lNormalYPos in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(5); // index of lNormalYNeg in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(5); // index of lNormalYNeg in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(5); // index of lNormalYNeg in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(5); // index of lNormalYNeg in the direct array.


	FbxVector4 lControlPoint0(-50, 0, 50);
	FbxVector4 lControlPoint1(50, 0, 50);
	FbxVector4 lControlPoint2(50, 100, 50);
	FbxVector4 lControlPoint3(-50, 100, 50);
	FbxVector4 lControlPoint4(-50, 0, -50);
	FbxVector4 lControlPoint5(50, 0, -50);
	FbxVector4 lControlPoint6(50, 100, -50);
	FbxVector4 lControlPoint7(-50, 100, -50);

	lMesh->InitControlPoints(24);
	FbxVector4* lControlPoints = lMesh->GetControlPoints();
	lControlPoints[0] = lControlPoint0;
	lControlPoints[1] = lControlPoint1;
	lControlPoints[2] = lControlPoint2;
	lControlPoints[3] = lControlPoint3;
	lControlPoints[4] = lControlPoint1;
	lControlPoints[5] = lControlPoint5;
	lControlPoints[6] = lControlPoint6;
	lControlPoints[7] = lControlPoint2;
	lControlPoints[8] = lControlPoint5;
	lControlPoints[9] = lControlPoint4;
	lControlPoints[10] = lControlPoint7;
	lControlPoints[11] = lControlPoint6;
	lControlPoints[12] = lControlPoint4;
	lControlPoints[13] = lControlPoint0;
	lControlPoints[14] = lControlPoint3;
	lControlPoints[15] = lControlPoint7;
	lControlPoints[16] = lControlPoint3;
	lControlPoints[17] = lControlPoint2;
	lControlPoints[18] = lControlPoint6;
	lControlPoints[19] = lControlPoint7;
	lControlPoints[20] = lControlPoint1;
	lControlPoints[21] = lControlPoint0;
	lControlPoints[22] = lControlPoint4;
	lControlPoints[23] = lControlPoint5;

	int lPolygonVertices[] = { 0, 1, 2, 3,
		4, 5, 6, 7,
		8, 9, 10, 11,
		12, 13, 14, 15,
		16, 17, 18, 19,
		20, 21, 22, 23 };



	//每3个点一个面，a代表了除以几，是一个面
	int a = 3;
	for (int aa = 0; aa < 6; aa++)
	{
		lMesh->BeginPolygon(-1, -1, -1, false);
		for (int bb = 0; bb < 4; bb++)
		{
			lGeometryElementNormal->GetIndexArray().Add(0);
			lMesh->AddPolygon(lPolygonVertices[aa * 4 + bb]);

		}
		lMesh->EndPolygon();
	}

	FbxNode* lNode = FbxNode::Create(pScene, pName);
	lNode->SetNodeAttribute(lMesh);
	pScene->GetRootNode()->AddChild(lNode);
	SaveScene(pManager, pScene, "textcube.fbx");

	DestroySdkObjects(pManager, PResult);



}






