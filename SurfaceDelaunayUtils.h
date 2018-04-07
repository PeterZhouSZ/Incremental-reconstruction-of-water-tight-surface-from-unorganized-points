// referenced by CGAL

#ifndef SURFACEDELAUNAYUTILS
#define SURFACEDELAUNAYUTILS


template < class T = void >
struct Triangulation_cw_ccw_static_2 {

	static const int ccw_map[3];
	static const int cw_map[3];
};
template < class T >
const int Triangulation_cw_ccw_static_2<T>::ccw_map[3] = { 1, 2, 0 };

template < class T >
const int Triangulation_cw_ccw_static_2<T>::cw_map[3] = { 2, 0, 1 };

class Triangulation_cw_ccw_2
	: public  Triangulation_cw_ccw_static_2<>
{
public:
	static int ccw(const int i)
	{
		return ccw_map[i];
	}

	static int cw(const int i)
	{

		return cw_map[i];
	}
};

	// We use the following template class in order to avoid having a static data
	// member of a non-template class which would require src/Triangulation_3.C .
	template < class T = void >
	struct Triangulation_utils_base_3
	{
		static const char tab_next_around_edge[4][4];
		static const int tab_vertex_triple_index[4][3];
		static const int tab_cell_index_in_triple_index[4][4];
	};

	template < class T >
	const char Triangulation_utils_base_3<T>::tab_next_around_edge[4][4] = {
			{ 5, 2, 3, 1 },
			{ 3, 5, 0, 2 },
			{ 1, 3, 5, 0 },
			{ 2, 0, 1, 5 } };

	template < class T >
	const int Triangulation_utils_base_3<T>::tab_vertex_triple_index[4][3] = {
			{ 1, 3, 2 },
			{ 0, 2, 3 },
			{ 0, 3, 1 },
			{ 0, 1, 2 }
	};

	template < class T >
	const int Triangulation_utils_base_3<T>::tab_cell_index_in_triple_index[4][4] = {
			{ 5, 0, 0, 0 },
			{ 0, 5, 2, 1 },
			{ 2, 1, 5, 2 },
			{ 1, 2, 1, 5 } };
	// We derive from Triangulation_cw_ccw_2 because we still use cw() and ccw()
	// in the 2D part of the code.  Ideally, this should go away when we re-use
	// T2D entirely.

	struct Triangulation_utils_3
		: public Triangulation_cw_ccw_2,
		public Triangulation_utils_base_3<>
	{
		//i-j-return�Ķ���-ʣ��Ķ���,�������ֶ���;���ߣ������������򣬴�Ĵָ����Ϊ�����i->j����return��-ʣ�����������ָ����һ��
		static int next_around_edge(const int i, const int j)
		{
			// index of the next cell when turning around the
			// oriented edge vertex(i) vertex(j) in 3d
			/*CGAL_triangulation_precondition((i >= 0 && i < 4) &&
				(j >= 0 && j < 4) &&
				(i != j));*/
			return tab_next_around_edge[i][j];
		}


		//iΪ0/1/2/3Ϊ��������Զ�����,j(0/1/2)��Ķ����ţ���Ķ��㰴��ʱ���ţ��۲췽��Ϊ���������򣩣�����i��Ե����j�Ŷ��㣬�ڸ�������ı��
		static int vertex_triple_index(const int i, const int j)
		{
			// indexes of the  jth vertex  of the facet of a cell
			// opposite to vertx i
			/*CGAL_triangulation_precondition((i >= 0 && i < 4) &&
				(j >= 0 && j < 3));*/
			return tab_vertex_triple_index[i][j];
		}

		//i,j��������Ա��(0/1/2/3),
		//�������j����,i�ڸ����еı��
		static int cell_index_in_triple_index(const int i, const int j)
		{
			return tab_cell_index_in_triple_index[i][j];
		}

	};



#endif // SURFACEDELAUNAYUTILS
