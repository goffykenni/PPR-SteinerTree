#include "stack.h"

Stack::Stack() {
  this->size = 0;
  this->data = 0L;
}

StFrame Stack::peek() {
  // Check
  StFrame result;
  result.level = data->level;
  result.value = data->value;
  result.next = data->next;
  return result;
}

StFrame Stack::pop() {
  // Check
  StFrame result;
  result.level = data->level;
  result.value = data->value;
  result.next = data->next;
  
  StFrame *prev = data;
  data = data->next;
  
  delete(prev);
  size--;
  return result;
}

void Stack::push(int level, int value) {
  StFrame *frame = new StFrame();
  frame->level = level;
  frame->value = value;
  frame->next = data;
  data = frame;
  size++;
}

bool Stack::isEmpty() {
  return (size == 0);
}

Stack::~Stack() {
  StFrame *act = data;
  StFrame *prev = act;
  while (prev != 0L) {
    act = prev->next;
    delete(prev);
    prev = act;
  }    
}

void Stack::Print() {
  using namespace std;
  StFrame *act = data;
  while (act != 0L) {
    cout << "(" << act->level << ", " << act->value << "), ";
    act = act->next;
  }
  cout << endl;
}