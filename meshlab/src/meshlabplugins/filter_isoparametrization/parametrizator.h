#ifndef _PARAMETRIZATOR
#define _PARAMETRIZATOR

#include <defines.h>

#include <param_collapse.h>
#include <param_flip.h>


#include <param_mesh.h>

///auxiliary structures
#include <vcg/complex/trimesh/clean.h>
#include <vcg/complex/trimesh/append.h>
#include <vcg/complex/trimesh/update/topology.h>
#include <vcg/complex/trimesh/update/bounding.h>

// update topology
#include <vcg/complex/trimesh/update/normal.h>
#include <vcg/complex/trimesh/update/edges.h>
#include <vcg/complex/trimesh/update/flag.h>


// local optimization
#include <vcg/complex/local_optimization.h>


#include <local_parametrization.h>
#include <mesh_operators.h>
#include <vcg/space/color4.h>
#include <dual_coord_optimization.h>
#include <float.h>
#include <lm.h>
#include <wrap/io_trimesh/export_ply.h>
//#include <EquilaterizeMesh.h>

#include <opt_patch.h>
#include <local_optimization.h>
#include <statistics.h>
#include <stat_remeshing.h>

//extern int step_global;
//
//int time_opt;

///Flip function
class MyTriEdgeFlip : public ParamEdgeFlip<BaseMesh>{};
class MyTriEdgeCollapse: public ParamEdgeCollapse<BaseMesh>{};

///THIS CLASS MAINTAINS STRUCTURES WICH ARE USED FOR PARAMETRIZATION
///TOGHETHER WITH METHOD FOR COLORIZATION FOR VISUALIZATION PURPOSES

class IsoParametrizator{

public:
	BaseMesh final_mesh;
	BaseMesh base_mesh;
	typedef BaseMesh::ScalarType ScalarType;
	typedef BaseMesh::CoordType CoordType;
	vcg::CallBackPos *cb;
private:
	//Statistic stat;
	/*vcg::GlTrimesh<BaseMesh> glWrap;
	vcg::GlTrimesh<BaseMesh> glWrapAbstract;*/

	template <class MeshType>
	void InitializeStructures(MeshType *mesh)
	{
		///cleaning of initial mesh
		/*int dup = */vcg::tri::Clean<MeshType>::RemoveDuplicateVertex(*mesh);
		/*int unref =  */vcg::tri::Clean<MeshType>::RemoveUnreferencedVertex(*mesh);
		
		///copy
		vcg::tri::Append<BaseMesh,MeshType>::Mesh(base_mesh,*mesh);
		vcg::tri::Append<BaseMesh,MeshType>::Mesh(final_mesh,*mesh);

		///update auxiliary structures
		UpdateStructures(&base_mesh);
		UpdateStructures(&final_mesh);
		vcg::tri::UpdateTopology<BaseMesh>::TestFaceFace(base_mesh);
		vcg::tri::UpdateTopology<BaseMesh>::TestFaceFace(final_mesh);

		///initialization of statistics
		//stat.Init(&base_mesh,&final_mesh);

		///copy original color
		for (unsigned int i=0;i<final_mesh.vert.size();i++)
			final_mesh.vert[i].OriginalCol=final_mesh.vert[i].C();
		
		///set brother vertices and default father
		unsigned int i=0;
		for (i=0;i<base_mesh.vert.size();i++)
		{
			base_mesh.vert[i].brother=&final_mesh.vert[i];
			base_mesh.vert[i].RPos=base_mesh.vert[i].P();
		}

		///area deviation respect to original
		for (i=0;i<base_mesh.face.size();i++)
			base_mesh.face[i].areadelta=0;

		for (i=0;i<final_mesh.vert.size();i++)
			final_mesh.vert[i].area=0;
		
		for (i=0;i<base_mesh.vert.size();i++){
			base_mesh.vert[i].brother=&final_mesh.vert[i];
			final_mesh.vert[i].father=base_mesh.vert[i].VFp();
			CoordType bary=CoordType(0,0,0);
			bary.V(base_mesh.vert[i].VFi())=1;
			final_mesh.vert[i].Bary=bary;
		}

		///initialize area per vertex
		for (i=0;i<final_mesh.face.size();i++)
		{
			BaseFace *f=&final_mesh.face[i];
			ScalarType areaf=(((f->V(1)->P()-f->V(0)->P())^(f->V(2)->P()-f->V(0)->P())).Norm())/2.0;
			f->V(0)->area+=areaf/(ScalarType)3.0;
			f->V(1)->area+=areaf/(ScalarType)3.0;
			f->V(2)->area+=areaf/(ScalarType)3.0;
		}

		///testing everithing is ok
		for (i=0;i<final_mesh.vert.size();i++)
		{
			final_mesh.vert[i].RPos=final_mesh.vert[i].P();
			CoordType test=ProjectPos(final_mesh.vert[i]);
			assert((test-final_mesh.vert[i].P()).Norm()<0.000001);
		}	
	}

	

	void ParaDecimate(const int &targetFaces,
					  const int &interval)
	{
		vcg::LocalOptimization<BaseMesh> DeciSession(base_mesh);
		DeciSession.Init<MyTriEdgeCollapse >();
		MyTriEdgeCollapse::Accuracy()=accuracy;

		int flip_todo=4;
		int next_flip_num=targetFaces;
		std::vector<int> stop_points;
		stop_points.resize(flip_todo+2);///number of flips + save stack point + final stop

		for (int i=0;i<flip_todo;i++)
		{
			next_flip_num=(int)((ScalarType)next_flip_num*flip_factor);
			stop_points[i]=next_flip_num;
		}	
		stop_points[flip_todo]=targetFaces+interval;
		stop_points[flip_todo+1]=targetFaces;

		std::sort(stop_points.begin(),stop_points.end());
		///find the point where save stack is defined 
		int pos_stack=-1;
		for (unsigned int i=0;i<stop_points.size();i++)
		{
			if (stop_points[i]==(targetFaces+interval))
				pos_stack=i;
		}
		assert(pos_stack!=-1);

		bool do_flip=false;
		bool save_status=false;
		int curr_limit=stop_points.size()-1;
		//bool check_status=false;
		while (base_mesh.fn>targetFaces)
		{
			do_flip=false;

			int curr_num=base_mesh.fn;
				
			int next_num;
			if (!save_status)
				next_num=curr_num-1000;
			else
				next_num=curr_num-2;
			
			if ((curr_limit>=0)&&(next_num<stop_points[curr_limit]))
			{
				next_num=stop_points[curr_limit];
				
				if (curr_limit==pos_stack)
					save_status=true;
				else
				if (curr_limit!=0)
					do_flip=true;

				curr_limit--;
			}
			

			DeciSession.SetTargetSimplices(next_num);
			DeciSession.DoOptimization();

			
			if (do_flip)
			{
				FlipStep();
				vcg::tri::UpdateTopology<BaseMesh>::FaceFace(base_mesh);
				vcg::tri::UpdateTopology<BaseMesh>::VertexFace(base_mesh);
				DeciSession.h.clear();
				DeciSession.Init<MyTriEdgeCollapse >();
				if (flip_todo>0)
				{
					next_flip_num=(int)(((ScalarType)next_flip_num)/flip_factor);
					flip_todo--;
				}
			}
			/*#ifndef _MESHLAB
			
			#endif*/
			PrintAttributes();
			if (save_status)
			{
				#ifndef _MESHLAB
				printf("SAVING CURRENT STATUS....face_num: %d\n",curr_num);
				#endif
				SaveCurrentStatus();
			}
		}
	
	}

	///ASSOCIATE SURVIVED VERTEX FROM DECIMATION
	void AssociateRemaining()
	{
		for (unsigned int i=0;i<base_mesh.vert.size();i++)
			if (base_mesh.vert[i].brother!=NULL)
			{
				BaseVertex* v=&base_mesh.vert[i];
				BaseVertex* vb=v->brother;
				vcg::face::VFIterator<BaseFace> vfi2(v);
				BaseFace *f=vfi2.F();
				int index=vfi2.I();
				CoordType bary=CoordType(0,0,0);
				bary[index]=1.f;
				f->vertices_bary.push_back(std::pair<BaseVertex*,vcg::Point3f>(vb,bary));
				vb->father=f;
				vb->Bary=bary;
				v->brother=NULL;
			}
		testParametrization(base_mesh,final_mesh);
	}

	typedef struct vert_para
		{
			ScalarType dist;
			BaseVertex *v;
			bool operator <(const vert_para &other)
			{return (dist>other.dist);}
		};

	void FinalOptimization()
	{
		char ret[200];
		sprintf(ret," PERFORM GLOBAL OPTIMIZATION initializing... ");
		(*cb)(0,ret);

		

		std::vector<vert_para> ord_vertex;
		ord_vertex.resize(base_mesh.vn);
		for (unsigned int i=0;i<base_mesh.vert.size();i++)
			if (!base_mesh.vert[i].IsD())
			{
				BaseVertex *v=&base_mesh.vert[i];
				ScalarType val=StarDistorsion<BaseMesh>(&base_mesh.vert[i]);
				ord_vertex[i].dist=val;
				ord_vertex[i].v=v;
			}
		
		std::sort(ord_vertex.begin(),ord_vertex.end());
		for (unsigned int i=0;i<ord_vertex.size();i++)
				SmartOptimizeStar<BaseMesh>(ord_vertex[i].v);
			
	}


	template <class MeshType>
	bool InitBaseMesh(MeshType *mesh,
		const int &targetFaces,
		const int &interval)
	{
		///INITIALIZATION
		InitializeStructures<MeshType>(mesh);

		///TEST PRECONDITIONS
		bool isOK=Preconditions(base_mesh);
		if (!isOK)
			return false;

		///DECIMATION & PARAMETRIZATION
		ParaDecimate(targetFaces,interval);

		///SET BEST FIND STOP POINT
		SetBestStatus();

		///THEN CLEAR STACK
		ClearStack();

		///LAST FLIP STEP
		FlipStep();

		vcg::tri::UpdateTopology<BaseMesh>::FaceFace(base_mesh);
		vcg::tri::UpdateTopology<BaseMesh>::VertexFace(base_mesh);
		UpdateStructures(&base_mesh);
		#ifndef _MESHLAB
		printf("\n POST LAST FLIP: \n");
		
		PrintAttributes();
		#endif
		///ASSOCIATE REMAINING VERTICES TO ONE FACE
		AssociateRemaining();
		
		///LAST OPTIMIZATION STEP ON STARS
		FinalOptimization();
		#ifndef _MESHLAB	
		printf("\n POST STAR OPT: \n");
		PrintAttributes();
		#endif
		return true;
	}

	///save the current status of the parameterization
	void SaveCurrentStatus()
	{
		///copy
		ParaStack.push_back(ParaInfo());
		ParaStack.back().AbsMesh=new BaseMesh();
		BaseMesh *mesh=ParaStack.back().AbsMesh;
		vcg::tri::Append<BaseMesh,BaseMesh>::Mesh(*mesh,base_mesh);

		///copy rest pos
		int k=0;
		for (unsigned int i=0;i<mesh->vert.size();i++)
			if (!base_mesh.vert[i].IsD())
			{
				mesh->vert[k].RPos=base_mesh.vert[i].RPos;
				k++;
			}

		///copy linking for parameterization informations
		k=0;
		for (unsigned int i=0;i<base_mesh.face.size();i++)
		{
			if (!base_mesh.face[i].IsD())
			{
				int size=base_mesh.face[i].vertices_bary.size();
				mesh->face[k].vertices_bary.resize(size);
				for (int j=0;j<size;j++)
				{
					mesh->face[k].vertices_bary[j].first=base_mesh.face[i].vertices_bary[j].first;
					mesh->face[k].vertices_bary[j].second=base_mesh.face[i].vertices_bary[j].second;
				}
				k++;
			}
		}
		//ParaStack.push_back(ParaInfo());
		/*ParaStack.back().AbsMesh=mesh;*/
		ScalarType valL2=ApproxL2Error(final_mesh);
		ParaStack.back().L2=valL2;
		ScalarType val0=ApproxAreaDistortion<BaseMesh>(final_mesh,mesh->fn);
		ParaStack.back().AreaDist=val0;
		ScalarType val1=ApproxAngleDistortion<BaseMesh>(final_mesh);
		ParaStack.back().AngleDist=val1;
		ParaStack.back().AggrDist=geomAverage<ScalarType>(val0+(ScalarType)1.0,val1+(ScalarType)1.0,3,1)-(ScalarType)1;
		ParaStack.back().Regular=NumRegular<BaseMesh>(*mesh);
		//ParaStack.back().distorsion=geomAverage<double>(val0,val1,3,1);
		ScalarType val2=ParaStack.back().AggrDist;
		ParaStack.back().num_faces=mesh->fn;
		ParaStack.back().ratio=val2*sqrt((ScalarType)mesh->fn);
	}

	void RestoreStatus(const int &index_status)
	{
		///delete current base mesh
		base_mesh.Clear();
		BaseMesh *to_restore=ParaStack[index_status].AbsMesh;

		///restore saved abstract mesh and link
		vcg::tri::Append<BaseMesh,BaseMesh>::Mesh(base_mesh,*to_restore);
		for (unsigned int i=0;i<to_restore->face.size();i++)
		{
			int size=to_restore->face[i].vertices_bary.size();
			base_mesh.face[i].vertices_bary.resize(size);
			for (int j=0;j<size;j++)
			{
				BaseVertex * vert=to_restore->face[i].vertices_bary[j].first;
				CoordType bary=to_restore->face[i].vertices_bary[j].second;
				base_mesh.face[i].vertices_bary[j].first=vert;
				base_mesh.face[i].vertices_bary[j].second=bary;
				vert->father=&base_mesh.face[i];
				vert->Bary=bary;
			}
		}
		UpdateTopologies<BaseMesh>(&base_mesh);

		///copy rest pos
		for (unsigned int i=0;i<to_restore->vert.size();i++)
			base_mesh.vert[i].RPos=to_restore->vert[i].RPos;
			
	}

	///clear the durrent stack
	void ClearStack()
	{
		for (unsigned int i=0;i<ParaStack.size();i++)
			delete(ParaStack[i].AbsMesh);
		ParaStack.clear();
	}

	ScalarType GetStatusVal(const int &index)
	{
		switch (SMode)
		{
		case (SM_Area): return ParaStack[index].AreaDist;
			break;
		case (SM_Angle):return ParaStack[index].AngleDist;
			break;
		case (SM_Corr):return ParaStack[index].AggrDist;
			break;
		case (SM_Reg):return (ScalarType)ParaStack[index].Regular;
			break;
		case (SM_Smallest):return (ScalarType)ParaStack[index].num_faces;
			break;
		case (SM_L2):return ParaStack[index].L2;
			break;
		default:return ParaStack[index].ratio;
			break;
		}
	}

	///set the best value considering the ratio value
	void SetBestStatus()
	{
		int indexmin=0;
		
		double valMin=GetStatusVal(0);
		for (unsigned int i=1;i<ParaStack.size();i++)
		{
			if (ParaStack[i].ratio<valMin)
			{
				valMin=GetStatusVal(i);
				indexmin=i;
			}
#ifndef _MESHLAB
			printf("faces %d, Area %.4f, Angle %.4f, Corr %.4f, non Reg %d,L2 %.4f,Heuristic %.4f\n",
				ParaStack[i].num_faces,
				ParaStack[i].AreaDist,
				ParaStack[i].AngleDist,
				ParaStack[i].AggrDist,
				ParaStack[i].Regular,
				ParaStack[i].L2,
				ParaStack[i].ratio);
#endif
		}
#ifndef _MESHLAB
		printf("Choosen to stop at %d faces \n",ParaStack[indexmin].num_faces);
#endif
		RestoreStatus(indexmin);
	}

public:

	enum StopMode{
		SM_Euristic,
		SM_Area,
		SM_Angle,
		SM_Corr,
		SM_Reg,
		SM_Smallest,
		SM_L2
	};

	///parameters
	StopMode SMode;
	int lower_limit;
	int interval;
	int accuracy;
	//edn parameters

	MyTriEdgeCollapse *next_oper; 
	MyTriEdgeFlip *next_flip;
	BaryOptimizatorDual<BaseMesh> *BaryOptDemo;
	vcg::LocalOptimization<BaseMesh> *FlipSession;
	
	int TimeStepDeci,TimeStepFlip;

	typedef struct ParaInfo
	{
		ScalarType AggrDist;
		ScalarType AreaDist;
		ScalarType AngleDist;
		int Regular;
		int num_faces;
		ScalarType ratio;
		ScalarType L2;
		BaseMesh * AbsMesh;
	};

	std::vector<ParaInfo> ParaStack;

	///PRECONDITIONS
	template <class MeshType>
	bool Preconditions(MeshType &mesh)
	{
		bool b;

		b=vcg::tri::Clean<MeshType>::IsTwoManifoldFace(mesh);
		if (!b)
			return false;

		b=vcg::tri::Clean<MeshType>::IsTwoManifoldVertexFF(mesh);
		if (!b)
			return false;

		b=vcg::tri::Clean<MeshType>::IsSizeConsistent(mesh);
		if (!b)
			return false;

		for (unsigned int i=0;i<mesh.face.size();i++)
			for (unsigned int j=0;j<3;j++)
				if (mesh.face[i].FFi(j)==-1)
					return false;

		return true;
	}
	
	///perform a global optimization step
	void GlobalOptimizeStep()
	{
#ifndef _MESHLAB
	  printf("\n GLOBAL OPTIMIZATION \n");
#endif
	  BaryOptimizatorDual<BaseMesh> BaryOpt;
	  BaryOpt.Init(base_mesh,final_mesh,cb,accuracy);
	  BaryOpt.Optimize();
#ifndef _MESHLAB
	  printf("\n POST DUAL OPT:	\n");
	  PrintAttributes();
#endif
	}

	///PARAMETRIZATION FUNCTIONS 
	///initialize the mesh 
	template <class MeshType>
	bool Parametrize(MeshType *mesh)
	{
		bool done;
		/*time_opt=0;
		int time0=clock();*/
		//vcg::tri::UpdateTopology<MeshType>::VertexFace(*mesh);
		
		done=InitBaseMesh<MeshType>(mesh,lower_limit,interval);
		if (!done)
			return false;
		/*int time1=clock();
#ifndef _MESHLAB
		printf("ELAPSED TIME:%d secs",(int)(time1-time0)/1000);
		printf("OPTIMIZATION TIME:%d percent of total",(int)(time_opt*100/(time1-time0)));
#endif*/

		GlobalOptimizeStep();
		return true;
	}

	///perform one or more flip steps
	void FlipStep()
	{
#ifndef _MESHLAB
		printf("\n STARTING FLIP SESSION \n");
#endif
		FlipSession=new vcg::LocalOptimization<BaseMesh>(base_mesh);
		FlipSession->Init<MyTriEdgeFlip>();	
		/*bool b=*/FlipSession->DoOptimization();
#ifndef _MESHLAB
		printf("\n END FLIP SESSION %d edges \n",FlipSession->nPerfmormedOps);
#endif
		UpdateTopologies(&base_mesh);
	}

	///equilateralize patch lenght and areas
	void EquiPatches()
	{
		PatchesOptimizer<BaseMesh> DomOpt(base_mesh,final_mesh);
		DomOpt.OptimizePatches();
		PrintAttributes();
	}
	
	/// I/O FUNCTIONS
	void SaveMCP(char* filename)
	{
		/*Warp(0);*/
		FILE *f;
		f=fopen(filename,"w+");
		std::map<BaseFace*,int> facemap;
		std::map<BaseVertex*,int> vertexmap;
		typedef std::map<BaseVertex*,int>::iterator iteMapVert;
		typedef std::map<BaseFace*,int>::iterator iteMapFace;

		///add vertices
		fprintf(f,"%d,%d \n",base_mesh.fn,base_mesh.vn);
		int index=0;
		for (unsigned int i=0;i<base_mesh.vert.size();i++)
		{
			BaseVertex* vert=&base_mesh.vert[i];
			if (!vert->IsD())
			{
				vertexmap.insert(std::pair<BaseVertex*,int>(vert,index));
				CoordType pos=vert->P();
				CoordType RPos=vert->RPos;
				fprintf(f,"%f,%f,%f;%f,%f,%f \n",pos.X(),pos.Y(),pos.Z(),RPos.X(),RPos.Y(),RPos.Z());
				index++;
			}
		}

		///add faces
		index=0;
		for (unsigned int i=0;i<base_mesh.face.size();i++)
		{
			BaseFace* face=&base_mesh.face[i];
			if (!face->IsD())
			{
				BaseVertex* v0=face->V(0);
				BaseVertex* v1=face->V(1);
				BaseVertex* v2=face->V(2);
				iteMapVert vertIte;
				vertIte=vertexmap.find(v0);
				assert(vertIte!=vertexmap.end());
				int index0=(*vertIte).second;
				vertIte=vertexmap.find(v1);
				assert(vertIte!=vertexmap.end());
				int index1=(*vertIte).second;
				vertIte=vertexmap.find(v2);
				assert(vertIte!=vertexmap.end());
				int index2=(*vertIte).second;
				assert((index0!=index1)&&(index1!=index2));
				fprintf(f,"%d,%d,%d \n",index0,index1,index2);
				facemap.insert(std::pair<BaseFace*,int>(face,index));
				index++;
			}
		}

		///high resolution mesh
		fprintf(f,"%d,%d \n",final_mesh.fn,final_mesh.vn);

		///add vertices
		vertexmap.clear();
		index=0;
		for (unsigned int i=0;i<final_mesh.vert.size();i++)
		{
			BaseVertex* vert=&final_mesh.vert[i];
			if (!vert->IsD())
			{
				vertexmap.insert(std::pair<BaseVertex*,int>(vert,index));
				CoordType pos=vert->P();
				CoordType bary=vert->Bary;
				BaseFace* father=vert->father;
				iteMapFace IteF=facemap.find(father);
				assert (IteF!=facemap.end());
				int indexface=(*IteF).second;
				fprintf(f,"%f,%f,%f;%f,%f,%f;%d,%d,%d;%d \n",
				pos.X(),pos.Y(),pos.Z(),bary.X(),bary.Y(),bary.Z(),
				vert->OriginalCol.X(),vert->OriginalCol.Y(),vert->OriginalCol.Z(),
				indexface);
				index++;
			}
		}

		///add faces
		for (unsigned int i=0;i<final_mesh.face.size();i++)
		{
			BaseFace* face=&final_mesh.face[i];
			if (!face->IsD())
			{
				BaseVertex* v0=face->V(0);
				BaseVertex* v1=face->V(1);
				BaseVertex* v2=face->V(2);
				iteMapVert vertIte;
				vertIte=vertexmap.find(v0);
				assert(vertIte!=vertexmap.end());
				int index0=(*vertIte).second;
				vertIte=vertexmap.find(v1);
				assert(vertIte!=vertexmap.end());
				int index1=(*vertIte).second;
				vertIte=vertexmap.find(v2);
				assert(vertIte!=vertexmap.end());
				int index2=(*vertIte).second;
				assert((index0!=index1)&&(index1!=index2));
				fprintf(f,"%d,%d,%d \n",index0,index1,index2);
			}
		}
		fclose(f);
	}

	int LoadMCP(char* filename)
	{
		FILE *f=NULL;              
		f=fopen(filename,"r");
		if (f==NULL)
			return -1;
		/*std::map<BaseFace*,int> facemap;
		std::map<BaseVertex*,int> vertexmap;
		typedef std::map<BaseVertex*,int>::iterator iteMapVert;
		typedef std::map<BaseFace*,int>::iterator iteMapFace;*/

		

		///add vertices
		base_mesh.Clear();
		fscanf(f,"%d,%d \n",&base_mesh.fn,&base_mesh.vn);
		base_mesh.vert.resize(base_mesh.vn);
		base_mesh.face.resize(base_mesh.fn);

		for (unsigned int i=0;i<base_mesh.vert.size();i++)
		{
			BaseVertex* vert=&base_mesh.vert[i];
			CoordType pos;
			CoordType RPos;
			fscanf(f,"%f,%f,%f;%f,%f,%f \n",&pos.X(),&pos.Y(),&pos.Z(),&RPos.X(),&RPos.Y(),&RPos.Z());
			vert->P()=pos;
			vert->RPos=RPos;
		}

		

		///add faces
		for (unsigned int i=0;i<base_mesh.face.size();i++)
		{
			BaseFace* face=&base_mesh.face[i];
			if (!face->IsD())
			{
				int index0,index1,index2;
				fscanf(f,"%d,%d,%d \n",&index0,&index1,&index2);
				base_mesh.face[i].V(0)=&base_mesh.vert[index0];
				base_mesh.face[i].V(1)=&base_mesh.vert[index1];
				base_mesh.face[i].V(2)=&base_mesh.vert[index2];
				base_mesh.face[i].group=vcg::Color4b(rand()%200,rand()%200,rand()%200,255);
			}
		}
		
		///high resolution mesh
		fscanf(f,"%d,%d \n",&final_mesh.fn,&final_mesh.vn);
		final_mesh.vert.resize(final_mesh.vn);
		final_mesh.face.resize(final_mesh.fn);

		///add vertices
		for (unsigned int i=0;i<final_mesh.vert.size();i++)
		{
			BaseVertex* vert=&final_mesh.vert[i];
			CoordType pos;
			CoordType bary;
			vcg::Color4b col;
			int index_face;
			int col0,col1,col2;
			fscanf(f,"%f,%f,%f;%f,%f,%f;%d,%d,%d;%d \n",
				  &pos.X(),&pos.Y(),&pos.Z(),
				  &bary.X(),&bary.Y(),&bary.Z(),
				  &col0,&col1,&col2,
				  &index_face);
			vert->P()=pos;
			vert->RPos=pos;
			vert->Bary=bary;
			vert->father=&base_mesh.face[index_face];
			col=vcg::Color4b(col0,col1,col2,255);
			vert->OriginalCol=col;
			/*if (i==final_mesh.vert.size()-1)
			{
				printf("sukka %d \n",index_face);
				char test;
				scanf ("%c",&test);
			}*/

		}

		///add faces
		for (unsigned int i=0;i<final_mesh.face.size();i++)
		{
			//BaseFace* face=&final_mesh.face[i];
			
			int index0,index1,index2;
			fscanf(f,"%d,%d,%d \n",&index0,&index1,&index2);
			final_mesh.face[i].V(0)=&final_mesh.vert[index0];
			final_mesh.face[i].V(1)=&final_mesh.vert[index1];
			final_mesh.face[i].V(2)=&final_mesh.vert[index2];
			
		}
		fclose(f);
		
		///update structures
		UpdateStructures(&base_mesh);
		UpdateStructures(&final_mesh);
		//Colo
		///clear father and bary
		for (unsigned int i=0;i<base_mesh.face.size();i++)
			base_mesh.face[i].vertices_bary.resize(0);

		///set face-vertex link
		for (unsigned int i=0;i<final_mesh.vert.size();i++)
		{
			BaseVertex *v=&final_mesh.vert[i];
			BaseFace *f=v->father;
			CoordType bary=v->Bary;
			f->vertices_bary.push_back(std::pair<BaseVertex *,CoordType>(v,bary));
		}
		
		//ColorByParametrization();
		PrintAttributes();
		//printf("faces:%5d AREA  distorsion:%lf\n",base_mesh.fn,ApproxAreaDistortion<BaseMesh>(final_mesh,base_mesh.fn));
		//printf("faces:%5d ANGLE distorsion:%lf\n",base_mesh.fn,ApproxAngleDistortion<BaseMesh>(final_mesh));
		
		vcg::tri::UpdateBounding<BaseMesh>::Box(final_mesh);

		///area perr vertex
		for (unsigned int i=0;i<final_mesh.vert.size();i++)
			//AuxHPara[i].area=0;
			final_mesh.vert[i].area=0;

		
		for (unsigned int i=0;i<final_mesh.face.size();i++)
		{
			BaseFace *f=&final_mesh.face[i];
			ScalarType areaf=(((f->V(1)->P()-f->V(0)->P())^(f->V(2)->P()-f->V(0)->P())).Norm())/(ScalarType)2.0;
			/*AuxHPara[f->V(0)].area+=areaf/3.0;
			AuxHPara[f->V(1)].area+=areaf/3.0;
			AuxHPara[f->V(2)].area+=areaf/3.0;*/
			f->V(0)->area+=areaf/(ScalarType)3.0;
			f->V(1)->area+=areaf/(ScalarType)3.0;
			f->V(2)->area+=areaf/(ScalarType)3.0;
		}

		return 0;
	}
	
	void ExportMeshes(ParamMesh &para_mesh,AbstractMesh &abs_mesh)
	{
		para_mesh.Clear();
		abs_mesh.Clear();

		///copy meshes
		vcg::tri::Append<AbstractMesh,BaseMesh>::Mesh(abs_mesh,base_mesh);
		vcg::tri::Append<ParamMesh,BaseMesh>::Mesh(para_mesh,final_mesh);

		///copy additional values
		for (unsigned int i=0;i<base_mesh.vert.size();i++)
		{
			assert (!base_mesh.vert[i].IsD());
			abs_mesh.vert[i].RPos=base_mesh.vert[i].RPos;
		}

		for (unsigned int i=0;i<final_mesh.vert.size();i++)
			para_mesh.vert[i].RPos=final_mesh.vert[i].RPos;

		///save in a table face-faces correspondences
		std::map<BaseFace*,int> faceMap;
		for (unsigned int i=0;i<base_mesh.face.size();i++)
			faceMap.insert(std::pair<BaseFace*,int>(&base_mesh.face[i],i));

		///then reassing with new mesh
		for (unsigned int i=0;i<final_mesh.vert.size();i++)
		{
			std::map<BaseFace*,int>::iterator cur  = faceMap.find(final_mesh.vert[i].father);
			CoordType bary=final_mesh.vert[i].Bary;
			int index=(*cur).second;
			para_mesh.vert[i].T().N()=index;
			para_mesh.vert[i].T().U()=bary.X();
			para_mesh.vert[i].T().V()=bary.Y();
		}
	}

	//int draw_mode;
	
	void PrintAttributes()
	{
		int done=(final_mesh.fn-base_mesh.fn);
		int total=(final_mesh.fn-lower_limit);
		ScalarType ratio=(ScalarType)done/total;
		ratio=pow(ratio,3);
		int percent=(int)(ratio*(ScalarType)100);
		ScalarType areaD=ApproxAreaDistortion<BaseMesh>(final_mesh,base_mesh.fn);
		ScalarType angleD=ApproxAngleDistortion<BaseMesh>(final_mesh);
		char ret[200];
		sprintf(ret," GATHERING ABSTRACT DOMAIN faces:%5d AREA  distorsion:%4f ; ANGLE distorsion:%4f ",base_mesh.fn,areaD,angleD);
		(*cb)(percent,ret);
	}

	void SetParameters(vcg::CallBackPos *_cb,
					   const int &_lower_limit=100,
					   const int &_interval=50,
					   StopMode _SMode=SM_Euristic,
					   const int &_accuracy=1)
	{
		lower_limit=_lower_limit;
		interval=_interval;
		accuracy=_accuracy;
		SMode=_SMode;
		cb=_cb;
	}
	
	void getValues(ScalarType &aggregate,
						ScalarType &L2,
						int &n_faces)
	{
	L2=ApproxL2Error(final_mesh);
	ScalarType val0=ApproxAreaDistortion<BaseMesh>(final_mesh,base_mesh.fn);
	ScalarType val1=ApproxAngleDistortion<BaseMesh>(final_mesh);
	aggregate=geomAverage<ScalarType>(val0+1.0,val1+1.0,3,1)-1;
	n_faces=base_mesh.fn;
	}

	IsoParametrizator()
	{
		///parameters
		SMode=SM_Euristic;
		lower_limit=40;
		interval=50;
		accuracy=1;
	}

};

#endif