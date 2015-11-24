#include "checker.h"

using namespace std;

Checker::Checker(const Graph *parent, int *terminal_nodes, int terminal_set_count) {
  this->parent = parent;
  this->parent_size = parent->get_vertex_count();
  
  this->current_vertices = new int[parent_size];
  this->best_vertices = new int[parent_size];
  this->current_hash = new bool[parent_size];
  this->translation = new int[parent_size - terminal_set_count];
  
  // Init current and best sets
  for (int i = 0; i < parent_size; i++) {
    this->best_vertices[i] = i;
    this->current_hash[i] = false;
  }
  for (int i = 0; i < terminal_set_count; i++) {
    this->current_vertices[i] = terminal_nodes[i];
    this->current_hash[terminal_nodes[i]] = true;
  }
  
  int shift = 0;
  for (int i = 0; i < parent_size - terminal_set_count; i++) {
    translation[i] = i > 0 ? translation[i - 1] + 1: i;
    while (current_hash[translation[i]])
      translation[i]++;
  }
  
  this->current_size = terminal_set_count;
  this->best_size = this->parent_size;
  this->global_best_size = this->parent_size;
  
  this->min_price = terminal_set_count;  
}

Checker::~Checker() {
  if (current_vertices != 0L)
    delete(current_vertices);
  if (best_vertices != 0L)
    delete(best_vertices);
  if (translation != 0L)
    delete(translation);
}

void Checker::update_context(int best_size_info, int* context, int context_size) {
  // Aktualizuj si informaci o velikosti nejlepsiho reseni
  if (best_size_info < global_best_size)
    global_best_size = best_size_info;
  
  // Nejprve je nutno odstranit aktualni kontext.
  for (int i = 0; i < current_size - min_price; i++)
    remove_last_vertex();
  // Nasledne narhaj novy kontext.
  for (int i = 0; i < context_size; i++)
    add_vertex(context[i]);
}

bool Checker::add_vertex(int n) {
  if (n < 0 || parent_size <= n - min_price) {
    cout << "Checker::add_vertex: Index (" << n <<") outside of range\n";
    return false;
  }
  if (current_size == parent_size) {
    cout << "Checker::add_vertex: Capacity is full.\n";
    return false;
  }
  
  int translated = translation[n];
  //cout << n << " : " << translated << endl;
  current_vertices[current_size++] = translated;
  current_hash[translated] = true;
  return true;
}

bool Checker::remove_last_vertex() {
  if (current_size == 0) {
    cout << "Checker::remove_last_vertex: Trying to remove from an empty subgraph.";
    return false;
  }
  /* No need to set anything in current_vertices as long as you
   * promise to never pass the current_size */
  current_size--;
  current_hash[current_vertices[current_size]] = false;
  return true;
}

bool Checker::remove_last(int count) {
  if (count < 0) {
    cout << "Checker::remove_last: Trying to remove negative number of vertices";
  }
  if (current_size - count + 1 == 0) {
    cout << "Checker::remove_last: Trying to remove more than currently have.";
    return false;
  }
  
  for (int i = 0; i < count; i++) {
    current_size--;
    current_hash[current_vertices[current_size]] = false;
  }
  return true;  
}

bool Checker::process_current_state() {
  if (current_size < global_best_size &&
      parent->is_sub_connected(current_vertices, current_size)) {
    // Update best
    for (int i = 0; i < current_size; i++) {
      best_vertices[i] = current_vertices[i];
    }
    best_size = current_size;
    global_best_size = current_size;
    
    return true;    
  }
  return false;
}
