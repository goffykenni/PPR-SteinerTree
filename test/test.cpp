#include <iostream>
#include <fstream>
#include <utility>

#include "../stack.h"

// Unit test

int main(int argc, char *argv[]) {
    
    //WWWWWWWWWWW
    //  Stack  WW
    //WWWWWWWWWWW
    std::cout << "TEST 1: stack" << std::endl;
    
    // 1. init, first push and check
    Stack stack;
    stack.pushTop(1, 10);
    StFrame frame = stack.checkTop();
    if (frame.value != 10) {
        std::cout << "checkTop FAILED" << std::endl;
        return 1;
    }
    std::cout << "1-1 OK" << std::endl;

    // 2. push more values
    stack.pushTop(1, 11);
    stack.pushTop(1, 12);
    stack.pushTop(1, 13);
    stack.pushTop(1, 14);
    std::cout << "1-2 OK" << std::endl;

    // 3. read some values
    frame = stack.grabTop();
    frame = stack.grabTop();
    if (frame.value != 13) {
        std::cout << "grabTop FAILED" << std::endl;
        return 1;
    }
    std::cout << "1-3 OK" << std::endl;
    
    // 4. check size
    if (stack.getSize() != 3) {
        std::cout << "stack getSize FAILED" << std::endl;
        return 1;
    }
    std::cout << "1-4 OK" << std::endl;
    
    // 5. push more values
    stack.pushTop(1, 21);
    stack.pushTop(1, 22);
    frame = stack.checkTop();
    if (frame.value != 22) {
        std::cout << "checkTop FAILED" << std::endl;
        return 1;
    }
    std::cout << "1-5 OK" << std::endl;

    // 6. print
    stack.Print();
    std::cout << "1-6 OK" << std::endl;
    
    // 7. grab all from top
    while(!stack.isEmpty()) {
        stack.grabTop();
    }
    std::cout << "1-7 OK" << std::endl;
    
    // 2. push values bottom, check, grab, size, print
    stack.pushBottom(2, 30);
    stack.pushBottom(2, 31);
    stack.pushBottom(2, 32);
    stack.pushBottom(2, 33);
    stack.pushBottom(2, 34);

    stack.pushTop(2, 29);
    stack.pushBottom(2, 35);
    stack.pushTop(2, 28);
    
    frame = stack.checkBottom();
    if (frame.value != 35) {
        std::cout << "checkBottom FAILED" << std::endl;
        return 1;
    }
    
    frame = stack.grabBottom();
    if (frame.value != 35) {
        std::cout << "grabBottom FAILED" << std::endl;
        return 1;
    }
    
    if (stack.getSize() != 7) {
        std::cout << "stack getSize FAILED" << std::endl;
        return 1;
    }
    
    // 28, 29, 30, 31, 32, 33, 34
    stack.Print();
    std::cout << "1-8 OK" << std::endl;
    
    // 9. grab all from bottom
    while(!stack.isEmpty()) {
        frame = stack.grabBottom();
    }
    std::cout << "1-9 OK" << std::endl;
    std::cout << "TEST 1 SUCCESS" << std::endl;
    
    // ---------------------------------------
    
    return 0;
}