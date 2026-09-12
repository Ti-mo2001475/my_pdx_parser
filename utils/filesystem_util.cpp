#include "filesystem_util.h"
#include <fstream>
#include <iostream>
void getAllFiles(std::string path, std::vector<std::string>& files) 
{
	std::filesystem::path dir(path);
	for (const auto& entry : std::filesystem::directory_iterator(dir)) {
		files.push_back(entry.path().string());
	}
}

std::string readAllLines(std::string path){
    std::ifstream fin(path);
    std::string out("");
    std::string line("");
    while(!fin.eof()){
        out.append(line);
        out.append("\n");
        std::getline(fin,line);
    }
    return out;
}

std::string readAllLinesWithoutComment(std::string path){
    std::ifstream fin(path);
    std::string out("");
    std::string line("");
    while(!fin.eof()){
        if(line[0] == '#') continue;
        if(size_t pos = line.find('#'); pos != std::string::npos) line.erase(pos);
        out.append(line);
        out.append("\n");
        std::getline(fin,line);
    }
    return out;
}

std::string readAllLinesWithoutComment(std::string path,char prefix,std::function<bool(std::string)> handler){
    
    std::ifstream fin(path);
    std::string out("");
    std::string line("");
    while(!fin.eof()){
        std::getline(fin,line);
        if(line.empty()) {
            out.append("\n");
            continue;
        }
        if(line[0] == '#') {
            continue;
        }
        if(size_t pos = line.find('#'); pos != std::string::npos) line.erase(pos);
        if(line[0] == prefix && !handler(line)) {
            continue;
        }
        out.append(line);
        out.append("\n");
    }
    return out;
}