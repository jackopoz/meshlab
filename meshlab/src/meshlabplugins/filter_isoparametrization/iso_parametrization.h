#ifndef TRI_PARAMETRIZATION
#define TRI_PARAMETRIZATION	

// stuff to define the mesh
#include <vcg/simplex/vertex/base.h>
#include <vcg/space/triangle3.h>
#include <vcg/simplex/face/base.h>
#include <vcg/simplex/face/component_rt.h>
#include <vcg/simplex/edge/base.h>
#include <vcg/complex/trimesh/base.h>
#include <local_parametrization.h>
#include <uv_grid.h>
#include <vcg/complex/trimesh/stat.h>

///ABSTRACT MESH THAT MAINTAINS THE WHOLE PARAMETERIZATION
class AbstractVertex;
class AbstractEdge;    
class AbstractFace;
class AbstractVertex  : public vcg::VertexSimp2< AbstractVertex, AbstractEdge, AbstractFace, 
	vcg::vertex::VFAdj, 
	vcg::vertex::Coord3f,
	vcg::vertex::TexCoord2f,
	vcg::vertex::BitFlags>
{
public:
	CoordType RPos;
	
};

class AbstractFace    : public vcg::FaceSimp2  < AbstractVertex, AbstractEdge, AbstractFace, 
	vcg::face::VFAdj, 
	vcg::face::FFAdj,
	vcg::face::VertexRef,
	vcg::face::Color4b,
	vcg::face::BitFlags>
{};

class AbstractMesh: public vcg::tri::TriMesh<std::vector<AbstractVertex>, std::vector<AbstractFace> > {
//public:
//	static const bool Has_Auxiliary(){return false;}
};

///HIGH RESOLUTION MESH THAT HAS TO BE PARAMETERIZED
class ParamVertex;
class ParamEdge;    
class ParamFace;

class ParamVertex: public vcg::VertexSimp2< ParamVertex, ParamEdge, ParamFace, 
	vcg::vertex::Normal3f, vcg::vertex::VFAdj, vcg::vertex::Coord3f,vcg::vertex::Color4b,vcg::vertex::TexCoord2f,vcg::vertex::BitFlags>
{
public:
	CoordType RPos;
	static const bool Has_Auxiliary(){return false;}
};

class ParamFace: public vcg::FaceSimp2  <  ParamVertex, ParamEdge, ParamFace, 
	vcg::face::VFAdj,vcg::face::FFAdj,vcg::face::VertexRef,vcg::face::Color4b,vcg::face::BitFlags,
	vcg::face::WedgeTexCoord2f>
{};
class ParamMesh: public vcg::tri::TriMesh<std::vector<ParamVertex>, std::vector<ParamFace> > 
{
//public:
	//static const bool Has_Auxiliary(){return false;}
};

template <class MeshType>
void CopyMeshFromFacesAbs(const std::vector<typename MeshType::FaceType*> &faces,
						  std::vector<typename MeshType::VertexType*> &orderedVertex,
						  MeshType & new_mesh)
{
	typedef typename MeshType::CoordType CoordType;
	typedef typename MeshType::ScalarType ScalarType;
	typedef typename MeshType::VertexType VertexType;
	typedef typename MeshType::FaceType FaceType;

	///get set of faces
	std::map<VertexType*,VertexType*> vertexmap;
        std::vector<VertexType*> vertices;
	FindVertices(faces,vertices);

	///initialization of new mesh
	new_mesh.Clear();
	new_mesh.vn=0;
	new_mesh.fn=0;
	new_mesh.face.resize(faces.size());
	new_mesh.vert.resize(vertices.size());
	new_mesh.vn=vertices.size();
	new_mesh.fn=faces.size();

	///add new vertices
        typename std::vector<VertexType*>::const_iterator iteV;
	int i=0;
	for (iteV=vertices.begin();iteV!=vertices.end();iteV++)
	{
		///copy position
		assert(!(*iteV)->IsD());
		new_mesh.vert[i].P()=(*iteV)->P();
		new_mesh.vert[i].RPos=(*iteV)->RPos;
		new_mesh.vert[i].T().P()=(*iteV)->T().P();
		new_mesh.vert[i].T().N()=(*iteV)->T().N();
		//new_mesh.vert[i].N()=(*iteV)->N();
		///*assert(new_mesh.vert[i].brother!=NULL);*/
		////if (MeshType::Has_Auxiliary())
		//new_mesh.vert[i].brother=(*iteV)->brother;
		new_mesh.vert[i].ClearFlags();
		
		orderedVertex.push_back((*iteV));
		vertexmap.insert(std::pair<VertexType*,VertexType*>((*iteV),&new_mesh.vert[i]));
		i++;
	}

	///setting of new faces
        typename std::vector<FaceType*>::const_iterator iteF;
        typename std::vector<FaceType>::iterator iteF1;
	for (iteF=faces.begin(),iteF1=new_mesh.face.begin()
		;iteF!=faces.end();iteF++,iteF1++)
	{
		//(*iteF1).areadelta=(*iteF)->areadelta;
	/*	if ((*iteF1).areadelta>1)
			assert(0);*/
		///for each vertex get new reference
		///and associate face-vertex
		for (int j=0;j<3;j++)
		{
			VertexType* v=(*iteF)->V(j);
                        typename std::map<VertexType*,VertexType*>::iterator iteMap=vertexmap.find(v);
			assert(iteMap!=vertexmap.end());
			(*iteF1).V(j)=(*iteMap).second;
		}
	}
}

///create a mesh considering just the faces that share all three vertex
template <class MeshType>
void CopyMeshFromVerticesAbs(std::vector<typename MeshType::VertexType*> &vertices,
						  std::vector<typename MeshType::VertexType*> &OrderedVertices,
						  std::vector<typename MeshType::FaceType*> &OrderedFaces,
					      MeshType & new_mesh)
{
        typedef typename MeshType::CoordType CoordType;
        typedef typename MeshType::ScalarType ScalarType;
        typedef typename MeshType::VertexType VertexType;
        typedef typename MeshType::FaceType FaceType;

        typename std::vector<VertexType*>::const_iterator iteV;
	for (iteV=vertices.begin();iteV!=vertices.end();iteV++)
		(*iteV)->ClearV();



	OrderedVertices.clear();

	///vertex-vertex reference
	std::map<VertexType*,VertexType*> vertexmap;

	///get set of faces
	std::vector<typename MeshType::FaceType*> faces;

	getSharedFace<MeshType>(vertices,faces);
	
	///initialization of new mesh
	new_mesh.Clear();
	new_mesh.vn=0;
	new_mesh.fn=0;

	///set vertices as selected
	
	for (iteV=vertices.begin();iteV!=vertices.end();iteV++)
		(*iteV)->SetV();

	///getting inside faces
        typename std::vector<FaceType*>::const_iterator iteF;
	for (iteF=faces.begin();iteF!=faces.end();iteF++)
	{
		///for each vertex get new reference
		///if there isn't one reference means the face does not appartain to group
		VertexType* v0=(*iteF)->V(0);
		VertexType* v1=(*iteF)->V(1);
		VertexType* v2=(*iteF)->V(2);
		bool inside=((*v0).IsV()&&(*v1).IsV()&&(*v2).IsV());
		if (inside)
			OrderedFaces.push_back((*iteF));
	}
	
	///find internal vertices
	FindVertices(OrderedFaces,OrderedVertices);

	///setting size
	new_mesh.face.resize(OrderedFaces.size());
	new_mesh.vert.resize(OrderedVertices.size());
	new_mesh.vn=OrderedVertices.size();
	new_mesh.fn=OrderedFaces.size();

	///setting of internal vertices
	int i=0;
        typename std::vector<typename MeshType::VertexType*>::iterator iteVI;
	for (iteVI=OrderedVertices.begin();iteVI!=OrderedVertices.end();iteVI++)
	{
		///copy position
		assert(!(*iteVI)->IsD());
		new_mesh.vert[i].P()=(*iteVI)->P();
		new_mesh.vert[i].RPos=(*iteVI)->RPos;
		new_mesh.vert[i].T().P()=(*iteVI)->T().P();
		new_mesh.vert[i].T().N()=(*iteVI)->T().N();
		/*new_mesh.vert[i].father=(*iteVI)->father;
		new_mesh.vert[i].Bary=(*iteVI)->Bary;*/
		//new_mesh.vert[i].Damp=(*iteVI)->Damp;
		/*new_mesh.vert[i].RestUV=(*iteVI)->RestUV;*/
		//new_mesh.vert[i].N()=(*iteVI)->N();
		//new_mesh.vert[i].C()=(*iteVI)->C();
		/*new_mesh.vert[i].OriginalCol=(*iteVI)->OriginalCol;*/
		new_mesh.vert[i].ClearFlags();
		///map setting
		vertexmap.insert(std::pair<VertexType*,VertexType*>((*iteVI),&new_mesh.vert[i]));
		i++;
	}

	///setting of new faces
        typename std::vector<FaceType>::iterator iteF1;
	for (iteF=OrderedFaces.begin(),iteF1=new_mesh.face.begin()
		;iteF!=OrderedFaces.end();iteF++,iteF1++)
	{
		///for each vertex get new reference
		///and associate face-vertex
		for (int j=0;j<3;j++)
		{
			VertexType* v=(*iteF)->V(j);
                        typename std::map<VertexType*,VertexType*>::iterator iteMap=vertexmap.find(v);
			assert(iteMap!=vertexmap.end());
			(*iteF1).V(j)=(*iteMap).second;
		}
	}

	///clear flags
	for (iteV=vertices.begin();iteV!=vertices.end();iteV++)
		(*iteV)->ClearV();

}




//template <class InputMesh>
//static int Parametrize(InputMesh &to_param,
//					   AbstractMesh &AbsMesh,
//					   InputMesh &Parametrized,
//					   int &approx_face_num)
//{
//	vcg::tri::UpdateTopology<InputMesh>::FaceFace(to_param);
//
//	///test input conditions
//	bool b=vcg::tri::Clean<InputMesh>::IsTwoManifoldFace(to_param);
//	b&=vcg::tri::Clean<InputMesh>::IsTwoManifoldVertexFF(to_param);
//	int n=vcg::tri::Clean<InputMesh>::BorderEdges(to_param);
//	b&=(n==0);
//	int num=vcg::tri::Clean<InputMesh>::ConnectedComponents(to_param);
//	b&=(num==1);
//	if (!b)
//		return -1;
//
//	///then parameteterize
//	IsoParametrizator TrImage;
//	TrImage.Parametrize<MyMesh>(&mesh,num1,num2);
//	TrImage.ExportMeshes(to_param,AbsMesh);
//	return 0;
//}

///class that maintains the whole meh parametrerization with relitive operators
class IsoParametrization
{
	AbstractMesh * abstract_mesh;
	ParamMesh	 * param_mesh;

public:
	typedef ParamMesh::ScalarType ScalarType;
	typedef ParamMesh::CoordType CoordType;

private:

	///this class maintains submeshes and hresolution meshes
	///for the selected parametrization domain
	struct param_domain{
		AbstractMesh *domain;
		std::vector<int> local_to_global;

		ParamMesh *HresDomain;
		UVGrid<ParamMesh> grid;
		std::vector<ParamFace*> ordered_faces;

		int Local2Global(const int &localFace)
		{return local_to_global[localFace];}

		int Global2Local(const int &GlobalFace)
		{
			int ret=-1;
			for (unsigned int i=0;i<local_to_global.size();i++)
				if (local_to_global[i]==GlobalFace)
					return i;
			return ret;
		}

		void InitGrid()
		{grid.Init(*HresDomain);}

		bool Project(vcg::Point2<ScalarType> UV,
					std::vector<ParamFace*> &face,
					std::vector<CoordType> &baryVal)
		{
			std::vector<ParamFace*> faceParam;
			bool found=grid.ProjectPoint(UV,faceParam,baryVal);
			if (!found)
				return false;
			///calculate the index corresponding to the face
			for (unsigned int i=0;i<faceParam.size();i++)
			{
				ParamFace* f=faceParam[i];
				int index=f-&(*HresDomain->face.begin());
				assert(index<HresDomain->fn);
				face.push_back(ordered_faces[index]);
			}
			return true;
		}

		bool getClosest(vcg::Point2<ScalarType> UV,
						std::vector<ParamFace*> &face,
						std::vector<CoordType> &baryVal)
		{
			face.resize(1);
			baryVal.resize(1);
			bool found=grid.getClosest(UV,face[0],baryVal[0]);
			return found;
		}
	};

	///summeshes and subdomains
	std::vector<param_domain> star_meshes;
	std::vector<param_domain> diamond_meshes;
	std::vector<param_domain> face_meshes;

	typedef std::pair<AbstractVertex*,AbstractVertex*> keyEdgeType;
	std::map<keyEdgeType,int> EdgeTab;

	///temporary structure for face_to_vert adjacency
	std::vector<std::vector<ParamVertex*> > face_to_vert;

	void InitFaceToVert()
	{
		face_to_vert.resize(abstract_mesh->face.size());
		for (unsigned int i=0;i<param_mesh->vert.size();i++)
		{
			int I=param_mesh->vert[i].T().N();
			face_to_vert[I].push_back(&param_mesh->vert[i]);
		}
	}


	void GetHresVert(int &I,std::vector<ParamVertex*> &HresVert)
	{
		for (unsigned int k=0;k<face_to_vert[I].size();k++)
		{
			ParamVertex* v=face_to_vert[I][k];
			HresVert.push_back(v);
		}
	}

	///initialize star parametrization
	void InitStar()
	{
		///for each vertex
		int index=0;
		for (unsigned int i=0;i<abstract_mesh->vert.size();i++)
		{
			if (!(abstract_mesh->vert[i].IsD()))
			{
				std::vector<AbstractVertex*> starCenter;
				starCenter.push_back(&abstract_mesh->vert[i]);

				star_meshes[index].domain=new AbstractMesh();
				star_meshes[index].HresDomain=new ParamMesh();

				///create star
				std::vector<AbstractFace*> ordered_faces;
				std::vector<AbstractVertex*> ordered_vert;
				//CreateMeshVertexStar(starCenter,ordered_faces,*star_meshes[index].domain);
				///get faces referenced by vertices
				getSharedFace<AbstractMesh>(starCenter,ordered_faces);
				CopyMeshFromFacesAbs<AbstractMesh>(ordered_faces,ordered_vert,*star_meshes[index].domain);

				UpdateTopologies(star_meshes[index].domain);

				///and parametrize it
				ParametrizeStarEquilateral<AbstractMesh>(*star_meshes[index].domain,1.0);
				
				///set other components as reefrence to original faces
				star_meshes[index].local_to_global.resize(star_meshes[index].domain->face.size());
				std::vector<ParamVertex*> HresVert;
				for (unsigned int k=0;k<star_meshes[index].domain->face.size();k++)
				{
					int IndexF;
					getFaceIndexFromPointer(ordered_faces[k],IndexF);
					star_meshes[index].local_to_global[k]=IndexF;
					///get H res vertex
					GetHresVert(IndexF,HresVert);
				}
				
				///copy Hres mesh
				std::vector<ParamVertex*> OrderedVertices;
				CopyMeshFromVerticesAbs(HresVert,OrderedVertices,star_meshes[index].ordered_faces,*star_meshes[index].HresDomain);
				///set new parametrization values
				for (unsigned int k=0;k<star_meshes[index].HresDomain->vert.size();k++)
				{
					ParamVertex * v=&star_meshes[index].HresDomain->vert[k];
					CoordType bary=CoordType(v->T().U(),v->T().V(),1-v->T().U()-v->T().V());
					AbstractMesh *paramDomain=star_meshes[index].domain;
					///get the right face on the parametrized domain
					int Father=v->T().N();
					int faceNum=-1;
					for (unsigned int i=0;i<star_meshes[index].local_to_global.size();i++)
					{
						if (star_meshes[index].local_to_global[i]==Father)
							faceNum=i;
					}
					AbstractFace *faceDom=&paramDomain->face[faceNum];
					v->T().P()=(faceDom->V(0)->T().P())*bary.X()+(faceDom->V(1)->T().P())*bary.Y()+(faceDom->V(2)->T().P())*bary.Z();
					assert(faceNum!=-1);
				}
				star_meshes[index].InitGrid();
				index++;
				
			}
		}		
	}



	void InitDiamond(const ScalarType &edge_len=1.0)
	{
		
		///for each face
		int index=0;
		EdgeTab.clear();
		for (unsigned int i=0;i<abstract_mesh->face.size();i++)
		{
			if (!(abstract_mesh->face[i].IsD()))
			{
				AbstractFace *f0=&abstract_mesh->face[i];
				//for each edge
				for (int j=0;j<3;j++)
				{
					AbstractFace * f1=f0->FFp(j);
					if (f1>f0)
					{
						
						int num0=j;
						int num1=f0->FFi(j);

						///add to domain map
						AbstractVertex *v0,*v1;
						v0=f0->V(j);
						v1=f0->V1(j);
						keyEdgeType k;
						if (v0<v1)
							k=keyEdgeType(v0,v1);
						else
							k=keyEdgeType(v1,v0);

						std::pair<keyEdgeType,int> entry=std::pair<keyEdgeType,int>(k,index);
						EdgeTab.insert(entry);



						///copy the mesh
						std::vector<AbstractFace*> faces;
						faces.push_back(f0);
						faces.push_back(f1);

						diamond_meshes[index].domain=new AbstractMesh();
						diamond_meshes[index].HresDomain=new ParamMesh();

						///create a copy of the mesh
						std::vector<AbstractVertex*> orderedVertex;
						CopyMeshFromFacesAbs<AbstractMesh>(faces,orderedVertex,*diamond_meshes[index].domain);
						UpdateTopologies<AbstractMesh>(diamond_meshes[index].domain);

						///set other components
						int index0,index1;
						getFaceIndexFromPointer(f0,index0);
						getFaceIndexFromPointer(f1,index1);
						diamond_meshes[index].local_to_global.resize(2);
						diamond_meshes[index].local_to_global[0]=index0;
						diamond_meshes[index].local_to_global[1]=index1;

						///parametrize locally
						ParametrizeDiamondEquilateral<AbstractMesh>(*diamond_meshes[index].domain,num0,num1,edge_len);
						///add h resolution vertices 
						std::vector<ParamVertex*> HresVert;
						GetHresVert(index0,HresVert);
						GetHresVert(index1,HresVert);
						std::vector<ParamVertex*> OrderedVertices;
						CopyMeshFromVerticesAbs(HresVert,OrderedVertices,diamond_meshes[index].ordered_faces,*diamond_meshes[index].HresDomain);
						///set new parametrization values
						for (unsigned int k=0;k<diamond_meshes[index].HresDomain->vert.size();k++)
						{
							ParamVertex * v=&diamond_meshes[index].HresDomain->vert[k];
							CoordType bary=CoordType(v->T().U(),v->T().V(),1-v->T().U()-v->T().V());
							AbstractMesh *paramDomain=diamond_meshes[index].domain;
							///get the rigth face on the parametrized domain
							int Father=v->T().N();
							int faceNum=-1;
							for (unsigned int i=0;i<diamond_meshes[index].local_to_global.size();i++)
							{
								if (diamond_meshes[index].local_to_global[i]==Father)
									faceNum=i;
							}
							assert(faceNum!=-1);
							AbstractFace *faceDom=&paramDomain->face[faceNum];
							v->T().P()=(faceDom->V(0)->T().P())*bary.X()+(faceDom->V(1)->T().P())*bary.Y()+(faceDom->V(2)->T().P())*bary.Z();
							
							
						}
						diamond_meshes[index].InitGrid();
						index++;
					}
				}
			}
		}
	}


	void InitFace(const ScalarType &edge_len=1)
	{
		///for each face
		int index=0;
		for (unsigned int i=0;i<abstract_mesh->face.size();i++)
		{
			if (!(abstract_mesh->face[i].IsD()))
			{
				AbstractFace *f0=&abstract_mesh->face[i];

				std::vector<AbstractFace*> faces;
				faces.push_back(f0);

				///create the mesh
				face_meshes[index].domain=new AbstractMesh();
				face_meshes[index].HresDomain=new ParamMesh();

				std::vector<AbstractVertex*> orderedVertex;
				CopyMeshFromFacesAbs<AbstractMesh>(faces,orderedVertex,*face_meshes[index].domain);

				assert(face_meshes[index].domain->vn==3);
				assert(face_meshes[index].domain->fn==1);

				///initialize auxiliary structures
				face_meshes[index].local_to_global.resize(1);
				face_meshes[index].local_to_global[0]=i;

				///parametrize it
				ParametrizeFaceEquilateral<AbstractMesh>(*face_meshes[index].domain,edge_len);

				///add h resolution vertices 
				std::vector<ParamVertex*> HresVert;
				GetHresVert(index,HresVert);
				std::vector<ParamVertex*> OrderedVertices;
				CopyMeshFromVerticesAbs(HresVert,OrderedVertices,face_meshes[index].ordered_faces,*face_meshes[index].HresDomain);
				///set new parametrization values
				for (unsigned int k=0;k<face_meshes[index].HresDomain->vert.size();k++)
				{
					ParamVertex * v=&face_meshes[index].HresDomain->vert[k];
					CoordType bary=CoordType(v->T().U(),v->T().V(),1-v->T().U()-v->T().V());
					AbstractMesh *paramDomain=face_meshes[index].domain;
					AbstractFace *faceDom=&paramDomain->face[0];
					v->T().P()=(faceDom->V(0)->T().P())*bary.X()+(faceDom->V(1)->T().P())*bary.Y()+(faceDom->V(2)->T().P())*bary.Z();
				}

				face_meshes[index].InitGrid();
				index++;
			}
		}
	}

	void getFaceIndexFromPointer(AbstractFace * f,int &index)
	{
		index=f-&(*abstract_mesh->face.begin());
	}

	void getStarFromPointer(AbstractVertex * center,int &index)
	{
		index=center-&(*abstract_mesh->vert.begin());
	}

	void getDiamondFromPointer(AbstractVertex * v0,AbstractVertex * v1,int &index)
	{
		assert(v0!=v1);
		keyEdgeType key;
		if (v0<v1)
			key=keyEdgeType(v0,v1);
		else
			key=keyEdgeType(v1,v0);

		std::map<keyEdgeType,int>::iterator k=EdgeTab.find(key);
		assert(k!=EdgeTab.end());
		index=((*k).second);
	}
	
	
	void Test()
	{
		/*int index=0;*/
		for (unsigned int i=0;i<abstract_mesh->face.size();i++)
		{
			if (!(abstract_mesh->face[i].IsD()))
			{
				AbstractFace *f0=&abstract_mesh->face[i];
				//for each edge
				for (int j=0;j<3;j++)
				{
					AbstractFace * f1=f0->FFp(j);
					if (f1>f0)
					{
						
						/*int num0=j;
						int num1=f0->FFi(j);*/

						///add to domain map
						AbstractVertex *v0,*v1;
						v0=f0->V(j);
						v1=f0->V1(j);
						keyEdgeType k;
						if (v0<v1)
							k=keyEdgeType(v0,v1);
						else
							k=keyEdgeType(v1,v0);

						//std::pair<keyEdgeType,int> entry=std::pair<keyEdgeType,int>(k,index);
						std::map<keyEdgeType,int>::iterator iteE=EdgeTab.find(k);
						
						int index0F,index1F;
						getFaceIndexFromPointer(f0,index0F);
						getFaceIndexFromPointer(f1,index1F);
#ifndef NDEBUG
						int edgeIndex=(*iteE).second;
						assert(diamond_meshes[edgeIndex].local_to_global[0]==index0F);
						assert(diamond_meshes[edgeIndex].local_to_global[1]==index1F);
#endif
					}
				}
			}
		}

		///test if for each face there is a right domain
		for (unsigned int i=0;i<param_mesh->face.size();i++)
		{
			ParamFace * f=&param_mesh->face[i];	
			vcg::Point2f uvI0,uvI1,uvI2;
			int IndexDomain=-1;
#ifndef NDEBUG
			int ret=InterpolationSpace(f,uvI0,uvI1,uvI2,IndexDomain);
			assert(ret!=-1);
#else
			InterpolationSpace(f,uvI0,uvI1,uvI2,IndexDomain);
#endif
		}

	}

	int getSharedVertices(AbstractFace *f0,AbstractFace *f1,AbstractFace *f2,
						  AbstractVertex *shared[3])
	{
		AbstractVertex *vert0[3],*vert1[3],*vert2[3];
		
		vert0[0]=f0->V(0);
		vert0[1]=f0->V(1);
		vert0[2]=f0->V(2);

		vert1[0]=f1->V(0);
		vert1[1]=f1->V(1);
		vert1[2]=f1->V(2);

		vert2[0]=f2->V(0);
		vert2[1]=f2->V(1);
		vert2[2]=f2->V(2);
		
		int num=0;
		for (int i=0;i<3;i++)
		{
			AbstractVertex * test=vert0[i];
			bool found0=false,found1=false;
			if ((vert1[0]==test)||(vert1[1]==test)||(vert1[2]==test))
					found0=true;
			if (found0)
			{
				if ((vert2[0]==test)||(vert2[1]==test)||(vert2[2]==test))
					found1=true;
			}
			if ((found0)&&(found1))
			{
				shared[num]=test;
				num++;
			}
		}

		return num;
	}

	void Clamp(vcg::Point2f &UV)
	{
		float eps=0.00001f;
		if ((UV.X()<eps)&&(UV.X()>-eps))
			UV.X()=0;
		if ((UV.X()<1+eps)&&(UV.X()>1-eps))
			UV.X()=1;
		if ((UV.Y()<eps)&&(UV.Y()>-eps))
			UV.Y()=0;
		if ((UV.Y()<1+eps)&&(UV.Y()>1-eps))
			UV.Y()=1;
	}
	
	
	float Area3d;
	float AbstractArea;

public:
	
	///return the minimum interpolation space shared by a face changing coordinates
	///return 0 if is a face 1 is a diamaond and 2 is a star
	int InterpolationSpace(ParamFace *f,
		vcg::Point2f &uvI0,
		vcg::Point2f &uvI1,
		vcg::Point2f &uvI2,
		int &IndexDomain)
	{
		ParamVertex *v0=f->V(0);
		ParamVertex *v1=f->V(1);
		ParamVertex *v2=f->V(2);

		int I0=v0->T().N();
		int I1=v1->T().N();
		int I2=v2->T().N();

		vcg::Point2f UV0=v0->T().P();
		vcg::Point2f UV1=v1->T().P();
		vcg::Point2f UV2=v2->T().P();

		///if they are equal it's is triavially the interpolation of UV coords
		///and the same face I as the domain
		if ((I0==I1)&&(I1==I2))
		{
			GE2(I0,UV0,uvI0);
			GE2(I1,UV1,uvI1);
			GE2(I2,UV2,uvI2);
			IndexDomain=I0;
			return 0;
		}

		///else find the right interpolation domain in which the face belongs
		///test if they share an edge then use half diamond
		AbstractFace *f0=&abstract_mesh->face[I0];
		AbstractFace *f1=&abstract_mesh->face[I1];
		AbstractFace *f2=&abstract_mesh->face[I2];
		AbstractVertex *shared[3];

		int num=getSharedVertices(f0,f1,f2,shared);
		assert((num==1)||(num==2));
		if (num==2)///ude diamond
		{
			AbstractVertex* v0=shared[0];
			AbstractVertex* v1=shared[1];
			int EdgeIndex;

			getDiamondFromPointer(v0,v1,EdgeIndex);			

			GE1(I0,UV0,EdgeIndex,uvI0);
			GE1(I1,UV1,EdgeIndex,uvI1);
			GE1(I2,UV2,EdgeIndex,uvI2);

			IndexDomain=EdgeIndex;
			return 1;
		}

		///use the star domain

		AbstractVertex* center=shared[0];
		int StarIndex;
		getStarFromPointer(center,StarIndex);
		bool b0=GE0(I0,UV0,StarIndex,uvI0);
		bool b1=GE0(I1,UV1,StarIndex,uvI1);
		bool b2=GE0(I2,UV2,StarIndex,uvI2);
		IndexDomain=StarIndex;
		if ((!b0)||(!b1)||(!b2))
			return -1;

		assert((uvI0.X()>=-1)&&(uvI0.Y()>=-1)&&(uvI0.X()<=1)&&(uvI0.Y()<=1));
		assert((uvI1.X()>=-1)&&(uvI1.Y()>=-1)&&(uvI1.X()<=1)&&(uvI1.Y()<=1));
		assert((uvI2.X()>=-1)&&(uvI2.Y()>=-1)&&(uvI2.X()<=1)&&(uvI2.Y()<=1)); 

		

		return 2;

	}

	int getHStarIndex(const int &I,
		const vcg::Point2<ScalarType> &UV)
	{
		int vertStar=2;
		CoordType bary=CoordType(UV.X(),UV.Y(),(ScalarType)1.0-UV.X()-UV.Y());

		if ((bary.X()>bary.Y())&&(bary.X()>bary.Z()))
			vertStar=0;
		else
			if ((bary.Y()>bary.X())&&(bary.Y()>bary.Z()))
				vertStar=1;

		AbstractFace *f=&abstract_mesh->face[I];
		AbstractVertex *Center=f->V(vertStar);

		int StarIndex;
		getStarFromPointer(Center,StarIndex);
		return StarIndex;
	}

	int getHDiamIndex(const int &I,
		const vcg::Point2<ScalarType> &UV)
	{
		CoordType bary=CoordType(UV.X(),UV.Y(),1-UV.X()-UV.Y());
		ScalarType sum0=bary.X()+bary.Y();
		ScalarType sum1=bary.Y()+bary.Z();
		ScalarType sum2=bary.X()+bary.Z();
		int edge=2;
		if ((sum0>sum1)&&(sum0>sum2))
			edge=0;
		else
			if ((sum1>sum0)&&(sum1>sum2))
				edge=1;

		///get the half-diamond mesh
		int DiamIndex;
		getDiamondFromPointer(abstract_mesh->face[I].V(edge),abstract_mesh->face[I].V1(edge),DiamIndex);
		return(DiamIndex);
	}

	///give the face and barycentric coordinate return I and UV coordinates
	void Phi(const ParamFace *f,
		const CoordType &bary3D,
		int &I,
		vcg::Point2<ScalarType> &UV)
	{
#ifndef NDEBUG
		float eps=0.00001f;
#endif
		///control right domain
		int I0=f->V(0)->T().N();
		int I1=f->V(1)->T().N();
		int I2=f->V(2)->T().N();

		///if they are equal it's is triavially the interpolation of UV coords
		///and the same face I as the domain
		if ((I0==I1)&&(I1==I2))
		{
			UV=bary3D.X()*f->V(0)->T().P()+bary3D.Y()*f->V(1)->T().P()+bary3D.Z()*f->V(2)->T().P();
			Clamp(UV);
			assert((UV.X()>=0)&&(UV.Y()>=0)&&(UV.X()<=1)&&(UV.Y()<=1)&&(UV.X()+UV.Y()<=1));
			I=I0;
			return;
		}
		

		///else find the right interpolation domain in which the face belongs
		///test if they share an edge then use half diamond
		AbstractFace *f0=&abstract_mesh->face[I0];
		AbstractFace *f1=&abstract_mesh->face[I1];
		AbstractFace *f2=&abstract_mesh->face[I2];
		AbstractVertex *shared[3];

		int num=getSharedVertices(f0,f1,f2,shared);
		assert((num==1)||(num==2));

		if (num==1)///use the star domain
		{
			//printf("phi 0\n");
			AbstractVertex* center=shared[0];
			int StarIndex;
			getStarFromPointer(center,StarIndex);
			vcg::Point2<ScalarType> UVs0,UVs1,UVs2;
			vcg::Point2<ScalarType> UV0=f->V(0)->T().P();
			vcg::Point2<ScalarType> UV1=f->V(1)->T().P();
			vcg::Point2<ScalarType> UV2=f->V(2)->T().P();

			GE0(I0,UV0,StarIndex,UVs0);
			GE0(I1,UV1,StarIndex,UVs1);
			GE0(I2,UV2,StarIndex,UVs2);
			assert((UVs0.X()>=-1)&&(UVs0.Y()>=-1)&&(UVs0.X()<=1)&&(UVs0.Y()<=1));
			assert((UVs1.X()>=-1)&&(UVs1.Y()>=-1)&&(UVs1.X()<=1)&&(UVs1.Y()<=1));
			assert((UVs2.X()>=-1)&&(UVs2.Y()>=-1)&&(UVs2.X()<=1)&&(UVs2.Y()<=1));

			///interpolate star value
			vcg::Point2<ScalarType> UV_interp=bary3D.X()*UVs0+bary3D.Y()*UVs1+bary3D.Z()*UVs2;
			inv_GE0(StarIndex,UV_interp,I,UV);
			Clamp(UV);
			assert((UV.X()>=0)&&(UV.Y()>=0)&&(UV.X()<=1)&&(UV.Y()<=1)&&(UV.X()+UV.Y()<=1+eps));
			return;
		}
		else			///use the diamond domain
		{
			//printf("phi 1\n");
			//std::set<AbstractVertex*>::iterator it=temp2.begin();
			//std::vector<AbstractVertex*>::iterator it=shared[0];
			AbstractVertex* v0=shared[0];
			//it++;
			AbstractVertex* v1=shared[1];
			int EdgeIndex;
			getDiamondFromPointer(v0,v1,EdgeIndex);

			vcg::Point2<ScalarType> UVd0,UVd1,UVd2;
			vcg::Point2<ScalarType> UV0=f->V(0)->T().P();
			vcg::Point2<ScalarType> UV1=f->V(1)->T().P();
			vcg::Point2<ScalarType> UV2=f->V(2)->T().P();

			GE1(I0,UV0,EdgeIndex,UVd0);
			GE1(I1,UV1,EdgeIndex,UVd1);
			GE1(I2,UV2,EdgeIndex,UVd2);

			///interpolate star value
			vcg::Point2<ScalarType> UV_interp=bary3D.X()*UVd0+bary3D.Y()*UVd1+bary3D.Z()*UVd2;
			inv_GE1(EdgeIndex,UV_interp,I,UV);
			Clamp(UV);
			assert((UV.X()>=0)&&(UV.Y()>=0)&&(UV.X()<=1)&&(UV.Y()<=1)&&(UV.X()+UV.Y()<=1+eps));
			assert((I==I0)||(I==I1)||(I==I2));
			return;
		}

	}

	///given the I and UV coordinates return the face and barycentric coords
	///return the domain used for interpolation 0=face 1=half diam 2=half star
	int Theta(const int &I,
		const vcg::Point2<ScalarType> &UV,
		std::vector<ParamFace*> &face,
		std::vector<CoordType> &baryVal)
	{
		#ifndef NDEBUG
		const ScalarType eps=(ScalarType)0.0001;
		#endif
		assert(UV.X()<=1);
		assert(UV.Y()<=1);
		assert(UV.X()>=0);
		assert(UV.Y()>=0);
		//printf("%f,%f \n",UV.X(),UV.Y());
		assert((UV.X()+UV.Y())<=(1+eps));
		//printf("%f,%f \n",UV.X(),UV.Y());
		///FACE SEARCH
		///first test if is in the face domain 
		//printf("face\n");
		bool found=false;
		int indexFace=I;
		vcg::Point2<ScalarType> UVFace;
		GE2(indexFace,UV,UVFace);
		found=face_meshes[indexFace].Project(UVFace,face,baryVal);
		if (found)
			return 0;
		
		///DIAMOND SEARCH
		///search in the diamond domain
		//printf("diam\n");
		int indexDiam=getHDiamIndex(I,UV);///get diamond index
		vcg::Point2<ScalarType> UVDiam;
		///transform UV coordids in diamond
		GE1(I,UV,indexDiam,UVDiam);
		found=diamond_meshes[indexDiam].Project(UVDiam,face,baryVal);
		if (found)
			return 1;

		///STAR SEARCH
		//printf("star\n");
		int indexStar=getHStarIndex(I,UV);///get star index
		vcg::Point2<ScalarType> UVStar;
		///transform UV coords in star
		GE0(I,UV,indexStar,UVStar);
		found=star_meshes[indexStar].Project(UVStar,face,baryVal);
		if (!found)
		{
			//printf("\n problems projecting u,v:%lf,%lf (RESOLVED)\n",UV.X(),UV.Y());
#ifndef NDEBUG
			bool found=star_meshes[indexStar].getClosest(UVStar,face,baryVal);
			assert(found);
#else
			star_meshes[indexStar].getClosest(UVStar,face,baryVal);
#endif
		}
		return 2;
	}

	

	///return the coordinate on the half star domain
	bool GE0(const int &I,
		const vcg::Point2<ScalarType> &UV,
		const int &StarIndex,
		vcg::Point2<ScalarType> &UVHstar)
	{
		//int vertStar=2;
		CoordType bary=CoordType(UV.X(),UV.Y(),1-UV.X()-UV.Y());

		///then transform to the star domain

		///get star mesh
		AbstractMesh* star_domain=star_meshes[StarIndex].domain;
		int LocalIndex=star_meshes[StarIndex].Global2Local(I);
		if (LocalIndex==-1)
			return false;

		GetUV<AbstractMesh>(&star_domain->face[LocalIndex],bary,UVHstar.X(),UVHstar.Y());
		return true;
	}

	///return the coordinate on the half star domain
	bool inv_GE0(int &StarIndex,
		vcg::Point2<ScalarType> &UVHstar,
		int &I,
		vcg::Point2<ScalarType> &UV)
	{
		AbstractMesh* star_domain=star_meshes[StarIndex].domain;
		CoordType bary;
		int index;
		bool done=GetBaryFaceFromUV(*star_domain,UVHstar.X(),UVHstar.Y(),bary,index);
		UV.X()=bary.X();
		UV.Y()=bary.Y();
		I=star_meshes[StarIndex].Local2Global(index);
		return done;
	}


	void InterpParam(vcg::Point2f p1,vcg::Point2f p2,vcg::Point2f p3,vcg::Point2f p_test,
					 ScalarType &b1,ScalarType &b2,ScalarType &b3)
	{
		ScalarType x0=p_test.X();
		ScalarType y0=p_test.Y();
		ScalarType x1=p1.X();
		ScalarType y1=p1.Y();
		ScalarType x2=p2.X();
		ScalarType y2=p2.Y();
		ScalarType x3=p3.X();
		ScalarType y3=p3.Y();
		
		ScalarType b0 =  (x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1);
		b1 = ((x2 - x0) * (y3 - y0) - (x3 - x0) * (y2 - y0)) / b0;
		b2 = ((x3 - x0) * (y1 - y0) - (x1 - x0) * (y3 - y0)) / b0;
		b3 = ((x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0)) / b0;
	}

	///return the coordinate on the half diamond domain
	void GE1(const int &I,
		const vcg::Point2<ScalarType> &UV,
		const int &DiamIndex,
		vcg::Point2<ScalarType> &UVDiam)
	{
		///get the right edge index
		CoordType bary=CoordType(UV.X(),UV.Y(),1-UV.X()-UV.Y());
		
		///then transform to the half-diamond domain
		///get egde mesh
		AbstractMesh* diam_domain=diamond_meshes[DiamIndex].domain;
		int LocalIndex=diamond_meshes[DiamIndex].Global2Local(I);
		if(LocalIndex!=-1)
		{
			GetUV<AbstractMesh>(&diam_domain->face[LocalIndex],bary,UVDiam.X(),UVDiam.Y());
			return;
		}
		///if ! found seach in the star space
		else
		{
			AbstractFace *f_diam=&diam_domain->face[0];
			int indexF0=diamond_meshes[DiamIndex].Local2Global(0);
			int indexF1=diamond_meshes[DiamIndex].Local2Global(1);

			//getFaceIndexFromPointer(f0,indexF0);
			vcg::Point2<ScalarType> UVHstar;
			//int StarIndex=getHStarIndex(I,UV);
			int StarIndex=getHStarIndex(I,UV);
			///get star coordinates
#ifndef NDEBUG
			bool found=GE0(I,UV,StarIndex,UVHstar);
			assert(found);
#else
			GE0(I,UV,StarIndex,UVHstar);
#endif

			///then find which face is in the star
			int indexParam;
			int indexParam0=star_meshes[StarIndex].Global2Local(indexF0);
			int indexParam1=star_meshes[StarIndex].Global2Local(indexF1);
			if(indexParam0==-1)
				indexParam=indexParam1;
			else
				indexParam=indexParam0;
			
			AbstractFace *f_param=&star_meshes[StarIndex].domain->face[indexParam];
			vcg::Point2f p0=f_param->V(0)->T().P();
			vcg::Point2f p1=f_param->V(1)->T().P();
			vcg::Point2f p2=f_param->V(2)->T().P();
			
			vcg::Point3f coord;
			//bool inside=vcg::InterpolationParameters2<float>(p0,p1,p2,UVHstar,coord);
			InterpParam(p0,p1,p2,UVHstar,coord.X(),coord.Y(),coord.Z());
			///finally reinterpolate in diamond space
			UVDiam=coord.X()*f_diam->V(0)->T().P()+coord.Y()*f_diam->V(1)->T().P()+coord.Z()*f_diam->V(2)->T().P();
			return;
		}
		//CoordType bary=CoordType(UV.X(),UV.Y(),1-UV.X()-UV.Y());
		
	}

	///given the diamond coordinates return the coordinates in the parametrization space
	void inv_GE1(const int &DiamIndex,
		const vcg::Point2<ScalarType> &UVDiam,
		int &I,
		vcg::Point2<ScalarType> &UV)
	{
		AbstractMesh* diam_domain=diamond_meshes[DiamIndex].domain;
		CoordType bary;
		int index;
#ifndef NDEBUG
		bool done=GetBaryFaceFromUV(*diam_domain,UVDiam.X(),UVDiam.Y(),bary,index);
		assert(done);
#else
		GetBaryFaceFromUV(*diam_domain,UVDiam.X(),UVDiam.Y(),bary,index);
#endif
		UV.X()=bary.X();
		UV.Y()=bary.Y();
		I=diamond_meshes[DiamIndex].Local2Global(index);
		
	}
	
	///given the diamond coordinates AS A QUAD return the coordinates in the parametrization space
	void inv_GE1Quad(const int &DiamIndex,
					const vcg::Point2<ScalarType> &UVQuad,
					int &I,
					vcg::Point2<ScalarType> &UV)
	{
		assert((UVQuad.X()>=0)&&(UVQuad.X()<=1));
		assert((UVQuad.Y()>=0)&&(UVQuad.Y()<=1));

		///get the abstract mesh
		AbstractMesh* diam_domain=diamond_meshes[DiamIndex].domain;
		///get the 4 vertices on the full diamond

		///transform in diamond coordinates
		vcg::Point2<ScalarType> c0=vcg::Point2<ScalarType>(0,(ScalarType)-0.5);
		vcg::Point2<ScalarType> c1=vcg::Point2<ScalarType>((sqrt((ScalarType)3.0)/(ScalarType)6.0),0);
		vcg::Point2<ScalarType> c2=vcg::Point2<ScalarType>(0,(ScalarType)0.5);
		vcg::Point2<ScalarType> c3=vcg::Point2<ScalarType>(-(sqrt((ScalarType)3.0)/(ScalarType)6.0),0);

		///get 2 directions
		vcg::Point2<ScalarType> v0=c1-c0;
		//v0.Normalize();
		vcg::Point2<ScalarType> v1=c3-c0;
		//v1.Normalize();
		
		///value in diamond coordinates
		vcg::Point2<ScalarType> UVDiam=c0+UVQuad.X()*v0;
		vcg::Point2<ScalarType> diry=UVQuad.Y()*v1;
		UVDiam=UVDiam+diry;
		
		//printf("Diamond: %d,%f,%f \n",DiamIndex,UVDiam.X(),UVDiam.Y());

		int index;
		CoordType bary;
#ifndef NDEBUG
		bool done=GetBaryFaceFromUV(*diam_domain,UVDiam.X(),UVDiam.Y(),bary,index);
#else
		GetBaryFaceFromUV(*diam_domain,UVDiam.X(),UVDiam.Y(),bary,index);
#endif
		UV.X()=bary.X();
		UV.Y()=bary.Y();

		I=diamond_meshes[DiamIndex].Local2Global(index);
		assert(done);
	}
	
	///TO RESCALE?
	///given the coordinates in the parametrization space return return the coordinates AS A QUAD 
	void GE1Quad(const int &I,
				 const vcg::Point2<ScalarType> &UV,
				 int &DiamIndex,
				 vcg::Point2<ScalarType> &UVQuad)
	{
		///first get the right half diamond index
		DiamIndex=getHDiamIndex(I,UV);
		
		///transform in diamond space
		vcg::Point2<ScalarType> UVDiam;
		GE1(I,UV,DiamIndex,UVDiam);
		
		///transform in diamond coordinates
		vcg::Point2<ScalarType> c0=vcg::Point2<ScalarType>(0,(ScalarType)-0.5);
		vcg::Point2<ScalarType> c1=vcg::Point2<ScalarType>((sqrt((ScalarType)3.0)/(ScalarType)6.0),0);
		vcg::Point2<ScalarType> c2=vcg::Point2<ScalarType>(0,(ScalarType)0.5);
		vcg::Point2<ScalarType> c3=vcg::Point2<ScalarType>(-(sqrt((ScalarType)3.0)/(ScalarType)6.0),0);

		///get 2 directions
		vcg::Point2<ScalarType> v0=c1-c0;
		//v0.Normalize();
		vcg::Point2<ScalarType> v1=c3-c0;
		//v1.Normalize();
		
		///translate respect to zero
		UVDiam-=c0;

		///then transform in new coordspace
		UVQuad.X()=v0.X()*UVDiam.X()+v0.Y()*UVDiam.Y();
		UVQuad.Y()=v1.X()*UVDiam.X()+v1.Y()*UVDiam.Y();

	}
	
	///given the diamond coordinates return return the coordinates AS A QUAD 
	void GE1Quad(const int &/*DiamIndex*/,
				 const vcg::Point2<ScalarType> &UVDiam,
				 vcg::Point2<ScalarType> &UVQuad)
	{
		//assert(UVDiam.Y()<0);
		///transform in diamond coordinates
		vcg::Point2<ScalarType> c0=vcg::Point2<ScalarType>(0,(ScalarType)-0.5);
		vcg::Point2<ScalarType> c1=vcg::Point2<ScalarType>((sqrt((ScalarType)3.0)/(ScalarType)6.0),0);
		vcg::Point2<ScalarType> c2=vcg::Point2<ScalarType>(0,(ScalarType)0.5);
		vcg::Point2<ScalarType> c3=vcg::Point2<ScalarType>(-(sqrt((ScalarType)3.0)/(ScalarType)6.0),0);
		
		///get 2 directions
		vcg::Point2<ScalarType> v0=c1-c0;
		vcg::Point2<ScalarType> v1=c3-c0;
		
		///translate respect to zero
		vcg::Point2<ScalarType> temp=UVDiam;
		//temp=c3;
		temp-=c0;
		ScalarType det=(ScalarType)1.0/(v0.X()*v1.Y()-v1.X()*v0.Y());
		///then transform in new coordspace
		UVQuad.X()=v1.Y()*temp.X()-v1.X()*temp.Y();
		UVQuad.Y()=-v0.Y()*temp.X()+v0.X()*temp.Y();
		UVQuad.X()*=det;
		UVQuad.Y()*=det;
	/*	printf("UVtr1=%f, %f \n",UVQuad.X(),UVQuad.Y());
		system("pause");*/
	}

	///return the coordinate on the face domain
	void GE2(const int &I,
		const vcg::Point2<ScalarType> &UV,
		vcg::Point2<ScalarType> &UVFace)
	{
		///get the right edge index
		CoordType bary=CoordType(UV.X(),UV.Y(),1-UV.X()-UV.Y());
		
		///then transform to the face domain
		///get face mesh
		AbstractMesh* face_domain=face_meshes[I].domain;
		AbstractFace* f=&(face_domain->face[0]);

		UVFace=bary.X()*f->V(0)->T().P()+bary.Y()*f->V(1)->T().P()+bary.Z()*f->V(2)->T().P();
	}

	///given the face coordinates return the coordinates in the parametrization space
	void inv_GE2(const int &FaceIndex,
		const vcg::Point2<ScalarType> &UVFace,
		vcg::Point2<ScalarType> &UV)
	{
		AbstractMesh* face_domain=face_meshes[FaceIndex].domain;
		CoordType bary;
		int index;
#ifndef NDEBUG
		bool done=GetBaryFaceFromUV(*face_domain,UVFace.X(),UVFace.Y(),bary,index);
		assert(done);
#else
		GetBaryFaceFromUV(*face_domain,UVFace.X(),UVFace.Y(),bary,index);
#endif
		UV.X()=bary.X();
		UV.Y()=bary.Y();

	}

	void Update()
	{
		UpdateTopologies(abstract_mesh);
		UpdateTopologies(param_mesh);
		float fix_num=sqrt((ScalarType)3.0)/(ScalarType)4.0;

		int edge_count=0;
		///cont number of edges
		for (unsigned int i=0;i<abstract_mesh->face.size();i++)
		{
			if (!(abstract_mesh->face[i].IsD()))
			{
				AbstractFace *f0=&abstract_mesh->face[i];
				//for each edge
				for (int j=0;j<3;j++)
				{
					AbstractFace * f1=f0->FFp(j);
					if (f1>f0)
						edge_count++;
				}
			}
		}

		Area3d=vcg::tri::Stat<ParamMesh>::ComputeMeshArea(*param_mesh);
		AbstractArea=(ScalarType)abstract_mesh->fn*fix_num;
		
		face_to_vert.clear();
		star_meshes.clear();
		face_meshes.clear();
		diamond_meshes.clear();
		star_meshes.resize(abstract_mesh->vn);
		face_meshes.resize(abstract_mesh->fn);
		diamond_meshes.resize(edge_count);
		InitFaceToVert();
		InitFace();
		InitDiamond();
		InitStar();
		Test();
	}

	void Init(AbstractMesh * _abstract_mesh,
			  ParamMesh	 * _param_mesh)
	{
		
		abstract_mesh=_abstract_mesh;
		param_mesh=_param_mesh;

		//UpdateTopologies(abstract_mesh);
		//UpdateTopologies(param_mesh);
		
		Update();
	}

	

	//void AreaDistorsion(ParamFace *f,ScalarType &distorsion,ScalarType &area_3d)
	//{
	//	const float epsilon=0.000001f;
	//	const float maxRatio=10.f;
	//	int indexDomain;
	//	vcg::Point2f UV0,UV1,UV2;
	//	int domainType=InterpolationSpace(f,UV0,UV1,UV2,indexDomain);

	//	
	//	area_3d=((f->P(1)-f->P(0))^(f->P(2)-f->P(0))).Norm()/2.0;
	//	ScalarType area_2d=((UV1-UV0)^(UV2-UV0))/2.0;
	//	area_3d/=Area3d;
	//	area_2d/=AbstractArea;

	//	if (fabs(area_2d)<epsilon)area_2d=epsilon;
	//	if (fabs(area_3d)<epsilon)area_3d=epsilon;

	//	ScalarType r0=area_3d/area_2d;
	//	ScalarType r1=area_2d/area_3d;
	//	if (r0>maxRatio)r0=maxRatio;
	//	if (r1>maxRatio)r1=maxRatio;

	//	distorsion=((r0+r1)/2.0)-1.0;
	//}
	//
	//void AngleDistorsion(ParamFace *f,ScalarType &distortion)
	//{
	//	const float epsilon=0.000001f;
	//	ScalarType area_3d=((f->P(1)-f->P(0))^(f->P(2)-f->P(0))).Norm();
	//	int indexDomain;
	//	vcg::Point2f UV0,UV1,UV2;
	//	int domainType=InterpolationSpace(f,UV0,UV1,UV2,indexDomain);
	//	ScalarType area_2d=fabs((UV1-UV0)^(UV2-UV0));
	//	ScalarType a2=(f->P(1)-f->P(0)).SquaredNorm();
	//	ScalarType b2=(f->P(2)-f->P(1)).SquaredNorm();
	//	ScalarType c2=(f->P(0)-f->P(2)).SquaredNorm();
	//	ScalarType cot_a=((UV2-UV1)*(UV0-UV2));
	//	ScalarType cot_b=((UV0-UV2)*(UV1-UV0));
	//	ScalarType cot_c=((UV1-UV0)*(UV2-UV1));

	//	ScalarType num;
	//	if ((fabs(area_2d)<epsilon)||(fabs(area_3d)<epsilon))
	//		num=0;
	//	else
	//		num=(cot_a*a2+cot_b*b2+cot_c*c2)/area_2d;

	//	distortion=fabs(num/(Area3d*4.0));
	//}

	//ScalarType AreaDistorsion()
	//{
	//	ScalarType sum=0;
	//	for (int i=0;i<param_mesh->face.size();i++)
	//	{
	//		float area3d=0;
	//		ScalarType distorsion=0;
	//		AreaDistorsion(&param_mesh->face[i],distorsion,area3d);
	//		sum+=distorsion*area3d;
	//	}
	//	return(fabs(sum));
	//}
	//
	//ScalarType AngleDistorsion()
	//{
	//	ScalarType sum=0;
	//	for (int i=0;i<param_mesh->face.size();i++)
	//	{
	//		ScalarType distorsion=0;
	//		AngleDistorsion(&param_mesh->face[i],distorsion);
	//		sum+=distorsion;
	//	}
	//	return (sum)-1.0;
	//}

	//ScalarType AggregateDistorsion()
	//{
	//	ScalarType d0=AreaDistorsion();
	//	ScalarType d1=AngleDistorsion();
	//	ScalarType ret=geomAverage<double>(d0+1.0,d1+1.0,3,1)-1;
	//	return ret;
	//}

	//ScalarType  L2Error()
	//{
	//	///equilateral triagle
	//	vcg::Point2f p0(-0.5,0.0);
	//	vcg::Point2f p1(0.5,0.0);
	//	vcg::Point2f p2(0,0.866025f);
	//	ParamMesh::FaceIterator Fi;

	//	float sum=0;
	//	float A3dtot=0;
	//	float A2dtot=0;
	//	for (Fi=param_mesh->face.begin();Fi!=param_mesh->face.end();Fi++)
	//	{
	//		if (!(*Fi).IsD())
	//		{
	//			ParamFace *f=&(*Fi);
	//			CoordType q1=(*Fi).V(0)->P();
	//			CoordType q2=(*Fi).V(1)->P();
	//			CoordType q3=(*Fi).V(2)->P();
	//			///map to equilateral triangle
	//			/*vcg::Point2f UV1=p0*(*Fi).V(0)->Bary.X()+p1*(*Fi).V(0)->Bary.Y()+p2*(*Fi).V(0)->Bary.Z();
	//			vcg::Point2f UV2=p0*(*Fi).V(1)->Bary.X()+p1*(*Fi).V(1)->Bary.Y()+p2*(*Fi).V(1)->Bary.Z();
	//			vcg::Point2f UV3=p0*(*Fi).V(2)->Bary.X()+p1*(*Fi).V(2)->Bary.Y()+p2*(*Fi).V(2)->Bary.Z();*/
	//			vcg::Point2f UV1,UV2,UV3;
	//			int indexDomain;
	//			int domainType=InterpolationSpace(f,UV1,UV2,UV3,indexDomain);
	//			ScalarType s1=UV1.X();
	//			ScalarType t1=UV1.Y();
	//			ScalarType s2=UV2.X();
	//			ScalarType t2=UV2.Y();
	//			ScalarType s3=UV3.X();
	//			ScalarType t3=UV3.Y();
	//			ScalarType A=fabs(((s2-s1)*(t3-t1)-(s3-s1)*(t2-t1))/2.0);
	//			if (A<0.00001)
	//				A=0.00001;
	//			ScalarType A3d=((q2 - q1) ^ (q3 - q1)).Norm()/2.0;
	//			A3dtot+=A3d;
	//			A2dtot+=A;
	//			CoordType Ss=(q1*(t2-t3)+q2*(t3-t1)+q3*(t1-t2))/(2.0*A);
	//			CoordType St=(q1*(s3-s2)+q2*(s1-s3)+q3*(s2-s1))/(2.0*A);
	//			ScalarType a=Ss*Ss;
	//			ScalarType b=Ss*St;
	//			ScalarType c=St*St;
	//			ScalarType L2=sqrt((a+c)/2.0);
	//			sum+=L2*L2*A3d;
	//		}
	//}
	//sum=sqrt(sum/A3dtot)*sqrt(A2dtot/A3dtot);
	//return sum;
	//}

	/*void PrintAttributes()
	{
		printf("\n STATISTICS \n"),
		printf("AREA	   distorsion:%lf ;\n",AreaDistorsion());
		printf("ANGLE	   distorsion:%lf ;\n",AngleDistorsion());
		printf("AGGREGATE  distorsion:%lf ;\n",AggregateDistorsion());
		printf("L2 STRETCH efficiency:%lf ;\n",L2Error());
	}*/

	AbstractMesh *&AbsMesh(){return abstract_mesh;} 
	ParamMesh	 *&ParaMesh(){return param_mesh;} 

};


#endif