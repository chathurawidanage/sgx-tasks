#include "executor.hpp"

tasker::JobExecutor::JobExecutor(tasker::Driver &driver) : driver(driver) {
}

std::shared_ptr<tasker::WorkerHandler> tasker::JobExecutor::AllocateWorker(Job &to, std::string worker_type) {
    this->workers_lock.lock();
    if (!available_workers.empty()) {
        std::shared_ptr<tasker::WorkerHandler> allocated_worker = available_workers.front();
        this->worker_assignment.insert(std::make_pair<>(allocated_worker->GetId(), to.GetId()));
        available_workers.pop();
        this->workers_lock.unlock();
        return allocated_worker;
    } else {
        this->workers_lock.unlock();
        return nullptr;
    }
}

void tasker::JobExecutor::AddWorker(std::string &worker_id, std::string &worker_type) {
    this->workers_lock.lock();
    this->available_workers.push(std::make_shared<tasker::WorkerHandler>(worker_id, worker_type, driver));
    this->workers_lock.unlock();
}

void tasker::JobExecutor::AddJob(std::shared_ptr<Job> job) {
    jobs_lock.lock();
    this->jobs.insert(std::make_pair<>(job->GetId(), job));
    jobs_lock.unlock();
}

void tasker::JobExecutor::Progress() {
    while (true) {
        if (this->jobs.empty()) {
            // todo replace with condition variables and locks
            std::this_thread::sleep_for(std::chrono::seconds(10));
            spdlog::info("No jobs to process....");
        }
        this->jobs_lock.lock();
        std::unordered_map<std::string, std::shared_ptr<Job>>::iterator i = this->jobs.begin();
        while (i != this->jobs.end()) {
            spdlog::info("Processing job {}", i->second->GetId());
            bool done = i->second->Progress();
            if (done) {
                spdlog::info("Job {} has been reported as done", i->second->GetId());
                i->second->Finalize();
                i = this->jobs.erase(i);
            } else {
                i++;
            }
        }
        this->jobs_lock.unlock();
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

void tasker::JobExecutor::Start() {
    spdlog::info("Starting job executor...");
    std::thread trd = std::thread(&JobExecutor::Progress, this);
    trd.join();
}

void tasker::JobExecutor::ReleaseWorker(Job &of, std::shared_ptr<WorkerHandler> worker) {
    std::unordered_map<std::string, std::string>::iterator it = this->worker_assignment.find(worker->GetId());
    // <worker_id, job_id>
    if (it != this->worker_assignment.end()) {
        if (it->second.compare(of.GetId()) == 0) {
            // remove the worker assignment
            this->worker_assignment.erase(it);

            // make this worker avaialble again
            this->available_workers.push(worker);
        } else {
            spdlog::warn("Job {} requested to release the worker {}, which is not assigned to that job",
                         of.GetId(), worker->GetId());
        }
    }
}

void tasker::JobExecutor::ForwardMsgToJob(std::string &from, std::string &msg) {
    std::unordered_map<std::string, std::string>::iterator it = this->worker_assignment.find(from);
    spdlog::debug("Finding worker {} to forward message", from);
    if (it != this->worker_assignment.end()) {
        spdlog::debug("Forwarding message from worker {} to {}", from, it->second);
        this->jobs.find(it->second)->second->OnWorkerMessage(from, msg);
    } else {
        spdlog::debug("Couldn't find worker {} to forward the message", from);
    }
}