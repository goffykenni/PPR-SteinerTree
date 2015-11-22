#include <iostream>
#include <fstream>
#include "stack.h"
#include <utility>
#include "graph.h"
#include "checker.h"
#include <string>
#include "mpi.h"


void print_graph(Graph*, int *label, int label_size);
void solve(Graph*, int*, int);
int read_input(Graph*& , int*&, int&, char* input_file);
void initial_work_distribution(Graph*, int*, int, Checker*&, Stack*&);
void accept_work(void*, int, Stack*&, Checker*&);

// States
enum State {INIT, WORKING, DONE};

// Message tags
const int MSG_WORK_REQUEST = 1001;
const int MSG_WORK_SENT = 1002;
const int MSG_WORK_EMPTY = 1003;
const int MSG_TOKEN = 1004;
const int MSG_TERMINATE = 1005;

// Vypocetni konstanty
// Musi byt aspon 0
const int cut_height = 1;

// Global variables
int my_rank;
int total_proc;
int total_working_proc;
MPI_Status status;
State my_state;

struct Solution {
  int size;
  int rank;  
} my_solution;
/*             _{\ _{\{\/}/}/}__
             {/{/\}{/{/\}(\}{/\} _
            {/{/\}{/{/\}(_)\}{/{/\}  _
         {\{/(\}\}{/{/\}\}{/){/\}\} /\}
        {/{/(_)/}{\{/)\}{\(_){/}/}/}/}
       _{\{/{/{\{/{/(_)/}/}/}{\(/}/}/}
      {/{/{\{\{\(/}{\{\/}/}{\}(_){\/}\}
      _{\{/{\{/(_)\}/}{/{/{/\}\})\}{/\}
     {/{/{\{\(/}{/{\{\{\/})/}{\(_)/}/}\}
      {\{\/}(_){\{\{\/}/}(_){\/}{\/}/})/}
       {/{\{\/}{/{\{\{\/}/}{\{\/}/}\}(_)
      {/{\{\/}{/){\{\{\/}/}{\{\(/}/}\}/}
       {/{\{\/}(_){\{\{\(/}/}{\(_)/}/}\}
         {/({/{\{/{\{\/}(_){\/}/}\}/}(\}
          (_){/{\/}{\{\/}/}{\{\)/}/}(_)
            {/{/{\{\/}{/{\{\{\(_)/}
             {/{\{\{\/}/}{\{\\}/}
              {){/ {\/}{\/} \}\}
              (_)  \.-'.-/
          __...--- |'-.-'| --...__
   _...--"   .-'   |'-.-'|  ' -.  ""--..__
 -"    ' .  . '    |.'-._| '  . .  '   jro
 .  '-  '    .--'  | '-.'|    .  '  . '
          ' ..     |'-_.-|
  .  '  .       _.-|-._ -|-._  .  '  .
              .'   |'- .-|   '.
  ..-'   ' .  '.   `-._.-´   .'  '  - .
   .-' '        '-._______.-'     '  .
        .      ~,  */

int main(int argc, char *argv[]) {

  MPI_Init(&argc, &argv);
  // Zjisti moje id
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  // Zjisti pocet procesoru celkem
  MPI_Comm_size(MPI_COMM_WORLD, &total_proc);
  
  Graph *g; int *terminal_set;
  int terminal_set_size;
  if (argc != 2 || read_input(g, terminal_set, terminal_set_size,
    argv[1]) != 0) {
    if (my_rank == 0)
      std::cout << "Program error, missing name of input file." << std::endl;
    return 1;
  }
  
  

  /* Vytvoreni zadani ulohy podle vstupu */  
  solve(g, terminal_set, terminal_set_size);
  
  if (g != NULL)
    delete(g);
  if (terminal_set != NULL)
    delete(terminal_set);  
  
  
  
  MPI_Finalize();
  return 0;
}

int min(int a, int b) {
  return a < b ? a : b;
}

/* Prijme praci z dane zpravy */
void accept_work(void* message, int message_len, Stack*& st, Checker*& checker) {
  using namespace std;
  if (st == NULL || checker == NULL) {
    cout << "P" << my_rank << ": Error (accept_work): My stack or checker are NULL!"
      << endl;
    return;
  }
  if (!st->is_empty()) {
    cout << "P" << my_rank << ": Error (accept_work): My stack is not empty!";
    return;
  }
  
  /* Nacti data ze zpravy */
  int* data = (int*) message;
  int stack_top = data[0];
  int frame_padding = data[1];
  int best_size = data[2];
  int context_len = data[3];
  
  int context[context_len];
  for (int i = 0; i < context_len; i++) {
    context[i] = data[4 + i];
  }
  /* Dej ramec na zasobnik a aktualizuj kontext. */
  st->pushTop(context_len + 1, stack_top, frame_padding);
  checker->update_context(best_size, context, context_len);  
}

/* Provede inicialni distribuci prace, kdy P0 naposila ostatnim procesum
 * ramce sveho zasobniku z prvni a druhe urovne sveho vlastniho zasobniku */
void initial_work_distribution(Graph* graph , int* terminal_set, int terminal_set_size,
    Checker*& checker, Stack*& st) {
  using namespace std;
  
  /* Vytvor pro vsechny vlastni inicialni kontext (checker) a prazdny
   * zasobnik */  
  checker = new Checker(graph, terminal_set, terminal_set_size);
  st = new Stack();
  
  int size = graph->get_vertex_count() - terminal_set_size;
  /* P0 distribuuje praci */
  if (my_rank == 0) {
    my_state = WORKING;
    
    /* Zkus, jestli nevyhovuje trivialni reseni. Pokud ano, vsechny
    ostatni preved do stavu DONE, problem uz nemusi resit. */
    if (checker->process_current_state()) {
      cout << "P0: Everybody stop! Trvial solution works." << endl;
      for (int i = 1; i < total_proc; i++) {
        MPI_Send(&i, 1, MPI_INT, i, MSG_TERMINATE, MPI_COMM_WORLD);
      }
      return;
    }
    // Pocet jiz obslouzenych procesoru (z mnoziny P1, P2, ...)
    int served = 0;
    // Kolik prace representovane ramci zasobniku muze P0 dat
    int frames_available = size - cut_height - 1;    
    
    /* Sestavi zpravu, ktera se bude posilat, viz komentar pred funkci
     * main pro jeji obecny format. */    
    int message_len = 5; int message[message_len];
    /* Posilane ramce nebudou mit padding, reseni lepsi nez cely graf
     * zatim nezname a pokud ty procesy, ktere dostanou ramec z druhe
     * urovne zasobniku P0 ho dostanou jako potomka uzlu 0 */
    message[1] = 0; message[2] = size; message[4] = 0;
    /* Kolik ramcu se bude posilat */
    int giveaway = min(total_proc - 1, frames_available);
    cout << "P0 will give away " << giveaway << " frames." << endl;
    bool from_first = false;
    int level_one_pos = 0;
    int level_two_pos = 0;
    
    /* Posilej praci ostatnim procesum. Ramce odebirej stridave ze sve
    druhe a prvni urovne zasobniku */
    for (int i = 0; i < giveaway; i++) { 
      /* Dej ramec z prvni urovne */     
      if (from_first) {
        message[0] = level_one_pos + 2;
        message[3] = 0;
        level_one_pos++;        
      }
      /* Dej ramec z druhe urovne */
      else {
        message[0] = level_two_pos + 1;
        message[3] = 1;
        level_two_pos++;        
      }
      /* Posli zpravu */
      MPI_Send(&message, message_len, MPI_INT, served + 1, MSG_WORK_SENT, MPI_COMM_WORLD);
      from_first = !from_first;
      served++;
    }    
    
    /* Uloz pocet procesoru, kteri budou mit praci (vcetne tohoto, tj. P0) */
    total_working_proc = served + 1;
    cout << "Total working processors: " << total_working_proc << endl;
    
    // Ukonci ty procesory, na ktere se prace nedostala
    for (int i = served + 1; i < total_proc; i++) {
      MPI_Send(&message, message_len, MPI_INT, i, MSG_TERMINATE, MPI_COMM_WORLD);
    }
    
    /* P0 rozvine svuj vlastni zasobnik podle toho, kolik toho daroval. */
    for (int i = size - 1; i >= level_one_pos + 2; i--) {
      st->pushTop(1, i, 0);      
    }
    if (size > 1)
      st->pushTop(1, 1, 0);
    st->pushTop(1, 0, level_two_pos);
  } 
  /* Ostatni procesory cekaji na zpravu */
  else {
    int message_len = 5; int message[message_len];
    MPI_Recv(&message, message_len, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    switch (status.MPI_TAG) {
      case MSG_WORK_SENT:
        accept_work(&message, message_len, st, checker);
        my_state = WORKING;
        break;
      case MSG_TERMINATE:
      default:
        my_state = DONE;
        break;
    }
  }
}

/* Nalezne nejmensi podgraf (z hlediska poctu vrcholu) daneho grafu
 * obsahujici uzly z cilove mnoziny, ktery je souvisly. Idea spociva
 * v tom systematicky overovat vsechny mozne kombinace vrcholu, ktere
 * lze do grafu pridavat. */
void solve(Graph *graph, int *terminal_set, int terminal_set_size) {
  using namespace std;
  my_state = INIT;
  Checker *checker;
  Stack *st;
  int size = graph->get_vertex_count() - terminal_set_size;
  //int parent_size = checker->get_parent_size();
  
  /* Pocatecni rozdeleni prace mezi procesory */
  initial_work_distribution(graph, terminal_set, terminal_set_size, checker, st); 
  cout << "P" << my_rank << " stack: ";
  st->Print();
  cout << "P" << my_rank << " context: ";
  checker->print_context();
  
  /*if (my_rank != 0)
    return;  */
  /*
  checker = new Checker(graph, terminal_set, terminal_set_size);
  st = new Stack();
  
  // Zkusime, jestli nahodou sama cilova mnozina nevyhovuje uloze.
  if (checker->process_current_state())
    return;
  // Na zasobnik umistime vychozi stavy
  for (int i = size - 1; i >= 0; i--) {
    // Uzly, co jsou v cilove mnozine, tam nedame
    //if (!checker->contains_vertex(i))
      st->pushTop(1, i, 0);
    }
  st->Print();
  */
 
 
  while (!st->is_empty()) {
    if (st->is_empty()) {
      my_state = DONE;
      continue;
    }       
    // Vyjmi uzel ze zasobniku a zacni ho zpracovavat.
    StFrame current = st->grabTop();
    
    /* Pokud pridani tohoto uzlu nevede ke zlepseni vysledku, do grafu
     ho nepridame, ze zasobniku odstranime vsechny jeho sourozence
     a jeho rodice odebereme z grafu. */
    if (!checker->can_adding_help()) {
      /* Na zasobniku mohou v tuto chvili byt sourozenci aktualniho
       uzlu (nepovinne) a za nim sourozenec jeho rodice (opet nepovinne).
       */
      // Vyhod ze zasobniku vsechny sourozence - netreba je jiz overovat.
      while (!st->is_empty() && current.level == st->checkTop().level) {
        st->grabTop();
      }
      
      /* Pokud ma rodic akt. uzlu sourozence, bude cyklus jeste
       pokracovat, proto z grafu odstranime rodice akt. uzlu. */
      if (!st->is_empty())
        checker->remove_last_vertex();
      continue;
    } 
    
    // Pridani uzlu muze vest ke zlepseni, tak to vyzkousejme
    checker->add_vertex(current.value);
    checker->process_current_state();

    /* Potomky akt. uzlu pridame na zasobnik. Nepridavame pritom ty uzly,
     ktere representuji takove vrcholy, ktere v grafu uz pridane. Navic
     pridavame jen vzdy uzly s vetsim cislem nez ma aktualni, abychom
     se vyhnuli duplicitnim stavum. */
    int children_count = 0;
    for (int i = size - 1; i >= current.value + 1 + current.padding; i--) {
      //if (!checker->contains_vertex(i)) {
        st->pushTop(current.level + 1, i, 0);
        children_count++;
      //}
    }
    /* Pokud uzel nemel potomky, uz tuto cestu nelze dal expandovat
     a musime ho odstranit z grafu. */
    if (children_count == 0) {
      //cout << "nema deti";
      checker->remove_last_vertex();
      /* Pokud tento uzel nema sourozence, tak musime z grafu odebrat
       i jeho rodice. Tim se dostane na vrchol zasobniku pro dalsi
       iteraci cyklu sourozenec rodice (pokud existuje). */
      if (!st->is_empty() && current.level >= st->checkTop().level)
        checker->remove_last_vertex();
    }
  }
  cout << "P" << my_rank << ": best = " << checker->get_best_size() << endl;
  
  /* Provedeni paralelni binarni redukce za ucelem zjisteni nejlepsiho reseni.
   * Vysledkem redukce je velikost nejmensiho reseni a nejmensi index procesu, ktery
   * jej nasel. Tato dvojice cisel se broadcastne vsem a ten proces, ktery teto
   * dvojici odpovida, reseni zkonstruuje a vypise, zatimco ostatni mohou koncit. */
  my_solution.size = checker->get_best_size();
  my_solution.rank = my_rank;
  Solution out;
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Reduce(&my_solution, &out, 1, MPI_2INT, MPI_MINLOC, 0, MPI_COMM_WORLD); 
  MPI_Bcast(&out, 1, MPI_DOUBLE_INT, 0, MPI_COMM_WORLD);
  /* Ten, kdo ma nejlepsi reseni, pripravi ho a vytiskne. Ostatni muzou
   * koncit. */  
  if (my_solution.rank == out.rank) {
    // Priprav a tiskni reseni
    cout << "P" << out.rank << " has best solution" << endl;
    Graph *r = graph->create_induced_subgraph(checker->get_best_vertices(),
    checker->get_best_size());
    r->remove_cycles();
    print_graph(r, checker->get_best_vertices(), checker->get_best_size());
    delete(r);
    /*
    cout << endl;
    int *pole = new int[8];
    pole[0] = 1; pole[1] = 4; pole[2] = 9;
    pole[3] = 15; pole[4] = 21; pole[5] = 0;
    pole[6] = 13; pole[7] = 23;
    Graph *g = graph->create_induced_subgraph(pole, 8);
    g->remove_cycles();
    print_graph(g, pole, 8);*/
  }    
  delete(st);
  delete(checker);
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

void print_graph(Graph *g, int *label, int label_size) {
  using namespace std;
  for (int i = 0; i < label_size; i++) {
    cout << label[i] << " ";
  }
  cout << endl << "-------------------------";
  for (int i = 0; i < g->get_vertex_count(); i++) {
    std::cout << std::endl;
    for (int j = 0; j < g->get_vertex_count(); j++) {
      std::cout << g->get(i, j) << " ";
    }
  }
  std::cout << std::endl;
}
