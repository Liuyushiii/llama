#ifndef LL_KHOP_TEMPLATE_H
#define LL_KHOP_TEMPLATE_H


template<class Graph, typename level_t>
class ll_khop_template{
    public:
        ll_khop_template(Graph& _G) :
                G(_G) {
            // TODO
        }

        void khop_test(){
            std::cout << "=====hello=====" << std::endl;
        }

    node_t get_node(ll_edge_iterator& iter) {
		return iter.last_node;
    }

    edge_t iter_next(ll_edge_iterator& iter) {
        return G.out_iter_next(iter);
    }

	void iter_begin(ll_edge_iterator& iter, node_t v) {
        G.out_iter_begin(iter, v);
    }

    void iterate_neighbor_small(node_t t) {

        std::cout << "check the neighbor of "<< t << std::endl; 

		ll_edge_iterator iter; iter_begin(iter, t);
		for (edge_t nx = iter_next(iter); nx != LL_NIL_EDGE; nx = iter_next(iter)) {
            node_t u = get_node(iter);

            std::cout << u << std::endl;
        }
    }
    private:
        level_t curr_level;
        node_t root;
        Graph& G;
};

#endif