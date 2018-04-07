//����ʹ������ģ���tuple��ƣ�ʹ��ǰӦ��ʼ��

#include "DataArray.h"
#include "PointLocator.h"
#include "Utility.h"
#include "SurfaceDelaunayUtils.h"
#include <stack>
#include <set>
#include <unordered_set>
#include <memory>
#ifndef DATASTRUCTURE_H
#define DATASTRUCTURE_H

typedef Idtype Facet_Idtype;
typedef std::pair<Cell_Idtype, int>              Facet;
typedef Triple<Vertex_Idtype, Vertex_Idtype, Vertex_Idtype> Vertex_triple;
typedef std::pair<Facet_Idtype,Indextype>        Edge;
typedef std::pair<Facet, int>                    EdgeInFacet;
typedef std::pair<Vertex_Idtype, Vertex_Idtype>  Vertex_pair;

size_t hash_function_pair_int(pair<int, int> p)
{
	return hash<int>()(p.first - p.second);
}
typedef  unordered_set < pair<int, int>, decltype(hash_function_pair_int)* > set_pair_int;

enum Locate_type {
	VERTEX = 0,
	EDGE, //1
	FACET, //2
	CELL, //3
	OUTSIDE_CONVEX_HULL, //4
	OUTSIDE_AFFINE_HULL //5
};

template<typename T,typename T_INFO>
class DataStructure
{

	//HHHHHHHHHHHHHHHHHHHHHHHHH------------------delaunay-------------------HHHHHHHHHHHHHHHHHHHHHHHH
	
	typedef DataArray<Idtype> IdList;
	typedef IdList Vertex_Id_List;
	typedef IdList Cell_Id_List;
	typedef DataArray<T> DataList;
	typedef DataArray<T_INFO> InfoList;
	typedef TypeList<bool> FlagList;


//��ȡ����
public:
	int K_NN;//ȡ��������ڵ�
	int get_vertex_num()
	{
		return VertexUse.get_max_id();
	}
	void get_vertex(Indextype IdV,double* PCoord,bool& Use)
	{
		const double* pp=point(IdV);
		PCoord[0]=pp[0];
		PCoord[1]=pp[1];
		PCoord[2]=pp[2];
	
		Use=VertexUse.get_element(IdV);
	}
	int get_facet_num()
	{
		return FacetUse.get_max_id();
	}
	void get_facet_vertex(Indextype IdF,const Indextype* IdVerF,bool& Use)
	{
		Vertex_triple vtriF=make_vertex_triple(get_facet_cell_link(IdF));
		*IdVerF++=vtriF.first;
		*IdVerF++=vtriF.second;
		*IdVerF=vtriF.third;
		Use=FacetUse.get_element(IdF);
	}



private:
	int Dimension;
	
	//DataList Points;
	PointLocator<T,T_INFO> PointDataSet;
	InfoList CellAttributes;//---------------��δ���壩 ���ε�Ԫ������
	Vertex_Id_List Cells;//Cell�ĳ�ʼ����Ϊ-1
	//IdList CellLocation;//���е�Cell����4���㣬���Բ���Ҫ��λ
	Cell_Id_List Neighbor;
	TypeList<Cell_Idtype> CellIncident;//��vertex������cell
	FlagList VertexUse;//true:����ʹ�ã�flase���Ѿ���ɾ��         ע���ʼ��
	FlagList CellUse;//true:����ʹ�ã�false���Ѿ���ɾ��                  ע���ʼ��
	TypeList<unsigned int> MarkCellConflictState;  //1:conflict;2:boundary;0:clear
	Vertex_Idtype Infinite;
	
	
	TypeList<unsigned int> VisitedForVertexExtractor;
	

	std::stack<Vertex_Idtype> Hole_Vertices;
	std::stack<Cell_Idtype> Hole_Cells; //ɾ����cell

	
	
public:
	//internally used for create_star_3(faster than a tuple)
	struct iAdjacency_info{
		int v1;
		Cell_Idtype v2;
		int v3;
		Cell_Idtype v4;
		int v5;
		int v6;
		iAdjacency_info(){}
		iAdjacency_info(int a1, Cell_Idtype a2, int a3, Cell_Idtype a4, int a5, int a6) :
			v1(a1), v2(a2), v3(a3), v4(a4), v5(a5), v6(a6) {}
		void update_variables(int& a1, Cell_Idtype& a2, int& a3, Cell_Idtype& a4, int& a5, int& a6)
		{
			a1 = v1;
			a2 = v2;
			a3 = v3;
			a4 = v4;
			a5 = v5;
			a6 = v6;
		}
	};

	
	

public:
	DataStructure() :Dimension(-2)
	{
		K_NN=5;
		Cells.init(4,0);
		Neighbor.init(4,0);
	
		FacetNeighbor.init(3, 0);
		FacetSurface.init(2, 0);
		CellSurface.init(4, 0);
		NegCos=0;		
	}

	void init_point_set(int level, double* bounds[3]){ this->PointDataSet.init_point_locator(level,bounds); };
	Vertex_Idtype insert_infinite_vertex(){Infinite=create_vertex();return Infinite;};
	Vertex_Idtype nearest_point_inexact(const T* p){ return PointDataSet.nearest_inexact(p); };
	void insert_first_finite_cell();
	Vertex_Idtype insert_first_finite_cell(Vertex_Idtype &v0, Vertex_Idtype &v1, Vertex_Idtype &v2, Vertex_Idtype &v3,
		Vertex_Idtype Infinite);
	Vertex_Idtype insert_increase_dimension(Idtype star = 0);

	Vertex_Idtype insert_in_edge(Cell_Idtype c,int i,int j);


	template<typename CellIt>
	Vertex_Idtype insert_in_hole( CellIt cell_begin, CellIt cell_end,
		Cell_Idtype begin, int i);

	template<typename CellIt>
	Vertex_Idtype insert_in_hole(CellIt cell_begin, CellIt cell_end,
		Cell_Idtype begin, int i,set_pair_int boundEdge,list<Edge> ConflictBoundEdges,
		std::map<Vertex_Idtype,size_t>& VertexDeletedFacet,
		std::set<Vertex_Idtype>& VertexCreatedFacet,vector<Facet>& NewBoundarySurfacets,
		vector<Facet>& NewConflictSurfacets,bool IsIsolate,bool isInside);

	Cell_Idtype create_star_3(Vertex_Idtype v, Cell_Idtype c,
		int li, int prev_ind2 = -1);

	Cell_Idtype create_star_3(Vertex_Idtype v, Cell_Idtype c,
		int li, int prev_ind2 ,set_pair_int boundEdge,list<Edge> ConflictBoundEdges,
		std::map<Vertex_Idtype,size_t>& VertexDeletedFacet,
		std::set<Vertex_Idtype>& VertexCreatedFacet,vector<Facet>& NewBoundarySurfacets,
		vector<Facet>& NewConflictSurfacets,bool IsoIsolate,bool isInside);

	Cell_Idtype recursive_create_star_3(Vertex_Idtype v, Cell_Idtype c, int li,
		int prev_ind2, int depth);

	Cell_Idtype surface_recursive_create_star_3(Vertex_Idtype v, Cell_Idtype c, int li,
		 int depth,set_pair_int boundEdge,
		 std::map<Vertex_Idtype,size_t>& VertexDeletedFacet,
		 std::set<Vertex_Idtype>& VertexCreatedFacet,
		 std::vector<Facet>& NewCreateBoundFacet,
		 bool label,bool SideChange=false,int prev_ind2=-1);


	void update_surface_connection(Vertex_Idtype V,list<Edge> ConflictBound,vector<Facet> NewBoundFacet,
		vector<Facet>& NewBonudarySurfacets);

	Cell_Idtype non_recursive_create_star_3(Vertex_Idtype v, Cell_Idtype c, int li, int prev_ind2);

	Cell_Idtype create_star_2(Vertex_Idtype v, Cell_Idtype c, int li);

	Facet mirror_facet(Facet f) ;
	EdgeInFacet mirror_edge(EdgeInFacet E);

	template<typename IncidentCellIterator, typename IncidentFacetIterator>
	void incident_cells_3(Vertex_Idtype v, Cell_Idtype d,std::pair<IncidentCellIterator,
														IncidentFacetIterator> it) ;
			

	template<typename OutputIteratorFacet, typename OutputIteratorVertex>
	void incident_cells(Vertex_Idtype v,
		OutputIteratorFacet outputFacets, OutputIteratorVertex outputVertex) ;

	template<typename OutputIteratorCell>
	void incident_cells(Vertex_Idtype v, OutputIteratorCell outputCell) ;

	template<typename OutputIteratorCell,typename OutputIteratorVertex,typename OutputIteratorFacet>
	void incident_cells_mark_side(Vertex_Idtype v, OutputIteratorCell outputCell,
		OutputIteratorVertex outputVertex,OutputIteratorFacet outputFacet,bool& IsIso,bool& PInside) ;

	template<typename OutputIteratorVertex, typename OutputIteratorFacet>
	void adjacent_vertices_facets(Vertex_Idtype v,
		OutputIteratorFacet outputFacets, OutputIteratorVertex vertices) ;
	
	//����vertex,������׷��
	Vertex_Idtype create_vertex()
	{
		//Vertex_Idtype IdV;
		if (!Hole_Vertices.empty())
		{
			Vertex_Idtype IdV = Hole_Vertices.top();
			Hole_Vertices.pop();
			clear_vertex_deleted(IdV);
			clear_vertex_visited(IdV);
			CellIncident.insert_element(IdV,-1);
			return IdV;
		}
		else
		{
			//clear_vertex_visited(IdV);
			VisitedForVertexExtractor.insert_next_element(false);
			CellIncident.insert_next_element(-1);
			return VertexUse.insert_next_element(true);
		}
	};
	//����cell,������׷�ӻ���������ɾ����
	Cell_Idtype create_cell()
	{
		Cell_Idtype* t=new Cell_Idtype[Cells.size_of_tuple()];
		for (int i = 0; i < Cells.size_of_tuple(); i++)
		{
			t[i] = -1;
		}
		if (!Hole_Cells.empty())
		{
			Cell_Idtype IdC = Hole_Cells.top();
			Hole_Cells.pop();
			clear_cell_deleted(IdC);
			clear(IdC);
			Cells.set_tuple(IdC,t);
			Neighbor.set_tuple(IdC, t);

			if (Surface)
			{
				CellSurface.insert_tuple(IdC, t);
				VisitedForCellExtractor.insert_element(IdC, false);
			}

			delete []t;
			return IdC;
		}
		else
		{
			MarkCellConflictState.insert_next_element(0);
			Neighbor.insert_next_tuple(t);
			Cells.insert_next_tuple(t);

			if (Surface)
			{
				CellSurface.insert_next_tuple(t);
				VisitedForCellExtractor.insert_next_element(false);
			}

			delete []t;
			return CellUse.insert_next_element(true);

		}
	}
	Cell_Idtype create_cell(Vertex_Idtype v0, Vertex_Idtype v1, Vertex_Idtype v2, Vertex_Idtype v3)
	{
		Cell_Idtype i = create_cell();
		set_vertex(i,0,v0);
		set_vertex(i, 1, v1);
		set_vertex(i, 2, v2);
		set_vertex(i, 3, v3);
		return i;
	}
	//ȡdimension
	int dimension(){ return Dimension; };
	//�趨dimension
	void set_dimension(int i){ this->Dimension = i; };
	//�ж��Ƿ�Ϊ���޵�
	bool is_infinite(Vertex_Idtype IdV){ return IdV == Infinite; };
	
	//ȡ���IdV������Cell
	Cell_Idtype cell(Vertex_Idtype IdV)
	{
		return CellIncident.get_element(IdV);
	};
	//��IdV��Cell�趨ΪIdC
	void set_cell(Vertex_Idtype IdV, Cell_Idtype IdC)
	{
		CellIncident.insert_element(IdV, IdC);//��insert���ȶ�
	};
	//�趨���ڹ�ϵ��IdC1�ĵ�I1��������IdC2��IdC2�ĵ�I2��������IdC1
	void set_adjacency(Cell_Idtype IdC1, int I1, Cell_Idtype IdC2, int I2)
	{
		//IdC1�ĵ�I1��������IdC2
		set_neighbor(IdC1,I1,IdC2);
		set_neighbor(IdC2,I2,IdC1);
	};
	//ȡIdC�ĵ�I������Cell
	Cell_Idtype neighbor(Cell_Idtype IdC, int I)
	{
		return Neighbor.get_tuple(IdC)[I];
	};
	//�趨����Cell,��IdC1�ĵ�I�������趨ΪIdC2
	void set_neighbor(Cell_Idtype IdC1, int I, Cell_Idtype IdC2)
	{
		Neighbor.insert_tuple_element(IdC1,I,IdC2);
	};
	//��Cell��IdC���ĵ�I�����趨ΪIdV
	void set_vertex(Cell_Idtype IdC, int I, Vertex_Idtype IdV)
	{
		Cells.insert_tuple_element(IdC,I,IdV);
	};
	void set_vertices(Cell_Idtype IdC, Vertex_Idtype IdV0, Vertex_Idtype IdV1, Vertex_Idtype IdV2, Vertex_Idtype IdV3)
	{
		Cells.insert_tuple_element(IdC, 0, IdV0);
		Cells.insert_tuple_element(IdC, 1, IdV1);
		Cells.insert_tuple_element(IdC, 2, IdV2);
		Cells.insert_tuple_element(IdC, 3, IdV3);

	};
	//ȡIdC�ĵ�I����
	Vertex_Idtype vertex(Cell_Idtype IdC, int I)
	{
		return Cells.get_tuple(IdC)[I];
	};

	Cell_Idtype create_face();
	Cell_Idtype create_face(Vertex_Idtype v0, Vertex_Idtype v1,
		Vertex_Idtype v2);
	Cell_Idtype create_face(Cell_Idtype f0, int i0,
		Cell_Idtype f1, int i1,
		Cell_Idtype f2, int i2);
	Cell_Idtype create_face(Cell_Idtype f0, int i0,
		Cell_Idtype f1, int i1);

	void set_point(Vertex_Idtype IdV, const T* p)
	{
		PointDataSet.set_point(IdV,p);
	};
	//ȡ������
	const const T* point(Vertex_Idtype IdV)
	{
		return PointDataSet.get_point(IdV);
	};
	//��IdV��IdC�ĵڼ�����
	int vertex_index(Cell_Idtype IdC, Vertex_Idtype IdV)
	{
		const Vertex_Idtype* vertices = Cells.get_tuple(IdC);
		for (int i = 0; i < Cells.size_of_tuple(); i++)
		{
			if (vertices[i] == IdV)
				return i;
		}
		return -1;
	};
	//ȡCell�е�һ��û�б�ɾ���ĵ�
	Cell_Idtype cells_begin()
	{
		for (int i = 0; i<=Cells.get_max_tuple_id(); i++)
		{
			if (!cell_is_deleted(i))
				return i;
		}
		return -1;
	};
	//�жϵ��Ƿ�ɾ����ɾ�򷵻�true
	bool vertex_is_deleted(Vertex_Idtype IdV)
	{
		if (VertexUse.get_element(IdV) == false)
			return true;
		else
			return false;
	};
	//�ж�Cell�Ƿ�ɾ,��ɾ�򷵻�true
	bool cell_is_deleted(Cell_Idtype IdC)
	{
		if (CellUse.get_element(IdC) == false)
			return true;
		else
			return false;
	};
	//�ж�IdC���Ƿ��е�IdV,���򷵻�true
	bool has_vertex(Cell_Idtype IdC, Vertex_Idtype IdV)
	{
		const Vertex_Idtype* vertices = Cells.get_tuple(IdC);
		for (int i = 0; i < Cells.size_of_tuple(); i++)
		{
			if (vertices[i] == IdV)
				return true;
		}
		return false;
	};
	//���أ������ڵ���index����i
	//������Idc�Ƿ��е�Idv,�����õ����������0��1��2��3����ֵ��I
	bool has_vertex(Cell_Idtype IdC, Vertex_Idtype IdV, int& I)
	{
		const Vertex_Idtype* vertices = Cells.get_tuple(IdC);
		for (int i = 0; i < Cells.size_of_tuple(); i++)
		{
			if (vertices[i] == IdV)
			{
				I = i;
				return true;
			}
				
		}
		return false;
	};
	//��IdC2��IdC1�ĵڼ����ٽ�Cell
	Indextype neighbor_index(Cell_Idtype IdC1, Cell_Idtype IdC2)
	{
		const Cell_Idtype* nei = Neighbor.get_tuple(IdC1);
		for (int i = 0; i < Neighbor.size_of_tuple(); i++)
		{
			if (nei[i] == IdC2)
				return i;
		}
	};

	void reorient();
	//���㹲���ٸ�vertex�������Ѿ���ɾ����,��ʵ����MaxId
	size_t num_of_vertices(){ return VertexUse.get_max_id()+1; };
	//���㹲�ж���cell,�����Ѿ���ɾ����,��ʵ����MaxId
	size_t num_of_cells(){ return CellUse.get_max_id() + 1; };
	//�ı��IdC�ķ���
	void change_orientation(Cell_Idtype IdC) ;
	//��IdC��ǳ�conflict(1)
	void mark_in_conflict(Cell_Idtype IdC){ MarkCellConflictState.insert_element(IdC,1); };
	//�ж�IdC�Ƿ�conflict(1)
	bool is_in_conflict(Cell_Idtype IdC){ return MarkCellConflictState.get_element(IdC)==1; };
	//��IdC��ǳ�boundary(2)
	void mark_on_boundary(Cell_Idtype IdC){ MarkCellConflictState.insert_element(IdC, 2); };
	//�ж�IdC�Ƿ�Ϊboundary(2)
	bool is_on_boundary(Cell_Idtype IdC){ return MarkCellConflictState.get_element(IdC)==2; };
	//��conflict_state��clear(0)
	void clear(Cell_Idtype IdC){ MarkCellConflictState.insert_element(IdC, 0); };
	//���IdC��MarkCellConflictState����Ƿ�clear(0)
	bool is_clear(Cell_Idtype IdC){ return MarkCellConflictState.get_element(IdC) == 0; };
	//delete vertex
	void delete_vertex(Vertex_Idtype IdV)
	{
		Hole_Vertices.push(IdV);
		mark_vertex_deleted(IdV);
	}

	void delete_cell(Cell_Idtype IdC)
	{
		Hole_Cells.push(IdC);
		mark_cell_deleted(IdC);
	}
	
	//ɾ��cells��ʵ����ֻ�Ǳ��
	template<class CellIt>
	void delete_cells(CellIt cell_begin, CellIt cell_end)
	{
		CellIt cit;
		for (cit = cell_begin; cit != cell_end; cit++)
		{
			//�ڴ���HHHHHHHHHHHHHHHHHHHHHHHHHHHHH
			Hole_Cells.push(*cit);
			mark_cell_deleted(*cit);
			//end----------------------------------
		}
	};
	
	//��cell���Ϊɾ��
	void mark_cell_deleted(Cell_Idtype IdC){ CellUse.insert_element(IdC,false); };
	//��vertex���Ϊɾ��
	void mark_vertex_deleted(Vertex_Idtype IdV){ VertexUse.insert_element(IdV, false); };
	//��cell���Ϊδɾ��
	void clear_cell_deleted(Cell_Idtype IdC){ CellUse.insert_element(IdC, true); };
	//��vertex���Ϊδɾ��
	void clear_vertex_deleted(Vertex_Idtype IdV){ VertexUse.insert_element(IdV, true); };
	
	//���Vertex�Ƿ񱻷��ʹ�
	unsigned int visited_for_vertex_extractor(Vertex_Idtype IdV){ return VisitedForVertexExtractor.get_element(IdV); };
	//��VisitedForVertexExtractor��Ϊtrue��Ĭ��Ϊfalse
	void mark_vertex_visited(Vertex_Idtype IdV)
	{
		
		unsigned int ii=VisitedForVertexExtractor.get_element(IdV);
		ii++;
		VisitedForVertexExtractor.insert_element(IdV,ii);
	};
	//��� VisitedForVertexExtractor
	void clear_vertex_visited(Vertex_Idtype IdV){ VisitedForVertexExtractor.insert_element(IdV,0); };

private:
	bool Surface;
	//bool ���ͱ�עÿ��cell�Ƿ�ΪInside��tuple=4
	FlagList CellInsideLabel;
	//��עÿ��cell���ĸ����Ƿ�Ϊ���棬��������Ϊ-1��������Ϊsurface��facet�ı�ʶ��,tuple=4
	DataArray<Facet_Idtype> CellSurface; 
	//����ʶ��˳��洢facet���ԣ�Cell_Idtype,index������ʽ,tuple=2
	DataArray<Cell_Idtype> FacetSurface;    
	//����ʶ��˳��洢facet���ڽ�facet��tuple=3
	DataArray<Facet_Idtype> FacetNeighbor;
	//��עfacet�Ƿ�ɾ��
	FlagList FacetUse;
	//�洢��ɾ����facet
	std::stack<Facet_Idtype> Hole_Facets;
	//��mark cell������³�ȡcell�������
	FlagList VisitedForCellExtractor;
	//��¼������
	std::list<Vertex_Idtype> IsolateVertices;
	//��¼facet�Ƿ�visited����visited�Ĵ���
	TypeList<unsigned int> VisitedForFacetExtractor;

	//���ڲ��Եı���
public:
	size_t NegCos;

public:
	void get_surface_data_structure(DataArray<T>& Point,DataArray<Idtype>& FacetVertex)
	{
		for(int i=0;i<=FacetSurface.get_max_tuple_id();i++)
		{
			if(is_facet_to_delete(i))
				continue;
			else
			{
				Facet f=get_facet_cell_link(i);
				Vertex_triple vertexFacet=make_vertex_triple(f);
				Vertex_Idtype v[3]={vertexFacet.first,vertexFacet.second,vertexFacet.third};
				FacetVertex.insert_next_tuple(v);
			}

		}

		PointDataSet.get_point_data(Point);


	}
	void set_surface_extractor(bool surface){ Surface = surface; }

	bool extract_surface(){ return this->Surface; };

	void clear_facet_deleted(Facet_Idtype IdF)
	{
		FacetUse.set_element(IdF,true);
	}
	Facet_Idtype create_surface_facet()
	{
		Facet_Idtype* t = new Facet_Idtype[FacetSurface.size_of_tuple()];
		Facet_Idtype t2[3]={-1,-1,-1};
		for (int i = 0; i < FacetSurface.size_of_tuple(); i++)
		{
			t[i] = -1;
		}

		if (!Hole_Facets.empty())
		{
			Facet_Idtype IdF = Hole_Facets.top();
			Hole_Facets.pop();
			clear_facet_deleted(IdF);			
			FacetSurface.set_tuple(IdF, t);
			clear_facet_visited(IdF);
			FacetNeighbor.set_tuple(IdF, t2);
			delete []t;
			return IdF;
		}
		else
		{
			FacetNeighbor.insert_next_tuple(t2);
			FacetSurface.insert_next_tuple(t);
			VisitedForFacetExtractor.insert_next_element(0);
			delete []t;
			return FacetUse.insert_next_element(true);

		}
		
	}

	Facet_Idtype create_surface_facet(Cell_Idtype IdC, Indextype ii)
	{
		Facet_Idtype idF = create_surface_facet();
		set_surface_facet(idF,IdC,ii);
		return idF;
	}

	Facet_Idtype create_surface_facet(Facet F)
	{
		Facet_Idtype idF=create_surface_facet();
		set_surface_facet(idF,F.first,F.second);
		return idF;
	}

	Vertex_Idtype vertex_facet(Facet F,Indextype I)
	{
		Vertex_triple vtriF=make_vertex_triple(F);
		if(I==0)
			return vtriF.first;
		if(I==1)
			return vtriF.second;
		if(I==2)
			return vtriF.third ;
		return -1;
	}

	//��E�ӣ���cell_id,0/1/2/3��,0/1/2���ĳ�(��id,0/1/2)
	Edge turn_edgeinfacet_to_edge(EdgeInFacet E)
	{
		return Edge(get_facet_index(E.first),E.second);
	}

	//��FΪ��cell,id����ʽ����F������idF���棨F.first,F.second���ĳ��棨IdC,ii����Ӧ
	void change_surface_facet_link(Facet F,Cell_Idtype IdC,Indextype ii)
	{
		//---------for test------//
		//Vertex_triple vtriiF=make_vertex_triple(Facet(IdC,ii));
		//========test end=======//
		Facet_Idtype idF=CellSurface.get_tuple_element(F.first,F.second);
		CellSurface.insert_tuple_element(F.first,F.second,-1);
		set_surface_facet(idF,IdC,ii);//������ΪIdF�������ã�IdC,ii��������FacetSurface���������棨IdC,ii���������ΪIdF������CellSurface��
	}
	//������ΪIdF�������ã�IdC,ii���������棨IdC,ii���������ΪIdF
	void set_surface_facet(Facet_Idtype IdF, Cell_Idtype IdC, Indextype ii)
	{
		//---------for test------//
		//���棨IdC��ii���а���ʱ��ʵ�������ţ�i,j,k��
		//Vertex_triple vtriiF=make_vertex_triple(Facet(IdC,ii));
		//========test end=======//
		FacetSurface.insert_tuple_element(IdF,0,IdC);
		FacetSurface.insert_tuple_element(IdF, 1, ii);
		CellSurface.insert_tuple_element(IdC,ii,IdF);
		
	}

	Facet get_facet_cell_link(Facet_Idtype IdF)
	{
		Facet f;
		f.first=FacetSurface.get_tuple_element(IdF,0);
		f.second = FacetSurface.get_tuple_element(IdF,1);
		return f;
	}
	Facet_Idtype get_facet_index(Facet F)
	{
		return CellSurface.get_tuple_element(F.first,F.second);
	}
	//��F1��F0�ĵڼ������ڵ�surface facet
	Indextype neighbor_index_facet(Facet F0,Facet F1)
	{
		Facet_Idtype idF0=CellSurface.get_tuple_element(F0.first,F0.second);
		Facet_Idtype idF1=CellSurface.get_tuple_element(F1.first,F1.second);
		for(int i=0;i<3;i++)
		{
			if(FacetNeighbor.get_tuple_element(idF0,i)==idF1)
				return i;
		}
	}
	//��IdF1��IdF0�ĵڼ������ڵ�surface facet
	Indextype neighbor_index_facet(Facet_Idtype IdF0,Facet_Idtype IdF1)
	{
		for(int i=0;i<3;i++)
		{
			if(FacetNeighbor.get_tuple_element(IdF0,i)==IdF1)
				return i;
		}
	}
	//������IdF0�ĵ�ii�����ڵ�����IdF1
	void set_facet_neighbor(Facet_Idtype IdF0, Indextype ii, Facet_Idtype IdF1)
	{
		FacetNeighbor.insert_tuple_element(IdF0,ii,IdF1);
	}

	//IdF0��i0����������IdF1,IdF1�ĵ�i1����������IdF0
	void set_facet_adjacency(Facet_Idtype IdF0, Indextype i0, Facet_Idtype IdF1, Indextype i1)
	{
		set_facet_neighbor(IdF0,i0,IdF1);
		set_facet_neighbor(IdF1,i1,IdF0);
	}
	void set_facet_adjacency(Facet F0,Indextype i0,Facet F1,Indextype i1)
	{
		Facet_Idtype idF0=get_facet_index(F0);
		Facet_Idtype idF1=get_facet_index(F1);
		set_facet_adjacency(idF0,i0,idF1,i1);
	}
	//���IdV��facet F�ĵڼ�����
	Indextype vertex_index_facet(Facet F,Vertex_Idtype IdV)
	{
		Vertex_triple vtriF=make_vertex_triple(F);
		if(vtriF.first==IdV)
			return 0;
		if(vtriF.second==IdV)
			return 1;
		if(vtriF.third==IdV)
			return 2;
		return -1;
	}
	void label_cell_side(Cell_Idtype IdC, bool IsInside)
	{
 		CellInsideLabel.insert_element(IdC,IsInside);
	}
	
	//��IdF���Ϊ�Ǳ��棬���������������ڹ�ϵ�����Ժ����������Ӧ�ü���������ڹ�ϵ�Ĵ���
	void delete_surface_facet(Facet_Idtype IdF)
	{
		Hole_Facets.push(IdF);
		mark_facet_to_deleted(IdF);
		Facet fTemp = get_facet_cell_link(IdF);
	
		CellSurface.insert_tuple_element(fTemp.first,fTemp.second,-1);
		clear_facet_visited(IdF);
	}
	//��IdF���Ϊ�Ǳ��棬���������������ڹ�ϵ�����Ժ����������Ӧ�ü���������ڹ�ϵ�Ĵ���
	void delete_surface_facet(Facet F)
	{
		Facet_Idtype idF = CellSurface.get_tuple_element(F.first,F.second);
		Hole_Facets.push(idF); //Hole_Facets�洢��ɾ����facet��id
		mark_facet_to_deleted(idF);
		clear_facet_visited(idF);
		CellSurface.insert_tuple_element(F.first, F.second, -1);
		
		
	}
	//���facet��conflict region��
	void mark_facet_to_deleted(Facet_Idtype IdF)
	{
		FacetUse.insert_element(IdF,false);
		
	}

	void mark_facet_to_deleted(Facet F)
	{
		Facet_Idtype idF = CellSurface.get_tuple_element(F.first, F.second);
		FacetUse.insert_element(idF, false);
	
	}

	bool is_facet_to_delete(Facet_Idtype IdF)
	{
		return !FacetUse.get_element(IdF);
	}

	bool is_facet_to_delete(Facet F)
	{
		Facet_Idtype idF = CellSurface.get_tuple_element(F.first, F.second);
		return !FacetUse.get_element(idF);
	}
	bool is_facet_in_conflict(Facet_Idtype IdF)
	{
		Facet F=get_facet_cell_link(IdF);
		return is_facet_in_conflict(F);
	}
	bool is_facet_in_conflict(Facet F)
	{
		Cell_Idtype c=F.first;
		Cell_Idtype cOpp=neighbor(c,F.second);
		bool b0=is_in_conflict(c);
		bool b1=is_in_conflict(cOpp);
		return (b0&&b1);
	}
	bool is_facet_on_conflict_boundary(Facet F)
	{
		Cell_Idtype c=F.first;
		Cell_Idtype cOpp=neighbor(c,F.second);
		bool b0=is_in_conflict(c);
		bool b1=is_in_conflict(cOpp);
		bool b2=is_on_boundary(c);
		bool b3=is_on_boundary(cOpp);
		return (b0&&b3)||(b2&&b1);
	}
	Vertex_triple make_vertex_triple(Facet& f);

	Vertex_pair make_vertex_pair(EdgeInFacet E)
	{
		Facet f=E.first;
		Vertex_triple vtrif=make_vertex_triple(f);
		if(E.second==0)
			return make_pair(vtrif.second,vtrif.third);
		if(E.second==1)
			return make_pair(vtrif.third,vtrif.first);
		if(E.second==2)
			return make_pair(vtrif.first,vtrif.second);
	}
	
	std::pair<Indextype,Indextype> make_edge_index(int I)
	{
		if(I==0)
			return make_pair(1,2);
		if(I==1)
			return make_pair(2,0);
		if(I==2)
			return make_pair(0,1);
	}

	
	bool is_label_inside(Cell_Idtype IdC)
	{
		return CellInsideLabel.get_element(IdC);
	}
	
	bool is_surface(Facet F)
	{
		if (CellSurface.get_tuple_element(F.first,F.second) == -1)
			return false;
		else
			return true;
	}
	//�жϵ��Ƿ��ڱ�����
	bool is_vertex_inside_surface(Locate_type lt, Cell_Idtype c, int li, int lj)
	{
		bool pInside=true;
	
		pInside=is_label_inside(c);
		if (lt==FACET&&is_surface(Facet(c,li)))
		{
			pInside=false;
		}
		if (lt==EDGE)
		{
			//���˱��Ƿ�Ϊ�����ϵı�
			bool isBegin=true;
			Cell_Idtype initCellEdge=c;
			Cell_Idtype tmpCellEdge=c;
			Vertex_Idtype vj1=vertex(c,li);
			Vertex_Idtype vj2=vertex(c,lj);
			//turn around the edge vj1 vj2
			while (initCellEdge!=tmpCellEdge||isBegin)
			{
				isBegin=false;
				if (pInside!=is_label_inside(tmpCellEdge))
				{
					pInside=false;
					break;
				}
				int zz = Triangulation_utils_3::next_around_edge(vertex_index(tmpCellEdge, vj1), vertex_index(tmpCellEdge, vj2));
				tmpCellEdge=neighbor(tmpCellEdge,zz);
			}
		}
		return pInside;
	}
	//��������V�����������������,�Ҳ��ظ�
	void insert_to_isolate_vertices(Vertex_Idtype V)
	{
		bool is_include=false;
		auto iiv=IsolateVertices.begin();
		while(iiv!=IsolateVertices.end())
		{
			if(*iiv==V)
			{
				is_include=true;
				break;
			}
			iiv++;
		}
		if(!is_include)
			IsolateVertices.push_back(V);
	}
	//��һ��������ӹ�����������ɾ��
	bool remove_from_isolate_vertices(Vertex_Idtype V)
	{
		
		auto iiv=IsolateVertices.begin();
		while(iiv!=IsolateVertices.end())
		{
			if(*iiv==V)
			{
				IsolateVertices.erase(iiv);
				return true;
			}
			iiv++;
		}
		return false;
	}
	//�ж�vertex�Ƿ�Ϊisolate vertex�������������˵���������Ե�surface facet
	bool is_vertex_isolate(Vertex_Idtype V,Facet& SurfaceFacet,bool& Inside);
	//����������������
	bool insert_vertex_into_surface(Vertex_Idtype V,Facet Surfacet,bool Inside);
	//��������V��������У��·�����,PInside��ע���������ڱ����ڻ�����
	bool insert_vertex_into_surface(Vertex_Idtype V);
	//��surface facet ĳһ�����ڵ�surface facet
	bool neighbor_surface_facet(EdgeInFacet SurfaceEdge,Facet& NeighborFacet,bool& IsToDelete);
	//��surface facet�ĵ�i�����ڵ�surface facet
	Facet neighbor_surfacet(Facet F,Indextype I)
	{
		Facet_Idtype idF0=CellSurface.get_tuple_element(F.first,F.second);
		Facet_Idtype idF1=FacetNeighbor.get_tuple_element(idF0,I);
		//����idF1���ɣ�cell,0/1/2/3����ʽ
		return get_facet_cell_link(idF1);
	}
	Facet_Idtype neighbor_surfacet(Facet_Idtype IdF,Indextype I)
	{
		return FacetNeighbor.get_tuple_element(IdF,I);;
	}
	//�Ʊ���������Ƭ����Ӧ�ı���ת�����壬��ת���������Ϊ�������������
	void neighbor_surfacet_around_outside(Facet F0,Indextype I0,Facet& F1,Indextype& I1);
	//���Cell�Ƿ񱻷��ʹ�
	bool visited_for_cell_extractor(Cell_Idtype IdC){ return VisitedForCellExtractor.get_element(IdC); };
	//��VisitedForCellExtractor��Ϊtrue��Ĭ��Ϊfalse
	void mark_cell_visited(Cell_Idtype IdC){ VisitedForCellExtractor.insert_element(IdC, true); };
	//���VisitedForCellExtractor
	void clear_cell_visited(Cell_Idtype IdC){ VisitedForCellExtractor.insert_element(IdC, false); };
	//���facet�Ƿ񱻷��ʹ�
	int visited_for_facet_extractor(Facet_Idtype IdF){return VisitedForFacetExtractor.get_element(IdF);};
	int visited_for_facet_extractor(Facet F)
	{
		Facet_Idtype idF=CellSurface.get_tuple_element(F.first,F.second);
		return VisitedForFacetExtractor.get_element(idF);
	};
	//��facet����Ϊ�Ѿ����ʣ�true
	void mark_facet_visited(Facet_Idtype IdF){
		Indextype times=VisitedForFacetExtractor.get_element(IdF)+1;
		VisitedForFacetExtractor.insert_element(IdF,times);};
		
	void mark_facet_visited(Facet F)
	{
		Facet_Idtype idF=CellSurface.get_tuple_element(F.first,F.second);
		Indextype times=VisitedForFacetExtractor.get_element(idF)+1;
		VisitedForFacetExtractor.insert_element(idF,times);
	};
	//��facet����Ϊδ���ʣ�false
	void clear_facet_visited(Facet_Idtype IdF){VisitedForFacetExtractor.insert_element(IdF,0);};
	void clear_facet_visited(Facet F)
	{
		Facet_Idtype idF=CellSurface.get_tuple_element(F.first,F.second);
		VisitedForFacetExtractor.insert_element(idF,0);
	}
	//��ĳһ����surface�ϵ�umbrella���������
	template<class OutputIteratorFacet>
	void incident_surface_facet(Vertex_Idtype IdV, OutputIteratorFacet FacetDelete, OutputIteratorFacet FacetUndelete);
	//��ĳһ����surface�ϵ�umbrella���������
	template<class IteratorFacet>
	void incident_surface_facet(Vertex_Idtype IdV, IteratorFacet Surfacets);
	//��ĳһ���incident vertex����surface�ϵ�umbrella���������
	template<class IteratorVertex,class IteratorFacet>
	void incident_vertex_and_surface_facet(Vertex_Idtype IdV,IteratorVertex IncidentVertices,IteratorFacet IncidentFacets);
	//����cell��conflict mark��ǰ������incident_cell 3d
	template<typename IncidentCellIterator, class OutputIteratorFacet>
	void incident_cells_3_withoutmark(Vertex_Idtype IdV, Cell_Idtype d, IncidentCellIterator IncidentCell,
		OutputIteratorFacet, OutputIteratorFacet FacetUndelete);
	//����cell��conflict mark��ǰ������incident_cell 3d, ������Ƿ�ɾ��
	template<typename IteratorCell, class IteratorFacet>
	void incident_cells_surfacet_3_withoutmark(Vertex_Idtype IdV, Cell_Idtype d,IteratorCell IncidentCells,
		IteratorFacet Surfacets);
	//�����ĳһ�������surface facet
	bool surface_facet_nearest_to_point(const T* p, Facet& NearestFacet, std::vector<Facet>& FacetDelete, std::vector<Facet>& FacetUndelete);
	//��incident surface facet�е�ĳ��factʹ��point��������������������һ�����
	template<class IteratorIncidentFacet>
	Facet incident_facet_with_min_longest_edge(const T* p, IteratorIncidentFacet FacetBegin, IteratorIncidentFacet FacetEnd, Vertex_Idtype VertexIncident);

	//��point��vertex�ľ���
	double distant_to_vertex(const T* p,Vertex_Idtype IdV);

	//�󽻽��棬ʹ��Gabriel�Ĺ�������滻�����չ����Ҫ��Է���pseudo-concave��surface����͹��չ
	void find_conflict_surfacet(const T* P,Facet InitFacet,list<Edge>& BoundEdges,set_pair_int& BoundEdge,Facet& Begin,
		std::set<Vertex_Idtype>& VertexOnBoundary,map<Vertex_Idtype,size_t>& VertexOnConflictFacet,
		bool PInside,vector<Facet_Idtype>& SurfacetsInConflict,bool& quit);

	//��㵽���������������
	double longest_edge_to_facet(const T* p,Facet f);
	//��㵽������Ƭ�����������С����
	double shortest_edge_to_facet(const T* p,Facet f);
	//������ʸ�������ǵ�����ֵ
	double cos_dihedral(Facet F0,Facet F1)
	{
		Vertex_triple vf0=make_vertex_triple(F0);
		Vertex_triple vf1=make_vertex_triple(F1);
		return GeometricTraits<T>::cos_dihedral(point(vf0.first),point(vf0.second),point(vf0.third),
			point(vf1.first),point(vf1.second),point(vf1.third));
	}
	//��ĳһ��p����Facet�γɵ��������������������Facet��ɵ�������ǵ�����ֵ����Ӧ������ֵ��С��
	T cos_min_dihedral_point_to_facet(const T* P,Facet F)
	{
		Vertex_triple vf=make_vertex_triple(F);
		T cosD0=GeometricTraits<T>::cos_dihedral(point(vf.first),point(vf.second),point(vf.third),point(vf.first),P,point(vf.second));
		T cosD1=GeometricTraits<T>::cos_dihedral(point(vf.first),point(vf.second),point(vf.third),point(vf.second),P,point(vf.third));
		T cosD2=GeometricTraits<T>::cos_dihedral(point(vf.first),point(vf.second),point(vf.third),point(vf.third),P,point(vf.first));
		T maxCos=cosD0<cosD1?(cosD0<cosD2?cosD0:cosD2):(cosD1<cosD2?cosD1:cosD2);
		return maxCos;
	}
	//��ĳһ��P����Facet�γɵ����������������������Facet��ɵ���������ǵ�����ֵ
	void cos_dihedral_point_to_facet(const T* P,Facet F,T* CosDihe)
	{
		Vertex_triple vf=make_vertex_triple(F);
		
		CosDihe[0]=GeometricTraits<T>::cos_dihedral(point(vf.first),point(vf.second),point(vf.third),point(vf.second),P,point(vf.third));
		CosDihe[1]=GeometricTraits<T>::cos_dihedral(point(vf.first),point(vf.second),point(vf.third),point(vf.third),P,point(vf.first));
		CosDihe[2]=GeometricTraits<T>::cos_dihedral(point(vf.first),point(vf.second),point(vf.third),point(vf.first),P,point(vf.second));
		
	}
	//��ĳһ��P����Facet����ĳ����EdgeInFacet(F,i)�γɶ���ǵ�����ֵ
	double cos_dihedral_point_to_Edge(const T* P,Facet F,Indextype I)
	{
		Vertex_triple vf=make_vertex_triple(F);
		if(I==0)
			return GeometricTraits<T>::cos_dihedral(point(vf.first),point(vf.second),point(vf.third),point(vf.second),P,point(vf.third));
		if(I==1)
			return GeometricTraits<T>::cos_dihedral(point(vf.first),point(vf.second),point(vf.third),point(vf.third),P,point(vf.first));
		if(I==2)
			return GeometricTraits<T>::cos_dihedral(point(vf.first),point(vf.second),point(vf.third),point(vf.first),P,point(vf.second));
	}
	//���������
	void isolate_vertices(std::map<Vertex_Idtype,size_t> VertexWithSurfacetDeleted,std::set<Vertex_Idtype> VertexWithSurfacetCreated);
	//��������
	bool has_isolate_vertex(std::map<Vertex_Idtype,size_t> VertexWithSurfacetDeleted,std::set<Vertex_Idtype> VertexWithSurfacetCreated);

	//�ж���F����������,������һ���ڼ���Nearest��
	bool is_facet_in_neighbor(Facet F,list<Vertex_Idtype> Nearest)
	{
		Vertex_triple vtriF=make_vertex_triple(F);
		return(is_in_list(vtriF.first,Nearest.begin(),Nearest.end())||is_in_list(vtriF.second,Nearest.begin(),Nearest.end())||
			is_in_list(vtriF.third,Nearest.begin(),Nearest.end()));
	}

	//����һ���㣬����������б�
	void refresh_K_nearest(const T* p,Vertex_Idtype v,list<Vertex_Idtype>& nearest);
	//�Ƚ�һ���㵽������������
	Vertex_Idtype nearest_vertex(const T* p,Vertex_Idtype v,Vertex_Idtype w);
	//���������������
	bool link_isolate_to_surface(Vertex_Idtype V,Facet SeedFacet,bool PInside);
	//Ѱ�ҹ�������صĽ�����
	void find_isolate_conflict_surfacet(Vertex_Idtype V, Facet SeedFacet,bool PInside,std::set<Vertex_Idtype>& VertexOnBoundary,
		set<Vertex_Idtype>& VertexOnConflictFacet,vector<Facet>& Conflicts,
		list<Edge>& ConflictBoundEdges,vector<Facet>& NewCreateSurfacets,bool& quit);

	//�õ����ķ�ʽ������������surface
	void recursive_find_isolate_conflict_surfacet(Vertex_Idtype V, Facet SeedFacet,bool PInside,std::set<Vertex_Idtype>& VertexOnBoundary,
		set<Vertex_Idtype>& VertexOnConflictFacet,vector<Facet>& Conflicts,
		list<Edge>& ConflictBoundEdges,vector<Facet>& NewCreateSurfacets,int prev_ind3=-1);
	//��������ཻ���������������Ƭ
	bool seed_facet_of_isolate_vertex(Vertex_Idtype V,Facet InitFacet,bool PInside,Facet& SeedFacet);
	//������ڵķ�������candidate��ѡ��
	void nearest_surfacet(const T* P,vector<Facet> SurfacetsInternalConflict,
		vector<Facet> SurfacetsIncidentKNN,vector<Facet> SurfacetsIncidentSparse,
		bool PInside,Facet& NearestFacet,vector<Facet_Idtype>& SurfacetsInConflict,
		std::map<Vertex_Idtype,size_t>& verticesWithDeletedSurfacet);
	//��ĳһ��������ڵ�surface facet�����յ㵽�����γɶ���ǵ�������С��������
	void nearest_surface_facet(const T* p,list<Vertex_Idtype> NearestVertices,
		vector<Facet> InitFacet,bool PInside,Facet& NearestFacet,bool& IsConcave);
	//������ڵ��·��������뼸��Լ��������Լ��
	void nearest_surface_facet(const T* p,list<Vertex_Idtype> NearestVertices,
		vector<Facet> InitFacet,Triple<Facet,double,double> InitFacetConflict,bool PInside,Facet& NearestFacet);

	template<typename IterList,typename ValueTypeList>
	static bool is_in_list(ValueTypeList v,IterList IBegin,IterList Iend)
	{
		for(;IBegin!=Iend;IBegin++)
		{
			if(*IBegin==-1)
				break;
			if(*IBegin==v)
				return true;
		}
		return false;
	}
	template<typename IterList,typename ValueTypeList>
	static int rank_in_list(ValueTypeList v,IterList IBegin,IterList Iend)
	{
		int i=0;
		for(;IBegin!=Iend;IBegin++)
		{
			if(*IBegin==v)
				return i;
			i++;
		}
		return -1;
	}
	bool is_edge_in_surface(Cell_Idtype C,Indextype I0,Indextype I1);
	//sculpture����+�������ݽṹ����,CΪҪsculpture��cell��NumOfSurfaceΪԭ�е�surface facet�ĸ�����SurfaceIndexΪ��Щsurface facet��Ӧ�ı�Ǻš�
	void sculpture(Cell_Idtype C,int NumOfSuface,int* SurfaceIndex,vector<Facet>& NewCreateSurfacet);
	//inflate����+�������ݽṹ����
	void inflate(Cell_Idtype C,int NumOfSurface,int* SurfaceIndex,vector<Facet>& NewCreateSurfacet);
	//����sculpture���Ż�����,flip��������������E(S)������Ϊ���ơ�MarkIso��ʾ�Ƿ���Բ��������㣬MarkIso=trueʱ���ܲ������������������������һ��������
	//�Ҳ��ܽ�������VertexIso�������
	template<typename IteratorFacet>
	bool iterative_sculpture_with_mark(Facet F,Vertex_Idtype VertexIso,IteratorFacet ItSurF,Vertex_Idtype IdV,Facet& FCur,bool MarkIso);
	//����inflate���Ż�����,flip��������������E(S)������Ϊ���ƣ�������һ��������
	template<typename IteratorFacet>
	bool iterative_inflate_with_mark(Facet F,Vertex_Idtype VertexIso,IteratorFacet ItSurF,Vertex_Idtype IdV,Facet& FCur,bool MarkIso);
	//��������K-NN��incident surfacet����inflate��sculpture
	template<typename IteratorFacet>
	void update_surfacets_KNN(bool PInside,Vertex_Idtype VertexIso,list<Vertex_Idtype> NearestVertices,vector<Facet> InitFacets,IteratorFacet ItSurF,IteratorFacet ItSurSparse);


private:
	//�洢�����ʱ��,�ı����Ϣ,�����쳣�����ԭ

	//�洢�ݹ���������ɵ�cell
	unique_ptr<stack<Cell_Idtype> > unique_pCnews_stack;

	//�洢�ݹ�����������ɺ�ɾ���ı���
	struct MarkedSurfacet
	{
		Facet f;  //�ݹ�����������ɺ�ɾ���ı���
		bool mark; //trueΪ�ݹ��������ɱ���,falseΪ�ݹ���ɾ���ı���

		//�洢�ݹ��б�ɾ������ڽ���,���ָ�ʱ��ʹ��;�Եݹ�����������ɵı��治��Ҫ,
		Facet_Idtype original_nei0; 
		Facet_Idtype original_nei1;
		Facet_Idtype original_nei2;

		MarkedSurfacet(Facet f_, bool mark_,
			Facet_Idtype original_nei0_,Facet_Idtype original_nei1_,Facet_Idtype original_nei2_): 
		    f(f_),mark(mark_),
			original_nei0(original_nei0_),original_nei1(original_nei1_),original_nei2(original_nei2_){}

		MarkedSurfacet(Cell_Idtype c,Indextype i, bool mark_,
			Facet_Idtype original_nei0_,Facet_Idtype original_nei1_,Facet_Idtype original_nei2_): f(c,i),mark(mark_),
		    original_nei0(original_nei0_),original_nei1(original_nei1_),original_nei2(original_nei2_){}
	};
	unique_ptr<stack<MarkedSurfacet> > unique_pCreated_DeletedSurfacets_stack;

	//�洢�ݹ�����иı��(c,cnew,li),����һ��
	struct ChangedSurfacet
	{
		Cell_Idtype c;
		Cell_Idtype cnew;
		Indextype li;
		ChangedSurfacet(Cell_Idtype c_,Cell_Idtype cnew_,Indextype li_):
			c(c_),cnew(cnew_),li(li_){} 
	};
	unique_ptr<stack<ChangedSurfacet> > unique_pChangedSurfacets_stack;

	//�洢�ݹ�����иı��(c,c_li,ԭ����neighbor_index(c_li, c)),����һ��
	struct ChangedCellAjcacency 
	{
		Cell_Idtype c;
		Cell_Idtype c_li;
		Indextype original_index;
		ChangedCellAjcacency(Cell_Idtype c_,Cell_Idtype c_li_,Indextype original_index_):
			c(c_),c_li(c_li_),original_index(original_index_){}
	};
	unique_ptr<stack<ChangedCellAjcacency> > unique_pChangedCellAjcacency_stack;

	//�洢�ݹ�����иı��(����,��֮����cell),����һ��
	struct ChangedCellIncident
	{
		Vertex_Idtype changed_cell_vertex;
		Cell_Idtype original_cell;
		ChangedCellIncident(Vertex_Idtype changed_cell_vertex_,Cell_Idtype original_cell_):
			changed_cell_vertex(changed_cell_vertex_),original_cell(original_cell_){}
	};
	unique_ptr<stack<ChangedCellIncident> > unique_pChangedCellIncident_stack;

	void ini_uniqueptr_of_Changeds();
	void return_to_before_insert();

};



template<typename T, typename T_INFO>
void DataStructure<T,T_INFO>::
ini_uniqueptr_of_Changeds()
{
	unique_pCnews_stack=
		unique_ptr<stack<Cell_Idtype> >(new stack<Cell_Idtype>);
	unique_pCreated_DeletedSurfacets_stack=
		unique_ptr<stack<MarkedSurfacet> >(new stack<MarkedSurfacet>);
	unique_pChangedSurfacets_stack=
		unique_ptr<stack<ChangedSurfacet> >(new stack<ChangedSurfacet>);
	unique_pChangedCellAjcacency_stack=
		unique_ptr<stack<ChangedCellAjcacency> >(new stack<ChangedCellAjcacency>);
	unique_pChangedCellIncident_stack=
		unique_ptr<stack<ChangedCellIncident> >(new stack<ChangedCellIncident>);

}


template<typename T, typename T_INFO>
void DataStructure<T,T_INFO>::
return_to_before_insert()
{
	//�ݹ�����������ɵ���ɾ��,��ɾ���������»ָ�,���ұ�֤��id����ǰһ��
	while (!unique_pCreated_DeletedSurfacets_stack->empty())
	{
		auto tmp=unique_pCreated_DeletedSurfacets_stack->top();
		unique_pCreated_DeletedSurfacets_stack->pop();
		if (tmp.mark)
		{
			delete_surface_facet(tmp.f);
		}
		else
		{
			create_surface_facet(tmp.f);
			Facet_Idtype fid=CellSurface.get_tuple_element((tmp.f).first,(tmp.f).second);

			Facet_Idtype original_nei0=tmp.original_nei0;
			Facet_Idtype original_nei1=tmp.original_nei1;
			Facet_Idtype original_nei2=tmp.original_nei2;
			set_facet_neighbor(fid,0,original_nei0);
			set_facet_neighbor(fid,1,original_nei1);
			set_facet_neighbor(fid,2,original_nei2);
		}
	}
	//�ݹ��������change_surface_facet_link(Facet(c,li),cnew,li)�ı��״̬�ָ�
	while(!unique_pChangedSurfacets_stack->empty())
	{
		auto tmp=unique_pChangedSurfacets_stack->top();
		unique_pChangedSurfacets_stack->pop();
		Cell_Idtype c=tmp.c;
		Cell_Idtype cnew=tmp.cnew;
		Indextype li=tmp.li;
		change_surface_facet_link(Facet(cnew,li),c,li);
	}
	//�ݹ��������set_adjacency(cnew, li, c_li, neighbor_index(c_li, c))�ı��״̬�ָ�
	while (!unique_pChangedCellAjcacency_stack->empty())
	{
		auto tmp=unique_pChangedCellAjcacency_stack->top();
		unique_pChangedCellAjcacency_stack->pop();
		Cell_Idtype c=tmp.c;
		Cell_Idtype c_li=tmp.c_li;
		Indextype original_index=tmp.original_index;
		set_neighbor(c_li,original_index,c);
	}
	//�ݹ��������set_cell(vertex(cnew, ii), cnew);�ı��״̬�ָ�
	while (!unique_pChangedCellIncident_stack->empty())
	{
		auto tmp=unique_pChangedCellIncident_stack->top();
		unique_pChangedCellIncident_stack->pop();
		Vertex_Idtype changed_cell_vertex=tmp.changed_cell_vertex;
		Cell_Idtype original_cell=tmp.original_cell;
		set_cell(changed_cell_vertex,original_cell);
	}
	while (!unique_pCnews_stack->empty())
	{
		Cell_Idtype cnew=unique_pCnews_stack->top();
		unique_pCnews_stack->pop();
		delete_cell(cnew);
	}
}


template<typename T, typename T_INFO>
Vertex_Idtype DataStructure<T,T_INFO>::
insert_increase_dimension(Vertex_Idtype star)
{
	Vertex_Idtype idVertex = create_vertex();
	int dim = dimension();
	set_dimension(dim+1);
	switch (dim)
{
	
	case -2:
	    // insertion of the first vertex
	    // ( geometrically : infinite vertex )
	{
		Infinite=idVertex;
	
		Cell_Idtype c = create_face(idVertex, -1, -1);
		set_cell(idVertex,c);
		break;
	}

	case -1:
		// insertion of the second vertex
		// ( geometrically : first finite vertex )
	{
		Cell_Idtype d = create_face(idVertex, -1, -1);
		set_cell(idVertex,d);
		set_adjacency(d, 0, cell(star), 0);
		break;
	}

	case 0:
		// insertion of the third vertex
		// ( geometrically : second finite vertex )
	{
		Cell_Idtype c = cell(star);
		Cell_Idtype d = neighbor(c,0);

		set_vertex(c,1, vertex(d,0));
		set_vertex(d,1, idVertex);
		set_neighbor(d,1, c);
		Cell_Idtype e = create_face(idVertex, star, -1);
		set_adjacency(e, 0, c, 1);
		set_adjacency(e, 1, d, 0);

		set_cell(idVertex,d);
		break;
	}

	case 1:
		// general case : 4th vertex ( geometrically : 3rd finite vertex )
		// degenerate cases geometrically : 1st non collinear vertex
	{
		Cell_Idtype c = cell(star);
		int i = vertex_index(c,star); // i== 0 or 1
		
		int j = (i == 0) ? 1 : 0;
		Cell_Idtype d = neighbor(c,j);

		set_vertex(c,2, idVertex);

		Cell_Idtype e = neighbor(c,i);
		Cell_Idtype cnew = c;
		Cell_Idtype enew = -1;

		while (e != d){
			enew = create_cell();
			set_vertex(enew,i, vertex(e,j));
			set_vertex(enew,j, vertex(e,i));
			set_vertex(enew,2, star);

			set_adjacency(enew, i, cnew, j);
			// false at the first iteration of the loop where it should
			// be neighbor 2
			// it is corrected after the loop
			set_adjacency(enew, 2, e, 2);
			// neighbor j will be set during next iteration of the loop

			set_vertex(e,2, idVertex);

			e = neighbor(e,i);
			cnew = enew;
		}

		set_vertex(d,2, idVertex);
		set_adjacency(enew, j, d, 2);

		// corrections for star->cell() :
		c = cell(star);
		//set_neighbor(c, 2, c->neighbor(i)->neighbor(2)); 
		set_neighbor(c,2,neighbor(neighbor(c, i), 2));
		set_neighbor(c,j, d);

		set_cell(idVertex,d);

		break;
	}

	case 2:
		// general case : 5th vertex ( geometrically : 4th finite vertex )
		// degenerate cases : geometrically 1st non coplanar vertex
	{
		// used to store the new cells, in order to be able to traverse only
		// them to find the missing neighbors.
		std::vector<Cell_Idtype > new_cells;
		new_cells.reserve(16);

		// allowed since the dimension has already been set to 3
		//Cell_iterator it = cells_begin();
		Cell_Idtype iCell = cells_begin();
		
		// ok since there is at least one ``cell''
		set_cell(idVertex,iCell); 
		for (; iCell <= num_of_cells(); ++iCell) {
			// Here we must be careful since we create_cells in a loop controlled
			// by an iterator.  So we first take care of the cells newly created
			// by the following test :
			if (cell_is_deleted(iCell))
				continue;
			if (neighbor(iCell,0) == -1)
				continue;
			set_neighbor(iCell,3, -1);
			set_vertex(iCell,3, idVertex);
			if (!has_vertex(iCell,star)) {
				Cell_Idtype cnew = create_cell(vertex(iCell,0), vertex(iCell,2),
					vertex(iCell,1), star);
				// The Intel compiler has a problem with passing "it" directly to
				// function "set_adjacency": the adjacency is not changed.
				Cell_Idtype ch_it = iCell;
				set_adjacency(cnew, 3, ch_it, 3);
				set_neighbor(cnew,0, -1);
				new_cells.push_back(cnew);
			}
		}

		// traversal of the new cells only, to add missing neighbors
		for (typename std::vector<Cell_Idtype>::iterator ncit = new_cells.begin();
			ncit != new_cells.end(); ++ncit) {
			Cell_Idtype n = neighbor(*ncit,3); // opposite to star
			for (int i = 0; i<3; i++) {
				int j;
				if (i == 0) j = 0;
				else j = 3 - i; // vertex 1 and vertex 2 are always switched when
				// creating a new cell (see above)
				//Cell_Idtype c = n->neighbor(i)->neighbor(3);
				Cell_Idtype c = neighbor(neighbor(n,i),3);
				if (c != -1) {
					// i.e. star is not a vertex of n->neighbor(i)
					set_neighbor(*ncit,j, c);
					// opposite relation will be set when ncit arrives on c
					// this avoids to look for the correct index
					// and to test whether *ncit already has neighbor i
				}
				else {
					// star is a vertex of n->neighbor(i)
					set_adjacency(*ncit, j, neighbor(n,i), 3);//neighbor opposite to idVertex
				}
			}
		}
	}
}// end switch

return idVertex;
}

template<typename T, typename T_INFO>
Vertex_Idtype DataStructure<T, T_INFO>::
insert_in_edge(Cell_Idtype c, int i, int j)
{
	Vertex_Idtype v = create_vertex();
	Cell_Idtype cnew = create_face(v, vertex(c, 1), -1);
	set_cell(vertex(c, 1), cnew);
	set_vertex(c, 1, v);
	set_adjacency(cnew, 0, neighbor(c, 0), 1);
	set_adjacency(cnew, 1, c, 0);
	set_cell(v, cnew);
	return v;

}


template<typename T, typename T_INFO>
void DataStructure<T, T_INFO>::
reorient()
{
	for (rsize_t i = 0; i < num_of_cells(); i++)
	{
		if (!cell_is_deleted(i))
			change_orientation(i);
	}
}

template<typename T, typename T_INFO>
void DataStructure<T, T_INFO>::
change_orientation(Cell_Idtype c) 
{
	Vertex_Idtype tmp_v = vertex(c,0);
	set_vertex(c,0, vertex(c,1));
	set_vertex(c,1, tmp_v);
	Cell_Idtype tmp_c = neighbor(c,0);
	set_neighbor(c,0, neighbor(c,1));
	set_neighbor(c,1, tmp_c);
}

template<typename T, typename T_INFO>
template<typename CellIt>
Vertex_Idtype DataStructure<T, T_INFO>::
insert_in_hole(CellIt cell_begin, CellIt cell_end,
Cell_Idtype begin, int i)
{

	Vertex_Idtype newv = create_vertex();

	Cell_Idtype cnew;
	if (dimension() == 3)
		cnew = create_star_3(newv, begin, i);
	else
		cnew = create_star_2(newv, begin, i);

	set_cell(newv,cnew);
	delete_cells(cell_begin, cell_end);
	return newv;
}

//Ҫ��Ӱ�����cells��������߽�Ҳ����,p��Ӱ������
//���DelaunayӰ����ά������������ԣ����滻��Ϊ�߽��hole���������ע
//cell_beginΪ�������г�ʼɾ����cell;cell_endΪ���ɾ����cell;
//��begin,i����Ӱ����߽����棬��begin����Ӱ���򣬵�neighbor(begin,i)������Ӱ����
//boundEdgeΪ���滻��߽��ߣ��Զ˵�(vertex_id,vertex_id)��ʾ��ConflictBoundEdgesҲ�Ǳ߽��ߣ���Edge(��id,0/1/2)��ʾ
//VertexDeletedFacetΪ(���滻���ϵĶ���id,��֮�����ı���������Ƭ��ɾ���ĸ���);
//VertexCreatedFacetΪ�߽����ϵĵ�;
//NewBoundarySurfacets(cell,0/1/2/3)��ʾ�����γɱ��湲�滻��߽��ߵı���;NewConflictSurfacetsΪ���γɵķ��滻��ı���������Ƭ;
//IsoIsolate��ʾp��SeedFacet�Ƿ���Ӱ�����ڻ�߽��ϣ�isInsideΪp���ڱ����λ�ù�ϵ
template<typename T, typename T_INFO>
template<typename CellIt>
Vertex_Idtype DataStructure<T, T_INFO>::
insert_in_hole(CellIt cell_begin, CellIt cell_end,
		Cell_Idtype begin, int i, set_pair_int boundEdge,list<Edge> ConflictBoundEdges,
		std::map<Vertex_Idtype,size_t>& VertexDeletedFacet,
		std::set<Vertex_Idtype>& VertexCreatedFacet,vector<Facet>& NewBoundarySurfacets,
		vector<Facet>& NewConflictSurfacets,bool IsoIsolate,bool isInside)
{
	Vertex_Idtype newv = create_vertex();
	ini_uniqueptr_of_Changeds();
	try
	{
		Cell_Idtype cnew;
		if (dimension() == 3)
			cnew = create_star_3(newv, begin, i,-1,boundEdge,ConflictBoundEdges,VertexDeletedFacet,
				VertexCreatedFacet,NewBoundarySurfacets,NewConflictSurfacets,IsoIsolate,isInside);	
		else
			cnew = create_star_2(newv, begin, i);
		set_cell(newv, cnew);
		delete_cells(cell_begin, cell_end);
		update_surface_connection(newv,ConflictBoundEdges,NewConflictSurfacets,NewBoundarySurfacets);
		return newv;
	}
	catch(exception& )
	{
		//�ָ�����õ�֮ǰ״̬
		delete_vertex(newv);
		return_to_before_insert();
		return -1;
	}

}


template<typename T, typename T_INFO>
Cell_Idtype DataStructure<T, T_INFO>::
create_star_3(Vertex_Idtype v, Cell_Idtype c,
	int li, int prev_ind2 = -1)
{
	return recursive_create_star_3(v, c, li, prev_ind2, 0);
}

//Ҫ��Ӱ�����cells��������߽�Ҳ����,�¶���v��Ӱ������
//���DelaunayӰ����ά������������ԣ����滻��Ϊ�߽��hole���������ע
//v�²��붥���ţ���c,li����Ӱ����߽����棬��c����Ӱ���򣬵�neighbor(c,li)������Ӱ����
//prev_ind2��previous index
//boundEdgeΪ���滻��߽��ߣ��Զ˵�(vertex_id,vertex_id)��ʾ��ConflictBoundEdgesҲ�Ǳ߽��ߣ���Edge(��id,0/1/2)��ʾ
//VertexDeletedFacetΪ(���滻���ϵĶ���id,��֮�����ı���������Ƭ��ɾ���ĸ���);
//VertexCreatedFacetΪ�߽����ϵĵ㣻NewBoundarySurfacetsΪ���γɵ��滻���ϵ�������Ƭ;
//NewConflictSurfacetsΪ���γɵķ��滻��ı���������Ƭ��IsoIso��ʾp��SeedFacet�Ƿ���Ӱ�����ڻ�߽��ϣ�isInsideΪp���ڱ����λ�ù�ϵ
template<typename T, typename T_INFO>
Cell_Idtype DataStructure<T, T_INFO>::
create_star_3(Vertex_Idtype v, Cell_Idtype c,
int li, int prev_ind2,set_pair_int boundEdge,list<Edge> ConflictBoundEdges,
std::map<Vertex_Idtype,size_t>& VertexDeletedFacet,
std::set<Vertex_Idtype>& VertexCreatedFacet,vector<Facet>& NewBonudarySurfacets,
vector<Facet>& NewConflictSurfacets,bool IsIso,bool isInside)
{
	Cell_Idtype cnew;

	cnew= surface_recursive_create_star_3(v, c, li, 0,boundEdge,VertexDeletedFacet,
		VertexCreatedFacet,NewConflictSurfacets,true,false, prev_ind2);	

	return cnew;
}

template<typename T, typename T_INFO>
Cell_Idtype DataStructure<T, T_INFO>::
recursive_create_star_3(Vertex_Idtype v, Cell_Idtype c, int li,
						int prev_ind2, int depth)
{
	if (depth == 100) return non_recursive_create_star_3(v, c, li, prev_ind2);


	Cell_Idtype cnew = create_cell(vertex(c,0),
		vertex(c,1),
		vertex(c,2),
		vertex(c,3));
	set_vertex(cnew,li, v);
	Cell_Idtype c_li = neighbor(c,li);
	set_adjacency(cnew, li, c_li, neighbor_index(c_li,c));

	// Look for the other neighbors of cnew.
	for (int ii = 0; ii<4; ++ii) {
		//if (ii == prev_ind2 || neighbor(cnew,ii) != -1)
		if (ii==prev_ind2||ii==li)
			continue;
		//cnew->vertex(ii)->set_cell(cnew);
		set_cell(vertex(cnew,ii),cnew);
		// Indices of the vertices of cnew such that ii,vj1,vj2,li positive.
		Vertex_Idtype vj1 = vertex(c, Triangulation_utils_3::next_around_edge(ii, li));
		Vertex_Idtype vj2 = vertex(c, Triangulation_utils_3::next_around_edge(li, ii));
		Cell_Idtype cur = c;
		int zz = ii;
		Cell_Idtype n = neighbor(cur,zz);
		// turn around the oriented edge vj1 vj2
		while (is_in_conflict(n)) {
			cur = n;
			zz = Triangulation_utils_3::next_around_edge(vertex_index(n, vj1), vertex_index(n, vj2));
			n = neighbor(cur,zz);
		}
		// Now n is outside region, cur is inside.
		clear(n); // Reset the flag for boundary cells.

		int jj1 = vertex_index(n,vj1);
		int jj2 = vertex_index(n,vj2);
		Vertex_Idtype vvv = vertex(n, Triangulation_utils_3::next_around_edge(jj1, jj2));
		Cell_Idtype nnn = neighbor(n, Triangulation_utils_3::next_around_edge(jj2, jj1));
		int zzz = vertex_index(nnn,vvv);
		if (nnn == cur) {
			// Neighbor relation is reciprocal, ie
			// the cell we are looking for is not yet created.
			nnn = recursive_create_star_3(v, nnn, zz, zzz, depth + 1);
		}

		set_adjacency(nnn, zzz, cnew, ii);
	}

	return cnew;
}


//Ҫ��Ӱ�����cells��������߽�Ҳ����,�¶���v��Ӱ������
//���DelaunayӰ����ά������������ԣ����滻��Ϊ�߽��hole���������ע
//v�²��붥���ţ���c,li����Ӱ����߽����棬��c����Ӱ���򣬵�neighbor(c,li)������Ӱ����
//depth�ݹ���ȣ�boundEdgeΪ���滻��߽��ߣ��Զ˵�(vertex_id,vertex_id)��ʾ��
//VertexDeletedFacetΪ(���滻���ϵĶ���id,��֮�����ı���������Ƭ��ɾ���ĸ���);
//VertexCreatedFacetΪ�߽����ϵĵ㣻NewCreateBoundFacetΪ���γɵķ��滻���ϵ�������Ƭ;
//label��ʾ�²���cell�Ƿ�inside,SideChange:�������vj1->vj2��vj1��vj2Ϊ�ú����ڱ�����Ϊ���滻��߽���Ϊtrue,prev_ind2Ӱ�����������Ѿ����������Զ���
template<typename T, typename T_INFO>
Cell_Idtype DataStructure<T, T_INFO>::
surface_recursive_create_star_3(Vertex_Idtype v, Cell_Idtype c, int li,
		 int depth,set_pair_int boundEdge,
		 std::map<Vertex_Idtype,size_t>& VertexDeletedFacet,
		 std::set<Vertex_Idtype>& VertexCreatedFacet,std::vector<Facet>& NewCreateBoundFacet,
         bool label,bool SideChange,int prev_ind2)
{


	//---------for test-------//
	/*cout<<"recursive depth: "<<depth<<" starting,"<<endl;*/
	//=========test end=======//

	Cell_Idtype cnew = create_cell(vertex(c, 0),
		vertex(c, 1),
		vertex(c, 2),
		vertex(c, 3));
	//��Cell��cnew���ĵ�li�����趨Ϊv
	set_vertex(cnew, li, v);
    unique_pCnews_stack->push((cnew));

	Cell_Idtype c_li = neighbor(c, li);
	
	//--------------------��ȡ������ı���---------------
    if (SideChange)
	{
		if (label)
		{
			create_surface_facet(cnew,prev_ind2);
			unique_pCreated_DeletedSurfacets_stack->push(MarkedSurfacet(cnew,prev_ind2,true,-1,-1,-1));
			//---------for test-------//
			//Vertex_triple vtriFTT=make_vertex_triple(Facet(cnew,prev_ind2));
			//cout<<"!!create facet opposite to pre_ind2: ("<<vtriFTT.first<<","<<vtriFTT.second<<","<<vtriFTT.third<<")"
			//	<<"/("<<cnew<<","<<prev_ind2<<")"<<endl;
			//=========test end=======//
		
		}
		
	}


	label_cell_side(cnew,label);

	//----------for test------------//
	//Vertex_triple vtrT=make_vertex_triple(Facet(c,li));
	//cout<<"create cell "<<cnew<<"("<<vertex(cnew, 0)<<","<<vertex(cnew, 1)<<","<<vertex(cnew, 2)<<","<<vertex(cnew, 3)<<")"
	//	<<"[label true:"<<label<<"] opposite to facet ("<<c<<","<<li<<")/("<<vtrT.first<<","<<vtrT.second<<","<<vtrT.third<<")"
	//	<<"   c:"<<c<<"["<<is_label_inside(c)<<"]("<<vertex(c, 0)<<","<<vertex(c, 1)<<","<<vertex(c, 2)<<","<<vertex(c, 3)<<") "
	//	<<"c_li:"<<c_li<<"["<<is_label_inside(c_li)<<"]("<<vertex(c_li, 0)<<","<<vertex(c_li, 1)<<","<<vertex(c_li, 2)<<","<<vertex(c_li, 3)<<")"<<endl;
	//===========test end===========//

	//--------------�ж�boundary facet��surface facet״̬�ı仯---------
	if (label == is_label_inside(c_li)) //cnew��c_li���ڱ����ڻ������
	{
		if (is_surface(Facet(c, li)))
		{
			Facet_Idtype fid=CellSurface.get_tuple_element(c,li);
			Facet_Idtype original_nei0=neighbor_surfacet(fid,0);
			Facet_Idtype original_nei1=neighbor_surfacet(fid,1);
			Facet_Idtype original_nei2=neighbor_surfacet(fid,2);
			unique_pCreated_DeletedSurfacets_stack->push(MarkedSurfacet(c,li,false,
				original_nei0,original_nei1,original_nei2));
			delete_surface_facet(Facet(c,li));		
			//-------------��Ǳ�ɾ�������ϵĵ�--------------------//
			Vertex_triple vf=make_vertex_triple(Facet(c,li));
			//cout<<"!!delete facet: ("<<vf.first<<","<<vf.second<<","<<vf.third<<")"
			//	<<"/("<<c<<","<<li<<")"<<endl;
			VertexDeletedFacet[vf.first]++;
			VertexDeletedFacet[vf.second]++;
			VertexDeletedFacet[vf.third]++;
			//===================���end===========================//
		}
		else if (is_surface(Facet(c_li, neighbor_index(c_li, c))))    //neighbor_index(c_li, c)��c��c_li�ĵڼ����ٽ�Cell;��mirror_facet
		{
			int neiI=neighbor_index(c_li, c);
			Facet_Idtype fid=CellSurface.get_tuple_element(c_li,neiI);
			Facet_Idtype original_nei0=neighbor_surfacet(fid,0);
			Facet_Idtype original_nei1=neighbor_surfacet(fid,1);
			Facet_Idtype original_nei2=neighbor_surfacet(fid,2);
			unique_pCreated_DeletedSurfacets_stack->push(MarkedSurfacet(c_li,neiI,false,
				original_nei0,original_nei1,original_nei2));
			delete_surface_facet(Facet(c_li, neiI));
			//-------------��Ǳ�ɾ�������ϵĵ�--------------------//
			Vertex_triple vf=make_vertex_triple(Facet(c_li, neiI));
			//cout<<"!!delete facet: ("<<vf.first<<","<<vf.second<<","<<vf.third<<")"
			//	<<"/("<<c_li<<","<<neiI<<")"<<endl;
			VertexDeletedFacet[vf.first]++;
			VertexDeletedFacet[vf.second]++;
			VertexDeletedFacet[vf.third]++;
			//===================���end===========================//
		}
	}
	else
	{
		if (label) //cnew������,c_li������
		{
			if (is_surface(Facet(c, li)))
			{
				//delete_surface_facet(Facet(c,li));
				//�棨c,li���ı�Ÿĳ��棨cnew,li����Ӧ
				change_surface_facet_link(Facet(c,li),cnew,li);
				unique_pChangedSurfacets_stack->push(ChangedSurfacet(c,cnew,li));
				//-------------��Ǳ�ɾ�������ϵĵ�--------------------//
				Vertex_triple vfc=make_vertex_triple(Facet(c,li));
				//cout<<"!!change surface facet link: ("<<vfc.first<<","<<vfc.second<<","<<vfc.third<<")"
				//	<<"/("<<c<<","<<li<<")"<<endl;
				VertexCreatedFacet.insert(vfc.first);
				VertexCreatedFacet.insert(vfc.second);
				VertexCreatedFacet.insert(vfc.third);
				//===================���end===========================//
			}
			else
			{
				throw exception();
				//create_surface_facet(cnew,li);
				//NewCreateBoundFacet.push_back(Facet(cnew,li));
				////-----------------��������ɱ����ϵĵ�-------------------//
				//Vertex_triple vfc=make_vertex_triple(Facet(cnew,li));
				////cout<<"!!create facet without the new point: ("<<vfc.first<<","<<vfc.second<<","<<vfc.third<<")"
				////	<<"/("<<cnew<<","<<li<<")"<<endl;
				//VertexCreatedFacet.insert(vfc.first);
				//VertexCreatedFacet.insert(vfc.second);
				//VertexCreatedFacet.insert(vfc.third);
				////=======================���end=========================//
			}
		}
		else  //cnew������,c_li������
		{
			if (!is_surface(Facet(c_li, neighbor_index(c_li, c))))
			{
				throw exception();
				//int neiI=neighbor_index(c_li,c);
				//create_surface_facet(c_li, neiI);
				//NewCreateBoundFacet.push_back(Facet(c_li,neiI));
				////-----------------��������ɱ����ϵĵ�-------------------//
				//Vertex_triple vfc=make_vertex_triple(Facet(c_li,neiI));
				////cout<<"!!create facet without the new point: ("<<vfc.first<<","<<vfc.second<<","<<vfc.third<<")"
				////	<<"/("<<c_li<<","<<neiI<<")"<<endl;
				//VertexCreatedFacet.insert(vfc.first);
				//VertexCreatedFacet.insert(vfc.second);
				//VertexCreatedFacet.insert(vfc.third);
				////=======================���end=========================//
			}
				
		}
	}
	//================================�ж�end===============================
	////neighbor_index(c_li, c)��c��c_li�ĵڼ����ٽ�Cell
	//�趨���ڹ�ϵ��cnew�ĵ�li��������c_li��c_li�ĵ�neighbor_index(c_li, c)��������cnew
	unique_pChangedCellAjcacency_stack->push(ChangedCellAjcacency(c,c_li,neighbor_index(c_li, c)));
	set_adjacency(cnew, li, c_li, neighbor_index(c_li, c));

	// Look for the other neighbors of cnew.
	for (int ii = 0; ii<4; ++ii) {
		//if (ii == prev_ind2 || neighbor(cnew, ii) != -1)
		if (ii==prev_ind2||ii==li)
			continue;
		//cnew->vertex(ii)->set_cell(cnew);
		//��vertex(cnew, ii)��Cell�趨Ϊcnew
		unique_pChangedCellIncident_stack->push(ChangedCellIncident(vertex(cnew,ii),cell(vertex(cnew,ii))));
		set_cell(vertex(cnew, ii), cnew);
		// Indices of the vertices of cnew such that ii,vj1,vj2,li positive.
		Vertex_Idtype vj1 = vertex(c, Triangulation_utils_3::next_around_edge(ii, li));
		Vertex_Idtype vj2 = vertex(c, Triangulation_utils_3::next_around_edge(li, ii));
		Cell_Idtype cur = c;
		int zz = ii;
		Cell_Idtype n = neighbor(cur, zz);
		// turn around the oriented edge vj1 vj2
		//���������vj1->vj2����������vj1->vj2������������,����������vj1->vj2��������Ĵָ���򣩷���������������
		while (is_in_conflict(n)) {
			cur = n;
			zz = Triangulation_utils_3::next_around_edge(vertex_index(n, vj1), vertex_index(n, vj2));
			n = neighbor(cur, zz);
		}
		// Now n is outside region, cur is inside.
		//��cur��Ӱ������n=neighbor(cur,zz)������Ӱ����zz�ڵ��������������滻�β�li
		clear(n); // Reset the flag for boundary cells.

		int jj1 = vertex_index(n, vj1);
		int jj2 = vertex_index(n, vj2);
		Vertex_Idtype vvv = vertex(n, Triangulation_utils_3::next_around_edge(jj1, jj2));
		Cell_Idtype nnn = neighbor(n, Triangulation_utils_3::next_around_edge(jj2, jj1));
		//zzz��ʾ��nnn������Ӱ�������Ѵ��������Զ��㣬�ڵ��������������滻�β�pre_ind2
		int zzz = vertex_index(nnn, vvv);
		//-----------------------------�ж�label�ķ����Ƿ�ı�--------------------------
		bool label2=label;
		bool SideChange2=false;
		if (boundEdge.find(make_pair(vj1, vj2)) != boundEdge.end())
		{
			label2 = !label;
			SideChange2 = true;
			//--------------��ȡ�������еı���----------------//
			if (label == true)
			{
				create_surface_facet(cnew, ii);
				unique_pCreated_DeletedSurfacets_stack->push(MarkedSurfacet(cnew,ii,true,-1,-1,-1));
				//---------for test-------//
				Vertex_triple vtriFTT=make_vertex_triple(Facet(cnew,ii));
				//cout<<"!!create facet when finding the boundEdge: ("<<vtriFTT.first<<","<<vtriFTT.second<<","<<vtriFTT.third<<")"
				//	<<"/("<<cnew<<","<<ii<<")"<<endl;
				//=========test end=======//
		
			}
		}


		//=====================================�ж�end==================================
		//��nnn==curʱ����ʾnnn��Ȼ��Ҫ��һ�����������������壻
		if (nnn == cur) {
			// Neighbor relation is reciprocal, ie
			// the cell we are looking for is not yet created.
			//nnn��Ӱ����neighbor(nnn,zz)������Ӱ����,zz�ڵ��������������滻�β�li��zzz��ʾnnn������Ӱ�������Ѵ��������Զ��㣬�ڵ��������������滻�β�pre_ind2
			nnn = surface_recursive_create_star_3(v, nnn, zz, depth + 1,boundEdge,
				VertexDeletedFacet,VertexCreatedFacet,NewCreateBoundFacet,label2,SideChange2,zzz);
		}
		//��nnn!=curʱ������nnn�Ѹ���Ϊ�����ɵ�cell��ֻ��Ҫ������cnew���ڽӹ�ϵ
		set_adjacency(nnn, zzz, cnew, ii);
	}

	//---------for test-------//
	/*cout<<"recursive depth: "<<depth<<" ending!"<<endl;*/
	//=========test end=======//

	return cnew;
}

template<typename T, typename T_INFO>
Cell_Idtype DataStructure<T, T_INFO>::
non_recursive_create_star_3(Vertex_Idtype v, Cell_Idtype c, int li, int prev_ind2)
{
	

	Cell_Idtype cnew = create_cell(vertex(c,0),
		vertex(c,1),
		vertex(c,2),
		vertex(c,3));
	set_vertex(cnew,li, v);
	Cell_Idtype c_li = neighbor(c,li);
	set_adjacency(cnew, li, c_li, neighbor_index(c_li,c));

	//---------for test-------//
	//cout<<"[non recursive]create cell "<<cnew<<"("<<vertex(cnew, 0)<<","<<vertex(cnew, 1)
	//	<<","<<vertex(cnew, 2)<<","<<vertex(cnew, 3)<<")"<<endl;
	//=========test end=======//


	std::stack<iAdjacency_info> adjacency_info_stack;

	int ii = 0;
	do
	{
		// Look for the other neighbors of cnew.
		if (!(ii == prev_ind2 || neighbor(cnew,ii) != -1)){  //�˴�-1��Cell_Idtype()�滻
			set_cell(vertex(cnew,ii),cnew);

			// Indices of the vertices of cnew such that ii,vj1,vj2,li positive.
			Vertex_Idtype vj1 = vertex(c, Triangulation_utils_3::next_around_edge(ii, li));
			Vertex_Idtype vj2 = vertex(c, Triangulation_utils_3::next_around_edge(li, ii));
			Cell_Idtype cur = c;
			int zz = ii;
			Cell_Idtype n = neighbor(cur,zz);
			// turn around the oriented edge vj1 vj2
			//���������vj1->vj2����������vj1->vj2������������,����������vj1->vj2��������Ĵָ���򣩷���������������
			while (is_in_conflict(n)) {
				cur = n;
				zz = Triangulation_utils_3::next_around_edge(vertex_index(n, vj1), vertex_index(n, vj2));
				n = neighbor(cur,zz);
			}
			// Now n is outside region, cur is inside.
			clear(n); // Reset the flag for boundary cells.

			int jj1 = vertex_index(n,vj1);
			int jj2 = vertex_index(n,vj2);
			Vertex_Idtype vvv = vertex(n, Triangulation_utils_3::next_around_edge(jj1, jj2));
			Cell_Idtype nnn = neighbor(n, Triangulation_utils_3::next_around_edge(jj2, jj1));
			int zzz = vertex_index(nnn,vvv);
			//��nnn==curʱ����ʾnnn��Ȼ��Ҫ��һ�������������壻
			if (nnn == cur) {
				// Neighbor relation is reciprocal, ie
				// the cell we are looking for is not yet created.
				//re-run the loop
				adjacency_info_stack.push(iAdjacency_info(zzz, cnew, ii, c, li, prev_ind2));
				c = nnn;  li = zz; prev_ind2 = zzz; ii = 0;
				//copy-pasted from beginning to avoid if ii==0
				cnew = create_cell(vertex(c,0), vertex(c,1), vertex(c,2), vertex(c,3));
				set_vertex(cnew,li, v);
				c_li = neighbor(c,li);
				set_adjacency(cnew, li, c_li, neighbor_index(c_li,c));
				continue;
			}
			set_adjacency(nnn, zzz, cnew, ii);//��nnn!=curʱ������nnn�Ѹ���Ϊ�����ɵ�cell��ֻ��Ҫ������cnew���ڽӹ�ϵ
		}
		while (++ii == 4){
			if (adjacency_info_stack.empty()) return cnew;
			Cell_Idtype nnn = cnew;
			int zzz;
			adjacency_info_stack.top().update_variables(zzz, cnew, ii, c, li, prev_ind2);
			adjacency_info_stack.pop();
			set_adjacency(nnn, zzz, cnew, ii);
		}
	} while (true);
}

template<typename T, typename T_INFO>
Cell_Idtype DataStructure<T, T_INFO>::
create_star_2(Vertex_Idtype v, Cell_Idtype c, int li)
{

	Cell_Idtype cnew;

	// i1 i2 such that v,i1,i2 positive
	int i1 = Triangulation_cw_ccw_2::ccw(li);
	// traversal of the boundary of region in Triangulation_cw_ccw_2::ccw order to create all
	// the new facets
	Cell_Idtype bound = c;
	Vertex_Idtype v1 = vertex(c,i1);
	//int ind = c->neighbor(li)->index(c); // to be able to find the
	int ind = neighbor_index(neighbor(c,li),c);
	// first cell that will be created
	Cell_Idtype cur;
	//Cell_Idtype pnew = Cell_Idtype();
	Cell_Idtype pnew = -1;
	do {
		cur = bound;
		// turn around v2 until we reach the boundary of region
		while (is_in_conflict(neighbor(cur,Triangulation_cw_ccw_2::cw(i1)))) {
			// neighbor in conflict
			cur = neighbor(cur,Triangulation_cw_ccw_2::cw(i1));
			i1 = vertex_index(cur,v1);
		}
		//cur->neighbor(Triangulation_cw_ccw_2::cw(i1))->tds_data().clear();
		clear(neighbor(cur,Triangulation_cw_ccw_2::cw(i1)));
		// here cur has an edge on the boundary of region
		cnew = create_face(v, v1, vertex(cur,Triangulation_cw_ccw_2::ccw(i1)));
		set_adjacency(cnew, 0, neighbor(cur,Triangulation_cw_ccw_2::cw(i1)),
			neighbor_index(neighbor(cur,Triangulation_cw_ccw_2::cw(i1)),cur));
		set_neighbor(cnew,1, -1);
		set_neighbor(cnew,2, pnew);
		// pnew is null at the first iteration
		set_cell(v1,cnew);
		//pnew->set_neighbor( Triangulation_cw_ccw_2::cw(pnew->index(v1)), cnew );
		if (pnew != -1) { set_neighbor(pnew,1, cnew); }

		bound = cur;
		i1 = Triangulation_cw_ccw_2::ccw(i1);
		v1 = vertex(bound,i1);
		pnew = cnew;
		//} while ( ( bound != c ) || ( li != Triangulation_cw_ccw_2::cw(i1) ) );
	} while (v1 != vertex(c,Triangulation_cw_ccw_2::ccw(li)));
	// missing neighbors between the first and the last created cells
	cur = neighbor(neighbor(c,li),ind); // first created cell
	set_adjacency(cnew, 1, cur, 2);
	return cnew;
}



//���棨cell��Id���а���ʱ��ʵ�������ţ�i,j,k��,���Ҹ����������cell��ʣ��һ�����������������
template<typename T, typename T_INFO>
Vertex_triple DataStructure<T, T_INFO>::
make_vertex_triple(Facet& f)
{
	Cell_Idtype ch = f.first;
	int i = f.second;

	//vertex_triple_index(i, j),iΪ��������Զ����ţ�j(0/1/2)��i�������ı�ŵĵ㣬���ظõ��ڸ����������Ա��
	//vertex(Idc,I)ȡ���Idc��cell�ĵ�I���㣻
	return Vertex_triple(vertex(ch, Triangulation_utils_3::vertex_triple_index(i, 0)),
		vertex(ch, Triangulation_utils_3::vertex_triple_index(i, 1)),
		vertex(ch, Triangulation_utils_3::vertex_triple_index(i, 2)));
}

template<typename T, typename T_INFO>
void DataStructure<T, T_INFO>::
insert_first_finite_cell()
{
	const double *p0=point(1);
	const double *p1=point(2);
	const double *p2=point(3);
	const double *p3=point(4);

	if(GeometricTraits<T>::orientation(p0, p1, p2, p3) == NEGATIVE)
	{
		double pTemp[3];
		pTemp[0]=p0[0];
		pTemp[1]=p0[1];
		pTemp[2]=p0[2];
		set_point(1,p1);
		set_point(2,pTemp);
	}
	set_dimension(3);
	int v0=1,v1=2,v2=3,v3=4;
	//create_cell�ĸ��β��Ƕ����ţ����ζ�Ӧ���������λ��v0,v1,v2,v3
	Cell_Idtype c0123 = create_cell(v0, v1, v2, v3);         //�������ֶ���
	Cell_Idtype ci012 = create_cell(Infinite, v0, v1, v2);   //�������ֶ���
	Cell_Idtype ci103 = create_cell(Infinite, v1, v0, v3);   //�������ֶ���
	Cell_Idtype ci023 = create_cell(Infinite, v0, v2, v3);   //�������ֶ���
	Cell_Idtype ci132 = create_cell(Infinite, v1, v3, v2);   //�������ֶ���


	//����Infinite��cell�趨Ϊci012
	set_cell(Infinite,ci012);
	set_cell(v0,c0123);   //����v0��cell�趨Ϊc0123
	set_cell(v1,c0123);   //����v1��cell�趨Ϊc0123
	set_cell(v2,c0123);   //����v2��cell�趨Ϊc0123
	set_cell(v3,c0123);   //����v3��cell�趨Ϊc0123


	//�趨���ڹ�ϵ��c0123�ĵ�0��������ci132��ci132�ĵ�0��������c0123
	set_adjacency(c0123, 0, ci132, 0);  
	set_adjacency(c0123, 1, ci023, 0);    //�趨���ڹ�ϵ��c0123�ĵ�1��������ci023��ci023�ĵ�0��������c0123
	set_adjacency(c0123, 2, ci103, 0);    //�趨���ڹ�ϵ��c0123�ĵ�2��������ci103��ci103�ĵ�0��������c0123
	set_adjacency(c0123, 3, ci012, 0);    //�趨���ڹ�ϵ��c0123�ĵ�3��������ci012��ci012�ĵ�0��������c0123

	set_adjacency(ci012, 3, ci103, 3);
	set_adjacency(ci012, 2, ci023, 3);
	set_adjacency(ci012, 1, ci132, 2);
	set_adjacency(ci103, 1, ci023, 2);
	set_adjacency(ci023, 1, ci132, 1);
	set_adjacency(ci103, 2, ci132, 3);
	//HHHHHHHHHHHHHHHHHHH-------------------surface extractor-------------------HHHHHHHHHHHHHHHHHHHHHH
	if (this->Surface)
	{
		Facet_Idtype f012 = create_surface_facet(c0123, 3);
		Facet_Idtype f013 = create_surface_facet(c0123, 2);
		Facet_Idtype f023 = create_surface_facet(c0123, 1);
		Facet_Idtype f123 = create_surface_facet(c0123, 0);
		//����make_vertex_triple��ȡ��ķ���
		//FacetNeighbor��ʱ������
		Indextype i0,i1;
		

		//vertex_index_facet(Facet F,Vertex_Idtype IdV)���IdV��facet F�ĵڼ�����
		i0=vertex_index_facet(Facet(c0123,3),3);
		i1=vertex_index_facet(Facet(c0123,2),4);
		//f012��i0����������f013,f013�ĵ�i1����������f012
		set_facet_adjacency(f012,i0,f013,i1);

		i0=vertex_index_facet(Facet(c0123,3),2);
		i1=vertex_index_facet(Facet(c0123,1),4);
		set_facet_adjacency(f012,i0,f023,i1);

		i0=vertex_index_facet(Facet(c0123,3),1);
		i1=vertex_index_facet(Facet(c0123,0),4);
		set_facet_adjacency(f012,i0,f123,i1);

		i0=vertex_index_facet(Facet(c0123,2),2);
		i1=vertex_index_facet(Facet(c0123,1),3);
		set_facet_adjacency(f013,i0,f023,i1);

		i0=vertex_index_facet(Facet(c0123,2),1);
		i1=vertex_index_facet(Facet(c0123,0),3);
		set_facet_adjacency(f013,i0,f123,i1);

		i0=vertex_index_facet(Facet(c0123,1),1);
		i1=vertex_index_facet(Facet(c0123,0),2);
		set_facet_adjacency(f023,i0,f123,i1);

		//��ע
		label_cell_side(c0123, true);
		label_cell_side(ci012, false);
		label_cell_side(ci023, false);
		label_cell_side(ci103, false);
		label_cell_side(ci132, false);
	}

}

template<typename T, typename T_INFO>
Vertex_Idtype DataStructure<T, T_INFO>::
insert_first_finite_cell(Vertex_Idtype &v0, Vertex_Idtype &v1, Vertex_Idtype &v2, Vertex_Idtype &v3,
Vertex_Idtype Infinite)
{

	if (Infinite == -1)
		Infinite = create_vertex();
	//--------------------��data structure�м���infinite vertex-------------------//
	Infinite=Infinite;
	//============================����end=========================================//

	set_dimension(3);

	v0 = create_vertex();
	v1 = create_vertex();
	v2 = create_vertex();
	v3 = create_vertex();

	Cell_Idtype c0123 = create_cell(v0, v1, v2, v3);
	Cell_Idtype ci012 = create_cell(Infinite, v0, v1, v2);
	Cell_Idtype ci103 = create_cell(Infinite, v1, v0, v3);
	Cell_Idtype ci023 = create_cell(Infinite, v0, v2, v3);
	Cell_Idtype ci132 = create_cell(Infinite, v1, v3, v2);

	set_cell(Infinite,ci012);
	set_cell(v0,c0123);
	set_cell(v1,c0123);
	set_cell(v2,c0123);
	set_cell(v3,c0123);

	set_adjacency(c0123, 0, ci132, 0);
	set_adjacency(c0123, 1, ci023, 0);
	set_adjacency(c0123, 2, ci103, 0);
	set_adjacency(c0123, 3, ci012, 0);

	set_adjacency(ci012, 3, ci103, 3);
	set_adjacency(ci012, 2, ci023, 3);
	set_adjacency(ci012, 1, ci132, 2);
	set_adjacency(ci103, 1, ci023, 2);
	set_adjacency(ci023, 1, ci132, 1);
	set_adjacency(ci103, 2, ci132, 3);
	//HHHHHHHHHHHHHHHHHHH-------------------surface extractor-------------------HHHHHHHHHHHHHHHHHHHHHH
	if (this->Surface)
	{
		Facet_Idtype f012 = create_surface_facet(c0123, 3);
		Facet_Idtype f013 = create_surface_facet(c0123, 2);
		Facet_Idtype f023 = create_surface_facet(c0123, 1);
		Facet_Idtype f123 = create_surface_facet(c0123, 0);
		//??????????????����������ݽṹ��ô�洢��facet��û�д�㣬Ҳ��֪�����index��
		//����make_vertex_triple��ȡ��ķ���
		//FacetNeighbor��ʱ������
		Indextype i0,i1;
		
		i0=vertex_index_facet(Facet(c0123,3),3);
		i1=vertex_index_facet(Facet(c0123,2),4);
		set_facet_adjacency(f012,i0,f013,i1);

		i0=vertex_index_facet(Facet(c0123,3),2);
		i1=vertex_index_facet(Facet(c0123,1),4);
		set_facet_adjacency(f012,i0,f023,i1);

		i0=vertex_index_facet(Facet(c0123,3),1);
		i1=vertex_index_facet(Facet(c0123,0),4);
		set_facet_adjacency(f012,i0,f123,i1);

		i0=vertex_index_facet(Facet(c0123,2),2);
		i1=vertex_index_facet(Facet(c0123,1),3);
		set_facet_adjacency(f013,i0,f023,i1);

		i0=vertex_index_facet(Facet(c0123,2),1);
		i1=vertex_index_facet(Facet(c0123,0),3);
		set_facet_adjacency(f013,i0,f123,i1);

		i0=vertex_index_facet(Facet(c0123,1),1);
		i1=vertex_index_facet(Facet(c0123,0),2);
		set_facet_adjacency(f023,i0,f123,i1);

		//��ע
		label_cell_side(c0123, true);
		label_cell_side(ci012, false);
		label_cell_side(ci023, false);
		label_cell_side(ci103, false);
		label_cell_side(ci132, false);
	}
	

	return Infinite;
}



template<typename T, typename T_INFO>
template<typename OutputIteratorCell>
void DataStructure<T, T_INFO>::
incident_cells(Vertex_Idtype v, OutputIteratorCell outputCell) 
{


	if (dimension() < 2)
		return;

	//Visitor visit(v, outputCell, this, f);

	std::vector<Cell_Idtype> tmp_cells;
	tmp_cells.reserve(64);
	std::vector<Facet> tmp_facets;
	tmp_facets.reserve(64);
	std::vector<Vertex_Idtype> temp_vertices;
	if (dimension() == 3)
		incident_cells_3(v, cell(v), std::make_pair(std::back_inserter(tmp_cells), std::back_inserter(tmp_facets)));
	/*else
		incident_cells_2(v, cell(v), std::back_inserter(tmp_cells));*/

	typename std::vector<Cell_Idtype>::iterator cit;
	for (cit = tmp_cells.begin();
		cit != tmp_cells.end();
		++cit)
	{
		clear(*cit);

		*outputCell++ = *cit;
		
	}
	
}

template<typename T, typename T_INFO>
template<typename OutputIteratorCell,typename OutputIteratorVertex,typename OutputIteratorFacet>
void DataStructure<T, T_INFO>::
incident_cells_mark_side(Vertex_Idtype v, OutputIteratorCell outputCell,OutputIteratorVertex outputVertex,
	OutputIteratorFacet outputFacet,bool& IsIso,bool& PInside) 
{
	std::vector<Cell_Idtype> tmpCells;
	tmpCells.reserve(32);
	std::stack<Cell_Idtype> cell_stack;
	Cell_Idtype d=cell(v);
	cell_stack.push(d);
	mark_in_conflict(d);
	*outputCell++ = d;
	tmpCells.push_back(d);
	IsIso=true;
	PInside=is_label_inside(d);
	do {
		Cell_Idtype c = cell_stack.top();
		cell_stack.pop();

		for (int i = 0; i<4; ++i) {
			if (vertex(c,i) == v)
				continue;
			Cell_Idtype next = neighbor(c,i);
			if (is_surface(Facet(c,i)))
			{
				IsIso=false;
				*outputFacet++=Facet(c,i);
				for(int j=0;j<4;j++)
				{
					if(j!=i)
					{
						Vertex_Idtype vEdge=vertex(c,j);
						if(vEdge!=v&&visited_for_vertex_extractor(vEdge)==0)
						{
							*outputVertex++=vEdge;
							mark_vertex_visited(vEdge);
						}
					}
				}

			}
			if (!is_clear(next))
				continue;
			cell_stack.push(next);
			//next->tds_data().mark_in_conflict();
			mark_in_conflict(next);
			*outputCell++ = next;
			tmpCells.push_back(next);
		}
	} while (!cell_stack.empty());

	typename std::vector<Cell_Idtype>::iterator cit;
	for (cit = tmpCells.begin();
		cit != tmpCells.end();
		++cit)
	{
		clear(*cit);
	
		//=================vertex extractor===============
		for (int j = 0; j <= dimension(); ++j) {
			Vertex_Idtype w = vertex(*cit,j);
			
			if (w != v){

				if (visited_for_vertex_extractor(w)==0){
					//w->visited_for_vertex_extractor = true;
					if(is_label_inside(*cit))
					{
						mark_vertex_visited(w);
						mark_vertex_visited(w);
						mark_vertex_visited(w);
					}
					else
					{
						mark_vertex_visited(w);
						mark_vertex_visited(w);
					}
					
					*outputVertex++ = w;
					//treat(c, v, j);
				}
			}
		}
		//----------------------vertex extractor end-------------------
	}
	


}

template<typename T, typename T_INFO>
template<typename OutputIteratorFacet, typename OutputIteratorVertex>
void DataStructure<T, T_INFO>::
incident_cells(Vertex_Idtype v,  
				OutputIteratorFacet outputFacets, OutputIteratorVertex outputVertex) 
{

	if (dimension() < 2)
		return ;
	std::vector<Cell_Idtype> tmp_cells;
	tmp_cells.reserve(64);

	std::vector<Vertex_Idtype> tmp_vertices;

	if (dimension() == 3)
		incident_cells_3(v, cell(v), std::make_pair(std::back_inserter(tmp_cells), outputFacets));


	typename std::vector<Cell_Idtype>::iterator cit;
	for (cit = tmp_cells.begin();
		cit != tmp_cells.end();
		++cit)
	{
		clear(*cit);

		for (int j = 0; j <= dimension(); ++j) {
			Vertex_Idtype w = vertex(*cit,j);
			
			if (w != v){

				if (visited_for_vertex_extractor(w)==0){
					mark_vertex_visited(w);
					tmp_vertices.push_back(w);
					*outputVertex++ = w;
				}
			}
		}
	}
	for (std::size_t i = 0; i < tmp_vertices.size(); ++i){
		clear_vertex_visited(tmp_vertices[i]);
	}

}



template<typename T, typename T_INFO>
template<typename IncidentCellIterator, typename IncidentFacetIterator>
void DataStructure<T, T_INFO>::
incident_cells_3(Vertex_Idtype v, Cell_Idtype d,
						std::pair<IncidentCellIterator,
						IncidentFacetIterator> it) 
{

	std::stack<Cell_Idtype> cell_stack;
	cell_stack.push(d);
	mark_in_conflict(d);
	*it.first++ = d;

	do {
		Cell_Idtype c = cell_stack.top();
		cell_stack.pop();

		for (int i = 0; i<4; ++i) {
			if (vertex(c,i) == v)
				continue;
			Cell_Idtype next = neighbor(c,i);
			if (c < next)
				*it.second++ = Facet(c, i); // Incident facet.
			if (!is_clear(next))
				continue;
			cell_stack.push(next);
			mark_in_conflict(next);
			*it.first++ = next;
		}
	} while (!cell_stack.empty());

}



template<typename T, typename T_INFO>
template<typename OutputIteratorVertex, typename OutputIteratorFacet>
void DataStructure<T,T_INFO>::
adjacent_vertices_facets(Vertex_Idtype v,
		OutputIteratorFacet outputFacets, OutputIteratorVertex vertices) 
{
	if (dimension() == -1)
		return ;

	if (dimension() == 0) {
		//Vertex_Idtype v1 = v->cell()->neighbor(0)->vertex(0);
		Vertex_Idtype v1 = vertex(neighbor(cell(v),0),0);
		 *vertices++ = v1;
		return ;
	}

	if (dimension() == 1) {
		Cell_Idtype n0 = cell(v);
		const int index_v_in_n0 = vertex_index(n0,v);
		Cell_Idtype n1 = neighbor(n0,(1 - index_v_in_n0));
		const int index_v_in_n1 = vertex_index(n1,v);
		Vertex_Idtype v1 = vertex(n0,(1 - index_v_in_n0));
		Vertex_Idtype v2 = vertex(n1,(1 - index_v_in_n1));
		 *vertices++ = v1;
		 *vertices++ = v2;
		return ;
	}
	incident_cells(v, outputFacets, vertices);

}



//��p����f�����������У���ı�
template<typename T, typename T_INFO>
double DataStructure<T, T_INFO>::
longest_edge_to_facet(const T* p,Facet f)
{
	double longestLength = 0;
	Vertex_triple vertexFacet = make_vertex_triple(f);
	Vertex_Idtype vertexTemp = -1;
	for (int i = 0; i < 3; i++)
	{
		//vertexTemp = vertexFacet.get<i>();
		if (i == 0)
			vertexTemp = vertexFacet.first;
		else if (i == 1)
			vertexTemp = vertexFacet.second;
		else
			vertexTemp = vertexFacet.third;
			
		
		double distantTemp = distant_to_vertex(p,vertexTemp);
		if (distantTemp > longestLength)
			longestLength = distantTemp;
		
	}
	return longestLength;
}


template<typename T, typename T_INFO>
double DataStructure<T, T_INFO>::
shortest_edge_to_facet(const T* p,Facet f)
{
	double shortestLength = 0;
	Vertex_triple vertexFacet = make_vertex_triple(f);
	Vertex_Idtype vertexTemp = -1;
	for (int i = 0; i < 3; i++)
	{
		if (i == 0)
			vertexTemp = vertexFacet.first;
		else if (i == 1)
			vertexTemp = vertexFacet.second;
		else
			vertexTemp = vertexFacet.third;
			
		
		double distantTemp = distant_to_vertex(p,vertexTemp);
		if (distantTemp > shortestLength)
			shortestLength = distantTemp;
		
	}
	return shortestLength;
}

template<typename T, typename T_INFO>
bool DataStructure<T, T_INFO>::
neighbor_surface_facet(EdgeInFacet SurfaceEdge,Facet& NeighborFacet, bool& IsToDelete)
{
	Facet surfaceFacet = SurfaceEdge.first;
	int ii = SurfaceEdge.second;
	Cell_Idtype c= surfaceFacet.first;
	int li = surfaceFacet.second;
	Vertex_Idtype vj1 = vertex(c, Triangulation_utils_3::next_around_edge(ii, li));
	Vertex_Idtype vj2 = vertex(c, Triangulation_utils_3::next_around_edge(li, ii));
	Cell_Idtype cur = c;
	int zz = ii;
	Cell_Idtype n = neighbor(cur,zz);
	// turn around the oriented edge vj1 vj2
	while (is_label_inside(n)) {
		
		cur = n;
		zz = Triangulation_utils_3::next_around_edge(index(n, vj1), index(n, vj2));
		n = neighbor(cur,zz);
	}
	if (is_facet_to_delete(Facet(cur, zz)))
	{
		IsToDelete = true;
	}
	else
		IsToDelete = false;
	if (is_surface(Facet(cur, zz)))
	{
		NeighborFacet = Facet(cur, zz);
		return true;
	}
	else
		return false;
	
}


//�ڹ����������,ʹ��Gabriel�Ĺ�������滻�����չ
//V������;InitFacetΪ����������Ƭ;PInsideΪ������V���ڱ����λ��
//VertexOnBoundaryΪ�߽����ϵĶ��㣬���ڹ�����ļ��;
//VertexOnConflictFacetΪ���滻���������Ƭ�еĶ���,���ڹ�����ļ��;Conflicts�Ǳ��滻��;
//BoundEdgesΪ����ֵ�����滻��������ı߽��߼��ϣ���Edge��ʽ(��id,0/1/2)��ʾ,���Ҹ��治�滻
//NewCreateSurfacets�������ɱ���ļ���
template<typename T, typename T_INFO>
void DataStructure<T, T_INFO>::
find_isolate_conflict_surfacet(Vertex_Idtype V, Facet InitFacet,bool PInside,std::set<Vertex_Idtype>& VertexOnBoundary,
							set<Vertex_Idtype>& VertexOnConflictFacet,vector<Facet>& Conflicts,list<Edge>& BoundEdges,vector<Facet>& NewCreateSurfacets,bool& quit)
{
	const T* P=point(V);
	list<Facet> illSurfacets;
	vector<Facet> tmpSurfacets;
	list<Facet> neighborSurfacets;
	neighborSurfacets.push_back(InitFacet);
	tmpSurfacets.push_back(InitFacet);
	mark_facet_visited(InitFacet);
	mark_facet_visited(InitFacet);

	vector<Facet_Idtype> idtmpSurfacets;
	Facet_Idtype fid=CellSurface.get_tuple_element(InitFacet.first,InitFacet.second);
	idtmpSurfacets.push_back(fid);
	//-----------------��InitFacet����ı���������Ƭ������InitFacet�й�����ģ�------------------//
	while(!neighborSurfacets.empty())
	{
		Facet tmpSurfacet=*neighborSurfacets.begin();
		//--------------for test------------//
		Vertex_triple vtriT0=make_vertex_triple(tmpSurfacet);
		//===============test end==========//
		neighborSurfacets.pop_front();
		Cell_Idtype c = tmpSurfacet.first;//HHHHHHHHHHHHHHHHHHHHHHHHHH-------c�϶���labelΪinside
		int li = tmpSurfacet.second;
		Facet nei;
		for(int i=0;i<3;i++)
		{
			nei=neighbor_surfacet(tmpSurfacet,i);
			//--------------���ڹ�����Ĵ���------------//
			Cell_Idtype cur=nei.first;
			Indextype zz=nei.second;
			Cell_Idtype cOpp=neighbor(cur,zz);
			Indextype indOpp=neighbor_index(cOpp,cur);
			//==============���ڹ�����Ĵ���===========//
			//--------------for test------------//
			Vertex_triple vtriT1=make_vertex_triple(nei);
			//===============test end==========//
			if(visited_for_facet_extractor(nei)==0)
			{
				//�������²������
				if(is_facet_in_conflict(nei))
				{
					mark_facet_visited(nei);
					mark_facet_visited(nei);
					neighborSurfacets.push_back(nei);
					tmpSurfacets.push_back(nei);
				}
				//��ѡ���滻��Ӧ��Ӧ����һ��cell,������Թ�����
				if((PInside&&vertex(cur,zz)==V)||(!PInside&&vertex(cOpp,indOpp)==V))
				//if((!IsIsolate&&((is_on_boundary(nei.first)&&is_in_conflict(neighbor(nei.first,nei.second)))||
					//(is_on_boundary(neighbor(nei.first,nei.second))&&is_in_conflict(nei.first))))||(IsIsolate&&((PInside&&vertex(cur,zz)==V)||(!PInside&&vertex(cOpp,indOpp)==V))))
				{
					T cosDihedral[3];
					//if(PInside)
					cos_dihedral_point_to_facet(P,nei,cosDihedral);
				
					
		
					//���뷽���ж�
					Vertex_triple vf=make_vertex_triple(nei);
					Orientation o = GeometricTraits<T>::orientation(point(vf.first), point(vf.second), point(vf.third), P);
					if ((o!=NEGATIVE&&PInside)||(o!=POSITIVE&&!PInside))
					{
						//�жϼ���nei�Ƿ�Ϊ͹��չ
						bool isConvexExtension=true;
						for(int i=0;i<3;i++)
						{
							Facet neiEdge=neighbor_surfacet(nei,i);
							//--------------------------for test-------------------------//
							Vertex_triple vtriT3=make_vertex_triple(neiEdge);
							//==========================test end=========================//
							//if(cosDihedral[i]<0&&neighbor_surfacet())
							if(cosDihedral[i]<0&&!is_in_conflict(neiEdge.first)&&!is_in_conflict(neighbor(neiEdge.first,neiEdge.second)))
							{
								isConvexExtension=false;
								break;
							}
						}
						//�ı�ѡ���滻��Ĺ�����pInsideΪ���������Ƿ����Gabriel����Ϊ׼��
						if (PInside)
						{
							bool toBeSurface=false;

							for (int j = 0; j < 3; j++)
							{
								Facet NNei=neighbor_surfacet(nei,i);
								if (visited_for_facet_extractor(NNei)==2)
								{
									Vertex_Idtype vertexOpp=vertex_facet(nei,i);
									Vertex_pair vertexOppEdge=make_vertex_pair(EdgeInFacet(nei,j));
									bool isInShpere=GeometricTraits<T>::side_of_bounded_sphereC3(point(vertexOppEdge.first),P,
										point(vertexOppEdge.second),point(vertexOpp))==ON_BOUNDED_SIDE;
									if (isInShpere)
									{
										toBeSurface=true;
									}
								}
							}
							if (!toBeSurface)
							{
								Vertex_triple ftrip=make_vertex_triple(nei);
								toBeSurface=GeometricTraits<T>::side_of_bounded_sphereC3(point(ftrip.first),
									point(ftrip.second),point(ftrip.third),P)==ON_BOUNDED_SIDE;
							}
							if(toBeSurface)//&&isConvexExtension)
							{
								mark_facet_visited(nei);
								mark_facet_visited(nei);
								neighborSurfacets.push_back(nei);
								tmpSurfacets.push_back(nei);

								Facet_Idtype fid=CellSurface.get_tuple_element(nei.first,nei.second);
								idtmpSurfacets.push_back(fid);
							}
							else
							{
								mark_facet_visited(nei);
								illSurfacets.push_back(nei);
							}
						}
						else
						{
							bool toBeSurface=true;
							//Facet facetOpp=mirror_facet(nei);
							for (int j = 0; j < 3; j++)
							{
								Facet NNei=neighbor_surfacet(nei,i);
								if (visited_for_facet_extractor(NNei)!=2)
								{
									Vertex_Idtype vertexOpp=vertex_facet(nei,i);
									Vertex_pair vertexOppEdge=make_vertex_pair(EdgeInFacet(nei,j));
									bool isInShpere=GeometricTraits<T>::side_of_bounded_sphereC3(point(vertexOppEdge.first),
										point(vertexOppEdge.second),P,point(vertexOpp))==ON_BOUNDED_SIDE;
									if (isInShpere)
									{
										toBeSurface=false;
									}
								}
							}
							if(toBeSurface)//&&isConvexExtension)
							{
								mark_facet_visited(nei);
								mark_facet_visited(nei);
								neighborSurfacets.push_back(nei);
								tmpSurfacets.push_back(nei);

								Facet_Idtype fid=CellSurface.get_tuple_element(nei.first,nei.second);
								idtmpSurfacets.push_back(fid);
							}
							else
							{
								mark_facet_visited(nei);
								illSurfacets.push_back(nei);
							}
						}
					}
				}
			}
		}
	}
	//����һ������õĽ�����ɾ�����еĿ�������۵���surface facet
	while(!illSurfacets.empty())
	{
		Facet tmpIllSurfacet=*(illSurfacets.begin());
		clear_facet_visited(tmpIllSurfacet);
		illSurfacets.pop_front();
		for(int i=0;i<3;i++)
		{
			Facet nei=neighbor_surfacet(tmpIllSurfacet,i);
			//---------------for test------------//
			Vertex_triple vtriNei=make_vertex_triple(nei);
			//==============test end============//
			if(visited_for_facet_extractor(nei)==2&&!is_facet_in_conflict(nei)&&nei!=InitFacet)
			{
				Indextype iNei=neighbor_index_facet(nei,tmpIllSurfacet);
				if(cos_dihedral_point_to_Edge(P,nei,iNei)<0)
				{
					illSurfacets.push_back(nei);
					clear_facet_visited(nei);

					Facet_Idtype fid=CellSurface.get_tuple_element(nei.first,nei.second);
					idtmpSurfacets.erase(find(idtmpSurfacets.begin(),idtmpSurfacets.end(),fid));
				}
			}
		}
	}

 //   if(idtmpSurfacets.size()>0)
	//{

	//	for(auto itt=tmpSurfacets.begin();itt!=tmpSurfacets.end();itt++)
	//	{
	//		clear_facet_visited(*itt);
	//	}
	//	quit=true;
	//	return;
	//}

	//����������������ƬΪ�����������������Ľ�����������Ƭ����߽��
	//����������������ƬΪ�����������������Ľ�����������Ƭ����߽��
	//EdgeInFacet��(Facet,0/1/2)��ʾ,����Facet��(cell,0/1/2/3)��ʾ
	//BoundEdges���滻��������ı߽��߼��ϣ���Edge��ʽ(��id,0/1/2)��ʾ,���Ҹ��治�滻
	//�߽������ǰ���˳ʱ��˳�򣬳�ʼ��Ҳ����˳ʱ��(�ӱ����ڿ�,�����⿴����ʱ��),���԰�����Զ���0->2->1�����ʼ���߽��
	mark_facet_visited(InitFacet);
	EdgeInFacet tmpBoundEdge=mirror_edge(EdgeInFacet(InitFacet,0));
	BoundEdges.push_back(turn_edgeinfacet_to_edge(tmpBoundEdge));
	tmpBoundEdge=mirror_edge(EdgeInFacet(InitFacet,2));
	BoundEdges.push_back(turn_edgeinfacet_to_edge(tmpBoundEdge));
	tmpBoundEdge=mirror_edge(EdgeInFacet(InitFacet,1));
	BoundEdges.push_back(turn_edgeinfacet_to_edge(tmpBoundEdge));
	list<Facet> tmpConflitFacets;
	tmpConflitFacets.push_back(InitFacet);
	/*cout<<"���滻��: "<<endl;*/
	while(!tmpConflitFacets.empty())
	{
		Facet tmp=*(tmpConflitFacets.begin());
		Conflicts.push_back(tmp);
		tmpConflitFacets.pop_front();
		Indextype li=tmp.second;
		Vertex_triple vtrif=make_vertex_triple(tmp);
		VertexOnConflictFacet.insert(vtrif.first);
		VertexOnConflictFacet.insert(vtrif.second);
		VertexOnConflictFacet.insert(vtrif.third);
		/*VertexOnConflictFacet[vtrif.first]++;
		VertexOnConflictFacet[vtrif.second]++;
		VertexOnConflictFacet[vtrif.third]++;*/
		//------------------for test--------------//
		Vertex_triple vtriTC0=make_vertex_triple(tmp);
		Cell_Idtype tmpc=tmp.first;
		Facet_Idtype fid=CellSurface.get_tuple_element(tmpc,tmp.second);
		//cout<<fid<<"("<<vtriTC0.first<<","<<vtriTC0.second<<","<<vtriTC0.third<<")"
		//	<<"/("<<tmp.first<<","<<tmp.second<<")["<<tmpc<<"("
		//	<<vertex(tmpc, 0)<<","<<vertex(tmpc, 1)<<","<<vertex(tmpc, 2)<<","<<vertex(tmpc, 3)<<")]"<<endl;
		//==================test end==============//
		
		for(int i=0;i<3;i++)
		{
			Facet nei=neighbor_surfacet(tmp,i);
			//---------------for test------------//
			Vertex_triple vtriT=make_vertex_triple(nei);
			//===============test end============//
			Indextype iNei=neighbor_index_facet(nei,tmp);
			if(visited_for_facet_extractor(nei)==2)
			{
				mark_facet_visited(nei);
				tmpConflitFacets.push_back(nei);
				//���½�����ı߽�߶���
				vector<int> numNei;
				int numOfList=0;

				//--------------�޸�------------//
				auto itEdge=BoundEdges.begin();
				for(;itEdge!=BoundEdges.end();itEdge++)
				{
					numOfList++;
					Facet tmpBoundFacet0=get_facet_cell_link((*itEdge).first);
					//--------------for test----------//
					Vertex_triple vtriT0=make_vertex_triple(tmpBoundFacet0);
					//==============test end==========//
					if(tmpBoundFacet0==nei)
					{
						numNei.push_back((*itEdge).second);
						BoundEdges.erase(itEdge++);
						break;
					}
				}
				//��list���ĵ�һ����ɾ��ʱ��Ӧ�ü�����һ���Ƿ�ҲӦ�ñ�ɾ��
				if(numOfList==1)
				{
					auto endEdge=BoundEdges.back();
					Facet tmpBoundFacet00=get_facet_cell_link(endEdge.first);
					//--------------for test----------//
					Vertex_triple vtriT00=make_vertex_triple(tmpBoundFacet00);
					//==============test end=========//
					if(tmpBoundFacet00==nei)
					{
					
						numNei.push_back(endEdge.second);						
						BoundEdges.pop_back();
						endEdge=BoundEdges.back();
						Facet tmpBoundFacet01=get_facet_cell_link(endEdge.first);
						//--------------for test----------//
						Vertex_triple vtriT01=make_vertex_triple(tmpBoundFacet00);
						//==============test end=========//
						if(tmpBoundFacet01==nei)
						{
							numNei.push_back(endEdge.second);						
							BoundEdges.pop_back();
						}
					}
					
				}
				if(numNei.size()!=3)
				{
					//list �м�Ĳ�����ɾ��Ҫ�����
					Facet tmpBoundFacet1=make_pair(-1,-1);
					if(itEdge!=BoundEdges.end())
					{
						tmpBoundFacet1=get_facet_cell_link((*itEdge).first);
						//----------------for test------------//
						Vertex_triple vtriT1=make_vertex_triple(tmpBoundFacet1);
					}
					//================test end===========//
					if(itEdge!=BoundEdges.end()&&tmpBoundFacet1==nei)
					{
						numNei.push_back((*itEdge).second);
						BoundEdges.erase(itEdge++);
						Facet tmpBoundFacet1=make_pair(-1,-1);
						if(itEdge!=BoundEdges.end())
						{
							tmpBoundFacet1=get_facet_cell_link((*itEdge).first);
							//----------------for test------------//
							Vertex_triple vtriT1=make_vertex_triple(tmpBoundFacet1);
						}
						if(itEdge!=BoundEdges.end()&&tmpBoundFacet1==nei)
						{
							numNei.push_back((*itEdge).second);
							BoundEdges.erase(itEdge++);
						}
					}
					
				}
				if(numNei.size()==1)
				{
					int i0=numNei[0];
					auto leftRight=make_edge_index(i0);
					EdgeInFacet oppEdge0=mirror_edge(EdgeInFacet(nei,leftRight.second));
					EdgeInFacet oppEdge1=mirror_edge(EdgeInFacet(nei,leftRight.first));
					//--------------for test-----------//
					Vertex_triple vtriT3=make_vertex_triple(oppEdge0.first);
					Vertex_triple vtriT4=make_vertex_triple(oppEdge1.first);
					//==============test end==========//
					BoundEdges.insert(itEdge,turn_edgeinfacet_to_edge(oppEdge0));
					BoundEdges.insert(itEdge,turn_edgeinfacet_to_edge(oppEdge1));
				}
				else if (numNei.size()==2)
				{
					for(int i2=0;i2<3;i2++)
					{
						if(i2!=numNei[0]&&i2!=numNei[1])
						{
							EdgeInFacet oppEdge=mirror_edge(EdgeInFacet(nei,i2));
							//--------------for test-----------//
							Vertex_triple vtriT2=make_vertex_triple(oppEdge.first);
							//==============test end==========//
							BoundEdges.insert(itEdge,turn_edgeinfacet_to_edge(oppEdge));
							break;
						}
					}
				}


			}
			else if(visited_for_facet_extractor(nei)==0)
			{
				Vertex_Idtype vOpp=vertex_facet(tmp,i);
				
				if(PInside)
				{
					Indextype ii=vertex_index(tmp.first,vOpp);
					//create_surface_facet(InitFacet.first,ii);
					Facet newFacet=mirror_facet(Facet(tmp.first,ii));
					NewCreateSurfacets.push_back(newFacet);
				}
				else
				{
					Cell_Idtype cOpp=neighbor(tmp.first,li);
					//Vertex_Idtype vOppEdge=vertex(tmp.first,ii);
					Indextype iiNewSur=vertex_index(cOpp,vOpp);
					//create_surface_facet(cOpp,iiNewSur);
					NewCreateSurfacets.push_back(Facet(cOpp,iiNewSur));
				}
				Vertex_pair vEdge=make_vertex_pair(EdgeInFacet(tmp,i));
				VertexOnBoundary.insert(vEdge.first);
				VertexOnBoundary.insert(vEdge.second);
			}
		}
	}
	//-------------------for test(��ӡ BoundEdges)------------------//
	//EdgeInFacet��(Facet,0/1/2)��ʾ,����Facet��(cell,0/1/2/3)��ʾ
	//BoundEdges���滻��������ı߽��߼��ϣ���Edge��ʽ(��id,0/1/2)��ʾ,���Ҹ��治�滻
	//BoundEdgeҲ�Ǳ߽��߼��ϣ��Զ˵�(vertex_id,vertex_id)��ʽ��ʾ
	//cout<<"conflict boundary edge:"<<endl;
	//for(auto itBE=BoundEdges.begin();itBE!=BoundEdges.end();itBE++)
	//{
	//	Edge tmp=*itBE;
	//	Facet tmpF=get_facet_cell_link(tmp.first);
	//	//--------------for test----------//
	//	Cell_Idtype tmpc=tmpF.first;
	//	Vertex_triple vtrit = make_vertex_triple(tmpF);
	//	//==============test end=========//
	//	Vertex_pair vp=make_vertex_pair(EdgeInFacet(tmpF,tmp.second));
	//	cout<<"("<<vp.first<<","<<vp.second<<")"
	//		<<"/Edge("<<tmp.first<<","<<tmp.second<<") "<<tmp.first<<"["<<vtrit.first<<","<<vtrit.second<<","<<vtrit.third
	//		<<"]/EdgeInFacet(("<<tmpF.first<<","<<tmpF.second<<"),"<<tmp.second<<") "<<tmpF.first<<"["
	//		<<vertex(tmpc, 0)<<","<<vertex(tmpc, 1)<<","<<vertex(tmpc, 2)<<","<<vertex(tmpc, 3)<<"]"<<endl;
	//}
	//===================test end==================//
	
	quit=false;
	//�߽��߶���ֻ�ܹ��������߽��,�����滻�治������ĵ���2-D����
	for(auto itv=VertexOnBoundary.begin();itv!=VertexOnBoundary.end();itv++)
	{
		int singular_point=0;
		for (auto itBE=BoundEdges.begin();itBE!=BoundEdges.end();itBE++)
		{
			Edge tmp=*itBE;
			Facet tmpF=get_facet_cell_link(tmp.first);
			Vertex_pair vp=make_vertex_pair(EdgeInFacet(tmpF,tmp.second));
			if (*itv==vp.first||*itv==vp.second)
			{
				singular_point++;
				if (singular_point>2)
				{
					quit=true;
					break;
				}
			}

		}
		if (quit)
		{
			break;
		}
	}

	for(auto itt=tmpSurfacets.begin();itt!=tmpSurfacets.end();itt++)
	{
		clear_facet_visited(*itt);
	}

}

//�󽻽��棬ʹ��Gabriel�Ĺ�������滻�����չ����Ҫ��Է���pseudo-concave��surface����͹��չ
//������������ƬΪ��ʼ����BFS���������ڿ��ܱ��滻�ı���������Ƭ
//p����㣻InitFacetΪ����������Ƭ������������,���ڵ�cell��Ӱ������,����������,��mirror facet����cell������Ӱ���򣩣�
//BoundEdgesΪ����ֵ�����滻��������ı߽��߼��ϣ���Edge��ʽ(��id,0/1/2)��ʾ,���Ҹ��治�滻��BoundEdgeҲ�Ǳ߽��߼��ϣ��Զ˵�(vertex_id,vertex_id)��ʽ��ʾ
//Begin����cell����Ӱ����,mirror������cell������Ӱ����
//VertexOnBoundaryΪ�߽����ϵĶ��㣬���ڹ�����ļ�飻
//VertexOnConflictFacetΪ(���滻���ϵĶ���id,��֮�����ı���������Ƭ��ɾ���ĸ���)�����ڹ�����ļ��
//PInsideΪp���ڱ����λ��
//SurfacetsInConflictΪӰ�����ڵı���������Ƭ(���漰��mirror facet���ڵ�cell����conflict)
template<typename T, typename T_INFO>
void DataStructure<T, T_INFO>::
find_conflict_surfacet(const T* P,Facet InitFacet,list<Edge>& BoundEdges,set_pair_int& BoundEdge,Facet& Begin,std::set<Vertex_Idtype>& VertexOnBoundary,
															map<Vertex_Idtype,size_t>& VertexOnConflictFacet,bool PInside,
															vector<Facet_Idtype>& SurfacetsInConflict,bool& quit)
{
	list<Facet> illSurfacets;
	vector<Facet> tmpSurfacets;
	list<Facet> neighborSurfacets;
	neighborSurfacets.push_back(InitFacet);
	tmpSurfacets.push_back(InitFacet);
	mark_facet_visited(InitFacet);
	mark_facet_visited(InitFacet);

	vector<Facet_Idtype> idtmpSurfacets;
	Facet_Idtype fid=CellSurface.get_tuple_element(InitFacet.first,InitFacet.second);
	idtmpSurfacets.push_back(fid);
	//-----------------��InitFacet����ı���������Ƭ������InitFacet�й�����ģ�------------------//
	while(!neighborSurfacets.empty())
	{
		Facet tmpSurfacet=*neighborSurfacets.begin();
	
		//--------------for test------------//
		Vertex_triple vtriT0=make_vertex_triple(tmpSurfacet);
		//===============test end==========//
		neighborSurfacets.pop_front();
		Cell_Idtype c = tmpSurfacet.first;//HHHHHHHHHHHHHHHHHHHHHHHHHH-------c�϶���labelΪinside
		int li = tmpSurfacet.second;
		Facet nei;
		for(int i=0;i<3;i++)
		{
			nei=neighbor_surfacet(tmpSurfacet,i);
	
			//--------------for test------------//
			Vertex_triple vtriT1=make_vertex_triple(nei);
			//===============test end==========//
			if(visited_for_facet_extractor(nei)==0)
			{
				//�����broken surface facet
				if(is_facet_in_conflict(nei))//is_facet_in_conflict(nei)��������������cell����conflict
				{
					//��������Ǳ����򣬱��Ϊ2��������Ϊ1
					mark_facet_visited(nei);
					mark_facet_visited(nei);
					neighborSurfacets.push_back(nei);
					tmpSurfacets.push_back(nei);

					Facet_Idtype fid=CellSurface.get_tuple_element(nei.first,nei.second);
					idtmpSurfacets.push_back(fid);
				}
				//�ж������boundary surface facet(�����Ӧ��һ��cell��Ӱ������һ����boundary)
				else if((is_on_boundary(nei.first)&&is_in_conflict(neighbor(nei.first,nei.second)))||
					(is_on_boundary(neighbor(nei.first,nei.second))&&is_in_conflict(nei.first)))
				{
					T cosDihedral[3];//cosDihedral[0]����ŵ���������нǵ�����
					//if(PInside)
					cos_dihedral_point_to_facet(P,nei,cosDihedral);
					/*else
						cos_dihedral_point_to_facet(P,mirror_facet(nei),cosDihedral);*/
					//------------------for test----------------//
					/*T cosT[3];
					cos_dihedral_point_to_facet(P,nei,cosT);*/
					//==================test end================//

					
					
					//���뷽���ж�
					Vertex_triple vf=make_vertex_triple(nei);
					Orientation o = GeometricTraits<T>::orientation(point(vf.first), point(vf.second), point(vf.third), P);
					if ((o!=NEGATIVE&&PInside)||(o!=POSITIVE&&!PInside))
					{
						//�жϼ���nei�Ƿ�Ϊ͹��չ
						bool isConvexExtension=true;
						for(int j=0;j<3;j++)
						{
							Facet neiEdge=neighbor_surfacet(nei,j);
							//--------------------------for test-------------------------//
							Vertex_triple vtriT3=make_vertex_triple(neiEdge);
							//==========================test end=========================//
							//if(cosDihedral[i]<0&&neighbor_surfacet())
							if(cosDihedral[j]<0&&!is_in_conflict(neiEdge.first)&&!is_in_conflict(neighbor(neiEdge.first,neiEdge.second)))
							{
								isConvexExtension=false;
								break;
							}
						}
						//�ı�ѡ���滻��Ĺ�����pInsideΪ���������Ƿ����Gabriel����Ϊ׼��
						if (PInside)
						{
							bool toBeSurface=false;

							for (int j = 0; j < 3; j++)
							{
								Facet NNei=neighbor_surfacet(nei,j);
								if (visited_for_facet_extractor(NNei)==2)
								{
									Vertex_Idtype vertexOpp=vertex_facet(nei,j);
									Vertex_pair vertexOppEdge=make_vertex_pair(EdgeInFacet(nei,j));
									bool isInShpere=GeometricTraits<T>::side_of_bounded_sphereC3(point(vertexOppEdge.first),P,
										point(vertexOppEdge.second),point(vertexOpp))==ON_BOUNDED_SIDE;
									if (isInShpere)
									{
										toBeSurface=true;
									}
								}
							}
							if (!toBeSurface)
							{
								Vertex_triple ftrip=make_vertex_triple(nei);
								toBeSurface=GeometricTraits<T>::side_of_bounded_sphereC3(point(ftrip.first),
									point(ftrip.second),point(ftrip.third),P)==ON_BOUNDED_SIDE;
							}
							if(toBeSurface)//&&isConvexExtension)
							{
								mark_facet_visited(nei);
								mark_facet_visited(nei);
								neighborSurfacets.push_back(nei);
								tmpSurfacets.push_back(nei);

								Facet_Idtype fid=CellSurface.get_tuple_element(nei.first,nei.second);
								idtmpSurfacets.push_back(fid);
							}
							else
							{
								mark_facet_visited(nei);
								illSurfacets.push_back(nei);
							}
						}
						else
						{
							bool toBeSurface=true;
							for (int j = 0; j < 3; j++)
							{
								Facet NNei=neighbor_surfacet(nei,j);
								if (visited_for_facet_extractor(NNei)!=2)
								{
									Vertex_Idtype vertexOpp=vertex_facet(nei,j);
									Vertex_pair vertexOppEdge=make_vertex_pair(EdgeInFacet(nei,j));
									bool isInShpere=GeometricTraits<T>::side_of_bounded_sphereC3(point(vertexOppEdge.first),
										point(vertexOppEdge.second),P,point(vertexOpp))==ON_BOUNDED_SIDE;
									if (isInShpere)
									{
										toBeSurface=false;
									}
								}
							}
							if(toBeSurface)//&&isConvexExtension)
							{
								mark_facet_visited(nei);
								mark_facet_visited(nei);
								neighborSurfacets.push_back(nei);
								tmpSurfacets.push_back(nei);

								Facet_Idtype fid=CellSurface.get_tuple_element(nei.first,nei.second);
								idtmpSurfacets.push_back(fid);
							}
							else
							{
								mark_facet_visited(nei);
								illSurfacets.push_back(nei);
							}
						}

					
					}//end of if ((o!=NEGATIVE&&PInside)||(o!=POSITIVE&&!PInside))
				}// end of else if(...)
			
			}//end of if(visited_for_facet_extractor(nei)==0)
		}//end of for
	}//end of while

	//����һ������õĽ�����ɾ�����еĿ�������۵���surface facet
	while(!illSurfacets.empty())
	{
		Facet tmpIllSurfacet=*(illSurfacets.begin());
		clear_facet_visited(tmpIllSurfacet);
		illSurfacets.pop_front();
		for(int i=0;i<3;i++)
		{
			Facet nei=neighbor_surfacet(tmpIllSurfacet,i);
			//---------------for test------------//
			Vertex_triple vtriNei=make_vertex_triple(nei);
			//==============test end============//
			if(visited_for_facet_extractor(nei)==2&&!is_facet_in_conflict(nei)&&nei!=InitFacet)
			{
				Indextype iNei=neighbor_index_facet(nei,tmpIllSurfacet);
				if(cos_dihedral_point_to_Edge(P,nei,iNei)<0)
				{
					illSurfacets.push_back(nei);
					clear_facet_visited(nei);

					Facet_Idtype fid=CellSurface.get_tuple_element(nei.first,nei.second);
					idtmpSurfacets.erase(find(idtmpSurfacets.begin(),idtmpSurfacets.end(),fid));
				}
			}
		}
	}



	quit=false;
	if (SurfacetsInConflict.size()>idtmpSurfacets.size())
	{

		quit=true;
	}

	for (auto its=SurfacetsInConflict.begin();its!=SurfacetsInConflict.end();its++)
	{
		if (find(idtmpSurfacets.begin(),idtmpSurfacets.end(),*its)==idtmpSurfacets.end())
		{
			quit=true;
			break;
		}
	}

	//if (idtmpSurfacets.size()>4)
	//{
	//	quit=true;
	//}

	if (quit)
	{
		for(auto itt=tmpSurfacets.begin();itt!=tmpSurfacets.end();itt++)
		{
			clear_facet_visited(*itt);
		}
		return;
	}



	//����������������ƬΪ�����������������Ľ�����������Ƭ����߽��
	//EdgeInFacet��(Facet,0/1/2)��ʾ,����Facet��(cell,0/1/2/3)��ʾ
	//BoundEdges���滻��������ı߽��߼��ϣ���Edge��ʽ(��id,0/1/2)��ʾ,���Ҹ��治�滻
	//BoundEdgeҲ�Ǳ߽��߼��ϣ��Զ˵�(vertex_id,vertex_id)��ʽ��ʾ
	//�߽������ǰ���˳ʱ��˳�򣬳�ʼ��Ҳ����˳ʱ��(�ӱ����ڿ�,�����⿴����ʱ��),���԰�����Զ���0->2->1�����ʼ���߽��
	mark_facet_visited(InitFacet);
	//---------------for test------------//
	Vertex_triple vtri_tmp = make_vertex_triple(InitFacet);
	//===============test end============//

	EdgeInFacet tmpBoundEdge=mirror_edge(EdgeInFacet(InitFacet,0));
	//turn_edgeinfacet_to_edge(tmpBoundEdge)��tmpBoundEdge�ӣ���cell_id,0/1/2/3��,0/1/2���ĳ�(��id,0/1/2)
	Edge bound0= turn_edgeinfacet_to_edge(tmpBoundEdge);
	BoundEdges.push_back(bound0);
	//---------------for test------------//
	Vertex_triple vtri_tmp1 = make_vertex_triple(tmpBoundEdge.first);
	//===============test end============//

	tmpBoundEdge=mirror_edge(EdgeInFacet(InitFacet,2));
	//turn_edgeinfacet_to_edge(tmpBoundEdge)��tmpBoundEdge�ӣ���cell_id,0/1/2/3��,0/1/2���ĳ�(��id,0/1/2)
	Edge bound2= turn_edgeinfacet_to_edge(tmpBoundEdge);
	BoundEdges.push_back(bound2);
	//---------------for test------------//
	Vertex_triple vtri_tmp2 = make_vertex_triple(tmpBoundEdge.first);
	//===============test end============//

	tmpBoundEdge=mirror_edge(EdgeInFacet(InitFacet,1));
	//turn_edgeinfacet_to_edge(tmpBoundEdge)��tmpBoundEdge�ӣ���cell_id,0/1/2/3��,0/1/2���ĳ�(��id,0/1/2)
	Edge bound1= turn_edgeinfacet_to_edge(tmpBoundEdge);
	BoundEdges.push_back(bound1);
	//---------------for test------------//
	Vertex_triple vtri_tmp3 = make_vertex_triple(tmpBoundEdge.first);
	//===============test end============//

	list<Facet> tmpConflitFacets;
	tmpConflitFacets.push_back(InitFacet);
	
	//-----------------for test-------------//
	/*cout<<"���滻�棺     "<<endl;*/
	//=================test end==============//

	while(!tmpConflitFacets.empty())
	{
		Facet tmp=*(tmpConflitFacets.begin());
		tmpConflitFacets.pop_front();
		Vertex_triple vtrif=make_vertex_triple(tmp);
		/*VertexOnConflictFacet.insert(vtrif.first);
		VertexOnConflictFacet.insert(vtrif.second);
		VertexOnConflictFacet.insert(vtrif.third);*/
		VertexOnConflictFacet[vtrif.first]++;
		VertexOnConflictFacet[vtrif.second]++;
		VertexOnConflictFacet[vtrif.third]++;

		//------------------for test--------------//
		Vertex_triple vtriTC0=make_vertex_triple(tmp);
		Cell_Idtype tmpc=tmp.first;
		Facet_Idtype fid=CellSurface.get_tuple_element(tmpc,tmp.second);
		//cout<<fid<<"("<<vtriTC0.first<<","<<vtriTC0.second<<","<<vtriTC0.third<<")"
		//	<<"/("<<tmp.first<<","<<tmp.second<<")["<<tmpc<<"("
		//	<<vertex(tmpc, 0)<<","<<vertex(tmpc, 1)<<","<<vertex(tmpc, 2)<<","<<vertex(tmpc, 3)<<")]"<<endl;
		//==================test end==============//
		
		for (int i = 0; i<3; i++)
		{
			Facet nei = neighbor_surfacet(tmp, i);
			//---------------for test------------//
			Vertex_triple vtriT = make_vertex_triple(nei);
			//===============test end============//
			//��tmp��nei�ĵڼ������ڵ�surface facet
			Indextype iNei = neighbor_index_facet(nei, tmp);
			if (visited_for_facet_extractor(nei) == 2)
			{
				
				mark_facet_visited(nei);
				tmpConflitFacets.push_back(nei);

				//���½�����ı߽�߶���
				vector<int> numNei;
				int numOfList = 0;

				//--------------�޸�------------//
				auto itEdge = BoundEdges.begin();
				for (; itEdge != BoundEdges.end(); itEdge++)
				{
					numOfList++;
					Facet tmpBoundFacet0 = get_facet_cell_link((*itEdge).first);
					//--------------for test----------//
					Vertex_triple vtriT0 = make_vertex_triple(tmpBoundFacet0);
					//==============test end==========//
					if (tmpBoundFacet0 == nei)
					{
						numNei.push_back((*itEdge).second);
						BoundEdges.erase(itEdge++);
						break;
					}
				}
				//��list���ĵ�һ����ɾ��ʱ��Ӧ�ü�����һ���Ƿ�ҲӦ�ñ�ɾ��
				if (numOfList == 1)
				{
					auto endEdge = BoundEdges.back();
					Facet tmpBoundFacet00 = get_facet_cell_link(endEdge.first);
					//--------------for test----------//
					Vertex_triple vtriT00 = make_vertex_triple(tmpBoundFacet00);
					//==============test end=========//
					if (tmpBoundFacet00 == nei)
					{

						numNei.push_back(endEdge.second);
						BoundEdges.pop_back();
						endEdge = BoundEdges.back();
						Facet tmpBoundFacet01 = get_facet_cell_link(endEdge.first);
						//--------------for test----------//
						Vertex_triple vtriT01 = make_vertex_triple(tmpBoundFacet00);
						//==============test end=========//
						if (tmpBoundFacet01 == nei)
						{
							numNei.push_back(endEdge.second);
							BoundEdges.pop_back();
						}
					}

				}
				if (numNei.size() != 3)
				{
					//list �м�Ĳ�����ɾ��Ҫ�����
					Facet tmpBoundFacet1 = make_pair(-1, -1);
					if (itEdge != BoundEdges.end())
					{
						tmpBoundFacet1 = get_facet_cell_link((*itEdge).first);
						//----------------for test------------//
						Vertex_triple vtriT1 = make_vertex_triple(tmpBoundFacet1);
						int ttttt=0;
						//================test end===========//
					}				
					if (itEdge != BoundEdges.end() && tmpBoundFacet1 == nei)
					{
						numNei.push_back((*itEdge).second);
						BoundEdges.erase(itEdge++);
						Facet tmpBoundFacet1 = make_pair(-1, -1);
						if (itEdge != BoundEdges.end())
						{
							tmpBoundFacet1 = get_facet_cell_link((*itEdge).first);
							//----------------for test------------//
							Vertex_triple vtriT1 = make_vertex_triple(tmpBoundFacet1);
							//int ttttt=0;
							//================test end===========//
						}
						if (itEdge != BoundEdges.end() && tmpBoundFacet1 == nei)
						{
							numNei.push_back((*itEdge).second);
							BoundEdges.erase(itEdge++);
						}
					}

				}
				if (numNei.size() == 1)  //�¼������߽��
				{
					int i0 = numNei[0];
					auto leftRight = make_edge_index(i0);
					EdgeInFacet oppEdge0 = mirror_edge(EdgeInFacet(nei, leftRight.second));
					EdgeInFacet oppEdge1 = mirror_edge(EdgeInFacet(nei, leftRight.first));
					//--------------for test-----------//
					Vertex_triple vtriT3 = make_vertex_triple(oppEdge0.first);
					Vertex_triple vtriT4 = make_vertex_triple(oppEdge1.first);
					//==============test end==========//
					BoundEdges.insert(itEdge, turn_edgeinfacet_to_edge(oppEdge0));
					BoundEdges.insert(itEdge, turn_edgeinfacet_to_edge(oppEdge1));
				}
				else if (numNei.size() == 2)  //�¼�һ���߽��
				{
					for (int i2 = 0; i2<3; i2++)
					{
						if (i2 != numNei[0] && i2 != numNei[1])
						{
							EdgeInFacet oppEdge = mirror_edge(EdgeInFacet(nei, i2));
							//--------------for test-----------//
							Vertex_triple vtriT2 = make_vertex_triple(oppEdge.first);
							//==============test end==========//
							BoundEdges.insert(itEdge, turn_edgeinfacet_to_edge(oppEdge));
							break;
						}
					}
				}



			}
			else if (visited_for_facet_extractor(nei) == 0)
			{

				Vertex_pair vEdge = make_vertex_pair(EdgeInFacet(tmp, i));
				BoundEdge.insert(make_pair(vEdge.first, vEdge.second));
				BoundEdge.insert(make_pair(vEdge.second, vEdge.first));
				VertexOnBoundary.insert(vEdge.first);
				VertexOnBoundary.insert(vEdge.second);
			}
		}
	}


	//-------------------for test(��ӡ BoundEdges)------------------//
	//EdgeInFacet��(Facet,0/1/2)��ʾ,����Facet��(cell,0/1/2/3)��ʾ
	//BoundEdges���滻��������ı߽��߼��ϣ���Edge��ʽ(��id,0/1/2)��ʾ,���Ҹ��治�滻
	//BoundEdgeҲ�Ǳ߽��߼��ϣ��Զ˵�(vertex_id,vertex_id)��ʽ��ʾ
	//cout<<"conflict boundary edge:"<<endl;
	//for(auto itBE=BoundEdges.begin();itBE!=BoundEdges.end();itBE++)
	//{
	//	Edge tmp=*itBE;
	//	Facet tmpF=get_facet_cell_link(tmp.first);
	//	//--------------for test----------//
	//	Cell_Idtype tmpc=tmpF.first;
	//	Vertex_triple vtrit = make_vertex_triple(tmpF);
	//	//==============test end=========//
	//	Vertex_pair vp=make_vertex_pair(EdgeInFacet(tmpF,tmp.second));
	//	cout<<"("<<vp.first<<","<<vp.second<<")"
	//		<<"/Edge("<<tmp.first<<","<<tmp.second<<") "<<tmp.first<<"["<<vtrit.first<<","<<vtrit.second<<","<<vtrit.third
	//		<<"]/EdgeInFacet(("<<tmpF.first<<","<<tmpF.second<<"),"<<tmp.second<<") "<<tmpF.first<<"["
	//		<<vertex(tmpc, 0)<<","<<vertex(tmpc, 1)<<","<<vertex(tmpc, 2)<<","<<vertex(tmpc, 3)<<"]"<<endl;
	//}
	//===================test end==================//


	quit=false;
	//EdgeInFacet��(Facet,0/1/2)��ʾ,����Facet��(cell,0/1/2/3)��ʾ
	//BoundEdges���滻��������ı߽��߼��ϣ���Edge��ʽ(��id,0/1/2)��ʾ,���Ҹ��治�滻
	//BoundEdgeҲ�Ǳ߽��߼��ϣ��Զ˵�(vertex_id,vertex_id)��ʽ��ʾ
	//�߽��߶���ֻ�ܹ��������߽��,�����滻�治������ĵ���2-D����
	for(auto itv=VertexOnBoundary.begin();itv!=VertexOnBoundary.end();itv++)
	{
		int singular_point=0;
		for (auto itBE=BoundEdges.begin();itBE!=BoundEdges.end();itBE++)
		{
			Edge tmp=*itBE;
			Facet tmpF=get_facet_cell_link(tmp.first);
			Vertex_pair vp=make_vertex_pair(EdgeInFacet(tmpF,tmp.second));
			if (*itv==vp.first||*itv==vp.second)
			{
				singular_point++;
				if (singular_point>2)
				{
					quit=true;
					break;
				}
			}
		}
		if (quit)
		{
			break;
		}
	}

	if (BoundEdges.empty())
	{
		quit=true;
	}

	if (quit)
	{
		for(auto itt=tmpSurfacets.begin();itt!=tmpSurfacets.end();itt++)
		{
			clear_facet_visited(*itt);
		}
		return;
	}


	//��Begin
	auto edgeBound=BoundEdges.begin();
	Facet facetEdge=get_facet_cell_link((*edgeBound).first);
	Facet conflictBoundary=neighbor_surfacet(facetEdge,(*edgeBound).second);
	//---------------------------for test-----------------------//
	Vertex_triple vtriT=make_vertex_triple(conflictBoundary);
	//===========================test end=======================//
	if(!PInside&&is_on_boundary(conflictBoundary.first))//neighbor(conflictBoundary.first,conflictBoundary.second)))
	{
		Begin=mirror_facet(conflictBoundary);
	}
	else
	{
		Cell_Idtype c=conflictBoundary.first;
		Indextype li=conflictBoundary.second;
		Indextype iif=neighbor_index_facet(conflictBoundary,facetEdge);
		Vertex_Idtype vii=vertex_facet(conflictBoundary,iif);
		Indextype ii=vertex_index(c,vii);

		Vertex_Idtype vj1 = vertex(c, Triangulation_utils_3::next_around_edge(ii, li));
		Vertex_Idtype vj2 = vertex(c, Triangulation_utils_3::next_around_edge(li, ii));
		Cell_Idtype cur1 = c;
		int zz1 = ii;
		Cell_Idtype n1 = neighbor(cur1, zz1);
		// turn around the oriented edge vj1 vj2
		//���������vj1->vj2����������vj1->vj2������������,����������vj1->vj2��������Ĵָ���򣩷���������������
		//vj1,vj2Ϊ���滻��߽��߶���
		while (is_in_conflict(n1)) {

			cur1 = n1;
			zz1 = Triangulation_utils_3::next_around_edge(vertex_index(n1, vj1), vertex_index(n1, vj2));
			n1 = neighbor(cur1, zz1);
		}
		// Now n1 is outside region, cur1 is inside.
		//��ʱ��Begin���߽���,����cur1����Ӱ����,��neighbor(cur1,zz1)������Ӱ����
		Begin = Facet(cur1,zz1);
	}
	for(auto itt=tmpSurfacets.begin();itt!=tmpSurfacets.end();itt++)
	{
		clear_facet_visited(*itt);
	}

}



template<typename T, typename T_INFO>
bool DataStructure<T, T_INFO>::surface_facet_nearest_to_point(const T* p, Facet& NearestFacet,
	std::vector<Facet>& FacetToDelete, std::vector<Facet>& FacetUndelete)
{
	Vertex_Idtype nearestVertex = PointDataSet.nearest_inexact(p);
	
	incident_surface_facet(nearestVertex, std::back_inserter(FacetToDelete),std::back_inserter(FacetUndelete));
	if (FacetToDelete.empty())
	{
		NearestFacet = incident_facet_with_min_longest_edge(p,FacetUndelete.begin(),FacetUndelete.end(),nearestVertex);
		return false;
	}
	else
	{
		NearestFacet = incident_facet_with_min_longest_edge(p,FacetToDelete.begin(),FacetToDelete.end(),nearestVertex);
		return true;
	}
}

template<typename T, typename T_INFO>
template<class IteratorIncidentFacet>
Facet DataStructure<T, T_INFO>::
incident_facet_with_min_longest_edge(const T* p, IteratorIncidentFacet FacetBegin, 
	IteratorIncidentFacet FacetEnd, Vertex_Idtype VertexIncident)
{
	double minLongestLength=COMPUTATION_DOUBLE_MAX;
	Facet nearestFacet;
	IteratorIncidentFacet itFacet;
	for (itFacet = FacetBegin; itFacet != FacetEnd; itFacet++)
	{
		//-----------------itFacet����Ϊboundary facet��boundary facet�������conflict facet----------------//

		if (!is_in_conflict((*itFacet).first))
		{
			if (!is_in_conflict(neighbor((*itFacet).first, (*itFacet).second)))
				continue;
		}
		double longestLength = 0;
		Vertex_triple vertexFacet = make_vertex_triple(*itFacet);
		Vertex_Idtype vertexTemp = -1;
		for (int i = 0; i < 3; i++)
		{
			//vertexTemp = vertexFacet.get<i>();
			if (i == 0)
				vertexTemp = vertexFacet.first;
			else if (i == 1)
				vertexTemp = vertexFacet.second;
			else
				vertexTemp = vertexFacet.third;

			if (vertexTemp != VertexIncident)
			{
				double distantTemp = distant_to_vertex(p,vertexTemp);
				if (distantTemp > longestLength)
					longestLength = distantTemp;
			}
		}
		if (longestLength < minLongestLength)
		{
			minLongestLength = longestLength;
			nearestFacet = *itFacet;
		}
	}
	return nearestFacet;
}

template<typename T, typename T_INFO>
double DataStructure<T, T_INFO>::distant_to_vertex(const T* p, Vertex_Idtype IdV)
{
	const T* ptr = point(IdV);
	return NumericComputation<T>::SquareDistance(p,ptr);
}


template<typename T, typename T_INFO>
template<class OutputIteratorFacet>
void DataStructure<T, T_INFO>::
incident_surface_facet(Vertex_Idtype IdV, OutputIteratorFacet FacetToDelete, OutputIteratorFacet FacetUndelete)
{
	if (dimension() < 2)
		return;

	std::vector<Cell_Idtype> tmp_cells;
	tmp_cells.reserve(64);
	std::vector<Facet> tmp_facets;
	tmp_facets.reserve(64);
	std::vector<Vertex_Idtype> temp_vertices;
	if (dimension() == 3)
		incident_cells_3_withoutmark(IdV, cell(IdV), std::back_inserter(tmp_cells), FacetToDelete, FacetUndelete);


	typename std::vector<Cell_Idtype>::iterator cit;
	for (cit = tmp_cells.begin();
		cit != tmp_cells.end();
		++cit)
	{
		clear_cell_visited(*cit);
	}
}


template<typename T, typename T_INFO>
template<typename IncidentCellIterator, typename OutputIteratorFacet>
void DataStructure<T, T_INFO>::
incident_cells_3_withoutmark(Vertex_Idtype IdV, Cell_Idtype d, IncidentCellIterator IncidentCell,
	OutputIteratorFacet FacetToDelete, OutputIteratorFacet FacetUndelete)
{

	std::stack<Cell_Idtype> cell_stack;
	cell_stack.push(d);
	mark_cell_visited(d);
	*IncidentCell++ = d;

	do {
		Cell_Idtype c = cell_stack.top();
		cell_stack.pop();

		for (int i = 0; i < 4; ++i) {
			if (vertex(c, i) == IdV)
				continue;
			Cell_Idtype next = neighbor(c, i);

			if (is_surface(Facet(c, i)))
			{
				if (is_facet_in_conflict(Facet(c, i)))
				{
					*FacetToDelete++ = Facet(c, i);
				}
				else
				{
					*FacetUndelete++ = Facet(c, i);
				}
			}


			if (visited_for_cell_extractor(next))
				continue;
			cell_stack.push(next);
			mark_cell_visited(next);
			*IncidentCell++ = next;
		}
	} while (!cell_stack.empty());

}


//����cell��conflict mark��ǰ������incident_cell 3d, ������Ƿ�ɾ��
//�ҳ�����IdV������������ͱ�����Ƭ,����d��IdV�Ĺ���������,
//IncidentCells�ǹ��������弯�ϵĵ�������back_inserter����Surfacets�ǹ����ı��漯�ϵĵ�������back_inserter��
template<typename T, typename T_INFO>
template<typename IteratorCell, class IteratorFacet>
void DataStructure<T, T_INFO>::
incident_cells_surfacet_3_withoutmark(Vertex_Idtype IdV, Cell_Idtype d,IteratorCell IncidentCells,
		IteratorFacet Surfacets)
{
	std::stack<Cell_Idtype> cell_stack;
	cell_stack.push(d);
	mark_cell_visited(d);
	*IncidentCells++ = d;

	do {
		Cell_Idtype c = cell_stack.top();
		cell_stack.pop();

		for (int i = 0; i<4; ++i) {
			if (vertex(c, i) == IdV)
				continue;
			Cell_Idtype next = neighbor(c, i);

		    if (is_surface(Facet(c,i)))
			{
			   *Surfacets++=Facet(c,i);
			}
				


			if (visited_for_cell_extractor(next))
				continue;
			cell_stack.push(next);
			mark_cell_visited(next);
			*IncidentCells++ = next;
		}
	} while (!cell_stack.empty());
}

//�ж�V�Ƿ�Ϊ������;SurfaceFacet��V����������,����V�����,ʹ��V���������������������С����
//��Ϊ������,InsideΪ�ù������ڱ����ڻ��Ǳ�����
template<typename T, typename T_INFO>
bool DataStructure<T, T_INFO>::
is_vertex_isolate(Vertex_Idtype V,Facet& SurfaceFacet,bool& Inside)
{
	std::stack<Cell_Idtype> cell_stack;
	std::vector<Cell_Idtype> cellsIncident;
	Cell_Idtype d=cell(V);
	cell_stack.push(d);
	mark_cell_visited(d);
	cellsIncident.push_back(d);
	Inside=is_label_inside(d);
	bool isIso=true;
	double dis=COMPUTATION_DOUBLE_MAX;
	//-----------------for test----------------//
	//cout<<"to decide if vertex"<<V<<"is isolate"<<endl;
	//=================test end================//
	do {
		Cell_Idtype c = cell_stack.top();
		cell_stack.pop();

		for (int i = 0; i<4; ++i) {
			if (vertex(c, i) == V)
				continue;
			Cell_Idtype next = neighbor(c, i);

			if(Inside!=is_label_inside(next))
			{
				isIso=false;
				break;
			}

			if (visited_for_cell_extractor(next))
				continue;
			cell_stack.push(next);
			mark_cell_visited(next);
			cellsIncident.push_back(next);
		}
		if(isIso==false)
		{
			break;
		}

	} while (!cell_stack.empty());

	for(auto ic=cellsIncident.begin();ic!=cellsIncident.end();ic++)
	{
		clear_cell_visited(*ic);
	}

	return isIso;

}

//���������V
template<typename T, typename T_INFO>
bool DataStructure<T, T_INFO>::
insert_vertex_into_surface(Vertex_Idtype V)
{
	//Ѱ������ڵ�
	vector<Vertex_Idtype> vs;
	vector<Facet> incidentSurfacets;
	list<Vertex_Idtype> nearest(5,-1); //nearest��ʼ��Ϊ���-1
	bool pInside=is_label_inside(cell(V)); //pInsideΪ�������ڱ����ڻ��Ǳ�����
	//---------------for test------------//
	//cout<<"��������㣺"<<V<<endl;
	//cout<<"�������ڱ�����: "<<pInside<<endl;
	//===============test end============//

	
	const T* p=point(V);
	vs.reserve(32);
	vector<Facet> initFacet;
	incidentSurfacets.reserve(32);
	//�������V��������,vs��V��������ļ���,incidentSurfacets�ǹ����ı���(umbrella)�ļ���
	incident_vertex_and_surface_facet(V,std::back_inserter(vs),std::back_inserter(incidentSurfacets));
	//���¹�����p������ڵ�
	for(auto vsit=vs.begin();vsit!=vs.end();++vsit)
	{
		refresh_K_nearest(p,*vsit,nearest);			
	}
	//----------------------for test(�������ڵ�)------------------//
	//cout<<"����ڵ�:  ";
	//for(auto iNV=nearest.begin();iNV!=nearest.end();iNV++)
	//{
	//	cout<<*iNV<<" ";
	//}
	//cout<<endl;
	//==========================test end============================//

	vector<Facet> incidentSurfacets_KNN;
	vector<Facet> incidentSurfacets_Sparse;
	vector<Facet> surfacetInterConflict;
	vector<Facet_Idtype> surfacetsInConflict;
	std::map<Vertex_Idtype,size_t> vertexWithDeletedSurfacet;
	Facet nearestFacet(-1,-1);
	Facet seedFacet(-1,-1);
	auto fit=nearest.begin();

	if (!incidentSurfacets.empty())
	{
		return true;
	}

	//�ҳ�����*fit������������ͱ�����Ƭ,incidentSurfacets�ǹ����ı��漯��
	incident_surface_facet(*fit,std::back_inserter(incidentSurfacets));
	
	//pInside�ǹ�������ڱ����λ��,trueΪ�����ڲ�,falseΪ�����ⲿ;VΪ�����㶥����;
	//nearestΪ�������K�������,incidentSurfacetsΪ�����ڽ�������ĳ�ʼ����������Ƭ;
	//incidentSurfacets_KNN��Ϊ���ͺ���������KNN���������б���������Ƭ;
	//incidentSurfacets_SparseΪKNN�����ں��нϳ��ߵı���������Ƭ����
	update_surfacets_KNN(pInside,V,nearest,incidentSurfacets,std::back_inserter(incidentSurfacets_KNN),std::back_inserter(incidentSurfacets_Sparse));

	//�������������������Ƭ
	//surfacetInterConflict�Ǳ�����Ƭ(�����Ǳ���,�������ڵ�cell����Ӱ����,����mirror facet����cell����Ӱ����,������һ����),���������ʱ����Ҫ
	//incidentSurfacets_KNNΪ��KNN�����ı�ѡ����������Ƭ;incidentSurfacets_SparseΪKNN�����ں��нϳ��ߵı�ѡ����������Ƭ����
	//PInside��ʾ��������ڱ����λ��;nearestFacetΪ���ص����ڽ�����������Ƭ;
	//surfacetsInConflictΪӰ�����ڵı���������Ƭ(���漰��mirror facet���ڵ�cell����conflict),���������ʱ����Ҫ
	//vertexWithDeletedSurfacetΪ(ɾ���ı���������Ƭ�еĵ�id,��֮�����ı���������Ƭ��ɾ���ĸ���),���������ɵĹ�����ļ��
	nearest_surfacet(p,surfacetInterConflict,incidentSurfacets_KNN,incidentSurfacets_Sparse,pInside,nearestFacet,surfacetsInConflict,
		vertexWithDeletedSurfacet);

	if (nearestFacet.first==-1)
	{
		return false;
	}
	//------------------------for test---------------------//
	//if (nearestFacet.first!=-1)
	//{
	//	Vertex_triple vtriF0=make_vertex_triple(nearestFacet);
	//	cout<<"��ʼ���������: ("<<vtriF0.first<<","<<vtriF0.second<<","<<vtriF0.third<<")"
	//		<<"/("<<nearestFacet.first<<","<<nearestFacet.second<<")"<<endl;
	//}
	//�ɹ�����V���ڽ�����������ƬnearestFacet������������ƬseedFacet(������Ƭ����������cell,��һ��cell��,��������Ƭ��ԵĶ���Ϊ������)
	//VΪ������,pInsideΪ��������ڱ����λ��
	seed_facet_of_isolate_vertex(V,nearestFacet,pInside,seedFacet);

	//------------------------for test---------------------//
	//if (seedFacet.first!=-1)
	//{
	//	Vertex_triple vtriF1=make_vertex_triple(seedFacet);
	//	cout<<"������Ƭ: ("<<vtriF1.first<<","<<vtriF1.second<<","<<vtriF1.third<<")"
	//		<<"/("<<seedFacet.first<<","<<seedFacet.second<<")"<<endl;
	//}
	//=======================test end======================//
	Facet tmpF;
	if (!is_vertex_isolate(V,tmpF,pInside))
	{
		return true;
	}

	if (seedFacet.first!=-1)
	{
		//���������V
		//V������,seedFacet���ӱ���������Ƭ,pInsideΪ��������ڱ����λ��
		link_isolate_to_surface(V,seedFacet,pInside);
		return true;
	}
	else
	{
		return false;
	}



}


template<typename T, typename T_INFO>
bool DataStructure<T, T_INFO>::
insert_vertex_into_surface(Vertex_Idtype V,Facet Surfacet,bool Inside)
{
	if(Inside)
	{
		Cell_Idtype c=Surfacet.first;
		label_cell_side(c,false);
		delete_surface_facet(Surfacet);
		int iv=vertex_index(c,V);
		for(int i=0;i<4;i++)
		{
			if(i!=iv)
			{
				Facet sf=mirror_facet(Facet(c,i));
				create_surface_facet(sf.first,sf.second);
			}
		}
	}
	else
	{
		Facet sf=mirror_facet(Surfacet);
		Cell_Idtype c=sf.first;
		label_cell_side(c,true);
		delete_surface_facet(Surfacet);
		int iv=vertex_index(c,V);
		for(int i=0;i<4;i++)
		{
			if(i!=iv)
			{
				create_surface_facet(c,i);
			}
		}
	}
	return true;
}


//���������
//VertexWithSurfacetDeletedΪ(���滻���ϵĶ���id,��֮�����ı���������Ƭ��ɾ���ĸ���)
//VertexWithSurfacetCreatedΪ�߽����ϵĶ���
template<typename T, typename T_INFO>
void DataStructure<T, T_INFO>::
isolate_vertices(std::map<Vertex_Idtype,size_t> VertexWithSurfacetDeleted,std::set<Vertex_Idtype> VertexWithSurfacetCreated)
{
	//����Ƿ��й�����,ͬʱ������������������������,�Ҳ��ظ�
	if(has_isolate_vertex(VertexWithSurfacetDeleted,VertexWithSurfacetCreated))
	{
		
			
		bool loop=false;
		Vertex_Idtype loopStart=*(IsolateVertices.begin());
		bool insertSuccess=false;
		bool handle=false;
		int t=IsolateVertices.size()+1;
		while((!IsolateVertices.empty()&&!loop)&&t--)
		{
			//bool inside;
			Facet surfacet=Facet(-1,-1);
			auto iiv=IsolateVertices.begin();
			Vertex_Idtype viiv=*iiv;
			if(viiv==loopStart&&handle)
			{
				loop=true;
				continue;
			}
			handle=true;	
			//---------------------for test------------------//
			/*cout<<endl<<"���������: "<<viiv<<endl;*/
			//====================test end===================//
			//is_vertex_isolate(*iiv,surfacet,inside);
			//���������viiv
			if(insert_vertex_into_surface(viiv))
			{
				insertSuccess=true;
				if (!IsolateVertices.empty())
				{
					IsolateVertices.pop_front();
				}	
				//---------------------for test------------------//
				/*cout<<"insert isolate point:"<<viiv<<" success"<<endl<<endl;*/
				//====================test end===================//
			}
			else
			{
				if(insertSuccess)
				{
					loopStart=viiv;
				}
				if (!IsolateVertices.empty())
				{
					IsolateVertices.pop_front();
				}				
				IsolateVertices.push_back(viiv);
				//insert_to_isolate_vertices(viiv);
				insertSuccess=false;
				//---------------------for test------------------//
				/*cout<<"insert isolate point:"<<viiv<<" failure"<<endl<<endl;*/
				//====================test end===================//
			}
			
		}
	}
}

//VertexWithSurfacetDeletedΪ(���滻���ϵĶ���id,��֮�����ı���������Ƭ��ɾ���ĸ���)
//VertexWithSurfacetCreatedΪ�߽����ϵĶ���
//����Ƿ��й�����,ͬʱ������������������������,�Ҳ��ظ�
template<typename T, typename T_INFO>
bool DataStructure<T, T_INFO>::
has_isolate_vertex(std::map<Vertex_Idtype,size_t> VertexWithSurfacetDeleted,std::set<Vertex_Idtype> VertexWithSurfacetCreated)
{
	
	auto ivd=VertexWithSurfacetDeleted.begin();
	while (ivd!=VertexWithSurfacetDeleted.end())
	{
		//���滻����ĳ�����umbrella(���ٰ���3������)ȫ��������,����������,һ�����ڱ��滻��߽�����
		if(VertexWithSurfacetCreated.find(ivd->first)==VertexWithSurfacetCreated.end()&&
			ivd->second>2&&ivd->first!=0)
		{
			Vertex_Idtype vi=ivd->first;
			Facet fi=Facet(-1,-1);
			bool inside=false;
			bool is_isolate=is_vertex_isolate(vi,fi,inside);
			if(is_isolate)
			{
				//��������V�����������������,�Ҳ��ظ�
				insert_to_isolate_vertices(vi);
			}
		}
		ivd++;
	}
	if(IsolateVertices.empty())
		return false;
	else
		return true;
}



template<typename T, typename T_INFO>
Cell_Idtype DataStructure<T, T_INFO>::
create_face()
{
	return create_cell();
}

template<typename T, typename T_INFO>
Cell_Idtype DataStructure<T, T_INFO>::
create_face(Vertex_Idtype v0, Vertex_Idtype v1,
	Vertex_Idtype v2)
{
	return create_cell(v0, v1, v2, -1);
}

// The following functions come from TDS_2.
template<typename T, typename T_INFO>
Cell_Idtype DataStructure<T, T_INFO>::
create_face(Cell_Idtype f0, int i0,
	Cell_Idtype f1, int i1,
	Cell_Idtype f2, int i2)
{
	Cell_Idtype newf = create_face(vertex(f0,Triangulation_cw_ccw_2::cw(i0)),
		vertex(f1,Triangulation_cw_ccw_2::cw(i1)),
		vertex(f2,Triangulation_cw_ccw_2::cw(i2)));
	set_adjacency(newf, 2, f0, i0);
	set_adjacency(newf, 0, f1, i1);
	set_adjacency(newf, 1, f2, i2);
	return newf;
}

template<typename T, typename T_INFO>
Cell_Idtype DataStructure<T, T_INFO>::
create_face(Cell_Idtype f0, int i0,
	Cell_Idtype f1, int i1)
{
	Cell_Idtype newf = create_face(vertex(f0,Triangulation_cw_ccw_2::cw(i0)),
		vertex(f1,Triangulation_cw_ccw_2::cw(i1)),
		vertex(f1,Triangulation_cw_ccw_2::ccw(i1)));
	set_adjacency(newf, 2, f0, i0);
	set_adjacency(newf, 0, f1, i1);
	return newf;
}




template<typename T, typename T_INFO>
Facet DataStructure<T, T_INFO>::
mirror_facet(Facet f) 
{
	Cell_Idtype neighbor_cell = neighbor(f.first,f.second);
	const int opposite_index = neighbor_index(neighbor_cell,f.first);
	return Facet(neighbor_cell, opposite_index);
}

template<typename T, typename T_INFO>
EdgeInFacet DataStructure<T, T_INFO>::
mirror_edge(EdgeInFacet E)
{
	Facet fE=E.first;
	
	Facet oppFacet=neighbor_surfacet(fE,E.second);
	
	Indextype ii=neighbor_index_facet(oppFacet,fE);
	return EdgeInFacet(oppFacet,ii);
}


//��ĳһ���incident vertex����surface�ϵ�umbrella���������
//��IdV�������㣬IncidentVertices��IdV��������ĵ�������back_inserter����IncidentFacets�ǹ����ı���(Ϊumbrella)�ĵ�������back_inserter��
template<typename T, typename T_INFO>
template<class IteratorVertex,class IteratorFacet>
void DataStructure<T, T_INFO>::
incident_vertex_and_surface_facet(Vertex_Idtype IdV,IteratorVertex IncidentVertices,IteratorFacet IncidentFacets)
{
	if (dimension() < 2)
		return ;

	//Visitor visit(IdV, outputCell, this, f);

	std::vector<Cell_Idtype> tmp_cells;
	tmp_cells.reserve(64);
	/*std::vector<Facet> temp_facets;
	tmp_facets.reserve(64);*/
	std::vector<Vertex_Idtype> tmp_vertices;

	if (dimension() == 3)
		//�ҳ�����IdV������������ͱ�����Ƭ������cell(IdV)��IdV��CellIncident��¼�Ĺ���������
		//tmp_cells��IdV���������壬IncidentFacets�ǹ����ı���ĵ�������back_inserter��
		incident_cells_surfacet_3_withoutmark(IdV, cell(IdV),std::back_inserter(tmp_cells),IncidentFacets);


	typename std::vector<Cell_Idtype>::iterator cit;
	for (cit = tmp_cells.begin();
		cit != tmp_cells.end();
		++cit)
	{
		clear_cell_visited(*cit);
		

		//=================vertex extractor===============
		for (int j = 0; j <= dimension(); ++j) {
			Vertex_Idtype w = vertex(*cit,j);
			
			if (w != IdV){

				if (visited_for_vertex_extractor(w)==0){
					//w->visited_for_vertex_extractor = true;
					mark_vertex_visited(w);
					tmp_vertices.push_back(w);
					*IncidentVertices++ = w;
					//treat(c, IdV, j);
				}
			}
		}
		//----------------------vertex extractor end-------------------
	}
	for (std::size_t i = 0; i < tmp_vertices.size(); ++i){
		clear_vertex_visited(tmp_vertices[i]);
	}
}

template<typename T, typename T_INFO>
template<class IteratorFacet>
void DataStructure<T, T_INFO>::
incident_surface_facet(Vertex_Idtype IdV, IteratorFacet Surfacets)
{
	if (dimension() < 2)
		return ;

	std::vector<Cell_Idtype> tmp_cells;
	tmp_cells.reserve(64);

	std::vector<Vertex_Idtype> tmp_vertices;

	if (dimension() == 3)
    {
		//����cell��conflict mark��ǰ������incident_cell 3d, ������Ƿ�ɾ��
		//�ҳ�����IdV������������ͱ�����Ƭ,
		//tmp_cells�ǹ��������弯��,Surfacets�ǹ����ı��漯��
		incident_cells_surfacet_3_withoutmark(IdV, cell(IdV),std::back_inserter(tmp_cells),Surfacets);
	}


	typename std::vector<Cell_Idtype>::iterator cit;
	for (cit = tmp_cells.begin();
		cit != tmp_cells.end();
		++cit)
	{
		clear_cell_visited(*cit);
	}
}


//���µ�p������б�nearest,���v��nearest��ĵ����p�������͸���nearest�����򲻸���nearest
template<typename T, typename T_INFO>
void  DataStructure<T, T_INFO>::
refresh_K_nearest(const T* p,Vertex_Idtype v,list<Vertex_Idtype>& nearest)
{
	auto in=nearest.begin();
	for(;in!=nearest.end();in++)
	{
	//nearest_vertex(p,v,*in)�Ƚ�һ����p��������������
		if(*in==-1||(nearest_vertex(p,v,*in)==v))
		{
			if(*in!=v)
			{
				nearest.insert(in,v);
				nearest.pop_back();
			}
			break;
		}
	}
}

template<typename T, typename T_INFO>
Vertex_Idtype  DataStructure<T, T_INFO>::
nearest_vertex(const T* p,Vertex_Idtype v,Vertex_Idtype w)
{
	if(is_infinite(v))
		return w;
	if(is_infinite(w))
		return v;
	if(NumericComputation<T>::SquareDistance(p,point(v))<NumericComputation<T>::SquareDistance(p,point(w)))
		return v;
	else
		return w;
}

//�Ƿ�Ҫ����inflate/sculpture�Ĵ���
//�������ȵ�ϡ������ʱ��������Ƭ����ߵĳ������ж�
//InitFacetConflictΪ��Delaunay conflict region����õ�������棬��һ��Ϊfacet���ڶ���ΪcosDihedral,������Ϊ��cosDihedral>-0.1ʱ��distance
template<typename T, typename T_INFO>
void DataStructure<T, T_INFO>::
nearest_surface_facet(const T* p,list<Vertex_Idtype> NearestVertices,vector<Facet> InitFacet,
	Triple<Facet,double,double> InitFacetConflict, bool PInside,Facet& NearestFacet)
{

	//step1:��K-NN��incident surface facet�����п��ܵ�nearest surface facet���������������������Ƭ
	//��С����ǵ�����ֵ
	double cosMinDihedral=-1;
	//��С����Ƕ�Ӧ��������Ƭ
	Facet facetMinDihedral(-1,-1);
	//��P��������Ƭ�γɵĶ���Ǿ�Ϊ���ʱ��P���������С����
	double minDistance=COMPUTATION_DOUBLE_MAX;
	//��С�����Ӧ��������Ƭ
	Facet facetMinDistance(-1,-1);
	//K-NN��incident surfacet�е����������Ƭ
	double maxLengthEdge=0;
	Facet facetLarge(-1,-1);

	//markForVertices���ڱ�ǵ�i������ڵ��Ƿ���δ����δ0��������Ϊ2,���Ѿ������Ҳ��ǹ�����Ϊ1���ǹ�������߲���������Ϊ����ڵı���Ϊ-1
	int markForVertices[5]={0};
	//���ڴ洢K-NN��incident surfacet���Ҷ���P�ɼ�
	Facet facetNVertices[5]={Facet(-1,-1)};
	//�洢���ڴ����vertex 
	Vertex_Idtype vertexHandled=*NearestVertices.begin();
	int rankVertexHandled=0;
	//���ڴ洢vertexHandled��һ��incident surface facet���ҵ�һ�����ڽ���ʱ��facet����P�ɼ�  
	Facet facetHandledInit(-1,-1);
	if (!InitFacet.empty())
	{
		facetHandledInit=*InitFacet.begin();
	}
	//markForVertices[0]=2;
	//facetNVertices[0]=facetHandledInit;
	if (facetHandledInit.first!=-1)
	{
		markForVertices[0]=1;
		facetNVertices[0]=facetHandledInit;
	}
	else
	{
		markForVertices[0]=-1;
	}	
	//�������е�K-NN�Ƿ�����ϣ���Ϊfalse����Ϊtrue
	bool testForAll=true;

	while (testForAll)
	{
		double cosDihedral;
		
		double distance;
		Facet tmpFacet=facetHandledInit;
		bool markForCircleBegin=true;
		if (facetHandledInit.first!=-1)
		{
			while (facetHandledInit!=tmpFacet||markForCircleBegin)
			{
				//��tmpFacet��vertexHandled��תʱ�ĵ�ǰfacet		
				Indextype idVertexOfFacet=vertex_index_facet(tmpFacet,vertexHandled);
				std::pair<Indextype,Indextype> idVertexOfEdge=make_edge_index(idVertexOfFacet);
				markForCircleBegin=false;
				Vertex_triple vf=make_vertex_triple(tmpFacet);
				Orientation o = GeometricTraits<T>::orientation(point(vf.first), point(vf.second), point(vf.third), p);
				if ((o!=NEGATIVE&&PInside)||(o!=POSITIVE&&!PInside))
				{
					if (markForVertices[rankVertexHandled]==2)
					{
						markForVertices[rankVertexHandled]=1;
					}
					if(!PInside)
					{
						cosDihedral=cos_min_dihedral_point_to_facet(p,mirror_facet(tmpFacet));
					}
					else
					{
						cosDihedral=cos_min_dihedral_point_to_facet(p,tmpFacet);
					}
					if(cosDihedral>-0.0)
					{
						distance =GeometricTraits<T>::distance_point_to_facet(p,point(vf.first),
															point(vf.second),point(vf.third));
						if(!PInside)
						{
							distance=-distance;
						}
						if (distance<minDistance)
						{
							minDistance=distance;
							facetMinDistance=tmpFacet;
						}
					}
					else if(cosDihedral>cosMinDihedral)
					{
						cosMinDihedral=cosDihedral;
						facetMinDihedral=tmpFacet;
					}
					//�ж��Ƿ��д�������
					std::pair<Vertex_Idtype,Vertex_Idtype> verticesEdge=make_vertex_pair(EdgeInFacet(tmpFacet,idVertexOfFacet));
					double d0=NumericComputation<double>::SquareDistance(point(verticesEdge.first),point(verticesEdge.second));
					double d1=NumericComputation<double>::SquareDistance(point(vertexHandled),point(verticesEdge.second));
					if (d0>maxLengthEdge)
					{
						maxLengthEdge=d0;
						facetLarge=tmpFacet;
					}
					else if(d1>maxLengthEdge)
					{
						maxLengthEdge=d1;
						facetLarge=tmpFacet;
					}
				}
					
				
				Vertex_Idtype vertexLeftEdge=vertex_facet(tmpFacet,idVertexOfEdge.first);
				int iInNN=rank_in_list(vertexLeftEdge,NearestVertices.begin(),NearestVertices.end());
				if(iInNN!=-1&&markForVertices[iInNN]==0)
				{
					markForVertices[iInNN]=2;
					facetNVertices[iInNN]=tmpFacet;
				}
				tmpFacet=neighbor_surfacet(tmpFacet,idVertexOfEdge.first);
			}
			if (markForVertices[rankVertexHandled]==2)
			{
				markForVertices[rankVertexHandled]=-1;
			}
		
		}
		//�����һ���ڽ����incident surface facet
		testForAll=false;
		auto itN=NearestVertices.begin(); 
		for (int i = 0; i < K_NN; i++)
		{
			if (*itN<=0)
			{
				markForVertices[i]=-1;
				itN++;
				continue;
			}
			else if (markForVertices[i]==2)
			{
				testForAll=true;
				rankVertexHandled=i;
				vertexHandled=*itN;
				facetHandledInit=facetNVertices[i];
				break;
			}
			itN++;
		}
		itN=NearestVertices.begin();
		if (!testForAll)
		{
			for (int i = 0; i < K_NN; i++)
			{
				if (markForVertices[i]==0)
				{
					std::vector<Facet> incidentFacets;
					incident_surface_facet(*itN,std::back_inserter(incidentFacets));
					if (incidentFacets.empty())
					{
						markForVertices[i]=-1;
						continue;
					}
					else
					{
						testForAll=true;
						markForVertices[i]=2;
						vertexHandled=*itN;
						facetNVertices[i]=*incidentFacets.begin();
						facetHandledInit=*incidentFacets.begin();
					}
				}
				itN++;
			}
			
		}

	}
	//������������Ƭ�з�K-NN�ĵ��incident surface facet
	std::vector<Vertex_Idtype> VertexNearSparse;
	//ϡ����������С����ǵ�����ֵ
	double cosMinDihedralSparse=-1;
	//ϡ��������С����Ƕ�Ӧ��������Ƭ
	Facet facetMinDihedralSparse(-1,-1);
	//ϡ���������Ǿ�Ϊ����ҵ�P���������������Ƭ�ľ���
	double minDistanceSparse=0;
	//ϡ���������Ǿ�Ϊ����ҵ�P���������������Ƭ
	Facet facetMinDistanceSparse(-1,-1);

	for (int i = 0; i < 3; i++)
	{
		Vertex_Idtype vLargeFacet=vertex_facet(facetLarge,i);
		int rankVL=rank_in_list(vLargeFacet,NearestVertices.begin(),NearestVertices.end());
		if (rankVL==-1)
		{
			VertexNearSparse.push_back(vLargeFacet);
		}
	}
	for (auto itF=VertexNearSparse.begin();itF!=VertexNearSparse.end();itF++)
	{
		bool markForCircleBegin=true;
		Facet tmpFacet=facetLarge;
		vertexHandled=*itF;
		while (tmpFacet!=facetLarge||markForCircleBegin)
		{
			double cosDihedral;
			double distance;
			markForCircleBegin=false;
			//��tmpFacet��vertexHandled��תʱ�ĵ�ǰfacet
			Vertex_triple vf=make_vertex_triple(tmpFacet);
			Indextype idVertexOfFacet=vertex_index_facet(tmpFacet,vertexHandled);
			std::pair<Indextype,Indextype> idVertexOfEdge=make_edge_index(idVertexOfFacet);

			
			Orientation o = GeometricTraits<T>::orientation(point(vf.first), point(vf.second), point(vf.third), p);
			if ((o!=NEGATIVE&&PInside)||(o!=POSITIVE&&!PInside))
			{
				
				if(!PInside)
				{
					cosDihedral=cos_min_dihedral_point_to_facet(p,mirror_facet(tmpFacet));
				}
				else
				{
					cosDihedral=cos_min_dihedral_point_to_facet(p,tmpFacet);
				}
				if(cosDihedral>-0.0)
				{
					distance =GeometricTraits<T>::distance_point_to_facet(p,point(vf.first),
														point(vf.second),point(vf.third));
					if(!PInside)
					{
						distance=-distance;
					}
					if (distance<minDistance)
					{
						minDistance=distance;
						facetMinDistanceSparse=tmpFacet;
					}
				}
				else if(cosDihedral>cosMinDihedral)
				{
					cosMinDihedral=cosDihedral;
					facetMinDihedralSparse=tmpFacet;
				}
			}			
			Vertex_Idtype vertexLeftEdge=vertex_facet(tmpFacet,idVertexOfEdge.first);
		
			tmpFacet=neighbor_surfacet(tmpFacet,idVertexOfEdge.first);
		}
	}

	if (facetMinDistance.first!=-1)
	{
		double mminDistance=minDistance;
		NearestFacet=facetMinDistance;
		if (facetMinDistanceSparse.first!=-1)
		{
			if (minDistanceSparse<mminDistance)
			{
				mminDistance=minDistanceSparse;
				NearestFacet=facetMinDistanceSparse;
			}
		}
		if (InitFacetConflict.second>-0.1)
		{
			if (InitFacetConflict.third<mminDistance)
			{
				mminDistance=InitFacetConflict.third;
				NearestFacet=InitFacetConflict.first;
			}
		}
	}
	else
	{
		double mminDistance=shortest_edge_to_facet(p,facetMinDihedral);
		NearestFacet=facetMinDihedral;
		if (facetMinDistanceSparse.first!=-1)
		{
			if (minDistanceSparse<mminDistance)
			{
				mminDistance=minDistanceSparse;
				NearestFacet=facetMinDistanceSparse;
			}
		}
		else if(facetMinDihedralSparse.first!=-1)
		{
			double tmpDis=shortest_edge_to_facet(p,facetMinDihedralSparse);
			if (tmpDis<mminDistance)
			{
				if (cosMinDihedralSparse>cosMinDihedral)
				{
					mminDistance=tmpDis;
					NearestFacet=facetMinDihedralSparse;
				}
			}
		}
		if (InitFacetConflict.second>-0.1)
		{
			if (InitFacetConflict.third<mminDistance)
			{
				mminDistance=InitFacetConflict.third;
				NearestFacet=InitFacetConflict.first;
			}
		}

	}
}

template<typename T, typename T_INFO>
void DataStructure<T, T_INFO>::
nearest_surface_facet(const T* p,list<Vertex_Idtype> NearestVertices,vector<Facet> InitFacet,bool PInside,Facet& NearestFacet,bool& IsConcave)
{
	IsConcave=false;
	
	std::vector<Facet> tmpSurfacets;
	std::vector<bool> isConcaveNearestAcuteDihedral;
	std::vector<Facet> facetsOfNearestAcuteDihedral;

	Facet facetOfNearestMinDihedral(-1,-1);

	double cosNearestMinDiheral=-1;
	double minNearestDistance=COMPUTATION_DOUBLE_MAX;
	
	//��ʼ��NearestFacet
	NearestFacet=make_pair(-1,-1);
	
	for(auto fit=InitFacet.begin();fit!=InitFacet.end();fit++)
	{
		Facet tmpInitFacet=*fit;	
		//------------------for test----------------//
		//Vertex_triple vtriT0=make_vertex_triple(tmpInitFacet);
		//==================test end================//

		if(visited_for_facet_extractor(tmpInitFacet)==1)
		{	
			std::vector<bool> isConcaveAcuteDihedral;
			std::vector<Facet> facetsOfAcuteDihedral;
								
			Facet facetOfMinDihedral=tmpInitFacet;
			bool isConcaveMinDihedral;
			Facet tmpNearestFacet=tmpInitFacet;
			list<Facet> neighborSurfacets;
			neighborSurfacets.push_back(tmpInitFacet);
			double cosMinDihedral=-1;
			double minDistance=COMPUTATION_DOUBLE_MAX;
			if(!PInside)
			{
				cosMinDihedral=cos_min_dihedral_point_to_facet(p,mirror_facet(tmpInitFacet));
			}
			else
			{
				cosMinDihedral=cos_min_dihedral_point_to_facet(p,tmpInitFacet);
			}
			if(cosMinDihedral>-0.1)
			{
				facetsOfAcuteDihedral.push_back(tmpInitFacet);
			}
			while(!neighborSurfacets.empty())
			{
				Facet tmpSurfacet=*neighborSurfacets.begin();
				tmpSurfacets.push_back(tmpSurfacet);
				mark_facet_visited(tmpSurfacet);
				neighborSurfacets.pop_front();
				//-----------------------for test--------------------------//
				//Vertex_triple vTriT0=make_vertex_triple(tmpSurfacet);
				//=======================test end==========================//
		
				Cell_Idtype c = tmpSurfacet.first;
				int li = tmpSurfacet.second;
				// Look for the other neighbors of c.
				for(int ii=0;ii<3;++ii)	{
			
					Facet fCurNei=neighbor_surfacet(tmpSurfacet,ii);
					Vertex_triple vftT0=make_vertex_triple(fCurNei);
					//==================for test================//
					//bool isVis=(visited_for_facet_extractor(fCurNei)!=0);
					//bool isNei=is_facet_in_neighbor(fCurNei,NearestVertices);
					//==================test end================//
					if((visited_for_facet_extractor(fCurNei)==0)&&is_facet_in_neighbor(fCurNei,NearestVertices))
					{
						neighborSurfacets.push_back(fCurNei);
						
						Vertex_triple vf=make_vertex_triple(fCurNei);						
						Orientation o = GeometricTraits<T>::orientation(point(vf.first), point(vf.second), point(vf.third), p);
						if ((o!=NEGATIVE&&PInside)||(o!=POSITIVE&&!PInside))
						{
							double cosDihedral;				
					

							if(!PInside)
							{						
								cosDihedral=cos_min_dihedral_point_to_facet(p,mirror_facet(fCurNei));
							}
							else
							{
								cosDihedral=cos_min_dihedral_point_to_facet(p,fCurNei);
							}
							if(cosDihedral>cosMinDihedral)
							{
								facetOfMinDihedral=fCurNei;
								cosMinDihedral=cosDihedral;
							}
							if(cosDihedral>-0.1)
							{
								facetsOfAcuteDihedral.push_back(fCurNei);
							}
					
						}

					}
	
				}

			}
			if(facetsOfAcuteDihedral.size()>1)
			{
				for(auto fit=facetsOfAcuteDihedral.begin();fit!=facetsOfAcuteDihedral.end();fit++)
				{
					Vertex_triple vf=make_vertex_triple(Facet(*fit));
					double distance =GeometricTraits<T>::distance_point_to_facet(p,point(vf.first),
																						point(vf.second),point(vf.third));
					if(!PInside)
					{
						distance=-distance;										
					}
					if(distance<minDistance)
					{
						minDistance=distance;
						tmpNearestFacet=*fit;
						facetsOfNearestAcuteDihedral.push_back(tmpNearestFacet);
						isConcaveNearestAcuteDihedral.push_back(true);
					}
				}
			
			}
			else
			{		
				if(facetOfMinDihedral.first!=-1)
				{
					if(cosMinDihedral>cosNearestMinDiheral)
					{
						tmpNearestFacet=facetOfMinDihedral;
						cosNearestMinDiheral=cosMinDihedral;
						facetOfNearestMinDihedral=tmpNearestFacet;
						if(cosMinDihedral>-0.1)
						{
							facetsOfNearestAcuteDihedral.push_back(tmpNearestFacet);
							isConcaveNearestAcuteDihedral.push_back(false);
						}
					}
				}
			}
		}
	}

	if(!facetsOfNearestAcuteDihedral.empty())
	{
		auto itConcave=isConcaveNearestAcuteDihedral.begin();
		for(auto fit=facetsOfNearestAcuteDihedral.begin();
			fit!=facetsOfNearestAcuteDihedral.end();fit++,itConcave++)
		{
			Vertex_triple vf=make_vertex_triple(Facet(*fit));
			double distance =GeometricTraits<T>::distance_point_to_facet(p,point(vf.first),
														point(vf.second),point(vf.third));
			if(!PInside)
			{
				distance=-distance;										
			}
			if(distance<minNearestDistance)
			{
				minNearestDistance=distance;
				NearestFacet=*fit;				
				IsConcave=*itConcave;
			}
		}
	}
	else
	{
		IsConcave=false;
		NearestFacet=facetOfNearestMinDihedral;
		//if(cosNearestMinDiheral<-0.1&&PInside)
		//	cout<<"�˴�������ͶӰ���ڱ�����"<<endl;
	}
	
	for(auto fit=tmpSurfacets.begin();fit!=tmpSurfacets.end();fit++)
	{
		clear_facet_visited(*fit);
	}
	
	

}

//���������V
//V������,SeedFacet���ӱ���������Ƭ,PInsideΪ��������ڱ����λ��
template<typename T, typename T_INFO>
bool DataStructure<T, T_INFO>::
link_isolate_to_surface(Vertex_Idtype V,Facet SeedFacet,bool PInside)
{

	if(SeedFacet.first!=-1)
	{
		std::vector<Facet> conflictFacets;
		std::set<Vertex_Idtype> vertexOnBoundary;
		std::set<Vertex_Idtype> vertexOnConflict;
		std::vector<Facet> newCreateSurfacets;
		std::list<Edge> conflictBoundEdges;
		std::vector<Facet> newCreateBoundFacets;//û��
		vector<Facet> newBoundarySurfacets;
		vector<Facet> newConflictSurfacets;

		//�ڹ����������,ʹ��Gabriel�Ĺ�������滻�����չ
		//V������;SeedFacetΪ����������Ƭ;PInsideΪ������V���ڱ����λ��
		//VertexOnBoundaryΪ�߽����ϵĶ��㣬���ڹ�����ļ��;
		//VertexOnConflictΪɾ���ı���������Ƭ�еĶ���,���ڹ�����ļ��;conflictFacets�Ǳ��滻��;
		//conflictBoundEdgesΪ����ֵ�����滻��������ı߽��߼��ϣ���Edge��ʽ(��id,0/1/2)��ʾ,���Ҹ��治�滻
		//newCreateSurfacets�������ɱ���ļ���
		bool quit=false;
		find_isolate_conflict_surfacet(V,SeedFacet,PInside,vertexOnBoundary,vertexOnConflict,conflictFacets,conflictBoundEdges,newCreateSurfacets,quit);
		if (quit)
		{
			/*cout<<"[iso] quit point"<<endl;*/
			return false;
		}
		for(auto fit=conflictFacets.begin();fit!=conflictFacets.end();fit++)
		{
			delete_surface_facet(*fit);
			if(PInside)
			{
				label_cell_side((*fit).first,false);
			}
			else
			{
				Cell_Idtype cOpp=neighbor((*fit).first,(*fit).second);
				label_cell_side(cOpp,true);
			}
		}
		/*cout<<"�����ɱ���: "<<endl;*/
		for(auto fit=newCreateSurfacets.begin();fit!=newCreateSurfacets.end();fit++)
		{
			//-------------------for test-----------------//
			//Vertex_triple vftriT0=make_vertex_triple(*fit);
			//cout<<"("<<vftriT0.first<<","<<vftriT0.second<<","<<vftriT0.third<<")"
			//	<<"/("<<(*fit).first<<","<<(*fit).second<<")"<<endl;
			//===================test end=================//

			create_surface_facet(*fit);			
		}

		update_surface_connection(V,conflictBoundEdges,newCreateBoundFacets,newBoundarySurfacets);

		for(auto itF=newCreateSurfacets.begin();itF!=newCreateSurfacets.end();itF++)
		{
			vector<Facet> newUpdateSur;
			Facet tmpF(-1,-1);
			//��������,ʵ�ʲ���Ϊ������ڲ�����cell
			//*itFΪ��ʼ���͵ı���������Ƭ;�ڶ�������(�β�VertexIso)Ϊ���ܴ��ڵĹ�����,ȡ-1;newUpdateSurΪ�������͹��������γɵı���������Ƭ����
			//�ڶ���-1(�β�IdV)Ϊĳһ����;tmpFΪ���γɵ�����IdV�����ı���������Ƭ,��ʼ(-1,-1);true(�β�MarkIso)�����������
			bool isFlated=iterative_inflate_with_mark(*itF,-1,std::back_inserter(newUpdateSur),-1,tmpF,true);
			//�������,ʵ�ʲ����޳������ڲ�cell
			//*itFΪ��ʼ��̵ı���������Ƭ;�ڶ�������(�β�VertexIso)Ϊ���ܴ��ڵĹ�����,ȡ-1;newUpdateSurΪ�������͹��������γɵı���������Ƭ����
			//�ڶ���-1(�β�IdV)Ϊĳһ����;tmpFΪ���γɵ�����IdV�����ı���������Ƭ,��ʼ(-1,-1);true(�β�MarkIso)�����������
			bool isSculptured=iterative_sculpture_with_mark(*itF,-1,std::back_inserter(newUpdateSur),-1,tmpF,true);
		}

		return true;
	}
	else 
		return false;
}


template<typename T, typename T_INFO>
void DataStructure<T, T_INFO>::
recursive_find_isolate_conflict_surfacet(Vertex_Idtype V, Facet SeedFacet,bool PInside,
	std::set<Vertex_Idtype>& VertexOnBoundary,set<Vertex_Idtype>& VertexOnConflictFacet,
	vector<Facet>& Conflicts,list<Edge>& ConflictBoundEdges,vector<Facet>& NewCreateSurfacets,int prev_ind3)
{
	const T* p=point(V);
	//----------for test----------//
	//BeginFacet=Facet(-1,-1);
	Facet fT=mirror_facet(SeedFacet);
	Vertex_triple ftrip=make_vertex_triple(SeedFacet);
	Cell_Idtype cT=fT.first;
	bool isHull=has_vertex(cT,0);
	bool isNoGabriel;

	if(PInside)
	{
		isNoGabriel=GeometricTraits<T>::side_of_bounded_sphereC3(point(ftrip.first),point(ftrip.second),point(ftrip.third),p)==ON_BOUNDED_SIDE;
	}
	else
	{
		isNoGabriel=GeometricTraits<T>::side_of_bounded_sphereC3(point(ftrip.second),point(ftrip.first),point(ftrip.third),p)==ON_BOUNDED_SIDE;
	}
	cout<<ftrip.first<<" "<<ftrip.second<<" "<<ftrip.third<<"    "<<"is or not hull_"<<isHull<<"      "<<"is Gabriel_"<<isNoGabriel<<endl;
	
	Conflicts.push_back(SeedFacet);
	mark_facet_visited(SeedFacet);
	Vertex_triple vertexOnFacet=make_vertex_triple(SeedFacet);
	VertexOnConflictFacet.insert(vertexOnFacet.first);
	VertexOnConflictFacet.insert(vertexOnFacet.second);
	VertexOnConflictFacet.insert(vertexOnFacet.third);

	Cell_Idtype c = SeedFacet.first;
	
	int li = SeedFacet.second;
	// Look for the other neighbors of c.
	for (int ii = 0; ii<4; ++ii) {
		//if (ii == prev_ind3 || neighbor(c,ii) != -1)
		if (ii==prev_ind3||ii==li)
			continue;
		//cnew->vertex(ii)->set_cell(cnew);

		// Indices of the vertices of cnew such that ii,vj1,vj2,li positive.
		Vertex_Idtype vj1 = vertex(c, Triangulation_utils_3::next_around_edge(ii, li));
		Vertex_Idtype vj2 = vertex(c, Triangulation_utils_3::next_around_edge(li, ii));
		Cell_Idtype cur = c;
		int zz = ii;
		Cell_Idtype n = neighbor(cur,zz);
		// turn around the oriented edge vj1 vj2
		while (is_label_inside(n)) {
		
			cur = n;
			zz = Triangulation_utils_3::next_around_edge(vertex_index(n, vj1), vertex_index(n, vj2));
			n = neighbor(cur,zz);
		}
		// Now n is outside region, cur is inside.
		//-------------------for test Facet(cur,zz)---------------------//
		//bool isSurfceFacet=is_surface(Facet(cur,zz));
		//Vertex_triple vfnext=make_vertex_triple(Facet(cur,zz));
		//Vertex_Idtype vOpp=vertex(cur,zz);
		//======================test end================================//
		int jj1 = vertex_index(n, vj1);
		int jj2 = vertex_index(n, vj2);
		Vertex_Idtype vvv = vertex(n, Triangulation_utils_3::next_around_edge(jj1, jj2));
		Cell_Idtype nnn = neighbor(n, Triangulation_utils_3::next_around_edge(jj2, jj1));
		int zzz = vertex_index(nnn, vvv);

		bool isNoGabrielNew=false;
		if(PInside)
		{
			Vertex_triple ftrip=make_vertex_triple(Facet(cur,zz));
			isNoGabrielNew=GeometricTraits<T>::side_of_bounded_sphereC3(point(ftrip.first),
				point(ftrip.second),point(ftrip.third),p)==ON_BOUNDED_SIDE;
		}

		if(visited_for_facet_extractor(Facet(cur,zz))==0)
		{
			//�ж�facet�Ƿ���DelaunayӰ����߽��ϻ����ڲ�
			/*if(((is_in_conflict(cur)&&PInside)||(is_in_conflict(neighbor(cur,zz))&&!PInside))
				&&((!PInside)||(PInside&&isNoGabrielNew)))*/

			//�жϴ�facet�Ķ����Ƿ�Ϊ������			
			Cell_Idtype cOpp=neighbor(cur,zz);
			int indOpp=neighbor_index(cOpp,cur);
			
			if((PInside&&vertex(cur,zz)==V)||(!PInside&&vertex(cOpp,indOpp)==V))
			//�ж�facet�Ƿ�Ϊboundary facet
			/*if(((is_in_conflict(cur)&&is_on_boundary(neighbor(cur,zz)))||(is_in_conflict(neighbor(cur,zz))&&is_on_boundary(cur)))
				&&((!PInside)||(PInside&&isNoGabrielNew)))*/
			{	
				//���½�����ı߽�߶���
				Facet nei=make_pair(cur,zz);
				int i0=-1,i1=-1;
				int numOfList=0;
				bool handled=false;
				auto itEdge=ConflictBoundEdges.begin();
				for(;itEdge!=ConflictBoundEdges.end();itEdge++)
				{
					numOfList++;
					Facet fCurEdge=get_facet_cell_link((*itEdge).first);
					//------------for test-------------//
					Vertex_triple vTriT0=make_vertex_triple(fCurEdge);
					//============test end=============//
					if(fCurEdge==nei)
					{
						
						i0=(*itEdge).second;
						ConflictBoundEdges.erase(itEdge++);
						break;
					}
				}
					//��list���ĵ�һ����ɾ��ʱ��Ӧ�ü�����һ���Ƿ�ҲӦ�ñ�ɾ��
				if(numOfList==1)
				{
					auto endEdge=ConflictBoundEdges.back();
					Facet tmpBoundFacet00=get_facet_cell_link(endEdge.first);
					//--------------for test----------//
					Vertex_triple vtriT00=make_vertex_triple(tmpBoundFacet00);
					//==============test end=========//
					if(tmpBoundFacet00==nei)
					{
						handled=true;
						i1=endEdge.second;
						ConflictBoundEdges.pop_back();
						for(int i2=0;i2<3;i2++)
						{
							if(i2!=i0&&i2!=i1)
							{
								EdgeInFacet oppEdge=mirror_edge(EdgeInFacet(nei,i2));
								//--------------for test-----------//
								Vertex_triple vtriT2=make_vertex_triple(oppEdge.first);
								//==============test end==========//
								ConflictBoundEdges.insert(itEdge,turn_edgeinfacet_to_edge(oppEdge));
								break;
							}
						}
					}
					
				}
				//list �м�Ĳ�����ɾ��Ҫ�����
				if(!handled)
				{
					if(itEdge!=ConflictBoundEdges.end()&&get_facet_cell_link((*itEdge).first)==nei)
					{

						i1=(*itEdge).second;
						ConflictBoundEdges.erase(itEdge++);
						for(int i2=0;i2<3;i2++)
						{
							if(i2!=i0&&i2!=i1)
							{
								EdgeInFacet oppEdge=mirror_edge(EdgeInFacet(nei,i2));
								ConflictBoundEdges.insert(itEdge,turn_edgeinfacet_to_edge(oppEdge));
								break;
							}
						}
					}
					else
					{
						auto leftRight=make_edge_index(i0);
						EdgeInFacet oppEdge0=mirror_edge(EdgeInFacet(nei,leftRight.second));
						EdgeInFacet oppEdge1=mirror_edge(EdgeInFacet(nei,leftRight.first));
						//--------------for test-----------//
						Vertex_triple vtriT3=make_vertex_triple(oppEdge0.first);
						Vertex_triple vtriT4=make_vertex_triple(oppEdge1.first);
						//=============test end============//
						ConflictBoundEdges.insert(itEdge,turn_edgeinfacet_to_edge(oppEdge0));
						ConflictBoundEdges.insert(itEdge,turn_edgeinfacet_to_edge(oppEdge1));
					}
				}
				recursive_find_isolate_conflict_surfacet(V,Facet(cur,zz),PInside,VertexOnBoundary,VertexOnConflictFacet,Conflicts,ConflictBoundEdges,NewCreateSurfacets,zzz);		
			}
			else
			{
				if(PInside)
				{
					//create_surface_facet(SeedFacet.first,ii);
					Facet newFacet=mirror_facet(Facet(SeedFacet.first,ii));
					NewCreateSurfacets.push_back(newFacet);
				}
				else
				{
					Cell_Idtype cOpp=neighbor(SeedFacet.first,li);
					Vertex_Idtype vOppEdge=vertex(SeedFacet.first,ii);
					Indextype iiNewSur=vertex_index(cOpp,vOppEdge);
					//create_surface_facet(cOpp,iiNewSur);
					NewCreateSurfacets.push_back(Facet(cOpp,iiNewSur));
				}
			}
		}
	}

}

//�ɹ�����V���ڽ�����������ƬInitFacet������������ƬSeedFacet(������Ƭ����������cell,��һ��cell��,��������Ƭ��ԵĶ���Ϊ������)
//VΪ������,PInsideΪ��������ڱ����λ��
template<typename T, typename T_INFO>
bool DataStructure<T, T_INFO>::
seed_facet_of_isolate_vertex(Vertex_Idtype V,Facet InitFacet,bool PInside,Facet& SeedFacet)
{
	const T* p=point(V);
	SeedFacet=Facet(-1,-1);
	if(PInside)
	{
		if(vertex(InitFacet.first,InitFacet.second)==V)
		{
			SeedFacet=InitFacet;
			return true;
		}
		else  //ʹ��sculpture�������Բ���һ���ⲿ�����㣩
		{
			//������InitFacet�����surface facet������InitFacet�й�����ı���������Ƭ
			//��ô��Щ������Ƭ����ʲô���򣿾���/����ǣ���inflate/sculpture��˳����ʲô��
			//inflate/sculpture�����ַ�ʽ����flip������һ�������㣬����һ��������
			//InitFacet����İ�͹���ʣ�����ʱ�Ծ���ΪԼ����͹ʱ�Զ����ΪԼ���������ʱ������nearest_surface_facet���������еõ��İ�͹����
			//��ʱ���ϸ��������ù�����ȵ�˳�򣬸���nearest_surface_facet�����õ��İ�͹����
			std::list<Facet> neighborSurfacets;
			std::list<std::pair<Facet,double>> tmpSurfacetsDihedral;
			Vertex_triple vf=make_vertex_triple(InitFacet);
			std::list<Vertex_Idtype> nearestVertices;//(vf.first,vf.second,vf.third);
			nearestVertices.push_back(vf.first);
			nearestVertices.push_back(vf.second);
			nearestVertices.push_back(vf.third);
			double cosDihedral;								
			cosDihedral=cos_min_dihedral_point_to_facet(p,InitFacet);
	
			neighborSurfacets.push_back(InitFacet);
			tmpSurfacetsDihedral.push_back(make_pair(InitFacet,cosDihedral));
			mark_facet_visited(InitFacet);
			//-----------------��InitFacet����ı���������Ƭ������InitFacet�й�����ģ�------------------//
			while(!neighborSurfacets.empty())
			{
				Facet tmpSurfacet=*neighborSurfacets.begin();
				neighborSurfacets.pop_front();
		
				for (int ii = 0; ii<3; ++ii) {
					
					Facet surfacetNei=neighbor_surfacet(tmpSurfacet,ii);
					if(!visited_for_facet_extractor(surfacetNei)&&is_facet_in_neighbor(surfacetNei,nearestVertices))
					{
						Vertex_triple vf=make_vertex_triple(surfacetNei);						
						Orientation o = GeometricTraits<T>::orientation(point(vf.first), point(vf.second), point(vf.third), p);
						//p������,��������Ƭ�ڷ���һ��;p������,��������Ƭ�ⷨ��һ��
						if ((o!=NEGATIVE&&PInside)||(o!=POSITIVE&&!PInside))
						{
							mark_facet_visited(surfacetNei);
						
							double cosDihedral;								
		
							cosDihedral=cos_min_dihedral_point_to_facet(p,surfacetNei);
							bool isInserted=false;	
							for(auto iNF=tmpSurfacetsDihedral.begin();iNF!=tmpSurfacetsDihedral.end();iNF++)
							{
								if(cosDihedral>=(*iNF).second)
								{
									tmpSurfacetsDihedral.insert(iNF,make_pair(surfacetNei,cosDihedral));
									neighborSurfacets.push_back(surfacetNei);
									isInserted=true;
									break;
								}
							}
							if (!isInserted)
							{
								tmpSurfacetsDihedral.push_back(make_pair(surfacetNei,cosDihedral));
								neighborSurfacets.push_back(surfacetNei);
							}
				
						}

					}
	
				}
			

			}
			for(auto fit=tmpSurfacetsDihedral.begin();fit!=tmpSurfacetsDihedral.end();fit++)
			{
				clear_facet_visited((*fit).first);
			}
			//======================��InitFacet����ı���������Ƭ������InitFacet�й�����ģ�end===================//


			
			bool isSculpture=false;
			//----------------------��InitFacet������Ѱ������������Ƭ����sculpture-------------------------//
			for(auto fit=tmpSurfacetsDihedral.begin();fit!=tmpSurfacetsDihedral.end();fit++)
			{
				if (isSculpture)
				{
					fit=tmpSurfacetsDihedral.begin();
					isSculpture=false;
				}
				Facet tmpSurfacet=(*fit).first;
				if (!is_surface(tmpSurfacet))
				{
					continue;
				}
				Cell_Idtype c=(tmpSurfacet).first;
				int ii=(tmpSurfacet).second;
				vector<Facet> newSurfacets;
				if(is_surface(tmpSurfacet))
				{
					//-------------------------for test----------------------//
					Vertex_triple vft0=make_vertex_triple(tmpSurfacet);
					//=========================test end======================//
					
					if(vertex(c,ii)==V)
					{
						SeedFacet=tmpSurfacet;					
						return true;
					}
					else //sculpture
					{
						int fIndv[4];
						int numOfSurface=0;
						int numOfNSurface=0;
						for(int i=0;i<4;i++)
						{
							if(is_surface(Facet(c,i)))
							{
								
								fIndv[numOfSurface]=i;
								numOfSurface++;
							}
							else
							{
								numOfNSurface++;
								fIndv[4-numOfNSurface]=i;
							}
						}

						bool newEdgeShorter=true;
						if (numOfSurface==2)
						{
							double distanceNewEdge=NumericComputation<T>::SquareDistance(point(vertex(c,fIndv[0])),point(vertex(c,fIndv[1])));
							double distanceOldEdge=NumericComputation<T>::SquareDistance(point(vertex(c,fIndv[2])),point(vertex(c,fIndv[3])));
							if (distanceNewEdge>distanceOldEdge)
							{
								newEdgeShorter=false;
							}
						}

						Facet tmpNearFacet(-1,-1);
						bool isoPInside;
						//��numOfSurface=2ʱҪ�ж�inflate/sculpture�Ƿ����������
						bool isSingularityEdge=false;
						if(numOfNSurface==2)
						{
							isSingularityEdge=is_edge_in_surface(c,fIndv[2],fIndv[3]);

						}

						if((numOfSurface==3)||(numOfSurface==2&&!isSingularityEdge&&newEdgeShorter))//�����Ż�,�˴���flip��������һ����V����Ĺ�����
						{
							
							sculpture(c,numOfSurface,fIndv,newSurfacets);
							for(auto itNS=newSurfacets.begin();itNS!=newSurfacets.end();itNS++)
							{
								Vertex_triple vf=make_vertex_triple(*itNS);						
								Orientation o = GeometricTraits<T>::orientation(point(vf.first), point(vf.second), point(vf.third), p);
								if ((o!=NEGATIVE&&PInside)||(o!=POSITIVE&&!PInside))
								{
									double cosDihedral;								
		
									cosDihedral=cos_min_dihedral_point_to_facet(p,*itNS);
									bool isInserted=false;
									for(auto iNF=tmpSurfacetsDihedral.begin();iNF!=tmpSurfacetsDihedral.end();iNF++)
									{
										if(cosDihedral>=(*iNF).second)
										{
											tmpSurfacetsDihedral.insert(iNF,make_pair(*itNS,cosDihedral));
											
											isInserted=true;
											break;
										}
									}
									if (!isInserted)
									{
										tmpSurfacetsDihedral.push_back(make_pair(*itNS,cosDihedral));
										neighborSurfacets.push_back(*itNS);
									}
								}
							}
						
							fit=tmpSurfacetsDihedral.begin();
							isSculpture=true;

						}
					}
				}
			}
			return false;
			//========================��InitFacet������Ѱ������������Ƭ����sculpture===========================//
		}
		
	}
	else
	{
		Facet fOpp=mirror_facet(InitFacet);
		Cell_Idtype cOpp=fOpp.first;
		int iiOpp=fOpp.second;
		if(vertex(cOpp,iiOpp)==V)
		{
			SeedFacet=InitFacet;	
			return true;
		}
		else  //ʹ��inflate�����ܻ����һ���ڲ������㣩
		{
			//������InitFacet�����surface facet������InitFacet�й�����ı���������Ƭ
			//��ô��Щ������Ƭ����ʲô���򣿾���/����ǣ���inflate/sculpture��˳����ʲô��
			//InitFacet����İ�͹���ʣ�����ʱ�Ծ���ΪԼ����͹ʱ�Զ����ΪԼ���������ʱ������nearest_surface_facet���������еõ��İ�͹����
			//��ʱ���ϸ��������ù�����ȵ�˳�򣬸���nearest_surface_facet�����õ��İ�͹����

			std::list<Facet> neighborSurfacets;
			std::list<std::pair<Facet,double>> tmpSurfacetsDihedral;
			Vertex_triple vf=make_vertex_triple(InitFacet);
			std::list<Vertex_Idtype> nearestVertices;//(vf.first,vf.second,vf.third);
			nearestVertices.push_back(vf.first);
			nearestVertices.push_back(vf.second);
			nearestVertices.push_back(vf.third);
			double cosDihedral;								
			cosDihedral=cos_min_dihedral_point_to_facet(p,InitFacet);
	
			neighborSurfacets.push_back(InitFacet);
			tmpSurfacetsDihedral.push_back(make_pair(InitFacet,cosDihedral));
			mark_facet_visited(InitFacet);

			while(!neighborSurfacets.empty())
			{
				Facet tmpSurfacet=*neighborSurfacets.begin();
				neighborSurfacets.pop_front();
			
				for (int ii = 0; ii<3; ++ii) {
					
					Facet surfacetNei=neighbor_surfacet(tmpSurfacet,ii);

					if(!visited_for_facet_extractor(surfacetNei)&&is_facet_in_neighbor(surfacetNei,nearestVertices))
					{
						Vertex_triple vf=make_vertex_triple(surfacetNei);						
						Orientation o = GeometricTraits<T>::orientation(point(vf.first), point(vf.second), point(vf.third), p);
						//p������,��������Ƭ�ڷ���һ��;p������,��������Ƭ�ⷨ��һ��
						if ((o!=NEGATIVE&&PInside)||(o!=POSITIVE&&!PInside))
						{
							mark_facet_visited(surfacetNei);
	
							double cosDihedral;								
						
							cosDihedral=cos_min_dihedral_point_to_facet(p,mirror_facet(surfacetNei));
							bool isInserted=false;	
							for(auto iNF=tmpSurfacetsDihedral.begin();iNF!=tmpSurfacetsDihedral.end();iNF++)
							{
								if(cosDihedral>=(*iNF).second)
								{
									tmpSurfacetsDihedral.insert(iNF,make_pair(surfacetNei,cosDihedral));
									neighborSurfacets.push_back(surfacetNei);
									isInserted=true;
									break;
								}
							}
							if (!isInserted)
							{
								tmpSurfacetsDihedral.push_back(make_pair(surfacetNei,cosDihedral));
								neighborSurfacets.push_back(surfacetNei);
							}	
					
					
						}

					}
	
				}
			

			}
			for(auto fit=tmpSurfacetsDihedral.begin();fit!=tmpSurfacetsDihedral.end();fit++)
			{
				clear_facet_visited((*fit).first);
			}

			bool isInflate=false;
			for(auto fit=tmpSurfacetsDihedral.begin();fit!=tmpSurfacetsDihedral.end();fit++)
			{
				if (isInflate)
				{
					fit=tmpSurfacetsDihedral.begin();
					isInflate=false;
				}
				Facet tmpSurfacet=(*fit).first;
				if (!is_surface(tmpSurfacet))
				{
					continue;
				}
				vector<Facet> newSurfacets;
				Vertex_triple vtriF=make_vertex_triple(tmpSurfacet);
				if(is_surface(tmpSurfacet))
				{
					Cell_Idtype c=(tmpSurfacet).first;
					int ii=(tmpSurfacet).second;
					Cell_Idtype cOpp=neighbor(c,ii);
					int iiOpp=neighbor_index(cOpp,c);
					if(vertex(cOpp,iiOpp)==V)
					{
						SeedFacet=tmpSurfacet;
						
						return true;
					}
					else //inflate
					{
						int fIndv[4];
						int numOfSurface=0;
						int numOfNSurface=0;
						for(int i=0;i<4;i++)
						{
							if(is_surface(mirror_facet(Facet(cOpp,i))))
							{
								
								fIndv[numOfSurface]=i;
								numOfSurface++;
							}
							else
							{
								numOfNSurface++;
								fIndv[4-numOfNSurface]=i;
							}
						}

						Facet tmpNearFacet(-1,-1);
						bool isoPInside;
						//��numOfSurface=2ʱҪ�ж�inflate/sculpture�Ƿ����������
						bool isSingularityEdge=false;
						if(numOfNSurface==2)
						{
							isSingularityEdge=is_edge_in_surface(cOpp,fIndv[2],fIndv[3]);

						}
						//-------------------inflate֮�󲻻���infinite���ڱ�����---------------------//
					    if((numOfSurface==3)||(numOfSurface==2&&!isSingularityEdge))
						{
							
							inflate(cOpp,numOfSurface,fIndv,newSurfacets);
							
							for(auto itNS=newSurfacets.begin();itNS!=newSurfacets.end();itNS++)
							{
								Vertex_triple vf=make_vertex_triple(*itNS);						
								Orientation o = GeometricTraits<T>::orientation(point(vf.first), point(vf.second), point(vf.third), p);
								if ((o!=NEGATIVE&&PInside)||(o!=POSITIVE&&!PInside))
								{
									double cosDihedral;								
		
									cosDihedral=cos_min_dihedral_point_to_facet(p,*itNS);
									bool isInserted=false;
									for(auto iNF=tmpSurfacetsDihedral.begin();iNF!=tmpSurfacetsDihedral.end();iNF++)
									{
										if(cosDihedral>=(*iNF).second)
										{
											tmpSurfacetsDihedral.insert(iNF,make_pair(*itNS,cosDihedral));
											
											isInserted=true;
											break;
										}
									}
									if (!isInserted)
									{
										tmpSurfacetsDihedral.push_back(make_pair(*itNS,cosDihedral));
										neighborSurfacets.push_back(*itNS);
									}
								}
							}
							fit=tmpSurfacetsDihedral.begin();
							isInflate=true;
						
						}
					}
				}
			}

			return false;
		}
	}
}


//V�����붥��,ConflictBoundΪ���滻��߽��ߣ���Edge(��id,0/1/2)��ʾ�����治���滻�棩
//NewBoundFacetΪ���γɵķ��滻��(����umbrella����)�ı���������Ƭ;NewBoundarySurfacets(cell,0/1/2/3)��ʾ�����γɱ��湲�滻��(��umbrella����)�߽��ߵı���;
template<typename T, typename T_INFO>
void DataStructure<T, T_INFO>::
update_surface_connection(Vertex_Idtype V,list<Edge> ConflictBound,vector<Facet> NewBoundFacet,
	vector<Facet>& NewBonudarySurfacets)
{
	//----------------testing--------------------
	/*cout<<"starting update surface connection:"<<endl;*/
	//---------------testing end

	//���½�������ڽ���ϵ
	if(!ConflictBound.empty())
	{
		//Edge boundEdge=*(ConflictBound.begin());
		//ConflictBound.push_back(boundEdge);
		EdgeInFacet firstBoundEdge=make_pair(Facet(-1,-1),-1);
		EdgeInFacet former=make_pair(Facet(-1,-1),-1);
		for(auto itE=ConflictBound.begin();itE!=ConflictBound.end();itE++)
		{
			//�����itE��ż�����ڵ�surface facet�����������γɵ�surface facet
			//��itEָ����������תcell �� facet
			Facet_Idtype idF=(*itE).first;
			
		    //����id����(cell,id)
			Facet boundF=get_facet_cell_link(idF);
			NewBonudarySurfacets.push_back(boundF);
			//����boundF��(*itE).second�Ŷ���ʵ�ʱ��,�������б߽����ԵĶ���
			Vertex_Idtype vii=vertex_facet(boundF,(*itE).second);
			//��vii��������boundF.first�ĵڼ��Ŷ���
			Indextype ii=vertex_index(boundF.first,vii);
			//-----------------for test-------------//
			Vertex_triple vtriT0=make_vertex_triple(boundF);
			//=================test end============//
			Indextype li=boundF.second;
			Cell_Idtype c=boundF.first;
			//vj1,vj2Ϊ�߽��
			Vertex_Idtype vj1 = vertex(c, Triangulation_utils_3::next_around_edge(ii, li));
			Vertex_Idtype vj2 = vertex(c, Triangulation_utils_3::next_around_edge(li, ii));
			Cell_Idtype cur = c;
			int zz = ii;
			Cell_Idtype n = neighbor(cur,zz);

			while (1)
			{
				if (!is_label_inside(n))
				{
					Indextype indvT=Triangulation_utils_3::next_around_edge(vertex_index(n, vj1), vertex_index(n, vj2));
					Vertex_Idtype vT=vertex(n,indvT);
					bool isTure=vT==V;
					if (isTure&&is_surface(Facet(cur,zz)))
					{
						break;
					}
				}
				cur = n;
				zz = Triangulation_utils_3::next_around_edge(vertex_index(n, vj1), vertex_index(n, vj2));
				n = neighbor(cur,zz);

			}
			
			//���²����V��Facet(cur,zz)�ĵڼ�����
			Indextype iiNei=vertex_index_facet(Facet(cur,zz),V);

			EdgeInFacet curE=make_pair(Facet(cur,zz),iiNei);
			set_facet_adjacency(Facet(cur,zz),iiNei,boundF,(*itE).second);

			//----------------for test--------------//
			//Vertex_triple vtriT1=make_vertex_triple(Facet(cur,zz));
			//Vertex_triple vtriT1b=make_vertex_triple(boundF);
			//cout<<"connect conflict facet: ("<<cur<<","<<zz<<")/("<<vtriT1.first<<","<<vtriT1.second<<","<<vtriT1.third<<")-"<<iiNei
			//	<<"-("<<boundF.first<<","<<boundF.second<<")/("<<vtriT1b.first<<","<<vtriT1b.second<<","<<vtriT1b.third
			//	<<")-"<<(*itE).second<<endl;
			//================test end==============//
	

	

			if(former.second!=-1)
			{
				Vertex_pair vEdge0=make_vertex_pair(former);
				Indextype i0=vertex_index_facet(former.first,vEdge0.second);
				Vertex_pair vEdge1=make_vertex_pair(curE);
				Indextype i1=vertex_index_facet(Facet(cur,zz),vEdge1.first);
				set_facet_adjacency(former.first,i0,Facet(cur,zz),i1);
				//----------------------for test-----------------//
				Vertex_triple vtriT2=make_vertex_triple(former.first);
				Vertex_triple vtriT2b=make_vertex_triple(Facet(cur,zz));
				//cout<<"connect conflict facet: ("<<(former.first).first<<","<<(former.first).second<<")/("<<vtriT2.first<<","<<vtriT2.second<<","<<vtriT2.third
				//	<<")-"<<i0<<"-("<<cur<<","<<zz<<")/("<<vtriT2b.first<<","<<vtriT2b.second<<","<<vtriT2b.third
				//	<<")-"<<i1<<endl;
				//======================test end=================//
			}
			else
			{
				firstBoundEdge=curE;
			}
			former=curE;

		}
		//�������һ��������������һ������Ĺ�ϵ
		Vertex_pair vEdge0=make_vertex_pair(former);
		Indextype i0=vertex_index_facet(former.first,vEdge0.second);
		Vertex_pair vEdge1=make_vertex_pair(firstBoundEdge);
		Indextype i1=vertex_index_facet(firstBoundEdge.first,vEdge1.first);

		set_facet_adjacency(former.first,i0,firstBoundEdge.first,i1);

		//----------------------for test-----------------//
		//Vertex_triple vtriT3=make_vertex_triple(former.first);
        //Vertex_triple vtriT4=make_vertex_triple(firstBoundEdge.first);
		//cout<<"connect conflict facet: ("<<(former.first).first<<","<<(former.first).second<<")/("<<vtriT3.first<<","<<vtriT3.second<<","<<vtriT3.third
		//	<<")-"<<i0<<"-("<<(firstBoundEdge.first).first<<","<<(firstBoundEdge.first).second<<")/"<<vtriT4.first<<","<<vtriT4.second<<","<<vtriT4.third
		//	<<")-"<<i1<<endl;
		//======================test end=================//

	}
	

	//�������γɵ�boundary surface facet���ڽ���ϵ
	for(auto itB=NewBoundFacet.begin();itB!=NewBoundFacet.end();itB++)
	{
		//---------for test---------//
		Vertex_triple vtriF=make_vertex_triple(*itB);
		//=========test end=========//
		//NewConflictSurfacets.push_back(*itB);
		for(int ii=0;ii<4;ii++)
		{
			
			Cell_Idtype c=(*itB).first;
			Indextype li=(*itB).second;

			if(ii==li)
				continue;
			Vertex_Idtype vTemp=vertex(c,ii);
			Indextype i1=vertex_index_facet((*itB),vTemp);
			Facet_Idtype idFB=get_facet_index(*itB);
			/*if(neighbor_surfacet(idFB,i1)!=-1)
				continue;*/
			Vertex_Idtype vj1 = vertex(c, Triangulation_utils_3::next_around_edge(ii, li));
			Vertex_Idtype vj2 = vertex(c, Triangulation_utils_3::next_around_edge(li, ii));
			Cell_Idtype cur = c;
			int zz = ii;
			Cell_Idtype n = neighbor(cur,zz);
			// turn around the oriented edge vj1 vj2
			while (is_label_inside(n)) {
		
				cur = n;
				zz = Triangulation_utils_3::next_around_edge(vertex_index(n, vj1), vertex_index(n, vj2));
				n = neighbor(cur,zz);
			}
			
			Indextype indvOpp=Triangulation_utils_3::next_around_edge(vertex_index(n, vj1), vertex_index(n, vj2));
			Vertex_Idtype vOpp=vertex(n,indvOpp);
			Indextype i0=vertex_index_facet(Facet(cur,zz),vOpp);
			set_facet_adjacency(Facet(cur,zz),i0,(*itB),i1);

			//----------------------for test---------------------//
			Vertex_triple vtriFN=make_vertex_triple(Facet(cur,zz));
			Vertex_triple vtriFNB=make_vertex_triple((*itB));
			//cout<<"connect boundary facet: ("<<cur<<","<<zz<<")/("<<vtriFN.first<<","<<vtriFN.second<<","<<vtriFN.third<<")-"<<i0
			//	<<"("<<(*itB).first<<","<<(*itB).second<<")/"<<vtriFNB.first<<","<<vtriFNB.second<<","<<vtriFNB.third
			//	<<")-"<<i1<<endl;
			//======================test end=====================//
		}
	}


}

template<typename T, typename T_INFO>
void DataStructure<T, T_INFO>::
neighbor_surfacet_around_outside(Facet F0,Indextype I0,Facet& F1,Indextype& I1)
{
	Cell_Idtype c=F0.first;
	Indextype li=F0.second;
	
	
	Vertex_Idtype vTemp=vertex_facet(F0,I0);
	Indextype ii=vertex_index(c,vTemp);

	Cell_Idtype cOpp=neighbor(c,li);
	Vertex_Idtype liOpp=neighbor_index(cOpp,c);
	Vertex_Idtype iiOpp=vertex_index(cOpp,vTemp);
	Vertex_Idtype vj1Opp = vertex(cOpp, Triangulation_utils_3::next_around_edge(iiOpp, liOpp));
	Vertex_Idtype vj2Opp = vertex(cOpp, Triangulation_utils_3::next_around_edge(liOpp, iiOpp));
	Cell_Idtype cur = cOpp;
	int zz = iiOpp;
	Cell_Idtype n = neighbor(cur,zz);
	// turn around the oriented edge vj1 vj2
	while (!is_label_inside(n)) {
		
		cur = n;
		zz = Triangulation_utils_3::next_around_edge(vertex_index(n, vj1Opp), vertex_index(n, vj2Opp));
		n = neighbor(cur,zz);
	}
	Indextype indvN=Triangulation_utils_3::next_around_edge(vertex_index(n, vj1Opp), vertex_index(n, vj2Opp));
	Indextype indvF=Triangulation_utils_3::next_around_edge(vertex_index(n, vj2Opp), vertex_index(n, vj1Opp));
	Vertex_Idtype vN=vertex(n,indvN);
	Indextype i0=vertex_index_facet(Facet(n,indvF),vN);

	//set_facet_adjacency(Facet(n,indvF),i0,(*itB),i1);
	F1=Facet(n,indvF);
	I1=i0;
		
	
}

//���,�ѱ����ڵ�C���Ϊ������,NumOfSurfaceΪC�б������
//FIndvΪC��ǰ��NumOfSurface���洢��Ϊ������������,����FIndv��Ԫ�ظ���һ��Ϊ4
//NewCreateSurfacets��̺������ɱ���
template<typename T, typename T_INFO>
void DataStructure<T, T_INFO>::
sculpture(Cell_Idtype C,int NumOfSurface,int* FIndv,vector<Facet>& NewCreateSurfacets)
{
	//�洢ԭ��������ڹ�ϵ,
	//�洢��ʽ:ɾ����0�ŵ���ڽ���id,ɾ����������ڽ��������;ɾ����1�ŵ���ڽ���id,ɾ����������ڽ��������;ɾ����2�ŵ���ڽ���id,ɾ����������ڽ��������...
	//һ������,��������
	Facet_Idtype SurfacetNeighbor[18];

	//--------------------for test----------------//
	/*cout<<"sculpture cell "<<C<<":"<<is_label_inside(C)<<"->"<<false<<endl;*/
	//====================test end===============//

	label_cell_side(C,false);
	for(int i=0;i<4;i++)
	{
		if(i<NumOfSurface)
		{
			
			//����ԭ��������ڹ�ϵ
			//�洢��ʽ:��ɾ����0�ŵ���ڽ���id,��ɾ����������ڽ��������;
			//         ��ɾ����1�ŵ���ڽ���id,��ɾ����������ڽ��������;
			//         ��ɾ����2�ŵ���ڽ���id,��ɾ����������ڽ��������...
			//һ������,��������
			Facet_Idtype idF=get_facet_index(Facet(C,FIndv[i]));
			SurfacetNeighbor[i*6]=neighbor_surfacet(idF,0);
			SurfacetNeighbor[i*6+1]=neighbor_index_facet(SurfacetNeighbor[i*6],idF);
			SurfacetNeighbor[i*6+2]=neighbor_surfacet(idF,1);
			SurfacetNeighbor[i*6+3]=neighbor_index_facet(SurfacetNeighbor[i*6+2],idF);
			SurfacetNeighbor[i*6+4]=neighbor_surfacet(idF,2);
			SurfacetNeighbor[i*6+5]=neighbor_index_facet(SurfacetNeighbor[i*6+4],idF);

			//-------------------for test-------------//
			//Vertex_triple vft1=make_vertex_triple(Facet(C,FIndv[i]));	
			//Facet_Idtype fid=CellSurface.get_tuple_element(C,FIndv[i]);
			//cout<<"sculpture delete facet:"<<fid<<"("<<vft1.first<<","<<vft1.second<<","<<vft1.third<<")"
			//	<<"/("<<C<<","<<FIndv[i]<<")["<<C<<"("
			//	<<vertex(C, 0)<<","<<vertex(C, 1)<<","<<vertex(C, 2)<<","<<vertex(C, 3)<<")]"<<endl;
			//==================test end==============//

			//ɾ��C��ԭNumOfSurface������
			delete_surface_facet(Facet(C,FIndv[i]));

			if(NumOfSurface==1)
			{
				remove_from_isolate_vertices(vertex(C,FIndv[i]));
			}

		}
		else
		{
			Facet tmpF=mirror_facet(Facet(C,FIndv[i]));
			//��C��ԭ�����Ǳ����mirror_facet�����
			create_surface_facet(tmpF);
			NewCreateSurfacets.push_back(tmpF);
			if(NumOfSurface==3)
			{
				insert_to_isolate_vertices(vertex(C,FIndv[i]));
			}
			//-------------------for test-------------//
			//Vertex_triple vft1=make_vertex_triple(tmpF);
			//Cell_Idtype tmpc=tmpF.first;
			//Facet_Idtype fid=CellSurface.get_tuple_element(tmpc,tmpF.second);
			//cout<<"sculpture create facet:"<<fid<<"("<<vft1.first<<","<<vft1.second<<","<<vft1.third<<")"
			//	<<"/("<<tmpF.first<<","<<tmpF.second<<")["<<tmpc<<"("
			//	<<vertex(tmpc, 0)<<","<<vertex(tmpc, 1)<<","<<vertex(tmpc, 2)<<","<<vertex(tmpc, 3)<<")]"<<endl;
			//==================test end==============//
									
		}
	}

	//C��ɾһ������,��������mirror face����
	if(NumOfSurface==1)
	{
		//���θ��������³����뱻ɾ������ڽ�����ڽӹ�ϵ
		for(int i=0;i<3;i++)
		{
			Indextype iindv=FIndv[1+i];
			//fCur�����iindv�����ɱ���
			Facet fCur=mirror_facet(Facet(C,iindv));
			Facet_Idtype idFCur=get_facet_index(fCur);
			Facet_Idtype i0=vertex_index_facet(fCur,vertex(C,FIndv[0]));		
			Indextype iTemp=Triangulation_utils_3::cell_index_in_triple_index(iindv,FIndv[0]);
			Facet_Idtype idFNei=SurfacetNeighbor[2*iTemp];
			Indextype i1=SurfacetNeighbor[2*iTemp+1];

			//----------------------------for test----------------------//
			Vertex_Idtype vT0=vertex(C,iindv);
			Vertex_Idtype vT1=vertex(C,FIndv[0]);
			Vertex_triple vTriT0=make_vertex_triple(fCur);
			Vertex_triple vTriT1=make_vertex_triple(get_facet_cell_link(idFNei));
			//===========================test end======================//
			
			set_facet_adjacency(idFCur,i0,idFNei,i1);			
		}
		//���θ���������������˴��ڽӹ�ϵ
		for(int i=0;i<3;i++)
		{
			for(int j=i+1;j<3;j++)
			{
				Indextype iindv0=FIndv[i+1];
				Indextype iindv1=FIndv[j+1];
				Facet f1=mirror_facet(Facet(C,iindv0));
				Facet f2=mirror_facet(Facet(C,iindv1));
				Indextype i0=vertex_index_facet(f1,vertex(C,iindv1));
				Indextype i1=vertex_index_facet(f2,vertex(C,iindv0));
				set_facet_adjacency(f1,i0,f2,i1);
			}
		}
	}

	//C��ɾ��������,��������mirror face����
	if(NumOfSurface==2)
	{
		for(int i=0;i<2;i++)
		{
			//fCur�������ɱ���
			Facet fCur=mirror_facet(Facet(C,FIndv[2+i]));
			Facet_Idtype idFCur=get_facet_index(fCur);
			for(int j=0;j<2;j++)
			{
				//fDel�Ǳ�ɾ������
				Facet fDel=make_pair(C,FIndv[j]);
				Indextype i0=vertex_index_facet(fCur,vertex(C,FIndv[j]));
				Indextype iTemp=Triangulation_utils_3::cell_index_in_triple_index(FIndv[2+i], FIndv[j]);
				Facet_Idtype idFNei=SurfacetNeighbor[6*j+2*iTemp];
				Indextype i1=SurfacetNeighbor[6*j+2*iTemp+1];
				set_facet_adjacency(idFCur,i0,idFNei,i1);

			}
		}
		//�������������ɱ����ڽӹ�ϵ
		Facet fNew0=mirror_facet(Facet(C,FIndv[2]));
		Facet fNew1=mirror_facet(Facet(C,FIndv[3]));
		Indextype i0=vertex_index_facet(fNew0,vertex(C,FIndv[3]));
		Indextype i1=vertex_index_facet(fNew1,vertex(C,FIndv[2]));
		set_facet_adjacency(fNew0,i0,fNew1,i1);
	}

	//C��ɾ��������,����һ��mirror face����
	if(NumOfSurface==3)
	{
		Facet fNew=mirror_facet(Facet(C,FIndv[3]));
		Facet_Idtype idFNew=get_facet_index(fNew);
		//���θ��¸��³����뱻ɾ������ڽ�����ڽӹ�ϵ
		for(int i=0;i<3;i++)
		{
			Indextype i0=vertex_index_facet(fNew,vertex(C,FIndv[i]));
			int iTemp=Triangulation_utils_3::cell_index_in_triple_index(FIndv[3], FIndv[i]);
			Facet_Idtype idFNei=SurfacetNeighbor[6*i+2*iTemp];
			Indextype i1=SurfacetNeighbor[6*i+2*iTemp+1];
			set_facet_adjacency(idFNew,i0,idFNei,i1);
		}
	}

}


//����,�ѱ������C���Ϊ������,NumOfSurfaceΪC��mirror face�Ǳ���ĸ���
//FIndvΪC��ǰ��NumOfSurface���洢��Ϊ������������,����FIndv��Ԫ�ظ���һ��Ϊ4
//NewCreateSurfacets���ͺ������ɱ���
template<typename T, typename T_INFO>
void DataStructure<T, T_INFO>::
inflate(Cell_Idtype C,int NumOfSurface,int* FIndv,vector<Facet>& NewCreateSurfacets)
{
	//�洢ԭ��������ڹ�ϵ,
	//�洢��ʽ:ɾ����(����C�е�mirror face 0�ŵ�)����ڽ���id,ɾ����������ڽ��������;
	//         ɾ����(����C�е�mirror face 1�ŵ�)����ڽ���id,ɾ����������ڽ��������;
	//         ɾ����(����C�е�mirror face 2�ŵ�)����ڽ���id,ɾ����������ڽ��������...
	//һ������,��NumOfSurface��
	Facet_Idtype* SurfacetNeighbor=new Facet_Idtype[NumOfSurface*6];
	//-------------for test----------//
	//Facet_Idtype SurfacetNeighbor[18];
	/*cout<<"inflate cell "<<C<<":"<<is_label_inside(C)<<"->"<<true<<endl;*/
	//============test end==========//
	label_cell_side(C,true);
	for(int i=0;i<4;i++)
	{
		if(i<NumOfSurface)
		{
			Facet tmpF=mirror_facet(Facet(C,FIndv[i]));
			
			//����ԭ��������ڹ�ϵ
			//�洢��ʽ:ɾ����(����C�е�mirror face 0�ŵ�)����ڽ���id,ɾ����������ڽ��������;
			//         ɾ����(����C�е�mirror face 1�ŵ�)����ڽ���id,ɾ����������ڽ��������;
			//         ɾ����(����C�е�mirror face 2�ŵ�)����ڽ���id,ɾ����������ڽ��������...
			//һ������,��NumOfSurface��			
			for(int j=0;j<3;j++)
			{
				Facet_Idtype idF=get_facet_index(tmpF);
				Indextype iTri=vertex_index_facet(tmpF,vertex_facet(Facet(C,FIndv[i]),j));
				SurfacetNeighbor[i*6+2*j]=neighbor_surfacet(idF,iTri);
				SurfacetNeighbor[i*6+2*j+1]=neighbor_index_facet(SurfacetNeighbor[i*6+2*j],idF);
			}

			//-------------------for test-------------//
			//Vertex_triple vft1=make_vertex_triple(tmpF);
			//Cell_Idtype tmpc=tmpF.first;
			//Facet_Idtype fid=CellSurface.get_tuple_element(tmpc,tmpF.second);
			//cout<<"inflate delete facet:"<<fid<<"("<<vft1.first<<","<<vft1.second<<","<<vft1.third<<")"
			//	<<"/("<<tmpF.first<<","<<tmpF.second<<")["<<tmpc<<"("
			//	<<vertex(tmpc, 0)<<","<<vertex(tmpc, 1)<<","<<vertex(tmpc, 2)<<","<<vertex(tmpc, 3)<<")]"<<endl;
			//==================test end==============//

			delete_surface_facet(tmpF);
			if(NumOfSurface==1)
			{
				remove_from_isolate_vertices(vertex(C,FIndv[i]));
			}

		}
		else
		{
			//��C��ԭ�����Ǳ������Ƭ�����
			create_surface_facet(Facet(C,FIndv[i]));
			NewCreateSurfacets.push_back(Facet(C,FIndv[i]));
			if(NumOfSurface==3)
			{
				insert_to_isolate_vertices(vertex(C,FIndv[i]));
			}
			//-------------------for test-------------//
			Vertex_triple vft1=make_vertex_triple(Facet(C,FIndv[i]));	
			Facet_Idtype fid=CellSurface.get_tuple_element(C,FIndv[i]);
			//cout<<"inflate create facet:"<<fid<<"("<<vft1.first<<","<<vft1.second<<","<<vft1.third<<")"
			//	<<"/("<<C<<","<<FIndv[i]<<")["<<C<<"("
			//	<<vertex(C, 0)<<","<<vertex(C, 1)<<","<<vertex(C, 2)<<","<<vertex(C, 3)<<")]"<<endl;
			//==================test end==============//
		}
	}

	//C��ɾһ��mirror face����,������������
	if(NumOfSurface==1)
	{
		
		for(int i=0;i<3;i++)
		{
			Indextype iindv=FIndv[1+i];
			Indextype i0= Triangulation_utils_3::cell_index_in_triple_index(FIndv[0], iindv);
				
			Indextype iTemp=Triangulation_utils_3::cell_index_in_triple_index(iindv,FIndv[0]);
			Facet_Idtype idFNei=SurfacetNeighbor[2*iTemp];
			Indextype i1=SurfacetNeighbor[2*iTemp+1];
			Facet_Idtype idFCur=get_facet_index(Facet(C,iindv));
			set_facet_adjacency(idFCur,i0,idFNei,i1);
				
		}
		for(int i=0;i<3;i++)
			for(int j=i+1;j<3;j++)
			{
				Indextype iindv0=FIndv[i+1];
				Indextype iindv1=FIndv[j+1];
				Indextype i0=Triangulation_utils_3::cell_index_in_triple_index(iindv1, iindv0);
				Indextype i1=Triangulation_utils_3::cell_index_in_triple_index(iindv0, iindv1);
				set_facet_adjacency(Facet(C,iindv0),i0,Facet(C,iindv1),i1);
			}
	}

	//C��ɾ����mirror face����,������������
	if(NumOfSurface==2)
	{
		for(int i=0;i<2;i++)
		{
			//fCur�������ɱ���
			Facet fCur=Facet(C,FIndv[2+i]);
			Facet_Idtype idFCur=get_facet_index(fCur);
			for(int j=0;j<2;j++)
			{
				//fDel�Ǳ�ɾ������
				Facet fDel=make_pair(C,FIndv[j]);
				Indextype i0=Triangulation_utils_3::cell_index_in_triple_index(FIndv[j], FIndv[2+i]);
				Indextype iTemp=Triangulation_utils_3::cell_index_in_triple_index(FIndv[2+i], FIndv[j]);
				Facet_Idtype idFNei=SurfacetNeighbor[6*j+2*iTemp];
				Indextype i1=SurfacetNeighbor[6*j+2*iTemp+1];
				set_facet_adjacency(idFCur,i0,idFNei,i1);

			}
		}
		//�������������ɱ����ڽӹ�ϵ
		Facet fNew0=(Facet(C,FIndv[2]));
		Facet fNew1=(Facet(C,FIndv[3]));
		Indextype i0=Triangulation_utils_3::cell_index_in_triple_index(FIndv[3], FIndv[2]);
		Indextype i1=Triangulation_utils_3::cell_index_in_triple_index(FIndv[2], FIndv[3]);
		set_facet_adjacency(fNew0,i0,fNew1,i1);
	}

	//C��ɾ����mirror face����,����һ������
	if(NumOfSurface==3)
	{
		Facet fNew=(Facet(C,FIndv[3]));
		Facet_Idtype idFNew=get_facet_index(fNew);
		//���θ��¸��³����뱻ɾ������ڽ�����ڽӹ�ϵ
		for(int i=0;i<3;i++)
		{
			Indextype i0=Triangulation_utils_3::cell_index_in_triple_index(FIndv[i], FIndv[3]);
			int iTemp=Triangulation_utils_3::cell_index_in_triple_index(FIndv[3], FIndv[i]);
			Facet_Idtype idFNei=SurfacetNeighbor[6*i+2*iTemp];
			Indextype i1=SurfacetNeighbor[6*i+2*iTemp+1];
			set_facet_adjacency(idFNew,i0,idFNei,i1);
		}
	}

	delete []SurfacetNeighbor;


}

//C��,I0-I1��Ա��Ƿ�Ϊ����ı�
template<typename T, typename T_INFO>
bool DataStructure<T, T_INFO>::
is_edge_in_surface(Cell_Idtype C,Indextype I0,Indextype I1)
{
			
	bool isCellInside=is_label_inside(C);
	Vertex_Idtype vj1 = vertex(C, Triangulation_utils_3::next_around_edge(I0, I1));
	Vertex_Idtype vj2 = vertex(C, Triangulation_utils_3::next_around_edge(I1, I0));
	Cell_Idtype cur = C;
	int zz = I0;
	Cell_Idtype n = neighbor(cur,zz);
	
	// turn around the oriented edge vj1 vj2
	while (n!=C) {

		if(is_label_inside(n)!=isCellInside)
		{
			return true;
		}
		cur = n;
		zz = Triangulation_utils_3::next_around_edge(vertex_index(n, vj1), vertex_index(n, vj2));
		n = neighbor(cur,zz);		
	}
	return false;
}


//����������ʵ�ʲ����޳������ڲ�cell
//FΪ��ʼ��������������Ƭ;VertexIsoΪ���ܴ��ڵĹ�����,��ʼΪ-1;ItSurfΪ���γɵı���������Ƭ����;
//IdVΪĳһ����,��ʼΪ-1;FcurΪ���γ�����IdV�����ı���������Ƭ,��ʼ(-1,-1);MarkIsoΪ�Ƿ��������,��ʼtrue
template<typename T, typename T_INFO>
template<typename IteratorFacet>
bool DataStructure<T, T_INFO>:: 
iterative_sculpture_with_mark(Facet F,Vertex_Idtype VertexIso,IteratorFacet ItSurF,Vertex_Idtype IdV,Facet& FCur,bool MarkIso)
{
	//----------------for test-------------//
	bool isS=is_surface(F);
	if (!isS)
	{
		//cout<<"error!:iterative_sculpture_with_mark no surface"<<endl;
		return false;
	}
	//================test end============//	
	Cell_Idtype c=F.first;
	int ii=F.second;
	int fIndv[4];//fIndvΪcOppǰ��numOfSurface���洢����mirror_facetΪ������������������numOfNSurface�ǷǱ�����������
	int numOfSurface=0;
	int numOfNSurface=0;//numOfSurface+numOfNSurface=4����
	vector<Facet> newSurfacets;
	bool isInSphere=false;
	bool containFCur=false;

	for(int i=0;i<4;i++)
	{
		if(is_surface(Facet(c,i)))
		{
								
			fIndv[numOfSurface]=i;
			numOfSurface++;
			Vertex_triple vNSur=make_vertex_triple(Facet(c,i));

			//��������Զ��
			if (is_infinite(vNSur.first)||is_infinite(vNSur.second)||
				is_infinite(vNSur.third)||is_infinite(vertex(c,i)))
			{
				return false;
			}

			bool isInSphereT=GeometricTraits<T>::side_of_bounded_sphereC3(point(vNSur.first),point(vNSur.second),
										point(vNSur.third),point(vertex(c,i)))==ON_BOUNDED_SIDE;
			if (isInSphereT)
			{
				isInSphere=true;
			}
			if (Facet(c,i)==FCur)
			{
				containFCur=true;
			}
		}
		else
		{
			numOfNSurface++;
			fIndv[4-numOfNSurface]=i;
		}
	}

	//���滻�ı�С�ڱ��滻�ı�ʱflip sculpture
	bool newEdgeShorter=true;
	if (numOfSurface==2)
	{
		double distanceNewEdge=NumericComputation<T>::SquareDistance(point(vertex(c,fIndv[0])),point(vertex(c,fIndv[1])));
		double distanceOldEdge=NumericComputation<T>::SquareDistance(point(vertex(c,fIndv[2])),point(vertex(c,fIndv[3])));
		if (distanceNewEdge>distanceOldEdge)
		{
			newEdgeShorter=false;
		}
	}

	Facet tmpNearFacet(-1,-1);
	bool isoPInside;
	//��numOfSurface=2ʱҪ�ж�inflate/sculpture�Ƿ����������
	bool isSingularityEdge=false;
	if(numOfNSurface==2)
	{
		isSingularityEdge=is_edge_in_surface(c,fIndv[2],fIndv[3]);
	}

	if (containFCur&&IdV==-1)
	{
		return false;
	}
	if (MarkIso&&(numOfSurface==3))
	{
		if (IdV==-1)
		{
			return false;
		}
		else
		{
			FCur=F;
			return false;
		}
	}
	if ((vertex(c,ii)==VertexIso)&&(numOfSurface==1))
	{
		if (IdV==-1)
		{
			return false;
		}
		else
		{
			FCur=F;
			return false;
		}
	}

	if(isInSphere&&(numOfSurface==2&&!isSingularityEdge&&newEdgeShorter))//�����Ż�,�˴���flip��������һ����V����Ĺ�����
	{
		sculpture(c,numOfSurface,fIndv,newSurfacets);
		if (IdV!=-1)
		{
			int iFNext=-1;
			Facet* newSur=new Facet[numOfNSurface];
			int numOfIncidentSur=0;
			int numOfNIncidentSur=0;
			int iVF=-1;
			for(auto itNS=newSurfacets.begin();itNS!=newSurfacets.end();itNS++)
			{
				*ItSurF++=*itNS;
				int iVF_tmp=vertex_index_facet(*itNS,IdV);
				if (iVF_tmp>=0)
				{
					mark_facet_visited(*itNS);
					newSur[numOfIncidentSur]=*itNS;
					if(numOfIncidentSur==0)
					{
						iVF=iVF_tmp;
					}
					numOfIncidentSur++;
				}
				else
				{
					numOfNIncidentSur++;
					newSur[numOfNSurface-numOfNIncidentSur]=*itNS;
				}
			}
			
			//����ʱ�뷽�����ĳһ������ı���������Ƭ
			if (numOfIncidentSur==1)
			{
				iFNext=0;
			}
			else if (numOfIncidentSur==2)
			{
				std::pair<int,int> idVEdge=make_edge_index(iVF);
				Facet FNext_tmp=neighbor_surfacet(newSur[0],idVEdge.first);
				iFNext=0;
				if (FNext_tmp==newSur[1])
				{
					iFNext=1;
				}
			}
			if (numOfSurface!=3)
			{
				
				for (int i = 0; i < numOfNSurface; i++)
				{
					Facet FCur_tmp=newSur[iFNext];
					if (i==iFNext)
					{
						continue;
					}
					else
					{
						if (is_surface(newSur[i]))
						{
							iterative_sculpture_with_mark(newSur[i],VertexIso,ItSurF,-1,FCur_tmp,MarkIso);
						}
						
					}
				}
				iterative_sculpture_with_mark(newSur[iFNext],VertexIso,ItSurF,IdV,FCur,MarkIso);
			}
			else
			{
				if (numOfIncidentSur=0)
				{
					FCur=Facet(-1,-1);
				}
				else
				{
					FCur=newSur[iFNext];
				}
			}
			delete [] newSur;
		}//end of if (IdV!=-1)
		else
		{
			if (numOfSurface!=3)
			{
				for(auto itNS=newSurfacets.begin();itNS!=newSurfacets.end();itNS++)
				{
					
					if (is_surface(*itNS))
					{
						*ItSurF++=*itNS;
						iterative_sculpture_with_mark(*itNS,VertexIso,ItSurF,-1,FCur,MarkIso);
					}
				
				}
			}
			
			
		}
		return true;
	}//end of if(isInSphere&&...
	else
	{
		if (IdV!=-1)
		{
			FCur=F;
		
		}
	
		return false;
	}
}


//��������,ʵ�ʲ���Ϊ������ڲ�����cell
//FΪ��ʼ���͵ı���������Ƭ;VertexIsoΪ���ܴ��ڵĹ�����,��ʼ-1;ItSurFΪ�������͹��������γɵı���������Ƭ����
//IdVΪĳһ����,��ʼ-1;FCurΪ���γɵ�����IdV�����ı���������Ƭ,��ʼ(-1,-1);MarkIsoΪ�Ƿ���������ʶ,��ʼtrue
template<typename T, typename T_INFO>
template<typename IteratorFacet>
bool DataStructure<T, T_INFO>::
iterative_inflate_with_mark(Facet F,Vertex_Idtype VertexIso,IteratorFacet ItSurF,Vertex_Idtype IdV,Facet& FCur,bool MarkIso)
{
	//----------------for test-------------//
	bool isS=is_surface(F);
	if (!isS)
	{
		//--------------------for test--------------//
		//Vertex_triple vtriF=make_vertex_triple(F);
		//====================test end==============//
		//cout<<"error!:iterative_inflate_with_mark no surface"<<endl;
		return false;
	}
	//================test end============//
	Cell_Idtype c=F.first;
	int ii=F.second;
	//ȡc�ĵ�ii������Cell
	Cell_Idtype cOpp=neighbor(c,ii);
	//��c��cOpp�ĵڼ����ٽ�Cell
	int iiOpp=neighbor_index(cOpp,c);
	int fIndv[4];//fIndvΪcOppǰ��numOfSurface���洢����mirror_facetΪ������������������numOfNSurface�ǷǱ�����������
	int numOfSurface=0;
	int numOfNSurface=0;//numOfSurface+numOfNSurface=4����
	vector<Facet> newSurfacets;
	bool isInSphere=false;
	bool containFCur=false;

	int numInsphere=0;
	for(int i=0;i<4;i++)
	{
		if(is_surface(mirror_facet(Facet(cOpp,i))))
		{
								
			fIndv[numOfSurface]=i;
			numOfSurface++;
			if (mirror_facet(Facet(cOpp,i))==FCur)
			{
				containFCur=true;
			}
		}
		else
		{
			numOfNSurface++;
			fIndv[4-numOfNSurface]=i;
			Vertex_triple vNSur=make_vertex_triple(Facet(cOpp,i));

			//---------for test---------//
			//const double* p0=point(vNSur.first);
			//const double* p1=point(vNSur.second);
			//const double* p2=point(vNSur.third);
			//const double* p3=point(vertex(cOpp,i));
			//========test end==========//
			Vertex_Idtype vT = vertex(cOpp, i);

			//��������Զ��
			if (is_infinite(vNSur.first)||is_infinite(vNSur.second)||
				is_infinite(vNSur.third)||is_infinite(vT))
			{
				return false;
			}

			//side_of_bounded_sphereC3�жϵ�point(vertex(cOpp,i))��point(vNSur.first)��point(vNSur.second)��point(vNSur.third)����������/��/��
			//ON_BOUNDED_SIDEΪ��ʾ�ں������bounded sphere�ڲ�;ON_UNBOUNDED_SIDE��ʾ���ⲿ
			Bounded_side relative_position=GeometricTraits<T>::side_of_bounded_sphereC3(point(vNSur.first),point(vNSur.second),
				point(vNSur.third),point(vertex(cOpp,i)));
			bool isInSphereT=(relative_position==ON_BOUNDED_SIDE);
			if (isInSphereT)
			{
				numInsphere++;
			}
		}
	}
	
	if (numInsphere>0)
	{
		isInSphere=true;
	}


	Facet tmpNearFacet(-1,-1);
	bool isoPInside;
	//��numOfSurface=2ʱҪ�ж�inflate/sculpture�Ƿ����������
	bool isSingularityEdge=false;
	if(numOfNSurface==2)
	{
		isSingularityEdge=is_edge_in_surface(cOpp,fIndv[2],fIndv[3]);

	}
	//-------------------inflate֮�󲻻���infinite���ڱ�����---------------------//
	//����Gabriel�ж�
	if (containFCur&&IdV==-1)
	{
		return false;
	}
	if (MarkIso&&(numOfSurface==3))
	{
		if (IdV==-1)
		{
			return false;
		}
		else
		{
			FCur=F;
			return false;
		}
	}
	if ((vertex(cOpp,iiOpp)==VertexIso)&&(numOfSurface==1))
	{
		if (IdV==-1)
		{
			return false;
		}
		else
		{
			FCur=F;
			return false;
		}
	}

	
	if(!isInSphere&&(numOfSurface==2&&!isSingularityEdge))
	{
		//����,�ѱ������cOpp���Ϊ������,numOfSurfaceΪcOpp��mirror face�Ǳ���ĸ���
		//fIndvΪcOpp��ǰ��numOfSurface���洢��Ϊ������������,����fIndv��Ԫ�ظ���һ��Ϊ4
		//newSurfacets���ͺ������ɱ���		
		inflate(cOpp,numOfSurface,fIndv,newSurfacets);
		
		if (IdV!=-1)
		{
			int iFNext=-1;
			Facet* newSur=new Facet[numOfNSurface];
			int numOfIncidentSur=0;
			int numOfNIncidentSur=0;
			int iVF=-1;
			for(auto itNS=newSurfacets.begin();itNS!=newSurfacets.end();itNS++)
			{
				*ItSurF++=*itNS;
				int iVF_tmp=vertex_index_facet(*itNS,IdV);
				if (iVF_tmp>=0)
				{
					mark_facet_visited(*itNS);
					newSur[numOfIncidentSur]=*itNS;
					if(numOfIncidentSur==0)
					{
						iVF=iVF_tmp;
					}
					numOfIncidentSur++;
				}
				else
				{
					numOfNIncidentSur++;
					newSur[numOfNSurface-numOfNIncidentSur]=*itNS;
				}
			}
			
		
			if (numOfIncidentSur==1)
			{
				iFNext=0;
			}
			else if (numOfIncidentSur==2)
			{
				std::pair<int,int> idVEdge=make_edge_index(iVF);
				Facet FNext_tmp=neighbor_surfacet(newSur[0],idVEdge.first);
				iFNext=0;
				if (FNext_tmp==newSur[1])
				{
					iFNext=1;
				}
			}
			if (numOfSurface!=3)
			{
				
				for (int i = 0; i < numOfNSurface; i++)
				{
					Facet FCur_tmp=newSur[iFNext];
					if (i==iFNext)
					{
						continue;
					}
					else
					{
						if (is_surface(newSur[i]))
						{
							iterative_inflate_with_mark(newSur[i],VertexIso,ItSurF,-1,FCur_tmp,MarkIso);
						}
						
					}
				}
				iterative_inflate_with_mark(newSur[iFNext],VertexIso,ItSurF,IdV,FCur,MarkIso);
			}
			else
			{
				if (numOfIncidentSur=0)
				{
					FCur=Facet(-1,-1);
				}
				else
				{
					FCur=newSur[iFNext];
				}
	
			}
			
		}//end of if(IdV!=-1)
		else
		{
			if (numOfSurface!=3)
			{
				for(auto itNS=newSurfacets.begin();itNS!=newSurfacets.end();itNS++)
				{
					if (is_surface(*itNS))
					{
						*ItSurF++=*itNS;
						iterative_inflate_with_mark(*itNS,VertexIso,ItSurF,-1,FCur,MarkIso);
					}
					
				}
			}
	
			
		}
		return true;	
	}//end of if(!isInSphere&&...
	else
	{
		if (IdV!=-1)
		{
			FCur=F;
		}
	
		return false;
	}
}


//�����p���������ͻ�������ʹ�����ڵı��浽�����㼯�ľ�����С��
//pInside��p���ڱ����λ�ã�trueΪ�����ڲ�,falseΪ�����ⲿ;VertexIso���������(���ʷ�)ʱ��ʹ��,һ��Ϊ-1;
//NearestVerticesΪp��K������㣬InitFacetsΪ�����ڽ�������ĳ�ʼ������Ƭ;
//ItSurFΪ���,��Ϊ���ͺ���������KNN���������б���������Ƭ;
//ItSurSparseΪKNN�����ں��нϳ��ߵı���������Ƭ���ϣ������������������
template<typename T, typename T_INFO>
template<typename IteratorFacet>
void DataStructure<T, T_INFO>::
update_surfacets_KNN(bool PInside,Vertex_Idtype VertexIso,list<Vertex_Idtype> NearestVertices,vector<Facet> InitFacets,IteratorFacet ItSurF,IteratorFacet ItSurSparse)
{
	//markForVertices���ڱ�ǵ�i������ڵ��Ƿ���δ����Ϊ0��������Ϊ2,���Ѿ������Ҳ��ǹ�����Ϊ1���ǹ�������߲���������Ϊ����ڵı���Ϊ-1
	int markForVertices[5]={0};
	//���ڴ洢K-NN��incident surfacet���Ҷ���P�ɼ�
	Facet facetNVertices[5]={Facet(-1,-1)};
	//�洢���ڴ����vertex 
	Vertex_Idtype vertexHandled=*NearestVertices.begin();
	int rankVertexHandled=0;
	//���ڴ洢vertexHandled��һ��incident surface facet
	Facet facetHandledInit(-1,-1);
	//incident surfacets����ߵľ���
	double maxLengthEdge=0;
	//������ڵı���������Ƭ
	Facet facetLarge(-1,-1);
	if (!InitFacets.empty())
	{
		facetHandledInit=*InitFacets.begin();
	}
	//markForVertices[0]=2;
	//facetNVertices[0]=facetHandledInit;
	if (facetHandledInit.first!=-1)
	{
		markForVertices[0]=1;
		facetNVertices[0]=facetHandledInit;
	}
	else
	{
		markForVertices[0]=-1;
	}
	//�洢KNN��incident surfacets�������п������ظ�
	vector<Facet> tmpSurIncident;
	//�������е�K-NN�Ƿ�����ϣ���Ϊfalse����Ϊtrue
	bool testForAll=true;

	while (testForAll)
	{
		Facet tmpFacet=facetHandledInit;
		
		if (facetHandledInit.first!=-1)
		{
			//�洢��KNN��ĳһ����������б���������Ƭ���������ظ�
			vector<Facet> tmpIncidentSurfacets;
			while (visited_for_facet_extractor(tmpFacet)==0)
			{
				//---------------for test-----------//
				//Vertex_triple vtriFT0=make_vertex_triple(tmpFacet);
				//==============test end============//
				mark_facet_visited(tmpFacet);
				tmpIncidentSurfacets.push_back(tmpFacet);

				//---------------for test-----------//
				//Vertex_triple vtriFN1=make_vertex_triple(tmpFacet);
				//==============test end============//
				//��tmpFacet��vertexHandled��תʱ�ĵ�ǰfacet
				//���vertexHandled����tmpFacet�ĵڼ����㣨0/1/2��
				Indextype idVertexOfFacet=vertex_index_facet(tmpFacet,vertexHandled);
				//idVertexOfEdge������ڶ���idVertexOfFacet��0/1/2���ı�
				std::pair<Indextype,Indextype> idVertexOfEdge=make_edge_index(idVertexOfFacet);
				//�������Զ����ţ�0/1/2��idVertexOfEdge.first����fNext��ʵ�ʶ����ţ�
				Vertex_Idtype vertexLeftEdge=vertex_facet(tmpFacet,idVertexOfEdge.first);
				int iInNN=rank_in_list(vertexLeftEdge,NearestVertices.begin(),NearestVertices.end());
				if(iInNN!=-1&&markForVertices[iInNN]==0)
				{
					markForVertices[iInNN]=2;
					facetNVertices[iInNN]=tmpFacet;
				}
			    //�����tmpFacet�ĵ�idVertexOfEdge.first�����ڵı���
				tmpFacet=neighbor_surfacet(tmpFacet,idVertexOfEdge.first);
				//�ж��Ƿ��д�������
				
				//make_vertex_pair�ѱ���EdgeInFacet<�棬0/1/2>��ʽ����<����ʵ�ʱ�ţ�����ʵ�ʱ��>����ʽ
				std::pair<Vertex_Idtype,Vertex_Idtype> verticesEdge=make_vertex_pair(EdgeInFacet(tmpFacet,idVertexOfFacet));
				const double d0=NumericComputation<double>::SquareDistance(point(verticesEdge.first),point(verticesEdge.second));
				const double d1=NumericComputation<double>::SquareDistance(point(vertexHandled),point(verticesEdge.first));
				if (d0>maxLengthEdge)
				{
					maxLengthEdge=d0;
					facetLarge=tmpFacet;
				}
				else if(d1>maxLengthEdge)
				{
					maxLengthEdge=d1;
					facetLarge=tmpFacet;
				}
			}
			for(auto itIST=tmpIncidentSurfacets.begin();itIST!=tmpIncidentSurfacets.end();itIST++)
			{
				if (is_surface(*itIST))
				{
					//--------------------for test-------------//
					//Vertex_triple vtriF=make_vertex_triple(*itIST);
					//====================test end=============//
					if (visited_for_facet_extractor(*itIST)!=0)
					{
						clear_facet_visited(*itIST);					
					}
					tmpSurIncident.push_back(*itIST);
				}
				
			}
			markForVertices[rankVertexHandled]=1;
		}
		else
		{
			markForVertices[rankVertexHandled]=-1;
		}
		//�����һ���ڽ����incident surface facet
		testForAll=false;
		auto itN=NearestVertices.begin(); 
		for (int i = 0; i < K_NN; i++)
		{
			if (*itN<=0)
			{
				markForVertices[i]=-1;
				itN++;
				continue;
			}
			else if (markForVertices[i]==2)
			{
				testForAll=true;
				rankVertexHandled=i;
				vertexHandled=*itN;
				facetHandledInit=facetNVertices[i];
				break;
			}
			itN++;
		}
		itN=NearestVertices.begin();
		if (!testForAll)
		{
			for (int i = 0; i < K_NN; i++)
			{
				if (markForVertices[i]==0)
				{
					std::vector<Facet> incidentFacets;
					incident_surface_facet(*itN,std::back_inserter(incidentFacets));
					if (incidentFacets.empty())
					{
						markForVertices[i]=-1;
						itN++;
						continue;
					}
					else
					{
						testForAll=true;
						markForVertices[i]=2;
						vertexHandled=*itN;
						facetNVertices[i]=*incidentFacets.begin();
						facetHandledInit=*incidentFacets.begin();
					}
				}
				itN++;
			}
			
		}

	}
	vector<Facet> tmpSurfacetsIncident;//tmpSurfacetsIncidentΪtmpSurIncidentȥ���ظ���Ĺ���KNN�������Ƭ
	for (auto iTS=tmpSurIncident.begin();iTS!=tmpSurIncident.end();iTS++)
	{
		if (is_surface(*iTS))
		{
			if (visited_for_facet_extractor(*iTS)==0)
			{
				//--------------------for test-------------//
				//Vertex_triple vtriF=make_vertex_triple(*iTS);
				//====================test end=============//
				tmpSurfacetsIncident.push_back(*iTS);
				mark_facet_visited(*iTS);
			}
		}
	}
	for(auto iTS=tmpSurfacetsIncident.begin();iTS!=tmpSurfacetsIncident.end();iTS++)
	{
		*ItSurF++=*iTS;
		clear_facet_visited(*iTS);
	}

	if (tmpSurIncident.empty()&&tmpSurfacetsIncident.empty())
	{
		return;
	}
	//������������Ƭ�з�K-NN�ĵ��incident surface facet
	std::vector<Vertex_Idtype> VertexNearSparse;
	
	if (is_surface(facetLarge))
	{
		for (int i = 0; i < 3; i++)
		{
			Vertex_Idtype vLargeFacet=vertex_facet(facetLarge,i);
			int rankVL=rank_in_list(vLargeFacet,NearestVertices.begin(),NearestVertices.end());
			if (rankVL==-1)
			{
				VertexNearSparse.push_back(vLargeFacet);
			}
		}
	}
	
	vector<Facet> tmpSurS;
	for (auto itF=VertexNearSparse.begin();itF!=VertexNearSparse.end();itF++)
	{
		
		vertexHandled=*itF;
		Facet tmpFacet=facetLarge;
		vector<Facet> tmpIncidentSparse;
		while (visited_for_facet_extractor(tmpFacet)==0)
		{
		
			mark_facet_visited(tmpFacet);
			tmpIncidentSparse.push_back(tmpFacet);

			//��tmpFacet��vertexHandled��תʱ�ĵ�ǰfacet
			Vertex_triple vf=make_vertex_triple(tmpFacet);
			Indextype idVertexOfFacet=vertex_index_facet(tmpFacet,vertexHandled);
			std::pair<Indextype,Indextype> idVertexOfEdge=make_edge_index(idVertexOfFacet);
				
			Vertex_Idtype vertexLeftEdge=vertex_facet(tmpFacet,idVertexOfEdge.first);
		
			tmpFacet=neighbor_surfacet(tmpFacet,idVertexOfEdge.first);
		}
		for(auto itIST=tmpIncidentSparse.begin();itIST!=tmpIncidentSparse.end();itIST++)
		{
			if (is_surface(*itIST))
			{
				clear_facet_visited(*itIST);
				tmpSurS.push_back(*itIST);
			}
				
		}
	}
	vector<Facet> tmpSurfacetsIncidentSparse;
	for (auto iTS=tmpSurS.begin();iTS!=tmpSurS.end();iTS++)
	{
		if (visited_for_facet_extractor(*iTS)==0)
		{
			tmpSurfacetsIncidentSparse.push_back(*iTS);
			mark_facet_visited(*iTS);
		}
	}
	for(auto iTS=tmpSurfacetsIncidentSparse.begin();iTS!=tmpSurfacetsIncidentSparse.end();iTS++)
	{
		*ItSurSparse++=*iTS;
		clear_facet_visited(*iTS);
	}
}


//������p�������������Ƭ
//SurfacetsInternalConflict�Ǳ�����Ƭ�������Ǳ���,�������ڵ�cell����Ӱ����,����mirror facet����cell����Ӱ����,������һ���ɣ�
//SurfacetsIncidentKNNΪ��KNN�����ı�ѡ����������Ƭ��SurfacetsIncidentSparseΪKNN�����ں��нϳ��ߵı�ѡ����������Ƭ����
//PInside��ʾp���ڱ����λ�ã�NearestFacetΪ���ص����ڽ�����������Ƭ��
//SurfacetsInConflictΪӰ�����ڵı���������Ƭ(���漰��mirror facet���ڵ�cell����conflict)
// VerticesDeletedSurfacet(ɾ���ı���������Ƭ�еĵ�id,��֮�����ı���������Ƭ��ɾ���ĸ���)�����ڹ�����ļ��
template<typename T, typename T_INFO>
void DataStructure<T, T_INFO>::
nearest_surfacet(const T* P,vector<Facet> SurfacetsInternalConflict,vector<Facet> SurfacetsIncidentKNN,vector<Facet> SurfacetsIncidentSparse,
	bool PInside,Facet& NearestFacet,vector<Facet_Idtype>& SurfacetsInConflict,std::map<Vertex_Idtype,size_t>& VerticesDeletedSurfacet)
{
	//step1:��K-NN��incident surface facet�����п��ܵ�nearest surface facet���������������������Ƭ
	//��С����ǵ�����ֵ
	double cosMinDihedral_KNN=-1;
	//��С����Ƕ�Ӧ��������Ƭ
	Facet facetMinDihedral_KNN(-1,-1);
	//��P��������Ƭ�γɵĶ���Ǿ�Ϊ���ʱ��P���������С����
	double minDistance_KNN=COMPUTATION_DOUBLE_MAX;
	//��С�����Ӧ��������Ƭ
	Facet facetMinDistance_KNN(-1,-1);

	for(auto itFacetKNN=SurfacetsIncidentKNN.begin();itFacetKNN!=SurfacetsIncidentKNN.end();itFacetKNN++)
	{
		double cosDihedral=0;
		double distance=0;
		Facet tmpFacet=*itFacetKNN;
		if (tmpFacet.first==-1||!is_surface(tmpFacet))
		{
			continue;
		}
		Vertex_triple vf=make_vertex_triple(tmpFacet);
		if (is_facet_in_conflict(tmpFacet)) //�жϸ��漰��mirror facet���ڵ�cell����conflict
		{
			if (visited_for_facet_extractor(tmpFacet)==0)
			{
				Facet_Idtype idF=get_facet_index(tmpFacet);
				SurfacetsInConflict.push_back(idF);
				VerticesDeletedSurfacet[vf.first]++;
				VerticesDeletedSurfacet[vf.second]++;
				VerticesDeletedSurfacet[vf.third]++;
				mark_facet_visited(tmpFacet);
			}
			
		}
		Orientation o = GeometricTraits<T>::orientation(point(vf.first), point(vf.second), point(vf.third), P);
		if ((o!=NEGATIVE&&PInside)||(o!=POSITIVE&&!PInside))
		{
			
			cosDihedral=cos_min_dihedral_point_to_facet(P,mirror_facet(tmpFacet));
		
			if(cosDihedral>-0.0)
			{
				distance =GeometricTraits<T>::distance_point_to_facet(P,point(vf.first),
													point(vf.second),point(vf.third));
				if(!PInside)
				{
					distance=-distance;
				}
				if (distance<minDistance_KNN)
				{
					minDistance_KNN=distance;
					facetMinDistance_KNN=tmpFacet;
				}
			}
			else if(cosDihedral>cosMinDihedral_KNN)
			{
				cosMinDihedral_KNN=cosDihedral;
				facetMinDihedral_KNN=tmpFacet;
			}
		
		}
	}


	//step2:��sparse vertices��incident surface facet�����п��ܵ�nearest surface facet
	//��С����ǵ�����ֵ
	double cosMinDihedral_Sparse=-1;
	//��С����Ƕ�Ӧ��������Ƭ
	Facet facetMinDihedral_Sparse(-1,-1);
	//��P��������Ƭ�γɵĶ���Ǿ�Ϊ���ʱ��P���������С����
	double minDistance_Sparse=COMPUTATION_DOUBLE_MAX;
	//��С�����Ӧ��������Ƭ
	Facet facetMinDistance_Sparse(-1,-1);

	for(auto itFacetS=SurfacetsIncidentSparse.begin();itFacetS!=SurfacetsIncidentSparse.end();itFacetS++)
	{
		double cosDihedral=0;
		double distance=0;
		Facet tmpFacet=*itFacetS;
		if (tmpFacet.first==-1||!is_surface(tmpFacet))
		{
			continue;
		}
		Vertex_triple vf=make_vertex_triple(tmpFacet);
		if (is_facet_in_conflict(tmpFacet))  //�жϸ��漰��mirror facet���ڵ�cell����conflict
		{
			if (visited_for_facet_extractor(tmpFacet)==0)
			{
				Facet_Idtype idF=get_facet_index(tmpFacet);
				SurfacetsInConflict.push_back(idF);
				VerticesDeletedSurfacet[vf.first]++;
				VerticesDeletedSurfacet[vf.second]++;
				VerticesDeletedSurfacet[vf.third]++;
				mark_facet_visited(tmpFacet);
			}
			
		}
		Orientation o = GeometricTraits<T>::orientation(point(vf.first), point(vf.second), point(vf.third), P);
		if ((o!=NEGATIVE&&PInside)||(o!=POSITIVE&&!PInside))
		{
			
			cosDihedral=cos_min_dihedral_point_to_facet(P,mirror_facet(tmpFacet));
		
			if(cosDihedral>-0.0)
			{
				distance =GeometricTraits<T>::distance_point_to_facet(P,point(vf.first),
													point(vf.second),point(vf.third));
				if(!PInside)
				{
					distance=-distance;
				}
				if (distance<minDistance_Sparse)
				{
					minDistance_Sparse=distance;
					facetMinDistance_Sparse=tmpFacet;
				}
			}
			else if(cosDihedral>cosMinDihedral_Sparse)
			{
				cosMinDihedral_Sparse=cosDihedral;
				facetMinDihedral_Sparse=tmpFacet;
			}
		
		}
	}

	//step3:��conflict region�����п��ܵ�nearest surface facet,�����ڲ��ͱ߽��ϵı���������Ƭ
	//��С����ǵ�����ֵ
	double cosMinDihedral_Conflict=-1;
	//��С����Ƕ�Ӧ��������Ƭ
	Facet facetMinDihedral_Conflict(-1,-1);
	//��P��������Ƭ�γɵĶ���Ǿ�Ϊ���ʱ��P���������С����
	double minDistance_Conflict=COMPUTATION_DOUBLE_MAX;
	//��С�����Ӧ��������Ƭ
	Facet facetMinDistance_Conflict(-1,-1);

	for(auto itFacetConflict=SurfacetsInternalConflict.begin();itFacetConflict!=SurfacetsInternalConflict.end();itFacetConflict++)
	{
		double cosDihedral=0;
		double distance=0;
		Facet tmpFacet=*itFacetConflict;
		if (tmpFacet.first==-1||!is_surface(tmpFacet))
		{
			continue;
		}
		
		Vertex_triple vf=make_vertex_triple(tmpFacet);
		if (is_facet_in_conflict(tmpFacet))  //�жϸ��漰��mirror facet���ڵ�cell����conflict
		{
			if (visited_for_facet_extractor(tmpFacet)==0)
			{
				Facet_Idtype idF=get_facet_index(tmpFacet);
				SurfacetsInConflict.push_back(idF);
				VerticesDeletedSurfacet[vf.first]++;
				VerticesDeletedSurfacet[vf.second]++;
				VerticesDeletedSurfacet[vf.third]++;
				mark_facet_visited(tmpFacet);
			}
			
		}
		Orientation o = GeometricTraits<T>::orientation(point(vf.first), point(vf.second), point(vf.third), P);
		if ((o!=NEGATIVE&&PInside)||(o!=POSITIVE&&!PInside))
		{
			
			cosDihedral=cos_min_dihedral_point_to_facet(P,mirror_facet(tmpFacet));
		
			if(cosDihedral>-0.0)
			{
				distance =GeometricTraits<T>::distance_point_to_facet(P,point(vf.first),
													point(vf.second),point(vf.third));
				if(!PInside)
				{
					distance=-distance;
				}
				if (distance<minDistance_Conflict)
				{
					minDistance_Conflict=distance;
					facetMinDistance_Conflict=tmpFacet;
				}
			}
			else if(cosDihedral>cosMinDihedral_Conflict)
			{
				cosMinDihedral_Conflict=cosDihedral;
				facetMinDihedral_Conflict=tmpFacet;
			}
		
		}
	}

	for (auto itS=SurfacetsInConflict.begin();itS!=SurfacetsInConflict.end();itS++)
	{
		clear_facet_visited(*itS);
	}

	if (facetMinDistance_KNN.first!=-1)
	{
		double mminDistance=minDistance_KNN;
		NearestFacet=facetMinDistance_KNN;
		if (facetMinDistance_Sparse.first!=-1)
		{
			if (minDistance_Sparse<mminDistance)
			{
				mminDistance=minDistance_Sparse;
				NearestFacet=facetMinDistance_Sparse;
			}
		}
		if (facetMinDistance_Conflict.first!=-1)
		{
			if (minDistance_Conflict<mminDistance)
			{
				mminDistance=minDistance_Conflict;
				NearestFacet=facetMinDistance_Conflict;
			}
		}
	}
	else if (facetMinDihedral_KNN.first!=-1)
	{

		double mminDistance=shortest_edge_to_facet(P,facetMinDihedral_KNN);
		NearestFacet=facetMinDihedral_KNN;
		if (facetMinDistance_Sparse.first!=-1)
		{
			if (minDistance_Sparse<mminDistance)
			{
				mminDistance=minDistance_Sparse;
				NearestFacet=facetMinDistance_Sparse;
			}
		}
		else if(facetMinDihedral_Sparse.first!=-1)
		{
			double tmpDis=shortest_edge_to_facet(P,facetMinDihedral_Sparse);
			if (tmpDis<mminDistance)
			{
				if (cosMinDihedral_Sparse>cosMinDihedral_KNN)
				{
					mminDistance=tmpDis;
					NearestFacet=facetMinDihedral_Sparse;
				}
			}
		}
		if (facetMinDistance_Conflict.first!=-1)
		{
			if (minDistance_Conflict<mminDistance)
			{
				mminDistance=minDistance_Conflict;
				NearestFacet=facetMinDistance_Conflict;
			}
		}

	}
	else if (facetMinDistance_Sparse.first!=-1)
	{
		double mminDistance=shortest_edge_to_facet(P,facetMinDistance_Sparse);
		NearestFacet=facetMinDistance_Sparse;

		if(facetMinDihedral_Sparse.first!=-1)
		{
			double tmpDis=shortest_edge_to_facet(P,facetMinDihedral_Sparse);
			if (tmpDis<mminDistance)
			{
				if (cosMinDihedral_Sparse>cosMinDihedral_KNN)
				{
					mminDistance=tmpDis;
					NearestFacet=facetMinDihedral_Sparse;
				}
			}
		}
		if (facetMinDistance_Conflict.first!=-1)
		{
			if (minDistance_Conflict<mminDistance)
			{
				mminDistance=minDistance_Conflict;
				NearestFacet=facetMinDistance_Conflict;
			}
		}
	}
	else if (facetMinDihedral_Sparse.first!=-1)
	{
		double mminDistance=shortest_edge_to_facet(P,facetMinDihedral_Sparse);
		NearestFacet=facetMinDihedral_Sparse;

		if (facetMinDistance_Conflict.first!=-1)
		{
			if (minDistance_Conflict<mminDistance)
			{
				mminDistance=minDistance_Conflict;
				NearestFacet=facetMinDistance_Conflict;
			}
		}
	}
	else
	{
		NearestFacet=facetMinDistance_Conflict;
	}


}
#endif // !DATASTRUCTURE_H

