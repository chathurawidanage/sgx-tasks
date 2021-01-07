#ifndef C6F53121_2ADE_45EE_AE09_2A021436EEC5
#define C6F53121_2ADE_45EE_AE09_2A021436EEC5

#include <memory>
#include <string>

class Metadata {
    std::string id;
    std::string source;
    int32_t partitions;

    Metadata(std::string index_id) : id(index_id) {
    }

    void Read();

   public:
    Metadata(std::string index_id, std::string source, int32_t partitions) : id(index_id), source(source), partitions(partitions) {
    }

    std::string GetFile();

    bool Exists();

    void Write();

    static std::shared_ptr<Metadata> Load(std::string id);

    std::string GetSource() {
        return this->source;
    }

    std::string GetId() {
        return this->id;
    }

    int32_t GetPartitions() {
        return this->partitions;
    }
};

#endif /* C6F53121_2ADE_45EE_AE09_2A021436EEC5 */
