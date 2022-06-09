#pragma once

#ifndef IVFS_H
#define IVFS_H

#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <stdio.h>
#include <string>
#include <cstring>
#include <vector>

namespace TestTask {

    struct File {

        File(size_t begin, size_t cur, size_t max) : beginning(begin), currentSize(cur), maxSize(max) {}

        size_t beginning;
        size_t currentSize;
        size_t maxSize;

        bool isOpenForReading = false;
        bool isOpenForWriting = false;

    };

    struct IVFS {

        IVFS();
        ~IVFS();

        File* Open(const char*);
        File* Create(const char*);
        void Close(File*);

        size_t Read(File*, char*, size_t);
        size_t Write(File*, char*, size_t);

        void WriteToMap();
        std::map<std::string, File*> GetMap();

    private:

        std::map<std::string, File*> namesMap;
        std::string pathToMap = "namesMap.txt";

        std::string pathToCollection = "CollectionFile.txt";
        std::fstream streamToCollection;
        size_t collectionSize = 0;

        std::mutex taskMutex;
    };
}

#endif


