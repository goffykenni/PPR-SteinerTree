#include "stack.h"

using namespace std;

Stack::Stack() {
    this->size = 0;
    this->top = NULL;
    this->bottom = NULL;
}

Stack::~Stack() {
    if (this->top == NULL) {
        return;
    }
    
    StFrame* tmp = NULL;
    do {
        tmp = this->top->next;
        delete(this->top);
        this->top = tmp;
    } while (this->top != NULL);
}

StFrame Stack::checkTop()
{
    // Check
    StFrame result;
    result.level = top->level;
    result.value = top->value;
    result.next = top->next;
    result.prev = NULL;
    
    return result;
}

StFrame Stack::grabTop()
{
    StFrame result;
    result.level = top->level;
    result.value = top->value;
    result.next = top->next;
    result.prev = NULL;

    delete(this->top);
    this->top = result.next;
    size--;

    if (size == 0) {
        this->bottom = NULL;
    } else {
        this->top->prev = NULL;
    }
    
    return result;
}

void Stack::pushTop(int level, int value)
{
    StFrame *frame = new StFrame();
    frame->level = level;
    frame->value = value;
    frame->next = this->top;
    frame->prev = NULL;
    
    
    if (this->top != NULL) {
        this->top->prev = frame;
    } else {
        // prvni vlozena polozka
        this->bottom = frame;
    }
    this->top = frame;
    size++;
    
}

StFrame Stack::checkBottom() {
    StFrame result;
    result.level = bottom->level;
    result.value = bottom->value;
    result.next = bottom->next;
    result.prev = NULL;
    
    return result;
}

StFrame Stack::grabBottom() {
    // Check
    StFrame result;
    result.level = bottom->level;
    result.value = bottom->value;
    result.prev = bottom->prev;
    result.next = NULL;

    
    delete(this->bottom);
    this->bottom = result.prev;
    size--;
    if (size == 0) {
        this->top = NULL;
    } else {
        this->bottom->next = NULL;
    }
    return result;
}


void Stack::pushBottom(int level, int value) {
    StFrame *frame = new StFrame();
    frame->level = level;
    frame->value = value;
    frame->next = NULL;
    frame->prev = this->bottom;
    
    
    if (this->bottom != NULL) {
        this->bottom->next = frame;
    } else {
        // prvni vlozena polozka
        this->top = frame;
    }
    this->bottom = frame;
    size++;
}

bool Stack::isEmpty() {
    return (size == 0);
}



void Stack::Print() {
    if (this->top == NULL)
        return;
    
    StFrame* tmp = this->top;
    while (tmp != NULL) {
        cout << "(" << tmp->level << ", " << tmp->value << "), ";
        tmp = tmp->next;
    }
   cout << endl;
}

void Stack::PrintReverse() {
    if (this->top == NULL)
        return;
    
    StFrame* tmp = this->bottom;
    while (tmp != NULL) {
        cout << "(" << tmp->level << ", " << tmp->value << "), ";
        tmp = tmp->prev;
    }
   cout << endl;
}


int Stack::getSize() {
    return this->size;
}