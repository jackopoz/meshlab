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
/****************************************************************************

****************************************************************************/

#ifndef FILTERZIPPERING_H
#define FILTERZIPPERING_H

#include <QObject>

#include <meshlab/meshmodel.h>
#include <meshlab/interfaces.h>

#include <vcg/complex/trimesh/closest.h>
#include <vcg/space/index/grid_static_ptr.h>

#define SAMPLES_PER_EDGE 100

// Polyline (set of consecutive segments)
struct polyline {
    std::vector< vcg::Segment3<CMeshO::ScalarType> > edges;                         //polyline edges
    std::vector< std::pair<int, int> > verts;
};

//Auxiliar info for triangulation
struct aux_info {
    std::vector< polyline > conn;   //Close components (to be triangulated)
    std::vector< polyline > trash;  //Close components (to be deleted)
    std::vector< polyline > border; //Segment intersecting components

    //Add segment c to border
    virtual void AddToBorder( vcg::Segment3<CMeshO::ScalarType> c, std::pair<int, int> v  ) {
        /****Insert new segment****/
        //Search for segment S having S.P0() == c.P1 or S.P1 == c.P0()
        if ( v.first == v.second ) return;
        bool found = false;
        for ( int j = 0; j < border.size(); j ++ ) {
            for ( int i = 0; i < border[j].verts.size() && !found; i ++ ) {
                if ( border[j].verts[i].first == v.second ) { found = true; border[j].edges.insert( border[j].edges.begin() + i, c ); border[j].verts.insert( border[j].verts.begin() + i, v ); }           //insert before i-th element
                if ( border[j].verts[i].second == v.first ) { found = true; border[j].edges.insert( border[j].edges.begin() + i + 1, c ); border[j].verts.insert( border[j].verts.begin() + i + 1, v ); }   //insert after i-th element
            }
        }
        if (!found) {   //Create a new polyline ad add it to the border list
            polyline nwpoly; nwpoly.edges.push_back( c ); nwpoly.verts.push_back( v ); border.push_back( nwpoly );
        } else {
            //Merge consecutive polylines into a single one
            for ( int j = 0; j < border.size(); j ++ )
                for ( int i = j+1; i < border.size(); i ++ ) {
                    if ( border[j].verts.front().first == border[i].verts.back().second ) {
                        border[j].edges.insert( border[j].edges.begin(), border[i].edges.begin(), border[i].edges.end() );
                        border[j].verts.insert( border[j].verts.begin(), border[i].verts.begin(), border[i].verts.end() );
                        border.erase(border.begin() + i);
                    }
                    if ( border[j].verts.back().second == border[i].verts.front().first ) {
                        border[j].edges.insert( border[j].edges.end(), border[i].edges.begin(), border[i].edges.end() );
                        border[j].verts.insert( border[j].verts.end(), border[i].verts.begin(), border[i].verts.end() );
                        border.erase(border.begin() + i);
                    }
                }
        }//end if (!found)
    }//end AddToBorder

    // Add c.component
    virtual void AddCComponent( polyline c ) {
        conn.push_back(c);
    }
    // Add t.component
    virtual void AddTComponent( polyline t ) {
        trash.push_back(t);
    }

    // Set initial t.component
    virtual void Init( CMeshO::FaceType f, int a, int b, int c ) {
        if (!trash.empty()) return;
        polyline tri;   tri.edges.push_back( vcg::Segment3<CMeshO::ScalarType>(f.P(0), f.P(1)) );
                        tri.edges.push_back( vcg::Segment3<CMeshO::ScalarType>(f.P(1), f.P(2)) );
                        tri.edges.push_back( vcg::Segment3<CMeshO::ScalarType>(f.P(2), f.P(0)) );
                        tri.verts.push_back( std::make_pair(a, b) );
                        tri.verts.push_back( std::make_pair(b, c) );
                        tri.verts.push_back( std::make_pair(c, a) );
        AddTComponent( tri );
    }

    // Remove c.component
    virtual void RemoveCComponent( int i ) {
        conn.erase( conn.begin() + i );
    }

    // Remove t.component
    virtual void RemoveTComponent( int i ) {
        trash.erase( trash.begin() + i );
    }

    // Number of c.component
    virtual int nCComponent( ) {
        return conn.size();
    }

    // Number of t.component
    virtual int nTComponent( ) {
        return trash.size();
    }

};//end struct


class FilterZippering : public QObject, public MeshFilterInterface
{
	Q_OBJECT
	Q_INTERFACES(MeshFilterInterface)

        typedef vcg::GridStaticPtr<CMeshO::FaceType, CMeshO::ScalarType > MeshFaceGrid;
        typedef vcg::GridStaticPtr<CMeshO::VertexType, CMeshO::ScalarType > MeshVertGrid;



public:
        enum { FP_ZIPPERING  };

        FilterZippering();
	
        virtual const QString filterName(FilterIDType filter) const;
        virtual const QString filterInfo(FilterIDType filter) const;
	virtual bool autoDialog(QAction *) {return true;}
        virtual void initParameterSet(QAction *,MeshDocument &/*m*/, FilterParameterSet & /*parent*/);
        const int getRequirements(QAction *action);
        virtual bool applyFilter(QAction *filter, MeshDocument &md, FilterParameterSet & /*parent*/, vcg::CallBackPos * cb) ;
        virtual bool applyFilter(QAction *filter, MeshModel &md, FilterParameterSet & /*parent*/, vcg::CallBackPos * cb) {}
	const FilterClass getClass(QAction *a);


private:
        bool checkRedundancy(   CMeshO::FacePointer f,   //face
                                MeshModel *a,            //mesh A
                                MeshFaceGrid &grid,      //grid A
                                CMeshO::ScalarType max_dist );   //Max search distance
        bool isBorderVert( CMeshO::FacePointer f, int i);
        bool isOnBorder( CMeshO::CoordType point, CMeshO::FacePointer f );
        bool isAdjacent( CMeshO::FacePointer f1, CMeshO::FacePointer f2 );
        int  sharesVertex( CMeshO::FacePointer f1, CMeshO::FacePointer f2 );
        void handleBorder( aux_info &info,                                            //Auxiliar information for triangulatio
                           vcg::Point3<CMeshO::ScalarType> N,                        //face normal (useful for proiection)
                           std::vector<CMeshO::CoordType> &coords,          //output coords
                           std::vector<int> &output ); //output v. pointers
        polyline cutComponent(  polyline comp,                                   //Component to be cut
                                polyline border,                                 //border
                                vcg::Matrix44<CMeshO::ScalarType> rot_mat );     //Rotation matrix
        bool isRight (vcg::Point2< float > p1,       //ext.1
                      vcg::Point2< float > p2,       //ext.2
                      vcg::Point2< float > pn);      //query point
        bool debug_v;
        int searchComponent( aux_info &info,                            //Auxiliar info
                             vcg::Segment3<CMeshO::ScalarType> s,       //query segment
                             bool &conn );
        float eps;
};

#endif