/****************************************************************************
* MeshLab                                                           o o     *
* A versatile mesh processing toolbox                             o     o   *
*                                                                _   O  _   *
* Copyright(C) 2005                                                \/)\/    *
* Visual Computing Lab                                            /\/|      *
* ISTI - Italian National Research Council                           |      *
*                                                                    \      *
* All rights reserved.                                                      *
*                                                                           *
* This program is free software; you can redistribute it and/or modify      *   
* it under the terms of the GNU General Public License as published by      *
* the Free Software Foundation; either version 2 of the License, or         *
* (at your option) any later version.                                       *
*                                                                           *
* This program is distributed in the hope that it will be useful,           *
* but WITHOUT ANY WARRANTY; without even the implied warranty of            *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
* GNU General Public License (http://www.gnu.org/licenses/gpl.txt)          *
* for more details.                                                         *
*                                                                           *
****************************************************************************/

#include <QtGui>

#include <math.h>
#include <stdlib.h>
#include <time.h>

#include <meshlab/meshmodel.h>
#include <meshlab/interfaces.h>

#include <vcg/complex/trimesh/append.h>
#include <vcg/complex/trimesh/clean.h>
#include <vcg/complex/trimesh/update/normal.h>
#include <vcg/complex/trimesh/update/bounding.h>

#include <wrap/io_trimesh/export_vmi.h>

#include "filter_plymc.h"
#include "plymc.h"
#include "simplemeshprovider.h"

using namespace vcg;

// Constructor usually performs only two simple tasks of filling the two lists 
//  - typeList: with all the possible id of the filtering actions
//  - actionList with the corresponding actions. If you want to add icons to your filtering actions you can do here by construction the QActions accordingly

PlyMCPlugin::PlyMCPlugin() 
{ 
    typeList << FP_PLYMC;
  
  foreach(FilterIDType tt , types())
	  actionList << new QAction(filterName(tt), this);
}

// ST() must return the very short string describing each filtering action 
// (this string is used also to define the menu entry)
const QString PlyMCPlugin::filterName(FilterIDType filterId) const 
{
  switch(filterId) {
        case FP_PLYMC :  return QString("VCG Reconstruction");
		default : assert(0); 
	}
}

// Info() must return the longer string describing each filtering action 
// (this string is used in the About plugin dialog)
const QString PlyMCPlugin::filterInfo(FilterIDType filterId) const
{
  switch(filterId) {
        case FP_PLYMC :  return QString("The surface reconstrction algorithm that have been used for a long time inside the ISTI-Visual Computer Lab. It is mostly a variant of the Curless et al.  approach with some original weighting schemes, a different expansion rule, a new aprroach to hole filling through voluem filling.");
		default : assert(0); 
	}
	return QString("Unknown Filter");
}

// The FilterClass describes in which generic class of filters it fits. 
// This choice affect the submenu in which each filter will be placed 
// More than a single class can be choosen.
const PlyMCPlugin::FilterClass PlyMCPlugin::getClass(QAction *a)
{
  switch(ID(a))
	{
        case FP_PLYMC :  return MeshFilterInterface::Remeshing;
		default : assert(0); 
	}
	return MeshFilterInterface::Generic;
}

// This function define the needed parameters for each filter. Return true if the filter has some parameters
// it is called every time, so you can set the default value of parameters according to the mesh
// For each parmeter you need to define, 
// - the name of the parameter, 
// - the string shown in the dialog 
// - the default value
// - a possibly long string describing the meaning of that parameter (shown as a popup help in the dialog)
void PlyMCPlugin::initParameterSet(QAction *action,MeshModel &m, RichParameterSet & parlst) 
{
	 switch(ID(action))	 {
        case FP_PLYMC :
          parlst.addParam(new RichAbsPerc("voxSize",m.cm.bbox.Diag()/100.0,0,m.cm.bbox.Diag(),"Voxel Side", "VoxelSide"));
          parlst.addParam(new RichInt("subdiv",1,"SubVol Splitting","the level of recursive splitting of the volume"));
          parlst.addParam(new RichInt("geodesic",true,"Geodesic Weighting"," "));
          parlst.addParam(new RichBool("openResult",true,"Show Result","if not checked the result is only saved into the current directory"));
break;
		default : assert(0); 
	}
}

// The Real Core Function doing the actual mesh processing.
// Move Vertex of a random quantity
bool PlyMCPlugin::applyFilter(QAction */*filter*/, MeshDocument &md, RichParameterSet & par, vcg::CallBackPos * /*cb*/)
{
    MeshModel &m=*(md.mm());
		srand(time(NULL)); 
		SMesh sm;
	  m.updateDataMask(MeshModel::MM_FACEQUALITY);
		tri::Append<SMesh,CMeshO>::Mesh(sm,m.cm);
	 
		tri::io::ExporterVMI<SMesh>::Save(sm,"pippo.vmi");
    tri::PlyMC<SMesh,SimpleMeshProvider<SMesh> > pmc;
    tri::PlyMC<SMesh,SimpleMeshProvider<SMesh> >::Parameter &p = pmc.p;
		
		int subdiv=par.getInt("subdiv");
		p.IDiv=Point3i(subdiv,subdiv,subdiv);
		p.IPosS=Point3i(0,0,0);
		p.IPosE[0]=p.IDiv[0]-1; p.IPosE[1]=p.IDiv[1]-1; p.IPosE[2]=p.IDiv[2]-1;
		printf("AutoComputing all subVolumes on a %ix%ix%i\n",p.IDiv[0],p.IDiv[1],p.IDiv[2]);

	  p.VoxSize=par.getAbsPerc("voxSize");
		p.NCell=0; 
    pmc.MP.AddSingleMesh("pippo.vmi");
		pmc.Process();
		if(par.getBool("openResult"))
		{
        for(size_t i=0;i<p.OutNameVec.size();++i)
			{
				MeshModel *mp=md.addNewMesh(p.OutNameVec[i].c_str());
				tri::io::ImporterPLY<CMeshO>::Open(mp->cm,p.OutNameVec[i].c_str());
				tri::UpdateBounding<CMeshO>::Box(mp->cm);
				tri::UpdateNormals<CMeshO>::PerVertexPerFace(mp->cm);
			}
		}			
   QFile::remove("pippo.vmi");
	return true;
}

Q_EXPORT_PLUGIN(PlyMCPlugin)