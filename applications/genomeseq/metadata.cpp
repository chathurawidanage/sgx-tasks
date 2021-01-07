#include "metadata.hpp"

#include <filesystem>
#include <fstream>

#include "commands.hpp"

void Metadata::Read() {
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
    outfile << id;
    outfile << source;
    outfile << partitions;
    outfile.close();
}

std::shared_ptr<Metadata> Metadata::Load(std::string id) {
    auto m = std::shared_ptr<Metadata>{new Metadata(id)};
    m->Read();
    return m;
}
