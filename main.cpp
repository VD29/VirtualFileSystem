#include <fstream>
#include <iostream>

#include "IVFS.h"

using namespace TestTask;

const int buf = 10000;

int main() {

    std::ifstream file("IVFS.cpp");
    char rbuf[buf] = { 0 };
    file.read(rbuf, buf);

    IVFS fs;
    auto f = fs.Create("test.txt");
    printf("File = {length = %ld, pos = %ld, capacity = %ld}\n", f->currentSize, f->beginning, f->maxSize);
    fs.Write(f, rbuf, strlen(rbuf));
    printf("File = {length = %ld, pos = %ld, capacity = %ld}\n", f->currentSize, f->beginning, f->maxSize);
    fs.Close(f);

    auto f2 = fs.Open("test.txt");
    char rbuf2[buf] = { 0 };
    fs.Read(f2, rbuf2, buf);
    fs.Close(f2);
    printf("\nWas read:\n%s\n", rbuf2);

    return 0;
}