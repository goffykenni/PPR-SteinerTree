#ifndef QUICK_UNION_H
#define QUIKC_UNION
/* quick_union.h
 * Optimalizovana implementace datove struktury unionfind. Hodi se
 * napriklad pro nalezeni kostry grafu, odstraneni cyklu z grafu
 * a podobne.
 */

class UnionFind {
  private:
    int size;
    int *nodes;
    int *weights;
    
    int get_root(int node);
    
  public:
    bool find(int node_a, int node_b);
    void do_union(int node_a, int node_b);
    UnionFind(int size);
    ~UnionFind();
    void print_nodes();
};
#endif