#include<map>
#include<list>
#include<unordered_set>
#include <iterator>
#include"PointLocator.h"
#include"DataStructure.h"
#include"GeometricTraits.h"
#ifndef SURFACEDELAUNAY_H
#define SURFACEDELAUNAY_H


template<typename Delaunay_3D> class Vertex_remover  ;

template<typename T,typename T_INFO>
class SurfaceDelaunay 
{
	typedef SurfaceDelaunay<T,T_INFO> Self;
	typedef DataStructure<T,T_INFO> SDDataStructure;

//�����ⲿ�ӿں���
public:
	int get_vertex_num()
	{
		return Mesh.get_vertex_num();
	}
	void get_vertex(Indextype IdV,double* PCoord,bool& Use)
	{
		Mesh.get_vertex(IdV,PCoord,Use);
	}
	int get_facet_num()
	{
		return Mesh.get_facet_num();
	}
	void get_facet_vertex(Indextype IdF,const Indextype* IdVerF,bool& Use)
	{
		Mesh.get_facet_vertex(IdF,IdVerF,Use);
	}

	void insert_point(const double* P)
	{
		int num=Mesh.get_vertex_num();
		if(num<3)
		{
			//to make sure initial 3 points can't collinear
			if (num==2&&GeometricTraits<T>::collinear(point(1), point(2), P))
			{
				CandidatePoints.push_back(P);
				return;
			}
			Vertex_Idtype v=Mesh.create_vertex();
			Mesh.set_point(v,P);
		}
		else if(num==3)
		{
			//to make sure initial can't coplanar
			if (GeometricTraits<T>::orientation(point(1), point(2), point(3), P) == ZERO) 
			{
				CandidatePoints.push_back(P);
				return;
			}
			Vertex_Idtype v=Mesh.create_vertex();
			Mesh.set_point(v,P);
			Mesh.insert_first_finite_cell();

		}
		else
		{
			insert(P);
		}
	}

	void delete_vertex(Vertex_Idtype IdF)
	{
		remove(IdF);
	}
	bool delete_point(const double* P)
	{
		Locate_type lt;
		int li;
		int lj;
		Cell_Idtype c;
		c=locate_t(P,lt,li,lj,-1);
		//Ӧ��������ڵ㣬������ڵ㵽�˵�ľ���
		if(lt==VERTEX)
		{
			Vertex_Idtype v=vertex(c,li); 
			remove(v);
			return true;
		}
		else
			return false;
	}

private:
	SDDataStructure Mesh;
	Idtype infinite;
	int Level;
	T Bounds[2][3];
	std::list<const T*> CandidatePoints;

public:
	bool insert_iso=true; // true: insert isolated vertices; false: ignore isolated vertices

private:

	Vertex_triple make_vertex_triple( Facet& f) ;


	Bounded_side
		side_of_circle(Cell_Idtype c, int i,
		 const T* p, bool perturb) ;
	Bounded_side
		side_of_sphere(Vertex_Idtype v0, Vertex_Idtype v1,
		Vertex_Idtype v2, Vertex_Idtype v3,
		const T *p, bool perturb) ;
	Bounded_side
		coplanar_side_of_bounded_circle( const T* p0,  const T* p1,
		 const T* p2,  const T* p, bool perturb) ;	
	Oriented_side
		side_of_oriented_sphere( const T* p0,  const T* p1,  const T* p2,
		 const T* p3,  const T* p, bool perturb) ;
	Bounded_side
		side_of_sphere(Cell_Idtype c,  const T*  p,
		bool perturb = false) 
	{
		return side_of_sphere(vertex(c, 0), vertex(c, 1),
			vertex(c, 2), vertex(c, 3), p, perturb);
	};
	Bounded_side 
		side_of_segment( const T* p,
		 const T* p0,
		 const T* p1,
		Locate_type & lt, int & i) ;
	

protected:
	class Conflict_tester_3
	{
		 const T* p;
		 Self *t;

	public:

		Conflict_tester_3(const T *pt,  Self *tr)
			: p(pt), t(tr) {}

		bool operator()( Cell_Idtype c) 
		{
			return t->side_of_sphere(c, p, true) == ON_BOUNDED_SIDE;
		}

		bool test_initial_cell(Cell_Idtype) 
		{
			return true;
		}
	};

	class Conflict_tester_2
	{
		 const T* p;
		 Self *t;

	public:

		Conflict_tester_2( const T* pt,  Self *tr)
			: p(pt), t(tr) {}

		bool operator()( Cell_Idtype c)
		{
			return t->side_of_circle(c, 3, p, true) == ON_BOUNDED_SIDE;
		}

		bool test_initial_cell(Cell_Idtype) 
		{
			return true;
		}
	};

public:
	void set_surface_extrator(bool surface){ this->Mesh.set_surface_extractor(surface); };
	void get_surface_data_structure(DataArray<T>& Point,DataArray<Idtype>& FacetVertex)
	{Mesh.get_surface_data_structure(Point,FacetVertex);};


public:
	SurfaceDelaunay(int level,T* bounds[3],bool surface)
	{

		set_surface_extrator(surface);
		this->Level = level;

		for (int i = 0; i < 3; i++)
		{
			this->Bounds[0][i] = bounds[0][i];
			this->Bounds[1][i] = bounds[1][i];
		}
		init_ds(level,bounds);
	}
	SurfaceDelaunay( int level,T* bounds[3],T *p0,  T *p1,
		 T *p2,  T *p3,bool surface)
	{

		set_surface_extrator(surface);
		this->Level = level;

		for (int i = 0; i < 3; i++)
		{
			this->Bounds[0][i] = bounds[0][i];
			this->Bounds[1][i] = bounds[1][i];
		}
		if(GeometricTraits<T>::orientation(p0, p1, p2, p3) == POSITIVE)
		
			init_ds(level,bounds,p0, p1, p2, p3);

		else
			init_ds(level,bounds,p1, p0, p2, p3);

	}
	void init_ds(int level,T* bounds[3], const T* p0, const T* p1,
		 const T* p2,  const T* p3)
	{
		
		this->Mesh.init_point_set(level,bounds);
		Vertex_Idtype v0, v1, v2, v3;
		infinite = -1;
		infinite = Mesh.insert_first_finite_cell(v0, v1, v2, v3, infinite);
		set_point(v0,p0);
		set_point(v1,p1);
		set_point(v2,p2);
		set_point(v3,p3);
	}
	void init_ds(const T* p0, const T* p1, const T* p2, const T* p3, Vertex_Idtype& vh0, Vertex_Idtype& vh1, Vertex_Idtype& vh2, Vertex_Idtype& vh3)
	{
		infinite = Mesh.insert_first_finite_cell(vh0, vh1, vh2, vh3, infinite);
		set_point(vh0,p0);
		set_point(vh1,p1);
		set_point(vh2,p2);
		set_point(vh3,p3);
	}
		
	
	void init_ds(int level, T* bounds[3])
	{
		this->Mesh.init_point_set(level, bounds);
		infinite = Mesh.insert_increase_dimension();
	}
	int dimension() 
	{
		return this->Mesh.dimension();
	}
	//ȡһ��infinite cell,�˴�infinite��ʾ����Զ��
	Cell_Idtype infinite_cell(){ return Mesh.cell(infinite); };
	bool is_infinite_cell(Cell_Idtype IdC)
	{
		return Mesh.has_vertex(IdC,infinite);
	};
	Vertex_Idtype vertex(Cell_Idtype IdC, int i){ return Mesh.vertex(IdC,i); };
	//ȡ�������
	const T* point(Vertex_Idtype IdV){ return this->Mesh.point(IdV); };
	//�趨point�����꣬�ǲ���Ҫ��֮ǰ��point�ͷ��ڴ棿
	void set_point(Vertex_Idtype IdV, const T* p){ this->Mesh.set_point(IdV,p); };
	//�ж��Ƿ�Ϊ���޵�
	bool is_infinite(Vertex_Idtype IdV){ return IdV == infinite; };
	//
	Vertex_Idtype infinite_vertex(){ return this->infinite; };
	//�����p[3]
	Vertex_Idtype insert(const T* p,Cell_Idtype star=-1);
	
	
	//��λ��,��Octree�ṹ�ȶ�λ������ڵ㣬�ٶ�λ��cell��octreeִ��Ч��̫�ͣ���Ҫ�޸�
	Cell_Idtype locate(const T* p,Locate_type& lt,int& li,int& lj, Cell_Idtype start);
	//��λ�㣬ֱ����triangulation data structure��λ��cell
	Cell_Idtype locate_t(const T* p, Locate_type& lt, int& li, int& lj, Cell_Idtype start);

	Cell_Idtype inexact_locate(const T* p,Cell_Idtype start,int n_of_turns=2500);

	Cell_Idtype exact_locate(const T* p, Locate_type & lt, int & li, int & lj, Cell_Idtype start);
	//��һ�����K������ڵ��Լ����ڽ����incident umbrella����֪��˵������һ��cell
	void K_nearest_vertices(const T* p,list<Vertex_Idtype>& nearest,vector<Facet>& IncidentSurfacets, Cell_Idtype start=-1);
	//��һ��������ڽ��㣬�������ڱ�����������ڽ������Ͻ��������ӹ�ϵ�ĵ�
	void nearest_vertices_in_surface(const T* p,list<Vertex_Idtype>& nearest,Cell_Idtype start=-1);
	//��һ���㵽һ��cell���ĸ���������ĵ�
	Vertex_Idtype nearest_vertex_in_cell(const T* p,Cell_Idtype c);
	
	//��λ֮������
	Vertex_Idtype insert(const T* p, Locate_type lt, Cell_Idtype c,int li, int lj );
	//����������˻�ʱ��ģ������
	Vertex_Idtype insert_d(const T* p, Locate_type lt, Cell_Idtype c, int li, int lj);
	//������ڱ���
	Vertex_Idtype insert_in_edge(const T* p, Cell_Idtype c,int li,int lj);
	//�����������
	Vertex_Idtype insert_in_facet(const T* p, Cell_Idtype c, int li){};
	//�������Cell��˻���ʱ���ã����񲻻���֣�
	Vertex_Idtype insert_in_cell(const T* p, Cell_Idtype c){};
	//�������͹����
	Vertex_Idtype insert_outside_convex_hull(const T* p, Cell_Idtype c);
	//�������ԭͼ�����ڵĿռ�֮��
	Vertex_Idtype insert_outside_affine_hull(const T* p);
	//insert�ĸ���������ά��>=2ʱ
	template<class Conflict_tester>
	Vertex_Idtype insert_in_conflict(const T* p,Locate_type lt,Cell_Idtype c,int li,int lj,Conflict_tester tester);
	//find conflict cells
	template<class Conflict_test> 
	void find_conflicts(const T* p,Cell_Idtype d,  Conflict_test &tester,
		std::vector<Facet>& boundaryFacets,
		std::vector<Cell_Idtype>& cells,std::vector<Cell_Idtype>& bcells,
		std::vector<Facet>& internalFacets,
		std::vector<Facet>& internalSurfaceFacets,
		 Facet *this_facet_must_be_in_the_cz = NULL,
		bool *the_facet_is_in_its_cz = NULL);


	//Ҫ��Ӱ�����cells��������߽�Ҳ����,p��Ӱ������
	//���DelaunayӰ����ά������������ԣ����滻��Ϊ�߽��hole���������ע
	//pΪ�����;cell_beginΪ�������г�ʼɾ����cell;cell_endΪ���ɾ����cell;
	//��begin,i����Ӱ����߽����棬��begin����Ӱ���򣬵�neighbor(begin,i)������Ӱ����
	//boundEdgeΪ���滻��߽��ߣ��Զ˵�(vertex_id,vertex_id)��ʾ��ConflictBoundEdgesҲ�Ǳ߽��ߣ���Edge(��id,0/1/2)��ʾ
	//MapVertexΪ(���滻���ϵĶ���id,��֮�����ı���������Ƭ��ɾ���ĸ���);SetVertexΪ�߽����ϵĵ㣻
	//NewBoundarySurfacetsΪ���γɵ��滻���ϵ�������Ƭ;NewConflictSurfacetsΪ���γɵķ��滻��ı���������Ƭ��
	//IsIso��ʾp��SeedFacet�Ƿ���Ӱ�����ڻ�߽��ϣ�isInsideΪp���ڱ����λ�ù�ϵ
	template <class CellIt>	
	Vertex_Idtype	insert_in_hole(const T* p, CellIt cell_begin, CellIt cell_end,
	Cell_Idtype begin, int i,set_pair_int boundEdge,list<Edge> ConflictBoundEdges,
	std::map<Vertex_Idtype,size_t>& MapVertex,std::set<Vertex_Idtype>& SetVertex,
	vector<Facet>& NewBoundarySurfacets,vector<Facet>& NewConflictSurfacets,bool IsIso,bool isInside)
	{
		// Some geometric preconditions should be tested...
		Vertex_Idtype v;
		
		v = Mesh.insert_in_hole(cell_begin, cell_end, begin, i,boundEdge,ConflictBoundEdges,MapVertex,SetVertex,
			NewBoundarySurfacets,NewConflictSurfacets,IsIso,isInside);

		if (v==-1)
		{
			return -1;
		}

		set_point(v,p);
		return v;
	}

	template <class CellIt>	
	Vertex_Idtype	insert_in_hole(const T* p, CellIt cell_begin, CellIt cell_end,
	Cell_Idtype begin, int i)
	{
		// Some geometric preconditions should be tested...
		Vertex_Idtype v;
		
		v = Mesh.insert_in_hole(cell_begin, cell_end, begin, i);

		set_point(v,p);
		return v;
	}

	
	template<typename OutputIteratorVertex,typename OutputIteratorFacet>
	void adjacent_vertices_facets(Vertex_Idtype v, OutputIteratorVertex vertices, OutputIteratorFacet facets);

	

	//���������
	//VertexWithSurfacetDeletedΪ(���滻���ϵĶ���id,��֮�����ı���������Ƭ��ɾ���ĸ���)
	//VertexWithSurfacetCreatedΪ�߽����ϵĶ���
	void isolate_vertices(std::map<Vertex_Idtype,size_t> VertexWithSurfacetDeleted,std::set<Vertex_Idtype> VertexWithSurfacetCreated)
	{
		Mesh.isolate_vertices(VertexWithSurfacetDeleted,VertexWithSurfacetCreated);
	};
	//�󽻽��������������Ƭ
	//��p���ڽ�����������ƬInitFacet������������ƬSeedFacet
	//pΪ����㣬pInsideΪ�������ڱ����λ��
	bool seed_surface_facet(const T* p,Facet InitFacet,bool PInside,Facet& SeedFacet);
	//��ĳһ��������ڵ�surface facet�����յ㵽�����γɶ���ǵ�������С��������

	
	void handle_candidate_points();
	
};

template<typename T, typename T_INFO>
void SurfaceDelaunay<T,T_INFO>::
handle_candidate_points()
{
	if (CandidatePoints.empty())
	{
		return;
	}
	int t=1.1*CandidatePoints.size()+1;
	while ((!CandidatePoints.empty())&&(t--))
	{
		int r=rand()%CandidatePoints.size();
		std::list<const T*>::iterator itc=CandidatePoints.begin();
		advance(itc,r);
		const T* tmp=*itc;
		CandidatePoints.erase(itc);
		//----------for test-----------//
		//int num=Mesh.get_vertex_num();
		//cout<<endl;
		//cout<<"reinsert quit point:"<<num+1<<endl;
		//----------test end-----------//
		insert_point(tmp);
	}
	cout<<endl<<"quit point numbers:"<<CandidatePoints.size()<<endl;
}

template<class Delaunay_3D>
class Vertex_remover{
	typedef Delaunay_3D Delaunay;
public:

	Vertex_remover(Delaunay &tmp_) : tmp(tmp_) {}

	Delaunay &tmp;


	Bounded_side side_of_bounded_circle( const double* p,  const double* q,
		 const double* r,  const double* s, bool perturb = false)  {
		return tmp.coplanar_side_of_bounded_circle(p, q, r, s, perturb);
	}
};



template<typename T, typename T_INFO>
Vertex_Idtype SurfaceDelaunay<T,T_INFO>::
insert(const T* x, Cell_Idtype start)
 {
	Locate_type lt;
	int li;
	int lj;

	//��������p���ĸ�������c�У�lt�ж�c�Ƿ���͹���⣬p��c�ڣ�����c����/��/������
	//lt��͹���⣬liΪ����Զ����Ա��,ljδ��ʼ��
	//ltΪCELL,li,ljδ��ʼ��
	//ltΪFACET��li(0/1/2/3)�ĸ������Ӧ���棬ljδ��ʼ��
	//ltΪEDGE,li��lj�ֱ��ʾ�ñߵ���Ա�ţ�0/1/2/3��
	//ltΪVERTEX��li��ʾ��һ������(0/1/2/3)��ljδ��ʼ��
	Cell_Idtype c = locate_t(x, lt, li, lj, start);

	return insert(x, lt, c, li, lj);
}


template<typename T, typename T_INFO>
//template<class Conflict_tester>
Vertex_Idtype SurfaceDelaunay<T, T_INFO>::
insert(const T* p, Locate_type lt, Cell_Idtype c, int li, int lj)
{
	switch (dimension()) {
	case 3:
	{
		//tester(p,this)����������ʼ��
		//p���������������ڣ�����ON_BOUNDED_SIDE.����infinite cell���ǹ��������,p�ں�����Զ��һ��İ�ռ�,���������,p��������Ԫ���Բ��
		//p���������������ϣ�����ON_BOUNDARY.����infinite cell,��ʱp��������Ԫ����,�����������Բ��
		//p���������������⣬����ON_UNBOUNDED_SIDE.����infinite cell���ǹ��������,p�ڲ�������Զ��һ��İ�ռ�,���������,p������������Բ��
		Conflict_tester_3 tester(p, this);

		//p�����
		//lt��͹���⣬liΪ����Զ����Ա��,ljδ��ʼ��
		//ltΪCELL,li,ljδ��ʼ��
		//ltΪFACET��li(0/1/2/3)�ĸ������Ӧ���棬ljδ��ʼ��
		//ltΪEDGE,li��lj�ֱ��ʾ�ñߵ���Ա�ţ�0/1/2/3��
		//ltΪVERTEX��li��ʾ��һ������(0/1/2/3)��ljδ��ʼ��
		//cΪp���ڵ�cell��tester������
		Vertex_Idtype v = insert_in_conflict(p, lt, c, li, lj,
			tester);
		return v;
	}// dim 3
	case 2:
	{
		Conflict_tester_2 tester(p, this);
		return insert_in_conflict(p, lt, c, li, lj,
			tester);
	}//dim 2
	default:
		// dimension <= 1
		// Do not use the generic insert.
		return insert_d(p, lt, c, li, lj);
	}
}
//ע��insert_d��������ĺ�����1ά�����²��ܳ���
template<typename T, typename T_INFO>
Vertex_Idtype SurfaceDelaunay<T, T_INFO>::
insert_d(const T* p, Locate_type lt, Cell_Idtype c, int li, int lj)
{
	switch (lt) {
	case VERTEX:
		return Mesh.vertex(c,li);
	case EDGE:
		return insert_in_edge(p, c, li, lj);

	case OUTSIDE_CONVEX_HULL:
		return insert_outside_convex_hull(p, c);
	case OUTSIDE_AFFINE_HULL:
	default:
		return insert_outside_affine_hull(p);
	}
}

template<typename T, typename T_INFO>
Vertex_Idtype SurfaceDelaunay<T, T_INFO>::
insert_in_edge(const T* p, Cell_Idtype c, int li, int lj)
{
	
	//ֻ��1ά�����
	Vertex_Idtype v = Mesh.insert_in_edge(c, li, lj);
	set_point(v,p);
	return v;
}
template<typename T, typename T_INFO>
Vertex_Idtype SurfaceDelaunay<T, T_INFO>::
insert_outside_convex_hull(const T* p, Cell_Idtype c)
{
	//ֻ��1ά�����
	return  insert_in_edge(p,c,0,1);
	
}
template<typename T, typename T_INFO>
Vertex_Idtype SurfaceDelaunay<T, T_INFO>::
insert_outside_affine_hull(const T* p)
{
	bool reorient;
	switch (dimension()) {
	case 1:
	{
		Cell_Idtype c = infinite_cell();
		Cell_Idtype n = Mesh.neighbor(c, Mesh.vertex_index(c, infinite_vertex()));
		Orientation o = GeometricTraits<T>::coplanar_orientation(Mesh.point(vertex(n, 0)),
			Mesh.point(vertex(n, 1)), p);
		reorient = o == NEGATIVE;

		break;
	}
	case 2:
	{
		Cell_Idtype c = infinite_cell();
		Cell_Idtype n = Mesh.neighbor(c,Mesh.vertex_index(c,infinite_vertex()));
		Orientation o = GeometricTraits<T>::orientation(Mesh.point(vertex(n, 0)),
			Mesh.point(vertex(n, 1)),
			Mesh.point(vertex(n, 2)),p);
		reorient = o == NEGATIVE;
		break;
	}
	default:
		reorient = false;
	}
	Vertex_Idtype v= Mesh.insert_increase_dimension(infinite_vertex());

	set_point(v,p);

	if (reorient)
		Mesh.reorient();

	return v;
}


//p�����
//lt��͹���⣬liΪ����Զ����Ա��,ljδ��ʼ��
//ltΪCELL,li,ljδ��ʼ��
//ltΪFACET��li(0/1/2/3)�ĸ������Ӧ���棬ljδ��ʼ��
//ltΪEDGE,li��lj�ֱ��ʾ�ñߵ���Ա�ţ�0/1/2/3��
//ltΪVERTEX��li��ʾ��һ������(0/1/2/3)��ljδ��ʼ��
//cΪp���ڵ�cell��tester������
template<typename T,typename T_INFO>
template<class Conflict_tester>
Vertex_Idtype SurfaceDelaunay<T,T_INFO>::
insert_in_conflict(const T* p, Locate_type lt, Cell_Idtype c, int li, int lj, Conflict_tester tester)
{
	bool pInside=true;
	if(Mesh.extract_surface())
		pInside=Mesh.is_label_inside(c);
	//ֻ��3ά��2ά���
	switch (dimension()) {
	case 3:
	{
		if ((lt == VERTEX)) {
			return vertex(c,li);
		}
		//�жϵ��Ƿ��ڱ�����
		pInside=Mesh.is_vertex_inside_surface(lt,c,li,lj);
	
		Vertex_Idtype v=-1;
		std::vector<Cell_Idtype> cells,bcells;
		std::vector<Facet> boundaryFacets;
		std::vector<Facet> internalFacets;
		//����inflate��sculpture֮����conflict region���ڵı���������Ƭ
		std::vector<Facet> internalSurfaceFacets;
		//conflict region֮�ڵı���������Ƭ
		std::vector<Facet> surfacetsInConflict;
		std::vector<Facet_Idtype> idSurfacetsInConflict;
		Triple<Facet,double,double> nearestFacetConflict(Facet(-1,-1),-1,-1);
		std::map<Vertex_Idtype,size_t> vertexWithDeletedSurfacet;
		std::set<Vertex_Idtype> vertexWithCreateSurfacet;
		std::list<Edge> conflictBoundEdges;
		
		std::vector<Triple<Vertex_Idtype,Facet,bool>> isoVetices;
		std::list<Vertex_Idtype> nearestVertices(5,-1);
		std::vector<Facet> incidentFacets;
		
		//cΪp���ڵ������壬�ҵ�p�����5�����㣬������nearestVertices��,���������ı�����Ƭ����incidentFacets
		K_nearest_vertices(p,nearestVertices,incidentFacets,c);
		
		//--------------------for test---------------------//
		//cout<<"����ڵ�:      ";
		//for (auto itp=nearestVertices.begin();itp!=nearestVertices.end();itp++)
		//{
		//	cout<<*itp<<" ";
		//}
		//cout<<endl;
		//===================test end======================//
			
		cells.reserve(32);
		boundaryFacets.reserve(32);
		internalFacets.reserve(32);

		
		//��p��Ӱ����cΪp���ڵ������壬tester�ǿ�������࣬
		//boundaryFacetsӰ����߽��ϵ�������Ƭ(�������ڵ�cell����Ӱ����,����mirror facet����cell������Ӱ����),
		//cellsΪӰ���������弯��
		//internalFacets��Ӱ�����ڲ���������Ƭ���������ڵ�cell����Ӱ���򣬻���mirror facet����cell����Ӱ����,������һ���ɣ�
		//internalSurfaceFacets�Ǳ�����Ƭ�������Ǳ���,�������ڵ�cell����Ӱ���򣬻���mirror facet����cell����Ӱ����,������һ���ɣ�
		//�����������
		//find conflict cells
		find_conflicts(
			p,c,
			tester,
			boundaryFacets,
			cells,bcells,
			internalFacets,
			internalSurfaceFacets
			);

 		Facet nearestFacet(-1,-1);
		Facet beginFacet = boundaryFacets[0];
		if(Mesh.extract_surface())
		{
				
			set_pair_int boundEdge(10, hash_function_pair_int);
			Facet nearFacet(-1,-1);	
			//-----------------------�µ���������Ƭ�ķ���----------------------------//
			vector<Facet> incidentSurfacets_KNN;
			vector<Facet> incidentSurfacets_Sparse;

			pInside=Mesh.is_vertex_inside_surface(lt,c,li,lj);


			//�����p���������ͻ�����,ʹ�����ڵı��浽�����㼯�ľ�����С
			//pInside��p���ڱ����λ��,trueΪ�����ڲ�,falseΪ�����ⲿ;�ڶ�������һ��Ĭ��-1�����������ʱ��ʹ��;
			//nearestVerticesΪp��K�������,incidentFacetsΪ�����ڽ�������ĳ�ʼ����������Ƭ;
			//incidentSurfacets_KNN��Ϊ���ͺ���������KNN���������б���������Ƭ;
			//incidentSurfacets_SparseΪKNN�����ں��нϳ��ߵı���������Ƭ����
			Mesh.update_surfacets_KNN(pInside,-1,nearestVertices,incidentFacets,std::back_inserter(incidentSurfacets_KNN),std::back_inserter(incidentSurfacets_Sparse));	

			pInside=Mesh.is_vertex_inside_surface(lt,c,li,lj);

			//������p�������������Ƭ
			//internalSurfaceFacets�Ǳ�����Ƭ(�����Ǳ���,�������ڵ�cell����Ӱ����,����mirror facet����cell����Ӱ����,������һ����)
			//incidentSurfacets_KNNΪ��KNN�����ı�ѡ����������Ƭ;incidentSurfacets_SparseΪKNN�����ں��нϳ��ߵı�ѡ����������Ƭ����
			//PInside��ʾp���ڱ����λ��;nearFacetΪ���ص����ڽ�����������Ƭ;
			//idSurfacetsInConflictΪӰ�����ڵı���������Ƭ(���漰��mirror facet���ڵ�cell����conflict)
			//vertexWithDeletedSurfacetΪ(ɾ���ı���������Ƭ�еĵ�id,��֮�����ı���������Ƭ��ɾ���ĸ���),���ڹ�����ļ��
			Mesh.nearest_surfacet(p,internalSurfaceFacets,incidentSurfacets_KNN,incidentSurfacets_Sparse,pInside,
									nearFacet,idSurfacetsInConflict,vertexWithDeletedSurfacet);
			//--------------------for test---------------------//
			//if (nearFacet.first!=-1)
			//{
			//	Vertex_triple vf1=Mesh.make_vertex_triple(nearFacet);
			//	cout<<"��ʼ������ڱ���:  ("<<vf1.first<<","<<vf1.second<<","<<vf1.third<<")"
			//		<<"/("<<nearFacet.first<<","<<nearFacet.second<<")"<<endl;
			//}
			//===================test end======================//
			pInside=Mesh.is_vertex_inside_surface(lt,c,li,lj);
			/*cout<<"���ڱ����ڲ���"<<pInside<<endl;*/

			if (nearFacet.first!=-1)
			{
				//��p���ڽ�����������ƬnearFacet������������ƬnearestFacet(����������,���ڵ�cell��Ӱ������,����������,��mirror facet����cell������Ӱ����)
				//pΪ�����,pInsideΪ�������ڱ����λ��
				seed_surface_facet(p,nearFacet,pInside,nearestFacet);

				
			}
				
			//----------------for test----------//
			
			//if(nearestFacet.first!=-1)
			//{
			//			
			//	Vertex_triple vf2=Mesh.make_vertex_triple(nearestFacet);
			//			
			//	cout<<"������Ƭ: ("<<vf2.first<<","<<vf2.second<<","<<vf2.third<<")"
			//		<<"/("<<nearestFacet.first<<","<<nearestFacet.second<<")"<<endl;
			//}
			//========================�µ���������Ƭ�ķ���============================//
			
			//-------------------------surface extractor-----------------------------//
			if(nearestFacet.first!=-1)
			{
				//-------------------------for test(nearestFacet)-----------------------//
				Facet m_facet=Mesh.mirror_facet(nearestFacet);
				bool isSurfaceFacetTest=Mesh.is_surface(nearestFacet);
				//============================test end==================================//
					
			
				//Mesh.conflict_surface_boundary(p,pInside,nearestFacet, boundEdge, beginFacet,vertexWithCreateSurfacet);

				//�󽻽��棬ʹ��Gabriel�Ĺ�������滻�����չ����Ҫ��Է���pseudo-concave��surface����͹��չ
				//������������ƬΪ��ʼ����BFS���������ڿ��ܱ��滻�ı���������Ƭ
				//p����㣻nearestFacetΪ����������Ƭ������������,���ڵ�cell��Ӱ������,����������,��mirror facet����cell������Ӱ���򣩣�
				//conflictBoundEdgesΪ����ֵ�����滻����ı߽��ߣ���Edge(��id,0/1/2)��ʽ��ʾ,���Ҹ��治�滻��boundEdgeҲ���滻��������ı߽��ߣ��Զ˵���ʽ(vertex_id,vertex_id)��ʾ
				//beginFacet����cell����Ӱ����,mirror������cell������Ӱ����;���Һ��б��滻��߽���
				//vertexWithCreateSurfacetΪ�߽����ϵĶ���id�����ڹ�����ļ�飻
				//vertexWithDeletedSurfacetΪ(���滻���ϵĶ���id,��֮�����ı���������Ƭ��ɾ���ĸ���)�����ڹ�����ļ��
				//pInsideΪp���ڱ����λ��
				//idSurfacetsInConflictΪӰ�����ڵı���������Ƭ(���漰��mirror facet���ڵ�cell����conflict)
				bool quit=false;
				Mesh.find_conflict_surfacet(p,nearestFacet,conflictBoundEdges,boundEdge,beginFacet,vertexWithCreateSurfacet,
					vertexWithDeletedSurfacet,pInside,idSurfacetsInConflict,quit);
			
				if (quit)
				{
					//cout<<"quit point"<<endl;
					//v = Mesh.create_vertex();
					CandidatePoints.push_back(p);
					for (auto it=cells.begin();it!=cells.end();it++)
					{
						Mesh.clear(*it);
					}
					for (auto it=bcells.begin();it!=bcells.end();it++)
					{
						Mesh.clear(*it);
					}
					return v;
				}
				//--------------------for test---------------------//
				//Vertex_triple vfbeg=Mesh.make_vertex_triple(beginFacet);
				//cout<<"beginFacet: ("<<vfbeg.first<<","<<vfbeg.second<<","<<vfbeg.third<<")"
				//	<<"/("<<beginFacet.first<<","<<beginFacet.second<<")"<<endl;
				//===================test end======================//


		//========================surface extractor end==========================//
	

				//newv�뽻����߽��γɵı���������Ƭ
				vector<Facet> newBoundarySurfacets;
				//conflict�������γɵı���������Ƭ
				vector<Facet> newConflictSurfacets;

				//Ҫ��Ӱ�����cells��������߽�Ҳ����,p��Ӱ������
				//���DelaunayӰ����ά������������ԣ����滻��Ϊ�߽��hole���������ע
				//pΪ�����;cells.begin()Ϊ�������г�ʼɾ����cell;cells.end()Ϊ���ɾ����cell;
				//beginFacet is a facet on the boundary of the hole,beginFacet.first����Ӱ�����cell����beginFacet.first�ĵ�beginFacet.second��0/1/2/3)�����ڵ�cell����Ӱ����
				//boundEdgeΪ���滻����ı߽��ߣ��Զ˵�(vertex_id,vertex_id)��ʾ��conflictBoundEdgesҲ�Ǳ��滻����ı߽��ߣ���Edge(��id,0/1/2)��ʾ
				//vertexWithDeletedSurfacetΪ(���滻���ϵĶ���id,��֮�����ı���������Ƭ��ɾ���ĸ���);vertexWithCreateSurfacetΪ�߽����ϵĶ��㣻
				//NewBoundarySurfacets(cell,0/1/2/3)��ʾ�����γɱ��湲�滻��߽��ߵı���;newConflictSurfacetsΪ���γɵ��޹��ɵı���������Ƭ�������������
				//false��ʾp��SeedFacet�Ƿ���Ӱ�����ڻ�߽��ϣ�pInsideΪp���ڱ����λ�ù�ϵ
				v = insert_in_hole(p, cells.begin(), cells.end(),
					beginFacet.first, beginFacet.second,boundEdge,conflictBoundEdges,vertexWithDeletedSurfacet,vertexWithCreateSurfacet,
					newBoundarySurfacets,newConflictSurfacets,false,pInside);

				if (v==-1)
				{
					//cout<<"quit point"<<endl;
					//v = Mesh.create_vertex();
					CandidatePoints.push_back(p);
					for (auto it=cells.begin();it!=cells.end();it++)
					{
						Mesh.clear(*it);
					}
					for (auto it=bcells.begin();it!=bcells.end();it++)
					{
						Mesh.clear(*it);
					}
					return -1;
				}

				for (auto it = idSurfacetsInConflict.begin(); it != idSurfacetsInConflict.end(); it++)
				{
					//-------for test----//
					//Facet fT=Mesh.get_facet_cell_link(*it);
					//Cell_Idtype c_fT=fT.first;Cell_Idtype cli_fT=Mesh.neighbor(c_fT,fT.second);
					//Vertex_triple vtriF=make_vertex_triple(fT);
					//cout<<"!!delete facet(idSurfacetsInConflict): "<<*it<<"("<<vtriF.first<<","<<vtriF.second<<","<<vtriF.third<<")"
					//	<<"/("<<fT.first<<","<<fT.second<<")["<<fT.first<<"("
					//	<<vertex(c_fT, 0)<<","<<vertex(c_fT, 1)<<","<<vertex(c_fT, 2)<<","<<vertex(c_fT, 3)<<") "<<cli_fT<<"("
					//	<<vertex(cli_fT, 0)<<","<<vertex(cli_fT, 1)<<","<<vertex(cli_fT, 2)<<","<<vertex(cli_fT, 3)<<")]"<<endl;
					//=======test end====//
					Mesh.delete_surface_facet(*it);
				}
				
				for(auto itF=newBoundarySurfacets.begin();itF!=newBoundarySurfacets.end();itF++)
				{
					vector<Facet> newUpdateSur;
					Facet tmpF(-1,-1);
					//��������,ʵ�ʲ���Ϊ������ڲ�����cell
					//*itFΪ��ʼ���͵ı���������Ƭ;�ڶ�������(�β�VertexIso)Ϊ���ܴ��ڵĹ�����,ȡ-1;newUpdateSurΪ�������͹��������γɵı���������Ƭ����
					//�ڶ���-1(�β�IdV)Ϊĳһ����;tmpFΪ���γɵ�����IdV�����ı���������Ƭ,��ʼ(-1,-1);true(�β�MarkIso)�����������
					bool isFlated=Mesh.iterative_inflate_with_mark(*itF,-1,std::back_inserter(newUpdateSur),-1,tmpF,true);
					//�������,ʵ�ʲ����޳������ڲ�cell
					//*itFΪ��ʼ��̵ı���������Ƭ;�ڶ�������(�β�VertexIso)Ϊ���ܴ��ڵĹ�����,ȡ-1;newUpdateSurΪ�������͹��������γɵı���������Ƭ����
					//�ڶ���-1(�β�IdV)Ϊĳһ����;tmpFΪ���γɵ�����IdV�����ı���������Ƭ,��ʼ(-1,-1);true(�β�MarkIso)�����������
					bool isSculptured=Mesh.iterative_sculpture_with_mark(*itF,-1,std::back_inserter(newUpdateSur),-1,tmpF,true);
				}
				
				//���������
				//vertexWithDeletedSurfacetΪ(���滻���ϵĶ���id,��֮�����ı���������Ƭ��ɾ���ĸ���)
				//vertexWithCreateSurfacetΪ�߽����ϵĶ���
				if (insert_iso) // true: insert isolated vertices; false: ignore isolated vertices
				{
					isolate_vertices(vertexWithDeletedSurfacet, vertexWithCreateSurfacet);
				}
				

			}
			else
			{

				//--------------for test----------//
				/*cout<<"isolate point(without delaunay triangulation)"<<endl;*/
				//==============test end=========//


				/*cout<<"[iso]quit point"<<endl;*/
				//v = Mesh.create_vertex();
				CandidatePoints.push_back(p);
				for (auto it=cells.begin();it!=cells.end();it++)
				{
					Mesh.clear(*it);
				}
				for (auto it=bcells.begin();it!=bcells.end();it++)
				{
					Mesh.clear(*it);
				}
				return v;
																				
			}
		}
		else
		{
			v=insert_in_hole(p, cells.begin(), cells.end(),
					beginFacet.first, beginFacet.second);
		}
		

		// Store the hidden points in their new cells.
		//hider.reinsert_vertices(v);
		return v;
	}
	case 2:
	{
		// This check is added compared to the 3D case
		if (lt == OUTSIDE_AFFINE_HULL)
			return insert_outside_affine_hull(p);

		if ((lt == VERTEX)) {
			return vertex(c,li);
		}
		// If the new point is not in conflict with its cell, it is hidden.
		// Ok, we really insert the point now.
		// First, find the conflict region.
		Vertex_Idtype v=-1;
		std::vector<Cell_Idtype> cells,bcells;
		std::vector<Facet> boundaryFacets;
		std::vector<Facet> internalFacets;
		std::vector<Facet> internalSurfaceFacets;
		Triple<Facet,double,double> nearestFacetConflict(Facet(-1,-1),-1,-1);
		std::map<Vertex_Idtype,size_t> vertexWithDeletedSurfacet;
		std::set<Vertex_Idtype> vertexWithCreateSurfacet;
		std::list<Edge> conflictBoundEdges;

		std::list<Vertex_Idtype> nearestVertices(Mesh.K_NN,-1);
		std::vector<Facet> incidentSurfacets;

		std::vector<Facet_Idtype> idSurfacetsInConflict,replacedSurfaceIds;

		K_nearest_vertices(p,nearestVertices,incidentSurfacets,c);

		cells.reserve(32);
		boundaryFacets.reserve(32);
		internalFacets.reserve(32);
		find_conflicts
			(p,c, tester, 
			boundaryFacets,
			cells,bcells,
			internalFacets,
			internalSurfaceFacets
			);

		Facet beginFacet = boundaryFacets[0];
		
		if(nearestFacetConflict.first.first!=-1)
		{
			set_pair_int boundEdge(10, hash_function_pair_int);
		
			//==============test end=========//
			//newv�뽻����߽��γɵı���������Ƭ,�˴�û���õ�
			vector<Facet> newBoundarySurfacets;
			//conflict�������γɵı���������Ƭ
			vector<Facet> newConflictSurfacets;
		
			v = insert_in_hole(p, cells.begin(), cells.end(),
				beginFacet.first, beginFacet.second,boundEdge,conflictBoundEdges,vertexWithDeletedSurfacet,vertexWithCreateSurfacet,
				newBoundarySurfacets,newConflictSurfacets,false,pInside);

	
			if (Mesh.extract_surface())
			{
				for (auto it = internalSurfaceFacets.begin(); it != internalSurfaceFacets.end(); it++)
				{
					Mesh.delete_surface_facet(*it);
				}
			}


			//--------------------������㣬���������������-----------------------//
			if(Mesh.extract_surface())
			{
				isolate_vertices(vertexWithDeletedSurfacet,vertexWithCreateSurfacet);
			}
	
			//========================�����㴦��end===============================//
		}
		else
		{
			set_pair_int boundEdge(10, hash_function_pair_int);
			//==============test end=========//
			//newv�뽻����߽��γɵı���������Ƭ,�˴�û���õ�
			vector<Facet> newBoundarySurfacets;
			//conflict�������γɵı���������Ƭ
			vector<Facet> newConflictSurfacets;
		
			v = insert_in_hole(p, cells.begin(), cells.end(),
				beginFacet.first, beginFacet.second,boundEdge,conflictBoundEdges,vertexWithDeletedSurfacet,vertexWithCreateSurfacet,
				newBoundarySurfacets,newConflictSurfacets,true,pInside);

			
			if(Mesh.extract_surface())
			{
				Mesh.insert_to_isolate_vertices(v);
				for (auto it = internalSurfaceFacets.begin(); it != internalSurfaceFacets.end(); it++)
				{
					Mesh.delete_surface_facet(*it);
				}
			}
			
		}
		return v;
	}
	
	}
}



//��p��Ӱ����dΪp���ڵ������壬tester�ǿ�������࣬
//boundaryFacetsӰ����߽��ϵ�������Ƭ(�������ڵ�cell����Ӱ����,����mirror facet����cell������Ӱ����)
//cellsΪӰ���������弯��
//internalFacets��Ӱ�����ڲ���������Ƭ���������ڵ�cell����Ӱ����,����mirror facet����cellҲ����Ӱ����,������һ���ɣ�
//internalSurfaceFacets�Ǳ�����Ƭ�������Ǳ���,�������ڵ�cell����Ӱ����,����mirror facet����cell����Ӱ����,������һ���ɣ�
//�����������
template< typename T, typename T_INFO>
template<class Conflict_test>
void SurfaceDelaunay<T, T_INFO>::
find_conflicts(const T* p, Cell_Idtype d, Conflict_test &tester,
std::vector<Facet>& boundaryFacets,
std::vector<Cell_Idtype>& cells,std::vector<Cell_Idtype>& bcells,
std::vector<Facet>& internalFacets,
std::vector<Facet>& internalSurfaceFacets,
Facet *this_facet_must_be_in_the_cz,
bool *the_facet_is_in_its_cz)
{

	Facet facetAcuteDihedral(-1,-1);
	double cosDihedralAcuteFacet=-1;
	double minDistanceAcuteFacet=COMPUTATION_DOUBLE_MAX;
	Facet facetMinDihedral(-1,-1);
	double cosMinDihedral=-1;

	double cosDihedral=0;
	if (the_facet_is_in_its_cz)
		*the_facet_is_in_its_cz = false;
	std::stack<Cell_Idtype> cell_stack;
	cell_stack.push(d);//cells_stackΪ�������Ӱ���������弯��
	//��d��ǳ�conflict����1
	Mesh.mark_in_conflict(d);
	//�жϵ�p���ڱ����ڻ��Ǳ�����,trueΪ������
	bool pInside=Mesh.is_label_inside(d);
	//*it.second++ = d;
	cells.push_back(d);//cellsΪӰ���������弯��

	do 
	{
		Cell_Idtype c = cell_stack.top();//c��Ӱ������
		cell_stack.pop();

		// For each neighbor cell
		for (int i = 0; i<dimension() + 1; ++i) 
		{
			Cell_Idtype test = Mesh.neighbor(c, i);

			// "test" is either in the conflict zone,
			// either facet-adjacent to the CZ
			//�ж�test�Ƿ�conflict
			if (Mesh.is_in_conflict(test)) 
			{

				Facet f(c, i); // Internal facet.
				// Is it the facet where're looking for?
				if (this_facet_must_be_in_the_cz && the_facet_is_in_its_cz
					&& f == *this_facet_must_be_in_the_cz)
				{
					*the_facet_is_in_its_cz = true;
				}
				//---------------------��¼ʸ���棬��һ���������������-----------------------
	
				internalFacets.push_back(f);//internalFacets��Ӱ�����ڲ���������Ƭ

				//=============surface extractor============//
				if (Mesh.extract_surface())
				{
					if (Mesh.is_surface(f))
					{
						//-----------------for test------------//
						//Vertex_triple vtF=Mesh.make_vertex_triple(f);
						//=================test end============//
						internalSurfaceFacets.push_back(f);

					}			

				}
				//---------------surface extractor end-------------//

				//==================================��¼end=====================================
				continue; // test was already in conflict.
			}
			//���IdC��MarkCellConflictState����Ƿ�clear
			//tester(test)������
			//p���������������ڣ�����true.����infinite cell���ǹ��������,p�ں�����Զ��һ��İ�ռ�,���������,p��������Ԫ���Բ��
			//p���������������ϣ�����false.����infinite cell,��ʱp��������Ԫ����,�����������Բ��
			//p���������������⣬����false.����infinite cell���ǹ��������,p�ڲ�������Զ��һ��İ�ռ�,���������,p������������Բ��
			if (Mesh.is_clear(test)) 
           { 
				if (tester(test)) //����׼��
				{  
					Facet f(c, i); // Internal facet.
					// Is it the facet where're looking for?
					if (this_facet_must_be_in_the_cz && the_facet_is_in_its_cz
						&& f == *this_facet_must_be_in_the_cz)
					{
						*the_facet_is_in_its_cz = true;
					}
					//---------------------��¼ʸ���棬��һ���������������-----------------------
					internalFacets.push_back(f);
					//=============surface extractor============//
					if (Mesh.extract_surface())
					{
						//double distance = 0;
						if (Mesh.is_surface(f))
						{
							//-----------------for test------------//
							Vertex_triple vtF=Mesh.make_vertex_triple(f);
							//=================test end============//
							internalSurfaceFacets.push_back(f);		
						}

					}
					//==================================��¼end=====================================
					cell_stack.push(test);
					Mesh.mark_in_conflict(test);
					cells.push_back(test);
					continue;
				}

				//��test��ǳ�boundary,��2
				Mesh.mark_on_boundary(test);
				bcells.push_back(test);
			}

			Facet f(c, i); // Boundary facet.
			// Is it the facet where're looking for?
			if (this_facet_must_be_in_the_cz
				&& the_facet_is_in_its_cz
				&&
				(Mesh.mirror_facet(f) == *this_facet_must_be_in_the_cz
				|| f == *this_facet_must_be_in_the_cz))
			{
				*the_facet_is_in_its_cz = true;
			}

			boundaryFacets.push_back(f);//boundaryFacetsӰ����߽��ϵ�������Ƭ
			if (Mesh.extract_surface())
			{
				if (Mesh.is_surface(f))
				{
					//-----------------for test------------//
					//Vertex_triple vtF=Mesh.make_vertex_triple(f);
					//=================test end============//
					internalSurfaceFacets.push_back(f);				
					
				}
				else if (Mesh.is_surface(Mesh.mirror_facet(f)))
				{
					Facet opp_f = Mesh.mirror_facet(f);	
					//-----------------for test------------//
					//Vertex_triple vtF=Mesh.make_vertex_triple(opp_f);
					//=================test end============//
					internalSurfaceFacets.push_back(opp_f);				
				}

			}

		}
	} while (!cell_stack.empty());	

}



template<typename T, typename T_INFO>
template<typename OutputIteratorVertex, typename OutputIteratorFacet>
void SurfaceDelaunay<T, T_INFO>::
adjacent_vertices_facets(Vertex_Idtype v,OutputIteratorVertex vertices,OutputIteratorFacet facets)
{
	Mesh.adjacent_vertices_facets(v, facets, vertices);
	
}


template<typename T, typename T_INFO>
Vertex_triple SurfaceDelaunay<T, T_INFO>::
make_vertex_triple( Facet& f) 
{
	Cell_Idtype ch = f.first;
	int i = f.second;

	return Vertex_triple(vertex(ch, Triangulation_utils_3::vertex_triple_index(i, 0)),
		vertex(ch, Triangulation_utils_3::vertex_triple_index(i, 1)),
		vertex(ch, Triangulation_utils_3::vertex_triple_index(i, 2)));
}




//p��v0,v1,v2,v3����������������ڣ�����ON_BOUNDED_SIDE.
//����infinite cell,�ǹ��������,p�ں�����Զ��һ��İ�ռ�(p�뵱ǰ�ʷֹ��ڸ������),���������,p��������Ԫ���Բ��;
//p��v0,v1,v2,v3����������������ϣ�����ON_BOUNDARY;
//����infinite cell,��ʱp��������Ԫ����,�����������Բ��
//p��v0,v1,v2,v3����������������⣬����ON_UNBOUNDED_SIDE.
//����infinite cell,�ǹ��������,p�ڲ�������Զ��һ��İ�ռ�,���������,p������������Բ��
template<typename T, typename T_INFO>
Bounded_side SurfaceDelaunay<T, T_INFO>::
side_of_sphere(Vertex_Idtype v0, Vertex_Idtype v1,
					Vertex_Idtype v2, Vertex_Idtype v3,
					 const T *p, bool perturb) 
{

	if (is_infinite(v0)) {
		Orientation o = GeometricTraits<T>::orientation(point(v2), point(v1), point(v3), p);
		if (o != 0)
			return Bounded_side(o);
		return coplanar_side_of_bounded_circle(point(v2), point(v1), point(v3), p, perturb);
	}

	if (is_infinite(v1)) {
		Orientation o = GeometricTraits<T>::orientation(point(v2), point(v3), point(v0), p);
		if (o != 0)
			return Bounded_side(o);
		return coplanar_side_of_bounded_circle(point(v2), point(v3), point(v0), p, perturb);
	}

	if (is_infinite(v2)) {
		Orientation o = GeometricTraits<T>::orientation(point(v1), point(v0), point(v3), p);
		if (o != 0)
			return Bounded_side(o);
		return coplanar_side_of_bounded_circle(point(v1), point(v0), point(v3), p, perturb);
	}

	if (is_infinite(v3)) {
		Orientation o = GeometricTraits<T>::orientation(point(v0), point(v1), point(v2), p);
		if (o != 0)
			return Bounded_side(o);
		return coplanar_side_of_bounded_circle(point(v0), point(v1), point(v2), p, perturb);
	}

	return (Bounded_side)side_of_oriented_sphere(point(v0), point(v1), point(v2), point(v3), p, perturb);
}


//p0,p1,p2,p3Ҫ�������ֶ���
//p��������p0p1p2p3��������ϣ�����ON_ORIENTED_BOUNDARY��0��
//p��������p0p1p2p3��������⣬����ON_NEGATIVE_SIDE��-1��
//p��������p0p1p2p3��������ڣ�����ON_POSITIVE_SIDE��1��
template<typename T, typename T_INFO>
Oriented_side SurfaceDelaunay<T, T_INFO>::
side_of_oriented_sphere( const T* p0,  const T* p1,  const T* p2,
 const T* p3,  const T* p, bool perturb) 
{

	Oriented_side os =
		GeometricTraits<T>::Side_of_oriented_sphere_3(p0, p1, p2, p3, p);

	if (os != ON_ORIENTED_BOUNDARY || !perturb)
		return os;

}


//Ҫ��p0p1p2p3���棬p0p1p2��ʱ������
template<typename T,typename T_INFO>
Bounded_side SurfaceDelaunay<T,T_INFO>::
coplanar_side_of_bounded_circle( const T* p0,  const T* p1,
 const T* p2,  const T* p, bool perturb) 
{
	// In dim==2, we should even be able to assert orient == POSITIVE.

	Bounded_side bs =
		GeometricTraits<T>::coplanar_side_of_bounded_circleC3(p0, p1, p2, p);

	if (bs != ON_BOUNDARY || !perturb)
		return bs;

}




template<typename T, typename T_INFO>
Bounded_side SurfaceDelaunay<T, T_INFO>::
side_of_circle(Cell_Idtype c, int i,
 const T* p, bool perturb) 
// precondition : dimension >=2
// in dimension 3, - for a finite facet
// returns ON_BOUNDARY if the point lies on the circle,
// ON_UNBOUNDED_SIDE when exterior, ON_BOUNDED_SIDE
// interior
// for an infinite facet, considers the plane defined by the
// adjacent finite facet of the same cell, and does the same as in
// dimension 2 in this plane
// in dimension 2, for an infinite facet
// in this case, returns ON_BOUNDARY if the point lies on the
// finite edge (endpoints included)
// ON_BOUNDED_SIDE for a point in the open half-plane
// ON_UNBOUNDED_SIDE elsewhere
{

	int i3 = 5;

	if (dimension() == 2) {
		// the triangulation is supposed to be valid, ie the facet
		// with vertices 0 1 2 in this order is positively oriented
		if (!Mesh.has_vertex(c,infinite_vertex(), i3))
			return coplanar_side_of_bounded_circle(point(vertex(c,0)),
			point(vertex(c, 1)),
			point(vertex(c, 2)),
			p, perturb);
		// else infinite facet
		// v1, v2 finite vertices of the facet such that v1,v2,infinite
		// is positively oriented
		Vertex_Idtype v1 = vertex(c,Triangulation_cw_ccw_2::ccw(i3)),
			v2 = vertex(c,Triangulation_cw_ccw_2::cw(i3));

		Orientation o = GeometricTraits<T>::coplanar_orientation(point(v1), point(v2), p);
		if (o != COLLINEAR)
			return Bounded_side(o);
		// because p is in f iff
		// it does not lie on the same side of v1v2 as vn
		int i_e;
		Locate_type lt;
		// case when p collinear with v1v2
		return side_of_segment(p,
			point(v1), point(v2),
			lt, i_e);
	}

	if ((!Mesh.has_vertex(c,infinite_vertex(), i3)) || (i3 != i)) {
		// finite facet
		// initialization of i0 i1 i2, vertices of the facet positively
		// oriented (if the triangulation is valid)
		int i0 = (i>0) ? 0 : 1;
		int i1 = (i>1) ? 1 : 2;
		int i2 = (i>2) ? 2 : 3;
		
		return coplanar_side_of_bounded_circle(point(vertex(c,i0)),
			point(vertex(c,i1)),
			point(vertex(c,i2)),
			p, perturb);
	}

}

template<typename T, typename T_INFO>
Bounded_side SurfaceDelaunay<T, T_INFO>::
side_of_segment( const T* p,
 const T* p0,
 const T* p1,
Locate_type & lt, int & i) 
// p0, p1 supposed to be different
// p supposed to be collinear to p0, p1
// returns :
// ON_BOUNDED_SIDE if p lies strictly inside the edge
// ON_BOUNDARY if p equals p0 or p1
// ON_UNBOUNDED_SIDE if p lies strictly outside the edge
{

	switch (GeometricTraits<T>::collinear_position(p0, p, p1)) {
	case MIDDLE:
		lt = EDGE;
		return ON_BOUNDED_SIDE;
	case SOURCE:
		lt = VERTEX;
		i = 0;
		return ON_BOUNDARY;
	case TARGET:
		lt = VERTEX;
		i = 1;
		return ON_BOUNDARY;
	default: // case BEFORE: case AFTER:
		lt = OUTSIDE_CONVEX_HULL;
		return ON_UNBOUNDED_SIDE;
	}
}


template<typename T, typename T_INFO>
Cell_Idtype  SurfaceDelaunay<T, T_INFO>::
locate(const T* p, Locate_type& lt, int& li, int& lj, Cell_Idtype start)
{
		
	if (start == -1)
	{
		Vertex_Idtype nearest_vertex = Mesh.nearest_point_inexact(p);
		start = Mesh.cell(nearest_vertex);
		
	}
	int i;
	if (Mesh.has_vertex(start,infinite,i))
		start = Mesh.neighbor(start, i);

	switch (dimension())
	{
	case 3:
	{
		Cell_Idtype previous = -1;
		Cell_Idtype c = start;

		double o[4];

		bool try_next_cell = true;
		while (try_next_cell)
		{


			try_next_cell = false;
			// We know that the 4 vertices of c are positively oriented.
			// So, in order to test if p is seen outside from one of c's facets,
			// we just replace the corresponding point by p in the orientation
			// test.  We do this using the array below.
			double pts[4][3];
			for (int i = 0; i < 4; i++)
			{
				//pointTetra[i] = GetPoint(pointTetraId[i]);
				for (int j = 0; j < 3; j++)
					//pts[i][j] = pointTetra[i].GetPointCoordinate()[j];
					pts[i][j] = point(vertex(c, i))[j];
			}

			NumericComputation<T>::BarycentricCoords(p, pts[0], pts[1],
				pts[2], pts[3], o);

			double neg = 0;
			int numNeg = 0;
			int negI = 0;
			for (int i = 0; i < 4; i++)
			{
				if (o[i] < 0)
				{
					numNeg++;
					try_next_cell = true;
					Cell_Idtype next = Mesh.neighbor(c, i);
					if (Mesh.has_vertex(next,infinite,li))
					//if (is_infinite_cell(next))
					{
						lt = OUTSIDE_CONVEX_HULL;
						return next;
					}
					if (o[i] < neg)
					{
						neg = o[i];
						negI = i;
					}
											
				}
			}
			if (numNeg > 0)
			{
				c = Mesh.neighbor(c, negI);   //ȡbarycenterֵ��С��	}
			}
			

		} // next cell

		int sum = (o[0] == 0)
			+ (o[1] == 0)
			+ (o[2] == 0)
			+ (o[3] == 0);
		switch (sum) {
		case 0:
		{
			lt = CELL;
			break;
		}
		case 1:
		{
			lt = FACET;
			li = (o[0] == 0) ? 0 :
				(o[1] == 0) ? 1 :
				(o[2] == 0) ? 2 : 3;
			break;
		}
		case 2:
		{
			lt = EDGE;
			li = (o[0] != 0) ? 0 :
				(o[1] != 0) ? 1 : 2;
			lj = (o[li + 1] != 0) ? li + 1 :
				(o[li + 2] != 0) ? li + 2 : li + 3;

			break;
		}
		case 3:
		{
			lt = VERTEX;
			li = (o[0] != 0) ? 0 :
				(o[1] != 0) ? 1 :
				(o[2] != 0) ? 2 : 3;
			break;
		}
		}
		return c;

	}
	case 2:
	{
		Cell_Idtype c = start;

		//first tests whether p is coplanar with the current triangulation
		if (GeometricTraits<T>::orientation(point(vertex(c,0)),
			point(vertex(c, 1)),
			point(vertex(c, 2)),
			p) != DEGENERATE) {
			lt = OUTSIDE_AFFINE_HULL;
			li = 3; // only one facet in dimension 2
			return c;
		}
		// if p is coplanar, location in the triangulation
		// only the facet numbered 3 exists in each cell
		while (1) {
			int inf;
			if (Mesh.has_vertex(c,infinite, inf)) {
				// c must contain p in its interior
				lt = OUTSIDE_CONVEX_HULL;
				li = Triangulation_cw_ccw_2::cw(inf);
				lj = Triangulation_cw_ccw_2::ccw(inf);
				return c;
			}

			// else c is finite
			// we test its edges in a random order until we find a
			// neighbor to go further
			//int i = die3();
			int i = 0;
			const T* p0 = point(vertex(c,i));
			const T* p1 = point(vertex(c,Triangulation_cw_ccw_2::ccw(i)));
			const T* p2 = point(vertex(c,Triangulation_cw_ccw_2::cw(i)));
			Orientation o[3];
			o[0] = GeometricTraits<T>::coplanar_orientation(p0, p1, p);
			if (o[0] == NEGATIVE) {
				c = Mesh.neighbor(c,Triangulation_cw_ccw_2::cw(i));
				continue;
			}
			o[1] = GeometricTraits<T>::coplanar_orientation(p1, p2, p);
			if (o[1] == NEGATIVE) {
				c = Mesh.neighbor(c,i);
				continue;
			}
			o[2] = GeometricTraits<T>::coplanar_orientation(p2, p0, p);
			if (o[2] == NEGATIVE) {
				c = Mesh.neighbor(c,Triangulation_cw_ccw_2::ccw(i));
				continue;
			}

			// now p is in c or on its boundary
			int sum = (o[0] == COLLINEAR)
				+ (o[1] == COLLINEAR)
				+ (o[2] == COLLINEAR);
			switch (sum) {
			case 0:
			{
				lt = FACET;
				li = 3; // useless ?
				break;
			}
			case 1:
			{
				lt = EDGE;
				li = (o[0] == COLLINEAR) ? i :
					(o[1] == COLLINEAR) ? Triangulation_cw_ccw_2::ccw(i) :
					Triangulation_cw_ccw_2::cw(i);
				lj = Triangulation_cw_ccw_2::ccw(li);
				break;
			}
			case 2:
			{
				lt = VERTEX;
				li = (o[0] != COLLINEAR) ? Triangulation_cw_ccw_2::cw(i) :
					(o[1] != COLLINEAR) ? i :
					Triangulation_cw_ccw_2::ccw(i);
				break;
			}
			}
			return c;
		}
	}
	case 1:
	{

		Cell_Idtype c = start;

		//first tests whether p is collinear with the current triangulation
		if (!GeometricTraits<T>::collinear(p,
			point(vertex(c,0)),
			point(vertex(c,1)))) {
			lt = OUTSIDE_AFFINE_HULL;
			return c;
		}
		// if p is collinear, location :
		while (1) {
			if (Mesh.has_vertex(c,infinite)) {
				// c must contain p in its interior
				lt = OUTSIDE_CONVEX_HULL;
				return c;
			}

			// else c is finite
			// we test on which direction to continue the traversal
			switch (GeometricTraits<T>::collinear_position(point(vertex(c,0)),p,point(vertex(c,1)))) {
				
				
			case AFTER:
				c = Mesh.neighbor(c,0);
				continue;
			case BEFORE:
				c = Mesh.neighbor(c,1);
				continue;
			case MIDDLE:
				lt = EDGE;
				li = 0;
				lj = 1;
				return c;
			case SOURCE:
				lt = VERTEX;
				li = 0;
				return c;
			case TARGET:
				lt = VERTEX;
				li = 1;
				return c;
			}
		}
	}

	case 0:
	{
		//Finite_vertices_iterator vit = finite_vertices_begin();
		Vertex_Idtype vit = -1;
		for (size_t i = 0; i < Mesh.num_of_vertices(); i++)
		{
			if (i != infinite&&!Mesh.vertex_is_deleted(i))
			{
				vit = i;
				break;
			}
		}
			
		if (!GeometricTraits<T>::equal(p, point(vit))) {
			lt = OUTSIDE_AFFINE_HULL;
		}
		else {
			lt = VERTEX;
			li = 0;
		}
		return Mesh.cell(vit);
	}
	case -1:
	{
		lt = OUTSIDE_AFFINE_HULL;
		return -1;
	}
	default:
	{
		return -1;
	}

	}

}

//��������p���ĸ�������c�У�lt�ж�c�Ƿ���͹���⣬p��c�ڣ�����c����/��/������
//lt��͹���⣬liΪ����Զ����Ա��,ljδ��ʼ��
//ltΪCELL,li,ljδ��ʼ��
//ltΪFACET��li(0/1/2/3)�ĸ������Ӧ���棬ljδ��ʼ��
//ltΪEDGE,li��lj�ֱ��ʾ�ñߵ���Ա�ţ�0/1/2/3��
//ltΪVERTEX��li��ʾ��һ������(0/1/2/3)��ljδ��ʼ��
template<typename T, typename T_INFO>
Cell_Idtype  SurfaceDelaunay<T, T_INFO>::
locate_t(const T* p, Locate_type& lt, int& li, int& lj, Cell_Idtype start)
{
	Cell_Idtype ch = inexact_locate(
		p, start);
	return exact_locate(p, lt, li, lj, ch);
}


//ͨ��n_of_turns�ζ�λp���ĸ���������Զ����������У���n_of_turns����ᣬ�򷵻����һ��������
template<typename T, typename T_INFO>
Cell_Idtype  SurfaceDelaunay<T, T_INFO>::
inexact_locate(const T* p, Cell_Idtype start, int n_of_turns)
{
	if (dimension() < 3) return start;

	// Make sure we continue from here with a finite cell.
	if (start == -1)
		start = infinite_cell();

	

	int ind_inf;
	//������start�Ƿ��е�infinite,�����õ����������0��1��2��3����ֵ��ind_inf
	if (Mesh.has_vertex(start, infinite, ind_inf))
		start = Mesh.neighbor(start, ind_inf);  //��֤start��������Զ��

	// We implement the remembering visibility walk.
	// in this phase, no need to be stochastic

	// Remembers the previous cell to avoid useless orientation tests.
	Cell_Idtype previous = -1;
	Cell_Idtype c = start;



	// Now treat the cell c.

	try_next_cell:

	n_of_turns--;

	// We know that the 4 vertices of c are positively oriented.
	// So, in order to test if p is seen outside from one of c's facets,
	// we just replace the corresponding point by p in the orientation
	// test.  We do this using the array below.

	const double* pts[4];

	//pts[0]Ϊ������c�ĵ�0������ָ�룬pts[1]Ϊ������c�ĵ�1������ָ��...
	for(int i = 0; i < 4; i++)
	{
		Vertex_Idtype vT=vertex(c,i);
		pts[i] = point(vT);		
	}

	// the remembering visibility walk
	for (int i = 0; i != 4; ++i) {
		Cell_Idtype next = Mesh.neighbor(c,i);
		if (previous == next) continue;

		// We temporarily put p at i's place in pts.
		const T* backup = pts[i];
		pts[i] = p;
        
		//��ΪDelaunay�ʷ����������Ѿ���֤����������������(v0,v1,v2,v3)��
		//����orientation(p,v1,v2,v3)=-orientation(v1,v2,v3,p)=orientation(v1,v3,v2,p);
		//    orientation(v0,p,v2,v3)=orientation(v2,v3,v0,p)
		//    orientation(v0,v1,p,v3)=-orientation(v3,v0,v1,p)=orientation(v0,v3,v1,p);
		//    orientation(v0,v1,v2,p)����
		if (GeometricTraits<T>::inexact_orientation(pts[0], pts[1], pts[2], pts[3]) != NEGATIVE) {
			pts[i] = backup;
			continue;
		}
		if (Mesh.has_vertex(next,infinite)) {
			// We are outside the convex hull.
			return next;
		}
		previous = c;
		c = next;
		
		if (n_of_turns) goto try_next_cell;
	}

	return c;
}


//��������p���ĸ�������c�У�lt�ж�c�Ƿ���͹���⣬p��c�ڣ�����c����/��/������
//lt��͹���⣬liΪ����Զ����Ա��,ljΪ��ʼ��
//ltΪCELL,li,ljδ��ʼ��
//ltΪFACET��li(0/1/2/3)�ĸ������Ӧ���棬ljδ��ʼ��
//ltΪEDGE,li��lj�ֱ��ʾ�ñߵ���Ա�ţ�0/1/2/3��
//ltΪVERTEX��li��ʾ��һ������(0/1/2/3)��ljδ��ʼ��
template<typename T, typename T_INFO>
Cell_Idtype  SurfaceDelaunay<T, T_INFO>::
exact_locate(const T* p, Locate_type & lt, int & li, int & lj, Cell_Idtype start)
{
	
	if (dimension() >= 1) {
		// Make sure we continue from here with a finite cell.
		if (start == -1)
			start = infinite_cell();

		int ind_inf;
		if (Mesh.has_vertex(start,infinite, ind_inf))
			start = Mesh.neighbor(start,ind_inf);   //��֤start��������Զ��
	}

	switch (dimension()) {
	case 3:
	{
		
		// We implement the remembering visibility/stochastic walk.

		// Remembers the previous cell to avoid useless orientation tests.
		Cell_Idtype previous = -1;
		Cell_Idtype c = start;

		// Stores the results of the 4 orientation tests.  It will be used
		// at the end to decide if p lies on a face/edge/vertex/interior.
		Orientation o[4];

		// Now treat the cell c.
		bool try_next_cell = true;
		while (try_next_cell)
		{
			try_next_cell = false;
			// We know that the 4 vertices of c are positively oriented.
			// So, in order to test if p is seen outside from one of c's facets,
			// we just replace the corresponding point by p in the orientation
			// test.  We do this using the array below.
			
			const double* pts[4];

			for (int ii = 0; ii < 4; ii++)
			{
				pts[ii] = point(vertex(c, ii));
			}
			// For the remembering stochastic walk,
			// we need to start trying with a random index :
			int i = 0;
			// For the remembering visibility walk (Delaunay and Regular only), we don't :

			// for each vertex
			for (int j = 0; !try_next_cell && j != 4; ++j, i = (i + 1) & 3)
			{
				Cell_Idtype next = Mesh.neighbor(c,i);

				if (previous == next)
				{
					o[i] = POSITIVE;
				}
				else
				{
					// We temporarily put p at i's place in pts.
					const T* backup = pts[i];
					pts[i] = p;
					o[i] = GeometricTraits<T>::orientation(pts[0], pts[1], pts[2], pts[3]);
					if (o[i] != NEGATIVE)
					{
						pts[i] = backup;
					}
					else
					{
						if (Mesh.has_vertex(next,infinite, li))
						{
							// We are outside the convex hull.
							lt = OUTSIDE_CONVEX_HULL;
							return next;
						}
						previous = c;
						c = next;
						
						try_next_cell = true;
					}
				}
			} // next vertex
		} // next cell

		// now p is in c or on its boundary
		int sum = (o[0] == COPLANAR)
			+ (o[1] == COPLANAR)
			+ (o[2] == COPLANAR)
			+ (o[3] == COPLANAR);
		switch (sum) {
		case 0:
		{
			lt = CELL;
			break;
		}
		case 1:
		{
			lt = FACET;
			li = (o[0] == COPLANAR) ? 0 :
				(o[1] == COPLANAR) ? 1 :
				(o[2] == COPLANAR) ? 2 : 3;
			break;
		}
		case 2:
		{
			lt = EDGE;
			li = (o[0] != COPLANAR) ? 0 :
				(o[1] != COPLANAR) ? 1 : 2;
			lj = (o[li + 1] != COPLANAR) ? li + 1 :
				(o[li + 2] != COPLANAR) ? li + 2 : li + 3;

			break;
		}
		case 3:
		{
			lt = VERTEX;
			li = (o[0] != COPLANAR) ? 0 :
				(o[1] != COPLANAR) ? 1 :
				(o[2] != COPLANAR) ? 2 : 3;
			break;
		}
		}
		return c;
	}

		case 2:
		{
			Cell_Idtype c = start;

			//boost::uniform_smallint<> three(0, 2);
			//boost::variate_generator<boost::rand48&, boost::uniform_smallint<> > die3(rng, three);

			//first tests whether p is coplanar with the current triangulation
			if (GeometricTraits<T>::orientation(point(vertex(c, 0)),
				point(vertex(c, 1)),
				point(vertex(c, 2)),
				p) != DEGENERATE) {
				lt = OUTSIDE_AFFINE_HULL;
				li = 3; // only one facet in dimension 2
				return c;
			}
			// if p is coplanar, location in the triangulation
			// only the facet numbered 3 exists in each cell
			while (1) {
				int inf;
				if (Mesh.has_vertex(c, infinite, inf)) {
					// c must contain p in its interior
					lt = OUTSIDE_CONVEX_HULL;
					li = Triangulation_cw_ccw_2::cw(inf);
					lj = Triangulation_cw_ccw_2::ccw(inf);
					return c;
				}

				// else c is finite
				// we test its edges in a random order until we find a
				// neighbor to go further
				//int i = die3();
				int i = 0;
				const T* p0 = point(vertex(c, i));
				const T* p1 = point(vertex(c, Triangulation_cw_ccw_2::ccw(i)));
				const T* p2 = point(vertex(c, Triangulation_cw_ccw_2::cw(i)));
				Orientation o[3];
			
				o[0] = GeometricTraits<T>::coplanar_orientation(p0, p1, p);
				if (o[0] == NEGATIVE) {
					c = Mesh.neighbor(c, Triangulation_cw_ccw_2::cw(i));
					continue;
				}
				o[1] = GeometricTraits<T>::coplanar_orientation(p1, p2, p);
				if (o[1] == NEGATIVE) {
					c = Mesh.neighbor(c, i);
					continue;
				}
				o[2] = GeometricTraits<T>::coplanar_orientation(p2, p0, p);
				if (o[2] == NEGATIVE) {
					c = Mesh.neighbor(c, Triangulation_cw_ccw_2::ccw(i));
					continue;
				}

				// now p is in c or on its boundary
				int sum = (o[0] == COLLINEAR)
					+ (o[1] == COLLINEAR)
					+ (o[2] == COLLINEAR);
				switch (sum) {
				case 0:
				{
					lt = FACET;
					li = 3; // useless ?
					break;
				}
				case 1:
				{
					lt = EDGE;
					li = (o[0] == COLLINEAR) ? i :
						(o[1] == COLLINEAR) ? Triangulation_cw_ccw_2::ccw(i) :
						Triangulation_cw_ccw_2::cw(i);
					lj = Triangulation_cw_ccw_2::ccw(li);
					break;
				}
				case 2:
				{
					lt = VERTEX;
					li = (o[0] != COLLINEAR) ? Triangulation_cw_ccw_2::cw(i) :
						(o[1] != COLLINEAR) ? i :
						Triangulation_cw_ccw_2::ccw(i);
					break;
				}
				}
				return c;
			}
		}
		case 1:
		{
			Cell_Idtype c = start;

			//first tests whether p is collinear with the current triangulation
			if (!GeometricTraits<T>::collinear(p,
				point(vertex(c, 0)),
				point(vertex(c, 1)))) {
				lt = OUTSIDE_AFFINE_HULL;
				return c;
			}
			// if p is collinear, location :
			while (1) {
				if (Mesh.has_vertex(c, infinite)) {
					// c must contain p in its interior
					lt = OUTSIDE_CONVEX_HULL;
					return c;
				}

				// else c is finite
				// we test on which direction to continue the traversal
				switch (GeometricTraits<T>::collinear_position(point(vertex(c, 0)), p, point(vertex(c, 1)))) {


				case AFTER:
					c = Mesh.neighbor(c, 0);
					continue;
				case BEFORE:
					c = Mesh.neighbor(c, 1);
					continue;
				case MIDDLE:
					lt = EDGE;
					li = 0;
					lj = 1;
					return c;
				case SOURCE:
					lt = VERTEX;
					li = 0;
					return c;
				case TARGET:
					lt = VERTEX;
					li = 1;
					return c;
				}
			}
		}

		case 0:
		{
			//Finite_vertices_iterator vit = finite_vertices_begin();
			Vertex_Idtype vit = -1;
			for (size_t i = 0; i < Mesh.num_of_vertices(); i++)
			{
				if (i != infinite&&!Mesh.vertex_is_deleted(i))
				{
					vit = i;
					break;
				}
			}

			if (!GeometricTraits<T>::equal(p, point(vit))) {
				lt = OUTSIDE_AFFINE_HULL;
			}
			else {
				lt = VERTEX;
				li = 0;
			}
			return Mesh.cell(vit);
		}
		case -1:
		{
			lt = OUTSIDE_AFFINE_HULL;
			return -1;
		}
		default:
		{
			return -1;
		}

	}
}


//��һ����p��K������ڵ��Լ����ڽ����incident umbrella
//�ҵ�p�����5�����㣬������nearest�У����������ı�����Ƭ��incident umbrella������IncidentSurfacets,startΪp���ڵ�������
template<typename T, typename T_INFO>
void  SurfaceDelaunay<T, T_INFO>::
K_nearest_vertices(const T* p,list<Vertex_Idtype>& nearest,vector<Facet>& IncidentSurfacets,Cell_Idtype start=-1)
{
	Locate_type lt;
	int li,lj;
	Cell_Idtype c=locate_t(p,lt,li,lj,start);
	vector<Vertex_Idtype> vs;
	vs.reserve(32);
	if(lt==VERTEX)
	{
		//��ĳһ���incident vertex����surface�ϵ�umbrella���������
		//vertex(c,li)�Ǹö��㣬vs��ö���Ĺ������㣬IncidentSurfacets��ö�������ı�����Ƭ
		Mesh.incident_vertex_and_surface_facet(vertex(c,li),std::back_inserter(vs),std::back_inserter(IncidentSurfacets));
		for(auto vsit=vs.begin();vsit!=vs.end();++vsit)
		{
			Mesh.refresh_K_nearest(p,*vsit,nearest);			
		}
	}
	else
	{
		for(int i=0;i<4;i++)
			//���µ�p������б�nearest,���vertex(c,i)��nearest��ĵ����p�������͸���nearest�����򲻸���nearest
			Mesh.refresh_K_nearest(p,vertex(c,i),nearest);  
	
		while(true)
		{
			Vertex_Idtype tmp=*(nearest.begin());
			//��ĳһ���incident vertex����surface�ϵ�umbrella���������
			//tmp�Ǹö��㣬vs��ö���Ĺ������㣬IncidentSurfacets��ö�������ı�����Ƭ
			Mesh.incident_vertex_and_surface_facet(tmp,std::back_inserter(vs),std::back_inserter(IncidentSurfacets));
			for(auto vsit=vs.begin();vsit!=vs.end();++vsit)
			{
				Mesh.refresh_K_nearest(p,*vsit,nearest);			
			}
			if(tmp==*(nearest.begin()))
				break;
			vs.clear();
			IncidentSurfacets.clear();
		
		}
	}


}


template<typename T, typename T_INFO>
Vertex_Idtype  SurfaceDelaunay<T, T_INFO>::
nearest_vertex_in_cell(const T* p,Cell_Idtype c)
{
	Vertex_Idtype nearest=nearest_vertex(p,vertex(c,0),vertex(c,1));
	if(dimension()>2)
	{
		nearest=nearest_vertex(p,nearest,vertex(c,2));
		if(dimension()>3)
			nearest=nearest_vertex(p,nearest,vertex(c,3));
	}
	return nearest;
}


//��p���ڽ�����������ƬInitFacet������������ƬSeedFacet������������,���ڵ�cell��Ӱ������,����������,��mirror facet����cell������Ӱ����
//pΪ����㣬PInsideΪ�������ڱ����λ��
template<typename T, typename T_INFO>
bool SurfaceDelaunay<T, T_INFO>::
seed_surface_facet(const T* p,Facet InitFacet,bool PInside,Facet& SeedFacet)
{
	SeedFacet=Facet(-1,-1);
	if(PInside)
	{
		if(Mesh.is_in_conflict(InitFacet.first))
		{
			SeedFacet=InitFacet;
			return true;
		}
		else  //ʹ��sculpture�����ܻ����һ���ⲿ�����㣩
		{
			//������InitFacet�����surface facet������InitFacet�й�����ı���������Ƭ
			//��ô��Щ������Ƭ����ʲô���򣿾���/����ǣ���inflate/sculpture��˳����ʲô��
			//inflate/sculpture�����ַ�ʽ����flip������һ�������㣬����һ��������
			//�Ľ��󣺲����ð�͹���ʣ�ֱ�ӱȽ϶���ǣ������ʡ��
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
			cosDihedral=Mesh.cos_min_dihedral_point_to_facet(p,InitFacet);
	
			neighborSurfacets.push_back(InitFacet);
			//�洢(������Ƭ,�²��������Ƭ�����������ֵ)����,�����������С�������list��
			tmpSurfacetsDihedral.push_back(make_pair(InitFacet,cosDihedral));
			Mesh.mark_facet_visited(InitFacet);
			//-----------------��InitFacet����ı���������Ƭ������InitFacet�й�����ģ�------------------//
			while(!neighborSurfacets.empty())
			{
				Facet tmpSurfacet=*neighborSurfacets.begin();
				neighborSurfacets.pop_front();
		
				// Look for the other neighbors of c.
				for (int ii = 0; ii<3; ++ii) 
				{					
					Facet surfacetNei=Mesh.neighbor_surfacet(tmpSurfacet,ii);
					//������ƬsurfacetNeiδ������,�����䶥�����ٺ���nearestVertices�еĵ�
					if(!Mesh.visited_for_facet_extractor(surfacetNei)&&Mesh.is_facet_in_neighbor(surfacetNei,nearestVertices))
					{
						Vertex_triple vf=make_vertex_triple(surfacetNei);						
						Orientation o = GeometricTraits<T>::orientation(point(vf.first), point(vf.second), point(vf.third), p);
						//p������,��������Ƭ�ڷ���һ��;p������,��������Ƭ�ⷨ��һ��
						if ((o!=NEGATIVE&&PInside)||(o!=POSITIVE&&!PInside))
						{
							Mesh.mark_facet_visited(surfacetNei);
						
							double cosDihedral;								
		
							cosDihedral=Mesh.cos_min_dihedral_point_to_facet(p,surfacetNei);
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
						}//end of if((o!=NEGATIVE&&PInside)...
					}//end of if(!Mesh.visited_for_facet_extractor(surfacetNei)&&...	
				}//end of for (int ii = 0; ii<3; ++ii)
			}//end of while

			for(auto fit=tmpSurfacetsDihedral.begin();fit!=tmpSurfacetsDihedral.end();fit++)
			{
				Mesh.clear_facet_visited((*fit).first);
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
				Cell_Idtype c=(tmpSurfacet).first;
				int ii=(tmpSurfacet).second;
				vector<Facet> newSurfacets;
				if(Mesh.is_surface(tmpSurfacet))
				{
					////-------------------------for test----------------------//
					//Vertex_triple vft0=Mesh.make_vertex_triple(tmpSurfacet);
					////=========================test end======================//
					
					if(Mesh.is_in_conflict(c))
					{
						SeedFacet=tmpSurfacet;
						return true;
					}
					else //sculpture
					{
						int fIndv[4];////fIndvΪc��ǰ��numOfSurface���洢���Ǳ�����������������numOfNSurface�ǷǱ�����������
						int numOfSurface=0;
						int numOfNSurface=0;//numOfSurface+numOfNSurface=4����
						for(int i=0;i<4;i++)
						{
							if(Mesh.is_surface(Facet(c,i)))
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
							isSingularityEdge=Mesh.is_edge_in_surface(c,fIndv[2],fIndv[3]);
						}

						//if((numOfSurface==2&&!isSingularityEdge&&newEdgeShorter)||(numOfSurface==3&&shorter))//�����Ż�,�˴���flip��������һ����V����Ĺ�����
	                    if(numOfSurface==2&&!isSingularityEdge&&newEdgeShorter)//�����Ż�,�˴���flip��������һ����V����Ĺ�����
						{
							//���,�ѱ����ڵ�c���Ϊ������,NumOfSurfaceΪc�б������
							//fIndvΪc��ǰ��NumOfSurface���洢��Ϊ������������,����fIndv��Ԫ�ظ���һ��Ϊ4
							//newSurfacets��̺������ɱ���							
							Mesh.sculpture(c,numOfSurface,fIndv,newSurfacets);
							for(auto itNS=newSurfacets.begin();itNS!=newSurfacets.end();itNS++)
							{
								Vertex_triple vf=make_vertex_triple(*itNS);						
								Orientation o = GeometricTraits<T>::orientation(point(vf.first), point(vf.second), point(vf.third), p);
								//p������,��������Ƭ�ڷ���һ��;p������,��������Ƭ�ⷨ��һ��
								if ((o!=NEGATIVE&&PInside)||(o!=POSITIVE&&!PInside))
								{
									double cosDihedral;								
		
									cosDihedral=Mesh.cos_min_dihedral_point_to_facet(p,*itNS);
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
								}//end of if((numOfSurface>1&&...
							}//end of for(auto itNS=newSurfacets.begin();...
						
							fit=tmpSurfacetsDihedral.begin();
							isSculpture=true;

						}//end of if((numOfSurface>1&&!isSingularityEdge)...
					}// end of else
				}// end of if(Mesh.is_surface(tmpSurfacet))
			}//end of for(auto fit=tmpSurfacetsDihedral.begin()...
			return false;
			//========================��InitFacet������Ѱ������������Ƭ����sculpture===========================//
		}
		
	}
	else //p������
	{
		if(Mesh.is_in_conflict(Mesh.neighbor(InitFacet.first,InitFacet.second)))
		{
			SeedFacet=InitFacet;
			return true;
		}
		else  //ʹ��inflate�����ܻ����һ���ڲ������㣩
		{
			//������InitFacet�����surface facet,����InitFacet�й�����ı���������Ƭ
			//��ô��Щ������Ƭ����ʲô����?����/�����?��inflate/sculpture��˳����ʲô?
			//InitFacet����İ�͹����?����ʱ�Ծ���ΪԼ����͹ʱ�Զ����ΪԼ��.�����ʱ������nearest_surface_facet()�����еõ��İ�͹����
			//��ʱ���ϸ�����,���ù�����ȵ�˳��,����nearest_surface_facet()�õ��İ�͹����		

			std::list<Facet> neighborSurfacets;
			std::list<std::pair<Facet,double>> tmpSurfacetsDihedral;
			Vertex_triple vf=make_vertex_triple(InitFacet);
			std::list<Vertex_Idtype> nearestVertices;//(vf.first,vf.second,vf.third);
			nearestVertices.push_back(vf.first);
			nearestVertices.push_back(vf.second);
			nearestVertices.push_back(vf.third);
			double cosDihedral;								
			cosDihedral=Mesh.cos_min_dihedral_point_to_facet(p,InitFacet);	
			neighborSurfacets.push_back(InitFacet);
			//�洢(������Ƭ,�²��������Ƭ�����������ֵ)����,�����������С�������list��
			tmpSurfacetsDihedral.push_back(make_pair(InitFacet,cosDihedral));

			Mesh.mark_facet_visited(InitFacet);

			while(!neighborSurfacets.empty())
			{
				Facet tmpSurfacet=*neighborSurfacets.begin();
				neighborSurfacets.pop_front();
			
				for (int ii = 0; ii<3; ++ii) 
				{		
					Facet surfacetNei=Mesh.neighbor_surfacet(tmpSurfacet,ii);
					//������ƬsurfacetNeiδ������,�����䶥�����ٺ���nearestVertices�еĵ�
					if(!Mesh.visited_for_facet_extractor(surfacetNei)&&Mesh.is_facet_in_neighbor(surfacetNei,nearestVertices))
					{
						Vertex_triple vf=make_vertex_triple(surfacetNei);						
						Orientation o = GeometricTraits<T>::orientation(point(vf.first), point(vf.second), point(vf.third), p);
						//p������,��������Ƭ�ڷ���һ��;p������,��������Ƭ�ⷨ��һ��
						if ((o!=NEGATIVE&&PInside)||(o!=POSITIVE&&!PInside))
						{
							Mesh.mark_facet_visited(surfacetNei);
							double cosDihedral;													
							cosDihedral=Mesh.cos_min_dihedral_point_to_facet(p,Mesh.mirror_facet(surfacetNei));
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
						}//end of if ((o!=NEGATIVE&&PInside)||...
					}//end of if(!Mesh.visited_for_facet_extractor(surfacetNei)&&...
				}//end of for(int ii = 0; ii<3; ++ii)
			}//end of while(!neighborSurfacets.empty())

			for(auto fit=tmpSurfacetsDihedral.begin();fit!=tmpSurfacetsDihedral.end();fit++)
			{
				Mesh.clear_facet_visited((*fit).first);
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
				if (!Mesh.is_surface(tmpSurfacet))
				{
					continue;
				}
				vector<Facet> newSurfacets;
				Vertex_triple vtriF=make_vertex_triple(tmpSurfacet);
				if(Mesh.is_surface(tmpSurfacet))
				{
					Cell_Idtype c=(tmpSurfacet).first;
					int ii=(tmpSurfacet).second;
					Cell_Idtype cOpp=Mesh.neighbor(c,ii);
					int iiOpp=Mesh.neighbor_index(cOpp,c);
					if(Mesh.is_in_conflict(cOpp))
					{
						SeedFacet=tmpSurfacet;
						return true;
					}
					else //inflate
					{
						int fIndv[4];//fIndvǰ��numOfSurface���洢����mirror_facetΪ������������������numOfNSurface�ǷǱ�����������
						int numOfSurface=0;
						int numOfNSurface=0;//numOfSurface+numOfNSurface=4����
						int numInsphere=0;
						bool isInSphere=false;
						for(int i=0;i<4;i++)
						{
							if(Mesh.is_surface(Mesh.mirror_facet(Facet(cOpp,i))))
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
							isSingularityEdge=Mesh.is_edge_in_surface(cOpp,fIndv[2],fIndv[3]);
						}


						if(numOfSurface==2&&!isSingularityEdge)
						{
							
							Mesh.inflate(cOpp,numOfSurface,fIndv,newSurfacets);
							
							for(auto itNS=newSurfacets.begin();itNS!=newSurfacets.end();itNS++)
							{
								Vertex_triple vf=make_vertex_triple(*itNS);						
								Orientation o = GeometricTraits<T>::orientation(point(vf.first), point(vf.second), point(vf.third), p);
								if ((o!=NEGATIVE&&PInside)||(o!=POSITIVE&&!PInside))
								{
									double cosDihedral;								
		
									cosDihedral=Mesh.cos_min_dihedral_point_to_facet(p,*itNS);
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
						
						}//end of if((numOfSurface>1&&...
					}//end of else
				}//end of if(Mesh.is_surface(tmpSurfacet))
			}//end of for(auto fit=tmpSurfacetsDihedral.begin();...
			return false;
		}
	}
}














#endif // !SURFACEDELAUNAY_H

