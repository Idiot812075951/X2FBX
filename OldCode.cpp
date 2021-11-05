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


bool getXInfo (const std::string& lxoFilename, HWND hwnd, std::string ExportPath)
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

	g_d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING,&d3dpp, &g_device);
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

	TueVertex* pv, v1, v2, v3, v4;
	int i0, i00, i1, i2, i3;
	PWORD pi2 = NULL;
	PDWORD pi4 = NULL;
	// pdword就是一个正整数
	PDWORD pa;
	std::vector<TueTriVertex> vOrg;
	std::vector<TueTriVertex> vFinal;

	std::vector<MyTriVertex> vMyOrg;
	std::vector<MyTriVertex> vMyFinal;
	int subsetIndex;
	D3DXVECTOR3 position;
	D3DXVECTOR3 normal;
	D3DXVECTOR2 uv;
	D3DXVECTOR3 pMin;
	D3DXVECTOR3 pMax;
	i0 = g_mesh->GetNumFaces();
	i00 = g_mesh->GetNumVertices();

	IDirect3DIndexBuffer9* ppIB;
	g_mesh->GetIndexBuffer(&ppIB);
	D3DINDEXBUFFER_DESC pDesc;
	ppIB->GetDesc(&pDesc);
	int step = pDesc.Size / (i0 * 3);

	
	g_mesh->LockAttributeBuffer(0, &pa);
	if (step == 2)
		g_mesh->LockIndexBuffer(0, (void**)&pi2);
	else
		g_mesh->LockIndexBuffer(0, (void**)&pi4);
	g_mesh->LockVertexBuffer(0, (void**)&pv);

	std::vector<long long> IndexAry;
	std::vector<TueVertex> VerAry;
	for (int i = 0; i < i0; i++)
	{
		if (step == 2)
		{
			IndexAry.emplace_back(*pi2);
		}
		else
		{
			IndexAry.emplace_back(*pi4);
		}

		VerAry.push_back(*pv);
	}
	for (int i = 0; i < i0; i++)
	{
		if (step == 2)
		{
			i1 = *pi2;
			IndexAry.emplace_back(i1);
			pi2 += 1;
			i2 = *pi2;
			IndexAry.emplace_back(i2);
			pi2 += 1;
			i3 = *pi2;
			IndexAry.emplace_back(i3);
			pi2 += 1;
		}
		else
		{
			i1 = *pi4;
			IndexAry.emplace_back(i1);
			pi4 += 1;
			i2 = *pi4;
			IndexAry.emplace_back(i2);
			pi4 += 1;
			i3 = *pi4;
			IndexAry.emplace_back(i3);
			pi4 += 1;
		}


		// i1就是点的索引，每次用指针定位那个点，再通过 += ，-=来移动指针位置；
		pv += i1;
		v1 = *pv; //取到第一个点

		pv -= i1; //先退回去，再重新加
		pv += i2;
		v2 = *pv;

		pv -= i2;
		pv += i3;
		v3 = *pv;

		pv -= i3;
		//这是三角面的索引
		subsetIndex = *pa;
		pa += 1;

		TueTriVertex v = { subsetIndex, v1, v2, v3 };
	

		D3DXMATERIAL* tempMaterials = (D3DXMATERIAL*)(material_buffer)->GetBufferPointer();
		D3DCOLORVALUE diffuse = tempMaterials->MatD3D.Diffuse;
		D3DCOLORVALUE ambient = tempMaterials->MatD3D.Ambient;
		D3DCOLORVALUE emissive = tempMaterials->MatD3D.Emissive;
		D3DCOLORVALUE specular = tempMaterials->MatD3D.Specular;

		MyTriVertex my = { subsetIndex, v1, v2, v3 ,diffuse,ambient,emissive,specular };

		vOrg.push_back(v);
		vMyOrg.push_back(my);
	}


	g_mesh->UnlockAttributeBuffer();
	g_mesh->UnlockIndexBuffer();
	g_mesh->UnlockVertexBuffer();

	TueTriVertex vv;
	MyTriVertex mm;
	int count;

	//D3DXMATERIAL* tempMaterials = (D3DXMATERIAL*)(material_buffer)->GetBufferPointer();

	FMeshInfoMap MeshAry;
	std::vector<Material> vMaterial;
	for (unsigned int j = 0; j < g_num_materials; j++)
	{
		D3DXMATERIAL* tempMaterials = (D3DXMATERIAL*)(material_buffer)->GetBufferPointer();
		Material m;
		//tempMaterials += j;

		auto ptr = tempMaterials + j;

		m.diffuse= ptr->MatD3D.Diffuse;
		m.ambient = ptr->MatD3D.Ambient;
		m.emissive = ptr->MatD3D.Emissive;
		m.specular = ptr->MatD3D.Specular;
		vMaterial.push_back(m);

		

		FMeshInfo MeshInfo;

		if (tempMaterials[j].pTextureFilename)
		{
			MeshInfo.Name = tempMaterials[j].pTextureFilename;
		}

		vFinal.clear();
		vMyFinal.clear();
		for (std::vector<MyTriVertex>::iterator iter = vMyOrg.begin(); iter < vMyOrg.end(); ++iter)
		{
			mm = *iter;
			if (mm.subsetIndex == j)
			{
				vMyFinal.push_back(mm);
			}
		}

		std::vector<float> Vf;
		std::list<float> Lf;
		std::set<float> Sf;

		//for (int m = 0; m < vMyFinal.size(); m++)
		//{
		//	MyTriVertex setvert = vMyFinal[m];
		//	float a;
		//	float b;
		//	float c;
		//	a = setvert.v1.position.X + setvert.v1.position.Y + setvert.v1.position.Z;
		//	b = setvert.v2.position.X + setvert.v2.position.Y + setvert.v2.position.Z;
		//	c = setvert.v3.position.X + setvert.v3.position.Y + setvert.v3.position.Z;

		//	//sort(Vf.begin(), Vf.end());
		//	//Vf.erase(unique(Vf.begin(), Vf.end()), Vf.end());
		//	Sf.insert(a);
		//	Sf.insert(b);
		//	Sf.insert(c);

		//}

		printf("");

		//sort(Vf.begin(), Vf.end());
		//Vf.erase(unique(Vf.begin(), Vf.end()), Vf.end());


		for (int m = 0; m < vMyFinal.size(); m++)
		{
			int idd = 0;
			MyTriVertex setvert = vMyFinal[m];
			{
				//int num;
				//
				//float tmp;
				//tmp= setvert.v1.position.X + setvert.v1.position.Y + setvert.v1.position.Z;
				//num = std::count(Sf.begin(), Sf.end(), tmp);//统计出现的次数

				//if (num > 3)
				//{
				//	continue;
				//}
				//else
				{
					TueMeshTri MeshTri;
					MeshTri.p1.X = setvert.v1.position.X * 10;
					MeshTri.p1.Y = setvert.v1.position.Y * 10;
					MeshTri.p1.Z = setvert.v1.position.Z * 10;

					MeshTri.n1.X = setvert.v1.normal.X;
					MeshTri.n1.Y = setvert.v1.normal.Y;
					MeshTri.n1.Z = setvert.v1.normal.Z;
					

					MeshTri.CompareValue = pow(MeshTri.p1.X, 3); +pow(MeshTri.p1.Y, 3);+pow(MeshTri.p1.Z, 3);
			
					MeshTri.id = idd;

					MeshInfo.VecPtAry.emplace_back(MeshTri);
					MeshInfo.SetPtAry.insert(MeshTri);

				
					MeshInfo.MapPtAry[MeshTri]++;
					idd += 1;

				}

			}
			{
				//int num;

				//float tmp;
				//tmp = setvert.v2.position.X + setvert.v2.position.Y + setvert.v2.position.Z;

				//num = std::count(Vf.begin(), Vf.end(), tmp);//统计出现的次数

				//if (num> 3)
				//{
				//	continue;
				//}
				//else 
				{
					TueMeshTri MeshTri;
					MeshTri.p1.X = setvert.v2.position.X * 10;
					MeshTri.p1.Y = setvert.v2.position.Y * 10;
					MeshTri.p1.Z = setvert.v2.position.Z * 10;

					MeshTri.n1.X = setvert.v2.normal.X;
					MeshTri.n1.Y = setvert.v2.normal.Y;
					MeshTri.n1.Z = setvert.v2.normal.Z;


					//MeshTri.CompareValue = MeshTri.p1.X + MeshTri.p1.Y + MeshTri.p1.Z;
					MeshTri.CompareValue = pow(MeshTri.p1.X, 3); +pow(MeshTri.p1.Y, 3); +pow(MeshTri.p1.Z, 3);
					MeshTri.id = idd;

					MeshInfo.VecPtAry.emplace_back(MeshTri);
					MeshInfo.SetPtAry.insert(MeshTri);
					MeshInfo.MapPtAry[MeshTri]++;
					idd += 1;
				}



				
			}
			{
				//int num;

				//double tmp;
				//tmp = setvert.v3.position.X + setvert.v3.position.Y + setvert.v3.position.Z;

				//num = std::count(Vf.begin(), Vf.end(), tmp);//统计出现的次数

				//if (num > 3)
				//{
				//	continue;
				//}
				//else
				{
					TueMeshTri MeshTri;
					MeshTri.p1.X = setvert.v3.position.X * 10;
					MeshTri.p1.Y = setvert.v3.position.Y * 10;
					MeshTri.p1.Z = setvert.v3.position.Z * 10;

					MeshTri.n1.X = setvert.v3.normal.X;
					MeshTri.n1.Y = setvert.v3.normal.Y;
					MeshTri.n1.Z = setvert.v3.normal.Z;

					//MeshTri.CompareValue = MeshTri.p1.X + MeshTri.p1.Y + MeshTri.p1.Z;
					MeshTri.CompareValue = pow(MeshTri.p1.X, 3); +pow(MeshTri.p1.Y, 3); +pow(MeshTri.p1.Z, 3);
					MeshTri.id = idd;

					MeshInfo.VecPtAry.emplace_back(MeshTri);
					MeshInfo.SetPtAry.insert(MeshTri);
					MeshInfo.MapPtAry[MeshTri]++;
					idd += 1;
				}

				

			}
		}



		MeshAry.insert({ j, MeshInfo });
	}


	auto pManager = FbxManager::Create();
	if (!pManager)
	{
		//FBXSDK_printf("Error: Unable to create FBX Manager!\n");
		exit(1);
	}
	else
	{
		//FBXSDK_printf("Autodesk FBX SDK version %s\n", pManager->GetVersion());
	}
	//Create an IOSettings object.This object holds all import / export settings.
	FbxIOSettings* ios = FbxIOSettings::Create(pManager, IOSROOT);
	pManager->SetIOSettings(ios);

	//Load plugins from the executable directory(optional)
	FbxString lPath = FbxGetApplicationDirectory();
	pManager->LoadPluginsDirectory(lPath.Buffer());


	FbxScene* pScene = FbxScene::Create(pManager, "My Scene");
	const char* pName = "test";
	FbxMesh* lMesh = FbxMesh::Create(pScene, pName);

	int PtCount = 0;
	for (auto Iter : MeshAry)
	{
		PtCount += Iter.second.VecPtAry.size();
	}
	lMesh->InitControlPoints(PtCount);
	FbxVector4* lControlPoints = lMesh->GetControlPoints();
	int PtIndex = 0;

	


	for (auto Iter : MeshAry)
	{
		//注意，他这个是反过来的，索引0是Z，1是Y，2才是X
		//控制点不对

		//std::vector<TueMeshTri>tmp;
		//for (auto ii : Iter.second.SetPtAry)
		//{
		//	tmp.push_back(ii);
		//}

		for (int i = 0; i < Iter.second.VecPtAry.size()/2 ; i++)
		{
			if (1)
			{
				lControlPoints[PtIndex][0] = Iter.second.VecPtAry[i].p1.Z / 10;
				lControlPoints[PtIndex][1] = Iter.second.VecPtAry[i].p1.Y / 10;
				lControlPoints[PtIndex][2] = Iter.second.VecPtAry[i].p1.X / 10;
			}
			//lControlPoints[PtIndex][0] = Iter.second.VecPtAry[i].p1.Z / 10;
			//lControlPoints[PtIndex][1] = Iter.second.VecPtAry[i].p1.Y / 10;
			//lControlPoints[PtIndex][2] = Iter.second.VecPtAry[i].p1.X / 10;
			/*int tmp = (PtIndex % 6);
			if (tmp== 0 )
			{
				continue;
			}
			else
			{
				lControlPoints[PtIndex][0] = Iter.second.VecPtAry[i].p1.Z / 10;
				lControlPoints[PtIndex][1] = Iter.second.VecPtAry[i].p1.Y / 10;
				lControlPoints[PtIndex][2] = Iter.second.VecPtAry[i].p1.X / 10;
			}*/
			
			PtIndex++;
		}
	
	}

	FbxGeometryElementNormal* lNormalElement = lMesh->CreateElementNormal();
	lNormalElement->SetMappingMode(FbxGeometryElement::eByControlPoint);
	lNormalElement->SetReferenceMode(FbxGeometryElement::eDirect);

	//注意了，下面这个是要的

	for (auto Iter : MeshAry)
	{

		//std::vector<TueMeshTri>tmp;
		//for (auto ii : Iter.second.SetPtAry)
		//{
		//	tmp.push_back(ii);
		//}

		for (int i = 0; i < Iter.second.VecPtAry.size(); i++)
		//for (int i = 0; i < Iter.second.SetPtAry.size(); i++)
		{
			FbxVector4 lNormalZPos(
				//tmp[i].n1.X,
				//tmp[i].n1.Y,
				//tmp[i].n1.Z
				Iter.second.VecPtAry[i].n1.X,
				Iter.second.VecPtAry[i].n1.Y,
				Iter.second.VecPtAry[i].n1.Z

			);
			lNormalElement->GetDirectArray().Add(lNormalZPos);
		}


	//}

	//注意了，上面这个是要的


	//FbxGeometryElementUV* lUVDiffuseElement = lMesh->CreateElementUV(gDiffuseElementName);
	//FBX_ASSERT(lUVDiffuseElement != NULL);
	//lUVDiffuseElement->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
	//lUVDiffuseElement->SetReferenceMode(FbxGeometryElement::eIndexToDirect);
	//PtIndex = 0;
	//for (auto Iter : MeshAry)
	//{
	//	for (int i = 0; i < Iter.second.PtAry.size(); i++)
	//	{
	//		FbxVector2 lUVDiffuse(
	//			Iter.second.PtAry[i].uv1.X,
	//			Iter.second.PtAry[i].uv1.Y
	//		);
	//		//lUVDiffuseElement->GetDirectArray().Add(lUVDiffuse);
	//		PtIndex++;
	//	}
	//}


	

	//lUVDiffuseElement->GetIndexArray().SetCount(PtIndex);

	FbxGeometryElementMaterial* lMaterialElement = lMesh->CreateElementMaterial();
	lMaterialElement->SetMappingMode(FbxGeometryElement::eByPolygon);
	lMaterialElement->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

	PtIndex = 0;
	int ignore=0;

	std::vector<TueMeshTri> UniquePtAry;


	for (auto Iter : MeshAry)
	{
		//UniquePtAry = Iter.second.PtAry;
		//std::sort(UniquePtAry.begin(), UniquePtAry.end(), [](const TueMeshTri&Left, const TueMeshTri&Right) {
		//	return true;
		//});
//		UniquePtAry.erase(unique(UniquePtAry.begin(), UniquePtAry.end(), UniquePtAry.end()));
		for (int i = 0; i < Iter.second.VecPtAry.size() / 3; i++)
		{
	
			lMesh->BeginPolygon(Iter.first);
			//lMesh->BeginPolygon(-1,-1,false);
			//lMesh->BeginPolygon(-1, -1, false);
			for (int j = 0; j < 3; j++)
			{

				lMesh->AddPolygon(PtIndex);

				////lUVDiffuseElement->GetIndexArray().SetAt(PtIndex, PtIndex);
				PtIndex++;
			}
			lMesh->EndPolygon();
		}
	}





	//FbxFileTexture* lTexture = FbxFileTexture::Create(pScene, "Diffuse Texture");
	//bool isTexture = lTexture->SetFileName("0.jpg");

	//for (auto Iter : MeshAry)
	//{
	//	FbxNode* TextureNode = FbxNode::Create(pScene, Iter.second.Name.data());
	//}

	FbxNode* lNode = FbxNode::Create(pScene, pName);
	lNode->SetNodeAttribute(lMesh);


	int c = 0;
	for (auto i:vMaterial)
	{
		FbxString lMaterialName = "material";
		FbxDouble3 dif(i.diffuse.r, i.diffuse.g, i.diffuse.b);
		FbxDouble3 amb(i.ambient.r, i.ambient.g, i.ambient.b);
		FbxDouble3 emi(i.emissive.r, i.emissive.g, i.emissive.b);
		FbxDouble3 spe(i.specular.r, i.specular.g, i.specular.b);
		lMaterialName += c;
		c += 1;

		FbxDouble3 red(1.0, 0.0, 0.0);

		auto lMaterial = FbxSurfacePhong::Create(pScene, lMaterialName.Buffer());

		/*lMaterial->Diffuse.Set(dif);*/
		lMaterial->Diffuse.Set(dif);
		lMaterial->Ambient.Set(amb);
		lMaterial->Emissive.Set(emi);
		lMaterial->Specular.Set(spe);

		FbxNode* lNode = lMesh->GetNode();
		if (lNode)
			lNode->AddMaterial(lMaterial);
	}


	pScene->GetRootNode()->AddChild(lNode);


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
