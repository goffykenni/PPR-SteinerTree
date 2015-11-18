#ifndef GRAPH_H
#define GRAPH_H
/*
 * graph.h
 * Trida modelujici konecny neorientovany graf.
 * Trida jej representuje pomoci matice incidence a je mutable -
 * hrany mezi vrcholy je mozne odebirat i pridavat. Pri konstrukci
 * je dokonce mozne nastavit max. kapacitu grafu, do jejiz hodnoty
 * lze do grafu pridavat nove vrcholy - to se hodi pri pouziti
 * teto tridy pro reseni ulohy Steinerova stromu.
 */
#include <stack>
#include "unionfind.h"

class Graph {
  private:
    // Matice incidence
    bool** incidence_matrix;
    /* Pocet vrcholu v grafu. Muze byt mensi nez dimenze matice
     incidence. V podstate jde o "pocet aktivnich uzlu" v matici incidence */
    int vertex_count;
    // Cislo odpovidajici dimenzi matice incidence.
    int capacity;
    
    void create_matrix(int row_count);
    bool check_bounds(int row, int col) const;
  public:
    /* Vytvori novy graf o danem poctu vrcholu a zadnymi hranami.
     kapacita bude nastavena na pocet vrcholu, takze do tohoto grafu
     nepujde pridavat dalsi uzly. */
    Graph(int vertex_count);    
    /* Vytvori novy graf o danem poctu uzlu, zadnymi hranami a
     danou kapacitou. Musi platit
     vertext_count <= initial_capacity */
    Graph(int vertex_count, int initial_capacity);
    /* Vytvori novy graf z jiz existujici matice incidence. Do vytvareneho
     objektu se ulozi kopie predane matice. */  
    Graph(bool** incidence_matrix, int dimension);
    // Kopirovaci konstrukor
    Graph(const Graph &other);
    ~Graph();
    
    /* Vraci true, pokud je dany graf souvisly. Metoda pracuje v
     case O(V^2), kde V je pocet vrcholu. Lepsi casove slozitosti
     by se dalo dosahnout volbou jine grafove representace, napr.
     ajdacency list. */
    bool is_connected() const;
    
    /* Vraci true, pokud je podgraf indukovany danou mnozinou vrcholu
    souvisly */
    bool is_sub_connected(int vertices[], int limit) const;
    
    Graph* create_induced_subgraph(int vertices[], int vertices_size) const;
    
    /* Vraci true, pokud se na pozici (row, col) v matici
     incidence nachazi jednicka, tj. jsou-li odpovidajici vrcholy
     spojeny primo hranou. Jinak vraci false.*/
    bool get(int row, int col) const;
    /* Nastavi pozici (row, col) v matici incidence na jednicku, tzn.
     spoji odpovidajici vrcholy hranou. */
    void set(int row, int col);
    /* Nastavi pozici (row, col) v matici incidence na nulu, tzn.
     smaze hranu mezi odpovidajicimi vrcholy */
    void unset(int row, int col);
    // Vrati pocet vrcholu v grafu
    int get_vertex_count() const;
    
    /* Odstrani z grafu cykly. Pokud je graf souvisly, bude vysledkem
     strom */
    void remove_cycles();
    
    bool add_vertex();
    bool remove_last_vertex();
};

#endif
