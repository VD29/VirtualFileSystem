#include "IVFS.h"

using namespace TestTask;

/*  Method for writing a map with file information to a file,
    records the name of the file, the index of its beginning
    and dimensions*/
void IVFS::WriteToMap() {

    taskMutex.lock();

    char* buffer;
    std::ofstream os;
    size_t numberOfFiles = namesMap.size();

    if (numberOfFiles == 0) {
        taskMutex.unlock();
        return;
    }

    if (numberOfFiles > 1) {
        std::cout << "\nThere are " << numberOfFiles << " files in collection" << std::endl;
    }
    else {
        std::cout << "\nThere is only " << numberOfFiles << " file in collection" << std::endl;
    }

    os.open(pathToMap);
    os.write((char*)&numberOfFiles, sizeof(size_t));

    for (auto items : namesMap) {
        size_t size = items.first.size();
        os.write((char*)&size, sizeof(size_t));
        os.write(items.first.c_str(), size);
        os.write((char*)&items.second->beginning, sizeof(size_t));
        os.write((char*)&items.second->currentSize, sizeof(size_t));
        os.write((char*)&items.second->maxSize, sizeof(size_t));
    }

    taskMutex.unlock();
    os.close();
}

//  Method for reading the map according to the record format
std::map<std::string, File*> IVFS::GetMap() {

    std::ifstream is;
    size_t numberOfFiles{ 0 };

    is.open(pathToMap);
    is.read((char*)&numberOfFiles, sizeof(size_t));

    std::string name;
    size_t stringLength, beginning, currentSize, maxSize;

    for (int i = 0; i < numberOfFiles; ++i) {

        is.read((char*)&stringLength, sizeof(size_t));

        char* buffer = new char[stringLength];

        is.read(buffer, stringLength);

        name = buffer;

        is.read((char*)&beginning, sizeof(size_t));
        is.read((char*)&currentSize, sizeof(size_t));
        is.read((char*)&maxSize, sizeof(size_t));

        namesMap[name] = new File(beginning, currentSize, maxSize);

    }

    is.close();
    return namesMap;

}

/*  Constructor for interface - at the beginning map with files is created and populated,
    opens a stream to the block in which they are stored*/

IVFS::IVFS() {

    namesMap = GetMap();

    streamToCollection = std::fstream(pathToCollection, std::fstream::binary | std::fstream::in | std::fstream::out);

    if (!streamToCollection.is_open()) {
        std::ofstream(pathToCollection).close();
        streamToCollection = std::fstream(pathToCollection, std::fstream::binary | std::fstream::in | std::fstream::out);
    }

    streamToCollection.seekg(0, std::ios::end);
    collectionSize = streamToCollection.tellg();
}

/*  At the end of the interface work you need to write to a file
    and then release the map from the pointers*/
IVFS::~IVFS() {

    streamToCollection.close();

    WriteToMap();

    for (auto& value : namesMap) {
        delete value.second;
    }
}

/*  Opening or Creating writeonly
    To write the file is also either in the mape already existing or appended to the end of a large block.
    In order to there were no conflicts with multithreading recording part fenced by mutex*/
File* IVFS::Create(const char* name) {

    File* filePtr = nullptr;

    /// if a file exists, it is allowed to open to write
    if (namesMap[name] != nullptr) {
        filePtr = namesMap[name];

        if (filePtr->isOpenForWriting) {
            return filePtr;
        }
        filePtr->isOpenForWriting = true;
    }
    else {
        /// if the file does not exist, it is created at the end of the block
        taskMutex.lock();
        filePtr = new File(collectionSize, 0, 4096);
        char endBuffer[4096] = { 0 };
        streamToCollection.write(endBuffer, 4096);
        collectionSize += 4096;

        filePtr->isOpenForWriting = true;
        namesMap[name] = filePtr;
        taskMutex.unlock();
    }
    return filePtr;
}

File* IVFS::Open(const char* name) {

    File* filePtr = nullptr;

    if (namesMap[name] != nullptr) {

        filePtr = namesMap[name];

        if (filePtr->isOpenForReading) {
            return filePtr;
        }
        filePtr->isOpenForReading = true;
    }
    return filePtr;
}

/// Closing the file
/// When closing a file permissions for reading and writing are removed
void IVFS::Close(File* file) {

    if (file == nullptr) {
        return;
    }

    if (file->isOpenForWriting) {
        file->isOpenForWriting = false;
    }

    if (file->isOpenForReading) {
        file->isOpenForReading = false;
    }
}

/*  Reading from file;
    From a large block of information is read according
    with position in file - shift and size*/
size_t IVFS::Read(File* file, char* buffer, size_t length) {

    if (file == nullptr || !file->isOpenForReading) {
        return 0;
    }

    streamToCollection.seekg(file->beginning);
    streamToCollection.read(buffer, std::min(length, file->currentSize));

    return streamToCollection.gcount();
}

/*  Writing to a File;
    The file is checked for existence and the possibility to open it for writing;
    The mutex is then closed and the contents of the buffer are recorded
    taking into account whether there is sufficient space in the block area allocated to the file;
    - if there is not enough space, the file is moved to the end of the block and the block size is increased*/

size_t IVFS::Write(File* file, char* buffer, size_t length) {

    if (file == nullptr || !(file->isOpenForWriting)) {
        return 0;
    }
    taskMutex.lock();
    /// if file is placed in free space
    if (file->currentSize + length <= file->maxSize) {
        streamToCollection.seekp(file->beginning);
        streamToCollection.write(buffer, length);
        file->currentSize += length;
    }
    else {
        /// otherwise it is transferred to the end of the unit
        char* bufferRead = new char[file->currentSize];
        Read(file, bufferRead, file->currentSize);

        streamToCollection.write(bufferRead, file->currentSize);
        streamToCollection.write(buffer, length);

        file->beginning = collectionSize;
        file->maxSize = std::max(length + file->currentSize, file->maxSize * 2);
        file->currentSize += length;

        char* endBuffer = new char[file->maxSize - file->currentSize];
        streamToCollection.write(endBuffer, file->maxSize - file->currentSize);
        streamToCollection.seekg(0, std::ios::end);
        collectionSize = streamToCollection.tellg();

        delete[] bufferRead;
        delete[] endBuffer;
    }
    taskMutex.unlock();
    return length;
}
