#ifndef CHECKER_H
#define CHECKER_H
/* solution_checker.h
 * Trida jejiz objekty slouzi jako kontejnery pro uchovani
 * aktualnich stavu a jejich overovani pri reseni problemu minimalniho
 * Steinerova stromu. */
#include "graph.h"
#include <iostream>

class Checker {
  private:
    const Graph *parent;
    int parent_size;
    
    int *current_vertices;
    int current_size;
    bool *current_hash;
    int *best_vertices;
    int best_size;
    
    // Tesna dolni mez pro danou ulohu
    int min_price;
  
  public:
    Checker(const Graph *parent, int *terminal_nodes, int terminal_set_count);
    ~Checker();
    
    bool add_vertex(int n);
    bool remove_last_vertex();
    bool process_current_state();
    
    // Helper methods
    int get_parent_size() const {
      return parent_size;
    }
    
    bool can_adding_help() const {
      return best_size - current_size > 1;
    }
    
    bool contains_vertex(int n) const {
      return current_hash[n];
    }
    
    int get_current_size() const {
      return current_size;
    }
    
    int* get_current_vertices() {
      return current_vertices;
    }
    
    int get_best_size() const {
      return best_size;
    }
    
    int* get_best_vertices() {
      return best_vertices;
    }
};

#endif