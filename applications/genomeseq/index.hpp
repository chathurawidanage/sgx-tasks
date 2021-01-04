#ifndef D0719B71_A378_4E7E_B21B_27A0025A745B
#define D0719B71_A378_4E7E_B21B_27A0025A745B
#include <string>

class Index {
   private:
    int32_t partitions;
    std::string id;
    std::string source_file;

   public:
    Index(int32_t partitions, std::string id, std::string source_file) : partitions(partitions), id(id), source_file(source_file) {
    }

    std::string Print() {
        std::stringstream ss;

        ss << id << "\t" << source_file << "\t" << partitions;

        return ss.str();
    }
};
#endif /* D0719B71_A378_4E7E_B21B_27A0025A745B */
