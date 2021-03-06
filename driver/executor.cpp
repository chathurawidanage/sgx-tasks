#include "executor.hpp"

#include <chrono>

tasker::JobExecutor::JobExecutor(tasker::Driver &driver) : driver(driver) {
}

bool tasker::JobExecutor::HasWorker(std::string &worker_id) {
    return this->all_workers.find(worker_id) != this->all_workers.end();
}

std::shared_ptr<tasker::WorkerHandler> tasker::JobExecutor::AllocateWorker(Job &to, std::string worker_type) {
    this->workers_lock.lock();
    auto workers_of_type = this->available_workers.find(worker_type);
    if (workers_of_type != this->available_workers.end() && !workers_of_type->second.empty()) {
        std::shared_ptr<tasker::WorkerHandler> allocated_worker = workers_of_type->second.front();

        // check thether the worker is healthy
        this->ping_lock.lock();
        auto ping_it = this->ping_times.find(allocated_worker->GetId());
        if (ping_it != this->ping_times.end()) {
            int64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            if (timestamp - ping_it->second > this->ping_timeout) {
                spdlog::info("Dead worker detected. Removing {} from the available workers...",
                             allocated_worker->GetId());
                spdlog::info("Timestamp : {}, Last Ping: {}", timestamp, ping_it->second);
                // remove worker from avilable workers
                workers_of_type->second.pop();
                this->all_workers.erase(allocated_worker->GetId());

                // todo remove from ping map
                this->workers_lock.unlock();
                this->ping_lock.unlock();
                return nullptr;
            }
        }
        this->ping_lock.unlock();

        // assign worker to the job
        this->worker_assignment.insert(std::make_pair<>(allocated_worker->GetId(), to.GetId()));

        // remove from the available workers
        workers_of_type->second.pop();

        // add to the list of busy workers
        this->busy_workers.insert(std::make_pair<>(allocated_worker->GetId(), allocated_worker));
        this->workers_lock.unlock();
        return allocated_worker;
    } else {
        this->workers_lock.unlock();
        return nullptr;
    }
}

void tasker::JobExecutor::AddWorker(std::string worker_id, std::string worker_type) {
    this->workers_lock.lock();
    this->all_workers.insert(worker_id);

    if (this->available_workers.find(worker_type) == this->available_workers.end()) {
        this->available_workers.insert(std::make_pair<>(worker_type, std::queue<std::shared_ptr<tasker::WorkerHandler>>()));
    }

    this->available_workers.find(worker_type)->second.push(std::make_shared<tasker::WorkerHandler>(worker_id, worker_type, driver));
    this->workers_lock.unlock();

    // consider this as the first ping
    this->ping_lock.lock();
    int64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    this->ping_times.insert(std::make_pair<>(worker_id, timestamp));
    this->ping_lock.unlock();
}

void tasker::JobExecutor::AddJob(std::shared_ptr<Job> job) {
    if (this->jobs_lock.try_lock()) {
        this->jobs.insert(std::make_pair<>(job->GetId(), job));
        this->jobs_lock.unlock();
    } else {
        this->jobs_temp.insert(std::make_pair<>(job->GetId(), job));
    }
}

void tasker::JobExecutor::OnPing(std::string &from_worker) {
    this->ping_lock.lock();
    int64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    if (this->HasWorker(from_worker)) {
        if (this->ping_times.find(from_worker) == this->ping_times.end()) {
            this->ping_times.insert(std::make_pair<>(from_worker, timestamp));
        } else {
            this->ping_times[from_worker] = timestamp;
        }
    } else {
        spdlog::warn("Ping received from an unknown worker...");
    }
    this->ping_lock.unlock();
}

void tasker::JobExecutor::IdentifyFailures() {
    // this invloves reading/writing all data structures. Aquiring all locks
    this->ping_lock.lock();
    this->workers_lock.lock();
    this->jobs_lock.lock();
    int64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    // check for deadworkers in the already allocated workers
    auto worker_asg_it = this->worker_assignment.begin();
    while (worker_asg_it != this->worker_assignment.end()) {
        std::string worker_id = worker_asg_it->first;
        auto ping_for_worker = this->ping_times.find(worker_id);
        if (ping_for_worker != this->ping_times.end() && (timestamp - ping_for_worker->second) > this->ping_timeout) {
            // met the condition for ping timeout
            spdlog::info("Worker {} has been identified as a failure", worker_id);

            auto job_it = this->jobs.find(worker_asg_it->second);
            if (job_it == this->jobs.end()) {
                spdlog::warn("Worker assignment contains an unknown job id", worker_asg_it->second);
            } else {
                // report the job, so it can request for another worker
                job_it->second->OnWorkerRevoked(worker_id);

                // remove from the worker allocation and put it back to the queue, giving it more time to connect
                worker_asg_it = this->worker_assignment.erase(worker_asg_it);

                // remove it from the busy workers and putting back to the available queue
                auto busy_it = this->busy_workers.find(worker_id);
                if (busy_it == this->busy_workers.end()) {
                    spdlog::warn("Couldn't find the worker in the busy workers list. Something wrong!");
                } else {
                    this->available_workers.find(busy_it->second->GetType())->second.push(busy_it->second);
                    this->busy_workers.erase(worker_id);
                }
            }
        } else {
            worker_asg_it++;
        }
    }
    this->ping_lock.unlock();
    this->workers_lock.unlock();
    this->jobs_lock.unlock();
}

void tasker::JobExecutor::AddTempJobs() {
    if (this->jobs_temp.empty()) {
        return;
    }
    this->jobs_lock.lock();
    auto temp_jobs_it = this->jobs_temp.begin();
    spdlog::info("Adding {} jobs to the queue", this->jobs_temp.size());

    auto it = this->jobs_temp.begin();
    while (it != this->jobs_temp.end()) {
        this->jobs.insert(std::move(*it));
        it++;
    }

    this->jobs_temp.clear();

    spdlog::info("Jobs left in temp job queue {}. Jobs in jobs queue {}", this->jobs_temp.size(), this->jobs.size());

    this->jobs_lock.unlock();
}

void tasker::JobExecutor::Progress() {
    int32_t idle_count = 0;
    while (true) {
        this->IdentifyFailures();
        this->AddTempJobs();

        if (this->jobs.empty()) {
            // todo replace with condition variables and locks
            std::this_thread::sleep_for(std::chrono::seconds(10));
            continue;
        }
        this->jobs_lock.lock();
        std::unordered_map<std::string, std::shared_ptr<Job>>::iterator i = this->jobs.begin();
        while (i != this->jobs.end()) {
            spdlog::debug("Processing job {}", i->second->GetId());
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
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void tasker::JobExecutor::Start() {
    spdlog::info("Starting job executor...");
    std::thread trd = std::thread(&JobExecutor::Progress, this);
    trd.join();
}

void tasker::JobExecutor::ReleaseWorker(Job &of, std::shared_ptr<WorkerHandler> worker) {
    this->workers_lock.lock();
    std::unordered_map<std::string, std::string>::iterator it = this->worker_assignment.find(worker->GetId());
    // <worker_id, job_id>
    if (it != this->worker_assignment.end()) {
        if (it->second.compare(of.GetId()) == 0) {
            // remove the worker assignment
            this->worker_assignment.erase(it);

            // take it out from busy worker
            this->busy_workers.erase(worker->GetId());

            // make this worker avaialble again
            this->available_workers.find(worker->GetType())->second.push(worker);
        } else {
            spdlog::warn("Job {} requested to release the worker {}, which is not assigned to that job",
                         of.GetId(), worker->GetId());
        }
    }
    this->workers_lock.unlock();
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