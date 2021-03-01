#include "metadata.hpp"

#include <filesystem>
#include <fstream>
#include <spdlog/spdlog.h>

#include "commands.hpp"

void Metadata::Read() {
    spdlog::info("Reading metadata from {}", GetFile());
    std::ifstream infile(GetFile());
    infile >> id;
    infile >> source;
    infile >> partitions;
    infile.close();
}

std::string Metadata::GetFile() {
    return get_root() + "/" + this->id + "/metadata";
}

bool Metadata::Exists() {
    return std::filesystem::exists(GetFile());
}

void Metadata::Write() {
    std::ofstream outfile(GetFile());
    outfile << id << std::endl;
    outfile << source << std::endl;
    outfile << partitions << std::endl;
    outfile.close();
}

std::shared_ptr<Metadata> Metadata::Load(std::string id) {
    auto m = std::shared_ptr<Metadata>{new Metadata(id)}; // const is provate
    m->Read();
    return m;
}
