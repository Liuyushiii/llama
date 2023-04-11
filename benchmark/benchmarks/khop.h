#ifndef LL_KHOP_H
#define LL_KHOP_H

#include "llama/ll_khop_template.h"


template <class Graph>
class khop_khop : public ll_khop_template
	<Graph, short>
{
	public: 
		khop_khop(Graph& _G, node_t& _root, int32_t& _k)
		    : ll_khop_template<Graph, short>(_G),
			G(_G), root(_root), k(_k){}
	// node_t root;


	private:
	    Graph& G;
		node_t& root;
		int32_t& k;
};

template <class Graph>
class ll_b_khop : public ll_benchmark<Graph> {

	node_t root;
	int k;


public:

	/**
	 * Create the benchmark
	 *
	 * @param r the root
	 */
	ll_b_khop(node_t r, int khops)
		: ll_benchmark<Graph>("Khop - Query") {

		root = r;
		k = khops;
	}


	/**
	 * Destroy the benchmark
	 */
	virtual ~ll_b_khop(void) {
	}


	/**
	 * Run the benchmark
	 *
	 * @return the numerical result, if applicable
	 */
	virtual double run(void) {

		std::cout << "you are running "<< k << "-hop query targeted at "<< root << std::endl;

		Graph& G = *this->_graph;
		int32_t k = 1;

		khop_khop<Graph> _KHOP(G, root, k);

		_KHOP.khop_test();
		_KHOP.iterate_neighbor_small(root);

		return 123; 
	}
};

#endif