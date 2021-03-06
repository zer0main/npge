/*
 * NPG-explorer, Nucleotide PanGenome explorer
 * Copyright (C) 2012-2016 Boris Nagaev
 *
 * See the LICENSE file for terms of use.
 */

#include <vector>
#include "boost-xtime.hpp"
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

#include "thread_group.hpp"
#include "Exception.hpp"
#include "throw_assert.hpp"

namespace npge {

// No threads beyond this file!

ThreadTask::ThreadTask(ThreadWorker* worker):
    worker_(worker), thread_group_(worker_->thread_group()) {
}

ThreadTask::~ThreadTask() {
}

void ThreadTask::run() {
    run_impl();
}

ThreadWorker* ThreadTask::worker() const {
    return worker_;
}

ThreadGroup* ThreadTask::thread_group() const {
    return thread_group_;
}

ThreadWorker::ThreadWorker(ThreadGroup* thread_group):
    thread_group_(thread_group) {
}

ThreadWorker::~ThreadWorker() {
    thread_group()->check_worker(this);
}

void ThreadWorker::perform() {
    perform_impl();
}

void ThreadWorker::work() {
    work_impl();
}

void ThreadWorker::run(ThreadTask* task) {
    run_impl(task);
}

ThreadGroup* ThreadWorker::thread_group() const {
    return thread_group_;
}

const std::string& ThreadWorker::error_message() const {
    return error_message_;
}

void ThreadWorker::perform_impl() {
    if (thread_group()->workers() == 1) {
        work();
    } else {
        try {
            work();
        } catch (std::exception& e) {
            error_message_ = e.what();
        } catch (...) {
            error_message_ = "unknown error";
        }
    }
}

void ThreadWorker::work_impl() {
    while (true) {
        typedef boost::scoped_ptr<ThreadTask> ThreadTaskPtr;
        ThreadTaskPtr task(thread_group()->create_task(this));
        if (task) {
            run(task.get());
        } else {
            break;
        }
    }
}

void ThreadWorker::run_impl(ThreadTask* task) {
    task->run();
}

struct ThreadGroup::Impl {
    boost::mutex mutex_;
    int workers_;
    std::string error_message_;

    Impl():
        workers_(1) {
    }
};

ThreadGroup::ThreadGroup():
    impl_(new Impl) {
}

ThreadGroup::~ThreadGroup() {
    delete impl_;
    impl_ = 0;
}

void ThreadGroup::perform() {
    ASSERT_GTE(workers(), 1);
    impl_->error_message_ = "";
    perform_impl();
    if (!impl_->error_message_.empty()) {
        throw Exception(impl_->error_message_);
    }
}

ThreadTask* ThreadGroup::create_task(ThreadWorker* worker) {
    check_worker(worker);
    if (!impl_->error_message_.empty()) {
        return 0;
    }
    if (workers() == 1) {
        return create_task_impl(worker);
    } else {
        boost::mutex::scoped_lock lock(impl_->mutex_);
        return create_task_impl(worker);
    }
}

ThreadWorker* ThreadGroup::create_worker() {
    return create_worker_impl();
}

void ThreadGroup::check_worker(ThreadWorker* worker) {
    check_worker_impl(worker);
}

void ThreadGroup::set_workers(int workers) {
    impl_->workers_ = workers;
}

int ThreadGroup::workers() const {
    if (impl_->workers_ == -1) {
        return boost::thread::hardware_concurrency();
    } else {
        return impl_->workers_;
    }
}

struct AllJoiner {
    boost::thread_group& threads_;

    AllJoiner(boost::thread_group& threads):
        threads_(threads) {
    }

    ~AllJoiner() {
        threads_.join_all();
    }
};

void ThreadGroup::perform_impl() {
    typedef boost::shared_ptr<ThreadWorker> ThreadWorkerPtr;
    std::vector<ThreadWorkerPtr> workers_list;
    for (int i = 0; i < workers(); i++) {
        workers_list.push_back(ThreadWorkerPtr(create_worker()));
    }
    boost::thread_group threads;
    for (int i = 1; i < workers(); i++) {
        ThreadWorker* worker = workers_list[i].get();
        threads.create_thread(boost::bind(
                                  &ThreadWorker::perform,
                                  worker));
    }
    ThreadWorker* worker = workers_list[0].get();
    AllJoiner all_joiner(threads);
    worker->perform();
    // threads.join_all() is called here
    // workers are deleted here
}

ThreadWorker* ThreadGroup::create_worker_impl() {
    return new ThreadWorker(this);
}

void ThreadGroup::check_worker_impl(ThreadWorker* worker) {
    if (!worker->error_message().empty()) {
        boost::mutex::scoped_lock lock(impl_->mutex_);
        impl_->error_message_ = worker->error_message();
    }
}

}

