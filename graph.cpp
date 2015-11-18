#include "graph.h"

void Graph::create_matrix(int row_count) {
  this->incidence_matrix = new bool*[row_count];
  for (int i = 0; i < row_count; i++)
    incidence_matrix[i] = new bool[row_count];
}

Graph::Graph(int vertex_count) {
  create_matrix(vertex_count);
  for (int i = 0; i < vertex_count; i++)
    for (int j = 0; j < vertex_count; j++)
      this->incidence_matrix[i][j] = false;
  this->vertex_count = vertex_count;
  this->capacity = vertex_count;
}

Graph::Graph(int vertex_count, int initial_capacity) {
  if (initial_capacity < vertex_count)
    initial_capacity = vertex_count;
  create_matrix(initial_capacity);
  for (int i = 0; i < initial_capacity; i++)
    for (int j = 0; j < initial_capacity; j++)
      this->incidence_matrix[i][j] = false;
  this->capacity = initial_capacity;
  this->vertex_count = vertex_count;
}

Graph::Graph(bool** incidence_matrix, int row_count) {
  create_matrix(row_count);
  for (int i = 0; i < row_count; i++)
    for (int j = 0; j < row_count; j++)
      this->incidence_matrix[i][j] = incidence_matrix[i][j];
  this->capacity = row_count;
  this->vertex_count = row_count;
}

Graph::Graph(const Graph &other) {
  this->vertex_count = other.vertex_count;
  this->capacity = other.capacity;
  create_matrix(this->capacity);
  for (int i = 0; i < this->vertex_count; i++)
    for (int j = 0; j < this->vertex_count; j++)
      this->incidence_matrix[i][j] = other.get(i, j);
}

Graph::~Graph() {
  for (int i = 0; i < vertex_count; i++)
    delete(incidence_matrix[i]);
  delete(incidence_matrix);
}

bool Graph::is_connected() const {
  std::stack<int> st;
  bool visited[vertex_count];
  for (int i = 0; i < vertex_count; i++) {
    visited[vertex_count] = false;
  }
  int visited_count = 1;
  
  st.push(0);
  visited[0] = true;
  while (!st.empty()) {
    int current = st.top();
    st.pop();
    for (int other = 0; other < vertex_count; other++) {
      if (incidence_matrix[current][other] && !visited[other]) {
        st.push(other);
        visited[other] = true;
        visited_count++;
      }
    }
  }
  return visited_count == vertex_count;
}

bool Graph::is_sub_connected(int vertices[], int limit) const {
  // You may want to perform some checking
  // You may also want to consider some refactoring as the code
  // is almost identical to is_connected()
  std::stack<int> st;
  bool visited[limit];
  for (int i = 0; i < limit; i++) {
    visited[i] = false;
  }
  int visited_count = 1;
  
  st.push(0);
  visited[0] = true;
  while (!st.empty()) {
    int current = st.top();
    st.pop();
    for (int other = 0; other < limit; other++) {
      if (!visited[other] && incidence_matrix[vertices[current]][vertices[other]]) {
        st.push(other);
        visited[other] = true;
        visited_count++;
      }
    }
  }
  return visited_count == limit;
}


Graph* Graph::create_induced_subgraph(int vertices[], int vertices_size) const {
  Graph *result = new Graph(vertices_size);
  for (int i = 0; i < vertices_size; i++) {
    for (int j = i + 1; j < vertices_size; j++) {
      if (incidence_matrix[vertices[i]][vertices[j]]) {
        result->set(i, j);
      }
    }
  }
  return result;
}

void Graph::remove_cycles() {
  /* Pouzijeme Kruskaluv algoritmus s implementaci vyuzivajici strukturu
   Unionfind. Zde usnadnen tim, ze vsechny hrany maji stejnou vahu. */
  UnionFind *uf = new UnionFind(vertex_count);
  
  // V matici incidence projdeme vsechny hrany
  for (int i = 0; i < vertex_count; i++) {
    for (int j = i; j < vertex_count; j++) {
      if (!get(i, j))
        continue;
      /* Pokud jsme v ramci doposud prozkoumanych hran nenasli cestu
       mezi i a j, poznamenejme si do unionfind struktury, ze nyni
       uz ji mame. V opacnem pripade vyhodime z grafu hranu mezi i a j,
       at nemame cyklus. */      
      if (!uf->find(i, j)) {
        uf->do_union(i, j);
      } else {
        unset(i, j);
      }
    }
  }
}


bool Graph::check_bounds(int row, int col) const {
  return (0 <= row && 0 <= col && vertex_count > row && vertex_count > col);
}

void Graph::set(int row, int col) {
  if (!check_bounds(row, col))
    return;
  this->incidence_matrix[row][col] = true;
  this->incidence_matrix[col][row] = true;
}

void Graph::unset(int row, int col) {
  if (!check_bounds(row, col))
    return;
  this->incidence_matrix[row][col] = false;
  this->incidence_matrix[col][row] = false;
}

bool Graph::get(int row, int col) const {
  if (!check_bounds(row, col))
    return false;
  return this->incidence_matrix[row][col];
}

int Graph::get_vertex_count() const {
  return this->vertex_count;
}

bool Graph::add_vertex() {
  if (this->capacity == this->vertex_count)
    return false;
  this->vertex_count++;
  return true;
}

bool Graph::remove_last_vertex() {
  if (this->vertex_count == 0)
    return false;
  this->vertex_count--;
  return true;
}
  