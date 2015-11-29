#include <iostream>
#include <fstream>
#include <cmath>
#include <string>
#include <time.h>
#include <stdlib.h>
#include "stack.h"
#include "graph.h"
#include "checker.h"
#include "mpi.h"

void print_graph(Graph*, int *label, int label_size);
void solve(Graph*, int*, int);
int read_input(Graph*& , int*&, int&, char* input_file);
void initial_work_distribution(Graph*, int*, int, Checker*&, Stack*&);
void accept_work(void*, int, Stack*&, Checker*&);
bool split_and_send(void *, int, int, Stack*&, Checker*&);

/* Stavy procesu, kterymi prochazi behem vypoctu */
/* INIT = Inicializace; WORKING = Mam praci a pracuji;
 * IDLE = Nemam nic na praci; IDLE_EXPECTING = Nemam praci, ale pozadal
 * jsem si; DONE = Nemam praci a vim, ze vypocet muze skoncit */
enum State {INIT, WORKING, IDLE, IDLE_EXPECTING, DONE};

/* Barvy pro procesy a token v souladu s Dijstrovym algoritmem */
enum Color {BLACK = 0, WHITE};

/* Tagy pro zpravy */
const int MSG_WORK_REQUEST = 1001;
const int MSG_WORK_SENT = 1002;
const int MSG_WORK_EMPTY = 1003;
const int MSG_TOKEN = 1004;
const int MSG_TERM = 21005;
const int MSG_INIT_INFO = 1006;
const int MSG_ASK_FOR_BEST = 4007;
const int MSG_RECEIVE_BEST = 11008;

/* Formaty pro posilani zprav jsou az na jednu vyjimku jednoduche a vzdy
 * obsahuji pouze 1-2 INTy. Vyjimka je posilani prace. Buffer je pole INTu
 * a jeho format je:
 * [0] .. Posilany ramec zasobniku
 * [1] .. Padding tohoto ramce
 * [2] .. Delka nejlepsiho reseni znama odesilateli
 * [3] .. Delka kontextu
 *  .. zbytek uzly kontextu */

/* Vypocetni konstanty */
/* Rezna vyska, musi byt aspon 0. Je-li n velikost grafu,
 * pak zasobnikove ramce s hodnotami (n - 1) - cut_height
 * se uz nebudou darovat, protoze obsahuji relativne malo prace. */
const int cut_height = 5;
/* Pocet cyklu, po kterych se kontroluje fronta zprav */
const int msg_check_cycles = 205;
/* Pocet cyklu, po kterych se vymenuje info o delce nejlepsiho reseni */
const int best_check_cycles = 30000;

/* Globalni promenne pro procesy */
static int my_rank;  /* ID prcesu */
static State my_state; /* Aktualni stav procesu */
static Color my_color; /* Aktualni barva */
static bool has_token; /* Info o vlastnictvi tokenu */
static double start_time, end_time; /* Doba startu a konce vypoctu */
static int my_count; /* Citac pro posilani zprav - NOT USED ATM */

static int total_proc; /* Pocet procesoru na kterych program bezi */
static int total_working_proc; /* Pocet skutecne pracujicich */
static MPI_Status status; /* Status posilanych/prijimanych zprav */

/* Struktura zapouzdrujici info o procesoru a delce jeho nejlepsiho
 * reseni. Pouziva se na na konci vypoctu pri PBR */
struct Solution {
  int size;
  int rank;  
} my_solution;

/* Token pro Dijkstruv algoritmus */
struct Token {
  Color color;
  int count;
} token;

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
using namespace std;
  MPI_Init(&argc, &argv);
  // Zjisti moje id
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  // Zjisti pocet procesoru celkem
  MPI_Comm_size(MPI_COMM_WORLD, &total_proc);
  
  /* Synchronizace a start mereni casu */
  MPI_Barrier(MPI_COMM_WORLD);
  start_time = MPI_Wtime();
  
  /* Nacteni vstupu - zacatek */
  Graph *g; int *terminal_set;
  int terminal_set_size;
  if (argc != 2 || read_input(g, terminal_set, terminal_set_size,
    argv[1]) != 0) {
    if (my_rank == 0)
      std::cout << "Program error, missing name of input file." << std::endl;
    return 1;
  }
  /* Nacteni vstupu - konec */
  
  /* DEBUG - randomization */
  /* if (my_rank == 0) {
    srand((unsigned) time(NULL));
  
    int pole[g->get_vertex_count()];
    for (int i = 0; i < g->get_vertex_count(); i++) {
      pole[i] = i;
    }
    int temp, x;
    for (int i = g->get_vertex_count() - 1; i >= 0; i--) {
      temp = pole[i];
      x = rand() % (i + 1);
      pole[i] = pole[x];
      pole[x] = temp;
    }
    
    for (int i = 0; i < terminal_set_size; i++) {
      terminal_set[i] = pole[i];
      cout << "Item: " << pole[i] << endl;
    }
  }  
  MPI_Bcast(terminal_set, terminal_set_size, MPI_INT, 0, MPI_COMM_WORLD);*/
  /*terminal_set[0] = 19;
  terminal_set[1] = 4;
  terminal_set[2] = 3;
  terminal_set[3] = 23;
  terminal_set[4] = 29;*/
  /* DEBUG - konec */
  

  /* Reseni ulohy */ 
  solve(g, terminal_set, terminal_set_size);
  
  /* Uklidit */
  if (g != NULL)
    delete(g);
  if (terminal_set != NULL)
    delete(terminal_set);  
  
  /* Synchro a konec mereni */
  MPI_Barrier(MPI_COMM_WORLD);
  end_time = MPI_Wtime();
  
  /* Vypsat mereni. Nevim jak a zejmena kdo, takze P0, dokud se nedozvim
   * jine info */
  if (my_rank == 0)
    cout << "Elapsed time: " << (end_time - start_time) << " seconds." << endl;
  
  MPI_Finalize();
  return 0;
}

int min(int a, int b) {
  return a < b ? a : b;
}

/* Prijme praci z dane zpravy.
 * message - buffer se zpravou; message_len - delka bufferu;
 * st - adresa zasobniku volajiciho procesu
 * checker - adresa checkeru (tj. "kontextu zasobniku") volajiciho procesu */
void accept_work(void* message, int message_len, Stack*& st, Checker*& checker) {
  using namespace std;
  if (st == NULL || checker == NULL) {
    cout << "P" << my_rank << ": Error (accept_work): My stack or checker are NULL!"
      << endl;
    return;
  }
  if (!st->is_empty()) {
    cout << "P" << my_rank << ": Error (accept_work): My stack is not empty! Size: "
      << st->getSize() << endl;
    return;
  }
  
  /* Nacti data ze zpravy. Podle formatu uvedeneho pred funkci main. */
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
  /*cout << "Accepting context: ";
  for (int i = 0; i < context_len; i++)
    cout << context[i] << ", ";
  cout << endl;*/
  checker->update_context(best_size, context, context_len);  
}

/* Rozdeleni zasobniku a odeslani prace nebo odmitnuti.
 * buffer - buffer do ktereho se bude zapisovat; buffer_size - jeho velikost;
 * st - adresa zasobniku procesu (darce); checker - adresa checkeru procesu. */
bool split_and_send(void* buffer, int buffer_size,
    Stack*& st, Checker*& checker) {
  using namespace std;
  int *message = (int*) buffer;
  /* Pro ulozeni ramce na dne zasobniku */
  StFrame frame;
  /* Hranice pro ramce, ktere uz jsou tak male, ze je proces nedaruje. */
  int border = checker->get_parent_size() - 
    checker->get_terminal_set_size() - cut_height;
  
  /* Kdyz sam nic nemam nebo mam malo, tak nic nedam. */
  if (st->is_empty() || (frame = st->checkBottom()).value > border) {
    return false;
  }
  
  /* Urcim prvniho (a tedy nejvetsiho) potomka ramce na dne zasobniku. */
  int first_child = frame.value + 1 + frame.padding;
  /* Dno je pred reznou vyskou a jeho prvni potomek taky. */
  if (first_child < border) {
    /* Daruj prvorozeneho */
    message[0] = first_child;
    message[1] = 0;
    message[2] = checker->get_global_best_size();
    message[3] = frame.level;
    int *vertices = checker->get_current_vertices();
    for (int i = 0; i < message[3] - 1; i++) {
      message[4 + i] = vertices[i + checker->get_terminal_set_size()];
    }
    message[4 + message[3] - 1] = checker->translate(frame.value);
    st->grabBottom();
    frame.padding++;
    st->pushBottom(frame.level, frame.value, frame.padding);
    return true;
  } /* Potomek dna uz je na rezne vysce, ale zaroven ma proces bez
    tohoto dna dostatek prace. */ 
  else if (st->getSize() > 1 && (frame.prev)->level < border) {
    /* Daruj cele dno */
    message[0] = frame.value;
    message[1] = frame.padding;
    message[2] = checker->get_global_best_size();
    message[3] = frame.level - 1;
    int *vertices = checker->get_current_vertices();
    
    for (int i = 0; i < message[3]; i++) {
      message[4 + i] = vertices[i + checker->get_terminal_set_size()];
    }
    /* Sobe ho musim vyhodit. Nebyla by chyba nevyhodit ho, ale pak
     * se zbytecne overuji casti stavoveho prostoru vicekrat */
    st->grabBottom();
    return true; 
    
  }
  return false;  
}

/* Provede inicialni distribuci prace, kdy P0 naposila ostatnim procesum
 * ramce sveho zasobniku z prvni a druhe urovne sveho vlastniho zasobniku
 * graph - Vstupni graf; terminal_set - mnozina terminalnich uzlu;
 * terminal_set_size - jeji velikost; checker - adresa checkeru volajiciho;
 * st - adresa zasobniku volajiciho. */
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
      cout << "P0 message: Trivial solution works! EVERYBODY STOP NOW!\n" << endl;
      for (int i = 1; i < total_proc; i++) {
        MPI_Send(&i, 1, MPI_INT, i, MSG_TERM, MPI_COMM_WORLD);
      }
      my_state = DONE;
      return;
    }
    // Pocet jiz obslouzenych procesoru (z mnoziny P1, P2, ...)
    int served = 0;
    // Kolik prace representovane ramci zasobniku muze P0 dat
    int frames_available = size - cut_height - 1;    
    
    /* Sestavi zpravu pro darovani prace,
     * ktera se bude posilat, viz komentar pred funkci
     * main pro jeji obecny format. */    
    int message_len = 5; int message[message_len];
    /* Posilane ramce nebudou mit padding, reseni lepsi nez cely graf
     * zatim nezname a ty procesy, ktere dostanou ramec z druhe
     * urovne zasobniku P0, ho dostanou jako potomka uzlu 0 */
    message[1] = 0; message[2] = graph->get_vertex_count();
    message[4] = checker->translate(0);
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
    
    /* Ukonci ty procesory, na ktere se prace nedostala */
    for (int i = served + 1; i < total_proc; i++) {
      MPI_Send(&message, message_len, MPI_INT, i, MSG_TERM, MPI_COMM_WORLD);
    }
    
    /* P0 rozvine svuj vlastni zasobnik podle toho, kolik toho daroval. */
    st->pushTop(1, 0, level_two_pos);
    if (size > 1)
      st->pushTop(1, 1, 0);    
    for (int i = level_one_pos + 2; i <= size - 1; i++) {
      st->pushTop(1, i, 0);      
    }
    
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
      case MSG_TERM:
      default:
        my_state = DONE;
        break;
    }
  }
}

/* Nalezne nejmensi podgraf (z hlediska poctu vrcholu) daneho grafu
 * obsahujici uzly z cilove mnoziny, ktery je souvisly. Idea spociva
 * v tom systematicky overovat vsechny mozne kombinace vrcholu, ktere
 * lze do grafu pridavat. 
 * graph - vstupni graf; terminal_set - mnozina terminalnich uzlu; */
void solve(Graph *graph, int *terminal_set, int terminal_set_size) {
  using namespace std;
  
  /* Inicializace procesu. Nastaveni vychozich hodnot jejich promennych. */
  int size = graph->get_vertex_count() - terminal_set_size;
  my_state = INIT;
  
  /* Citac pro kontrolu fronty zprav */
  int n = 0;
  my_color = WHITE;
  my_count = 0;
  token.color = WHITE;
  token.count = 0;
  /* P0 ma na zacatku token */
  has_token = my_rank == 0 ? true : false;
  
  /* Zatim nevime, kolik bude pracovat, tak predpokladejme, ze vsichni. */
  total_working_proc = total_proc;  
  
  /* Buffery pro posilani zprav. */
  int donor_buff[2], request_buff, terminate_buff, refuse_buff, best_buff;
  int work_buff_len = size + 4;
  int work_buff[work_buff_len];
  /* Promenna pro neblokujici sendy */
  MPI_Request request;
  /* Promenna, ktera bude indikovat, jestli jsou ve fronte zpravy. */
  int flag = 0;
  
  /* Pocatecni rozdeleni prace mezi procesory */
  Checker *checker;
  Stack *st;
  initial_work_distribution(graph, terminal_set, terminal_set_size, checker, st); 
  /* P0 broadcastne informaci o celkovem poctu pracujicich procesoru .*/
  MPI_Bcast(&total_working_proc, 1, MPI_INT, 0, MPI_COMM_WORLD);
  
  /* Id distributora - tj. procesu, ktery bude periodicky sbirat a
   * poskytovat informace o delkach nejlepsich reseni. */
  int distributor_id = total_working_proc - 1;
  /* Taktika pro vyber darcu prace bude taktika lokalnich cyklickych
   * zadosti. Kadzy procesor bude mit svuj counter. */
  srand(time(NULL) * sin((double) my_rank / total_working_proc) + 11);
  int counter = rand() % total_working_proc;

  /* Kolikrat po sobe byl procesor odmitnut pri zadani o praci. */
  int denials = 0;
  /* Zazadal si procesor distributora o vymenu delky nejlepsiho reseni? */
  bool asked_for_best = false;

  while (my_state != DONE) {          
    n++;
    /* Je cas na kontrolu fronty zprav? */    
    if ((n % msg_check_cycles) == 0) {
      MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
      if (flag) {
        int sender = status.MPI_SOURCE;
        switch (status.MPI_TAG) {    
                      
          /* Prisla mi prace. */
          case MSG_WORK_SENT: {          
            MPI_Recv(&work_buff, work_buff_len, MPI_INT, sender, MSG_WORK_SENT,
              MPI_COMM_WORLD, &status);
            accept_work(&work_buff, work_buff_len, st, checker);
            my_state = WORKING;
            denials = 0;
            break;
          }
          
          /* Prisla zprava, ze ten, koho jsem zadal o praci, pro me zadnou nema. */
          case MSG_WORK_EMPTY: {
            MPI_Recv(&refuse_buff, 1, MPI_INT, sender, MSG_WORK_EMPTY,
              MPI_COMM_WORLD, &status);
            checker->update_global_best_size(refuse_buff);
            my_state = IDLE;
            denials++;
            break;
          }
          
          /* Nekdo me zada o praci */
          case MSG_WORK_REQUEST: {
            MPI_Recv(&request_buff, 1, MPI_INT, sender, MSG_WORK_REQUEST,
              MPI_COMM_WORLD, &status);
            /* V zadosti je ulozena delka nejlepsiho reseni zadatele. */
            checker->update_global_best_size(request_buff);
            /* Pokusim se najit praci */
            bool sent = split_and_send(&work_buff, work_buff_len, st, checker);
            if (sent) {
              MPI_Send(&work_buff, work_buff_len, MPI_INT, sender, MSG_WORK_SENT,
                MPI_COMM_WORLD);
            } else {
              refuse_buff = checker->get_global_best_size();
              MPI_Send(&refuse_buff, 1, MPI_INT, sender, MSG_WORK_EMPTY,
                MPI_COMM_WORLD);              
            }
            /* Pokud jsem poslal praci procesoru s mensim ID, musim
             * se ocernit podle Dijkstra. */
            if (sent && sender < my_rank) {
              my_color = BLACK;
            }
            break;
          }
          
          /* Prisel token. */
          case MSG_TOKEN: {
            MPI_Recv(&token, 1, MPI_2INT, sender, MSG_TOKEN,
              MPI_COMM_WORLD, &status);
            has_token = true;
            
            if (my_rank == 0) {
              /* P0 dostal zpatky bily token. Konec */
              if (token.color == WHITE) {
                for (int i = 1; i < total_working_proc; i++) {
                  MPI_Send(&terminate_buff, 1, MPI_INT, i, MSG_TERM,
                    MPI_COMM_WORLD);
                }
                my_state = DONE;
              }  else {
                token.color = WHITE;
              }
            } 
            /* Pokud jsem cerny, musim ocernit i token podle Dijkstra. */
            else if (my_color == BLACK) {
              token.color = BLACK;
              cout << "Token colored to BLACK" << endl;
            }            
            cout << "P" << my_rank << " : Got token, color " << token.color << endl;
            break;
          }
            
          /* Prisla zprava indikujici ukonceni vypoctu. */
          case MSG_TERM: {
            MPI_Recv(&terminate_buff, 1, MPI_INT, 0, MSG_TERM,
              MPI_COMM_WORLD, &status);
            my_state = DONE;            
            continue;   
          }
          
          /* Nekdo me zada o me znamou delku nejlepsiho reseni */
          case MSG_ASK_FOR_BEST: {
            MPI_Recv(&best_buff, 1, MPI_INT, sender, MSG_ASK_FOR_BEST,
              MPI_COMM_WORLD, &status);
            /* Zaroven mi posila svoji delku nejlepsiho znameho reseni. */  
            checker->update_global_best_size(best_buff);
            best_buff = checker->get_global_best_size();
            MPI_Isend(&best_buff, 1, MPI_INT, sender, MSG_RECEIVE_BEST,
              MPI_COMM_WORLD, &request);
            break;
          }   
          
          /* Nekdo mi posila jemu znamou delku nejlepsiho reseni */
          case MSG_RECEIVE_BEST: {
            MPI_Recv(&best_buff, 1, MPI_INT, sender, MSG_RECEIVE_BEST,
              MPI_COMM_WORLD, &status);
            checker->update_global_best_size(best_buff);
            /* Priste se muzu zase zeptat */
            asked_for_best = false;
            break;
          }
          
          default:
            cout << "ERROR: UNKNOWN MESSAGE";
            
        }        
      }
      /* Pokud jsem IDLE, musim preposlat token, pokud ho mam, a (nebo)
       * si zazadat o praci. */
      if (my_state == IDLE) {
        /* Pokud resi jen jediny procesor, znamena to konec vypoctu. */
        if (total_working_proc == 1) {
          my_state = DONE;
          continue;
        }
        /* Pokud mam token, poslu ho */
        my_color = WHITE;      
        if (has_token) {          
          MPI_Isend(&token, 1, MPI_2INT, (my_rank + 1) % total_working_proc,
              MSG_TOKEN, MPI_COMM_WORLD, &request);
          has_token = false;
        }
        
        /* Zazadam o praci podle hodnoty sveho citace. */
        /* Pokud jsem uz zadal hodnekrat, tak usoudim, ze asi prace neni
         * a uz nebudu otravovat. */
        if (denials < 15 * total_working_proc - 1) {
          /* Nesmim poslat zpravu sam sobe hlavne! */       
          if (my_rank == counter)
            counter = (counter + 1) % total_working_proc;
          request_buff = checker->get_global_best_size();
          MPI_Send(&request_buff, 1, MPI_INT, counter, MSG_WORK_REQUEST,
            MPI_COMM_WORLD);       
          counter = (counter + 1) % total_working_proc;
          /* Ocekavam praci nebo zamitnuti */
          my_state = IDLE_EXPECTING;
        }
      }     
    }
    
    /* Pokud je cas, pozadam si distributora o delku nejlepsiho znameho reseni,
     * pokud jsem se teda uz neptal a moje zadost jeste nebyla
     * uspokojena. */
    if ((n % best_check_cycles) == 0 && !asked_for_best
        && my_rank != distributor_id) {
      asked_for_best = true;
      best_buff = checker->get_global_best_size();
      MPI_Send(&best_buff, 1, MPI_INT, distributor_id, MSG_ASK_FOR_BEST,
        MPI_COMM_WORLD);
    }
    
    /* Dal smi pouze pracujici procesy. */
    if (my_state != WORKING)
      continue;
    
    /* Kdyz nemam praci, musim zmenit stav */
    if (st->is_empty()) {
      my_state = IDLE;
      continue;
    }
    
    // Vyjmi uzel ze zasobniku a zacni ho zpracovavat.
    StFrame current = st->grabTop();
  
    if (current.value < 0 || current.value >= size)
      cout << "Neplatna hodnota zasob (" << current.value  << "): (main cycle)\n";
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
      if (!st->is_empty()) {
        int level_difference = current.level - st->checkTop().level;
        checker->remove_last(level_difference);
      }
      continue;
    } 
    
    // Pridani uzlu muze vest ke zlepseni, tak to vyzkousejme
    checker->add_vertex(current.value, true);
    checker->process_current_state();

    /* Potomky akt. uzlu pridame na zasobnik. Nepridavame pritom ty uzly,
     ktere representuji takove vrcholy, ktere v grafu uz pridane. Navic
     pridavame jen vzdy uzly s vetsim cislem nez ma aktualni, abychom
     se vyhnuli duplicitnim stavum. */
    int children_count = 0;
    for (int i = current.value + 1 + current.padding; i <= size - 1; i++) {
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
      if (!st->is_empty()) {
        int level_difference = current.level - st->checkTop().level;
        checker->remove_last(level_difference);
      }
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
  MPI_Bcast(&out, 1, MPI_2INT, 0, MPI_COMM_WORLD);
  /* Ten, kdo ma nejlepsi reseni, pripravi ho a vytiskne. Ostatni muzou
   * koncit. */  
  if (my_solution.rank == out.rank) {
    // Priprav a tiskni reseni
    cout << "P" << out.rank << " has best solution (" << my_solution.size <<")" << endl;
    Graph *r = graph->create_induced_subgraph(checker->get_best_vertices(),
    checker->get_best_size());
    r->remove_cycles();
    print_graph(r, checker->get_best_vertices(), checker->get_best_size());
    delete(r);
  }    
  delete(st);
  delete(checker);
}

/* Metoda pro nacteni vstupu */
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
