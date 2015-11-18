#include <iostream>
#include <fstream>
#include <stack>
#include <utility>
#include "graph.h"
#include "solution_checker.h"


void print_graph(Graph*);
void solve(Checker*);
int read_input(Graph*& , int*&, int&, char* input_file);

int main(int argc, char *argv[]) {
  Graph *g; int *terminal_set;
  int terminal_set_size;
  if (argc != 2 || read_input(g, terminal_set, terminal_set_size,
    argv[1]) != 0) {
    printf("Program error");
    return 1;
  }

  /* Vytvoreni zadani ulohy podle vstupu */  
  Checker *checker = new Checker(g, terminal_set, terminal_set_size);
  solve(checker);
  checker->print_parent();
  checker->remove_cycles_from_best();
  checker->print_best();
  delete(checker);
  if (g != NULL)
    delete(g);
  if (terminal_set != NULL)
    delete(terminal_set);  
  /* Vytvoreni zadani ulohy podle vstupu - konec */
}

/* Nalezne nejmensi podgraf (z hlediska poctu vrcholu) daneho grafu
 * obsahujici uzly z cilove mnoziny, ktery je souvisly. Idea spociva
 * v tom systematicky overovat vsechny mozne kombinace vrcholu, ktere
 * lze do grafu pridavat. */
void solve(Checker *checker) {
  using namespace std; 
  int size = checker->get_parent_size();
  stack< pair<int, int> > st;
  
  // Zkusime, jestli nahodou sama cilova mnozina nevyhovuje uloze.
  if (checker->process_current_state())
    return;
  // Na zasobnik umistime vychozi stavy
  for (int i = size - 1; i >= 0; i--) {
    // Uzly, co jsou v cilove mnozine, tam nedame
    if (!checker->contains_vertex(i))
      st.push(pair<int, int>(i, 1));
  }
  
  
  while (!st.empty()) {    
    // Vyjmi uzel ze zasobniku a zacni ho zpracovavat.
    pair<int, int> current = st.top();
    st.pop();
    /* Pokud pridani tohoto uzlu nevede ke zlepseni vysledku, do grafu
     ho nepridame, ze zasobniku odstranime vsechny jeho sourozence
     a jeho rodice odebereme z grafu. */
    if (!checker->can_be_better()) {
      /* Na zasobniku mohou v tuto chvili byt sourozenci aktualniho
       uzlu (nepovinne) a za nim sourozenec jeho rodice (opet nepovinne).
       */
       
      // Vyhod ze zasobniku vsechny sourozence - netreba je jiz overovat.
      while (!st.empty() && current.second == st.top().second) {
        st.pop();
      }
      
      /* Pokud ma rodic akt. uzlu sourozence, bude cyklus jeste
       pokracovat, proto z grafu odstranime rodice akt. uzlu. */
      if (!st.empty())
        checker->remove_last_vertex();
      continue;
    } 
    
    // Pridani uzlu muze vest ke zlepseni, tak to vyzkousejme
    checker->add_vertex(current.first);
    checker->process_current_state();
    
    /* Potomky akt. uzlu pridame na zasobnik. Nepridavame pritom ty uzly,
     ktere representuji takove vrcholy, ktere v grafu uz pridane. Navic
     pridavame jen vzdy uzly s vetsim cislem nez ma aktualni, abychom
     se vyhnuli duplicitnim stavum. */
    int children_count = 0;
    for (int i = size - 1; i >= current.first + 1; i--) {
      if (!checker->contains_vertex(i)) {
        st.push(pair<int, int>(i, current.second + 1));
        children_count++;
      }
    }
    /* Pokud uzel nemel potomky, uz tuto cestu nelze dal expandovat
     a musime ho odstranit z grafu. */
    if (children_count == 0) {
      checker->remove_last_vertex();
      /* Pokud tento uzel nema sourozence, tak musime z grafu odebrat
       i jeho rodice. Tim se dostane na vrchol zasobniku pro dalsi
       iteraci cyklu sourozenec rodice (pokud existuje). */
      if (!st.empty() && current.second >= st.top().second)
        checker->remove_last_vertex();
    }
  }
}

int read_input(Graph*& g, int*& terminal_set, int& terminal_set_size,
  char *input_file) {
  bool** matice;
  
  std::ifstream in(input_file);
  if (!in) {
    std::cout << "Error opening file.";
    return 1;
  }
  
  int dim;
  in >> dim;
  in >> terminal_set_size;
  
  terminal_set = new int[terminal_set_size];  
  matice = new bool*[dim];
  for (int i = 0; i < dim; i++) {
    matice[i] = new bool[dim];
  }
  
  int vertex;
  for (int i = 0; i < terminal_set_size; i++) {    
    in >> vertex;
    terminal_set[i] = vertex;
  }
  
  int row = 0, col = 0;
  char act;
  while(in >> act) { 
    matice[row][col] = act - '0';
    col++;
    if (col == dim) {
      col = 0; row++;
    }
  }  
  in.close();
  
  g = new Graph(matice, dim);
  return 0;
}

void print_graph(Graph *g) {
   for (int i = 0; i < g->get_vertex_count(); i++) {
    std::cout << std::endl;
    for (int j = 0; j < g->get_vertex_count(); j++) {
      std::cout << g->get(i, j) << " ";
    }
   }
   std::cout << std::endl;
}

