#include "checker.h"

Checker::Checker(const Graph *parent, int *terminal_nodes, int terminal_set_count) {
  this->parent = parent;
  this->parent_size = parent->get_vertex_count();
  
  this->current_vertices = new int[parent_size];
  this->best_vertices = new int[parent_size];
  this->current_hash = new bool[parent_size];
  
  // Init current and best sets
  for (int i = 0; i < parent_size; i++) {
    this->best_vertices[i] = i;
    this->current_hash[i] = false;
  }
  for (int i = 0; i < terminal_set_count; i++) {
    this->current_vertices[i] = terminal_nodes[i];
    this->current_hash[terminal_nodes[i]] = true;
  }
  this->current_size = terminal_set_count;
  this->best_size = this->parent_size;
  
  this->min_price = terminal_set_count;  
}

Checker::~Checker() {
  if (current_vertices != 0L)
    delete(current_vertices);
  if (best_vertices != 0L)
    delete(best_vertices);
}

bool Checker::add_vertex(int n) {
  if (n < 0 || parent_size <= n) {
    // ERROR: Index outside of range
    return false;
  }
  if (current_size == parent_size) {
    // ERROR: Capacity full
    return false;
  }
  
  current_vertices[current_size++] = n;
  current_hash[n] = true;
  return true;
}

bool Checker::remove_last_vertex() {
  if (current_size == 0) {
    // ERROR: subgraph empty.
    return false;
  }
  /* No need to set anything in current_vertices as long as you
   * promise to never pass the current_size */
  current_size--;
  current_hash[current_vertices[current_size]] = false;
  return true;
}

bool Checker::process_current_state() {
  if (current_size < best_size &&
      parent->is_sub_connected(current_vertices, current_size)) {
    
    // Update best
    for (int i = 0; i < current_size; i++) {
      best_vertices[i] = current_vertices[i];
    }
    best_size = current_size;
    
    return true;    
  }
  return false;
}
