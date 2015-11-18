#include "unionfind.h"

UnionFind::UnionFind(int size) {
  this->size = size;
  this->nodes = new int[size];
  this->weights = new int[size];
  for (int i = 0; i < size; i++) {
    this->nodes[i] = i;
    this->weights[i] = 1;
  }
}

UnionFind::~UnionFind() {
  delete nodes;
  delete weights;
}

int UnionFind::get_root (int node) {
  while (nodes[node] != node) {
    nodes[node] = nodes[nodes[node]];
    node = nodes[node];
  }
  return node;
}

bool UnionFind::find (int node_a, int node_b) {
  return (nodes[node_a] == nodes[node_b]);
}

void UnionFind::do_union (int node_a, int node_b) {
  int root_a = get_root(node_a);
  int root_b = get_root(node_b);
  if (root_a == root_b)
    return;
  
  // Nikdy nezarazuj vetsi strom pod mensi
  if (weights[root_a] < weights[root_b]) {
    nodes[root_a] = nodes[root_b];
    weights[root_b] += weights[root_a];
  } else {
    nodes[root_b] = nodes[root_a];
    weights[root_a] += weights[root_b];
  }
}
/* Test unit
#include <iostream>
void UnionFind::print_nodes() {
  using namespace std;
  for (int i = 0; i < size; i++) {
    cout << nodes[i] << " ";
  }
  cout << endl;
}


int main(void) {
  using namespace std;
  UnionFind *uf = new UnionFind(12);
  uf->print_nodes();
  uf->do_union(2,4);
  uf->do_union(4,6);
  uf->do_union(8,6);
  uf->print_nodes();
  cout << uf->find(2,1) << endl;
  cout << uf->find(2,8) << endl;
} */