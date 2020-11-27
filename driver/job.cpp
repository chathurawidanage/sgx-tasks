#include "job.hpp"

tasker::Job::Job(std::string &client_id, std::shared_ptr<tasker::JobExecutor> executor) : client_id(client_id), executor(executor) {
}

bool tasker::Job::Progress() {
    return true;
}

void tasker::Job::Finalize() {
    // nothing will be done
}

void tasker::Job::OnWorkerMessage(std::string &worker_id, std::string &msg) {
    // nothing will be done
}