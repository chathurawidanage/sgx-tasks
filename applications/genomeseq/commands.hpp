#ifndef C37E4C99_F798_4490_A00F_CD48E100A69D
#define C37E4C99_F798_4490_A00F_CD48E100A69D
#include <command.hpp>
#include <memory>
#include <string>
#include <vector>

std::string create_response(int32_t error_code, std::string msg = "");

std::string get_root();

void tokenize(std::string &cmd, std::shared_ptr<std::vector<const char *>> &args);

class PartitionCommand : public tasker::Command {
   private:
    std::string src_file;
    std::string dst_folder;
    int32_t partitions;

    void Validate(int32_t *code, std::string *msg);

   public:
    PartitionCommand(std::string cmd) : tasker::Command(cmd) {
    }

    void Parse(int32_t *code, std::string *msg);

    std::string &GetSrcFile() {
        return this->src_file;
    }

    std::string &GetDstFolder() {
        return this->dst_folder;
    }

    int32_t &GetPartitions() {
        return this->partitions;
    }
};

class IndexCommand : public tasker::Command {
   private:
    std::string src_file;
    void Validate(int32_t *code, std::string *msg);

   public:
    IndexCommand(std::string cmd) : tasker::Command(cmd) {}

    void Parse(int32_t *code, std::string *msg);

    std::string &GetSrcFile() {
        return this->src_file;
    }
};

class ClientIndexCommand : public tasker::Command {
   private:
    std::string src_file;
    std::string relative_src_file;
    int32_t partitions;

    void Validate(int32_t *code, std::string *msg);

   public:
    ClientIndexCommand(std::string cmd) : tasker::Command(cmd) {}

    void Parse(int32_t *code, std::string *msg);

    std::string &GetSrcFile() {
        return this->src_file;
    }

    std::string &GetRelativeSrcFile() {
        return this->relative_src_file;
    }

    int32_t &GetPartitions() {
        return this->partitions;
    }
};

class DispatchCommand : public tasker::Command {
   private:
    /** Number of bits per item. */
    unsigned ibits = 8;

    /** The number of parallel threads. */
    unsigned threads = 0;

    /** The number of partitions. */
    int pnum = 1;

    /** The number of hash functions. */
    int nhash = 5;

    /** Minimum alignment length. */
    int alen = 20;

    /** The size of a b-mer. */
    int bmer = -1;

    /** The step size when breaking a read into b-mers. */
    int bmer_step = -1;

    /** single-end library. */
    int se = 1;

    /** fastq mode dispatch. */
    int fq = 0;

    /** Make bloom filters reusable*/
    int reuse_bf;

    /** Input files**/
    std::string input1, input2;

    /** ID of the index to use**/
    std::string index_id;

    /** search destination**/
    std::string destination;

    void Validate(int32_t *code, std::string *msg);

   public:
    DispatchCommand(std::string cmd) : tasker::Command(cmd) {
    }

    void Parse(int32_t *code, std::string *msg);

    std::string GetDestination() {
        return this->destination;
    }

    std::string GetInput1() {
        return this->input1;
    }

    std::string GetInput2() {
        return this->input2;
    }

    int32_t &GetSe() {
        return this->se;
    }

    int32_t &GetFq() {
        return this->fq;
    }

    std::string GetIndexFolder();

    int32_t &GetBmer() {
        return this->bmer;
    }

    unsigned &GetIBits() {
        return this->ibits;
    }

    int32_t &GetBmerStep() {
        return this->bmer_step;
    }

    void SetBmerStep(int32_t bmer_step) {
        this->bmer_step = bmer_step;
    }

    void SetBmer(int32_t bmer) {
        this->bmer = bmer;
    }

    int32_t &GetPartitions() {
        return this->pnum;
    }

    int32_t &GetAln() {
        return this->alen;
    }

    int32_t &GetNHash() {
        return this->nhash;
    }
};


class SearchCommand : public tasker::Command {
   private:
    std::string src_file;
    std::string index_file;
    std::string dst_file;
    void Validate(int32_t *code, std::string *msg);

   public:
    SearchCommand(std::string cmd) : tasker::Command(cmd) {}

    void Parse(int32_t *code, std::string *msg);

    std::string &GetSrcFile() {
        return this->src_file;
    }

    std::string &GetIndexFile(){
        return this->index_file;
    }

    std::string &DstFile(){
        return this->dst_file;
    }
};
#endif /* C37E4C99_F798_4490_A00F_CD48E100A69D */
