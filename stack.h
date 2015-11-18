#ifndef STACK_H
#define STACK_H

#include <iostream>

struct StFrame {
  int level;
  int value;
  StFrame *next;
};

class Stack {
  private:
    StFrame *data;
    int size;
  public:
    Stack();
    ~Stack();
    
    StFrame peek();
    
    StFrame pop();
    
    void push(int level, int value);
    
    bool isEmpty();
    
    void Print();
};

#endif
