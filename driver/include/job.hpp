#ifndef C06598CE_9FFF_46D9_9E7B_B74913B603B3
#define C06598CE_9FFF_46D9_9E7B_B74913B603B3

#include <memory>
#include <string>

namespace tasker {
class JobExecutor;

class Job {
   protected:
    std::string &client_id;
    std::shared_ptr<tasker::JobExecutor> executor;

   public:
    Job(std::string &client_id, std::shared_ptr<tasker::JobExecutor> executor);

    void OnWorkerMessage(std::string &worker_id, std::string &msg);

    /**
     * Returns true if job is completed
     * */
    bool Progress();

    void Finalize();
};
}  // namespace tasker

#endif /* C06598CE_9FFF_46D9_9E7B_B74913B603B3 */
