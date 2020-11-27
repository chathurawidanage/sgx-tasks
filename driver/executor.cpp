#include "executor.hpp"

tasker::JobExecutor::JobExecutor(tasker::Driver &driver) : driver(driver) {
}

std::shared_ptr<tasker::WorkerHandler> tasker::JobExecutor::AllocateWorker(Job &to, std::string worker_type) {
    if (!available_workers.empty()) {
        std::shared_ptr<tasker::WorkerHandler> allocated_worker = available_workers.front();
        available_workers.pop();
        return allocated_worker;
    } else {
        return NULL;
    }
}

void tasker::JobExecutor::AddJob(Job &job) {
    lock.lock();
    this->jobs.push_back(job);
    lock.unlock();
}

void tasker::JobExecutor::Progress() {
    while (true) {
        if (this->jobs.empty()) {
            // todo replace with condition variables and locks
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        this->lock.lock();
        std::list<Job>::iterator i = this->jobs.begin();
        while (i != this->jobs.end()) {
            bool done = (*i).Progress();
            if (done) {
                (*i).Finalize();
                this->jobs.erase(i);
            }
            i++;
        }
        this->lock.unlock();
    }
}

void tasker::JobExecutor::Start() {
    spdlog::info("Starting job executor...");
    std::thread trd = std::thread(&JobExecutor::Progress, this);
    trd.join();
}

void tasker::JobExecutor::ReleaseWorker(std::shared_ptr<WorkerHandler> worker) {
    this->available_workers.push(worker);
}