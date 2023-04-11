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

    private:
        level_t curr_level;
        node_t root;
        Graph& G;
};

#endif