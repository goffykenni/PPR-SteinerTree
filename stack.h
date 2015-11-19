#ifndef STACK_H
#define STACK_H

#include <iostream>

struct StFrame {
  int level;
  int value;
  StFrame *next;
  StFrame *prev;
};

/**
 * Implementuje oboustranny zasobnik pomoci oboustranného spojovaciho seznamu
 */

class Stack {
  private:
    StFrame *top;
    StFrame *bottom;
    int size;
  public:
    Stack();
    ~Stack();
    
    // podivat se na vrsek
    StFrame checkTop();
    
    // sebrat zezhora
    StFrame grabTop();
    
    // zaradit na vrsek
    void pushTop(int level, int value);
    
    // podivat se na spodek
    StFrame checkBottom();
    
    // sebrat ze spoda
    StFrame grabBottom();
    
    // zaradit do spoda
    void pushBottom(int level, int value);
    
    bool isEmpty();
    
    int getSize();
    
    void Print();
    void PrintReverse();
};

#endif
