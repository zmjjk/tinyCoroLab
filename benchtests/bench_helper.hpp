#pragma once

#include <functional>
#include <queue>
#include <thread>

#define loop_add                                                                                                       \
    size_t cnt{0};                                                                                                     \
    for (int i = 0; i < loop_num; i++)                                                                                 \
    {                                                                                                                  \
        benchmark::DoNotOptimize(cnt += 1);                                                                            \
    }

class [[deprecated("maybe unused")]] thread_pool
{
public:
    using task_type = std::function<void()>;
    explicit thread_pool(int thread_num = std::thread::hardware_concurrency()) noexcept
        : m_thread_num(thread_num),
          m_cur_pos(0)
    {
        m_ques.resize(m_thread_num);
    }

    void start() noexcept
    {
        for (int i = 0; i < m_thread_num; i++)
        {
            m_threads.push_back(std::thread([&, i]() { this->run(i); }));
        }
    }

    void submit_task(task_type task)
    {
        m_ques[m_cur_pos].push(std::move(task));
        m_cur_pos = (m_cur_pos + 1) % m_thread_num;
    }

    void run(int id) noexcept
    {
        while (!m_ques[id].empty())
        {
            auto task = m_ques[id].front();
            m_ques[id].pop();
            task();
        }
    }

    void join()
    {
        for (int i = 0; i < m_thread_num; i++)
        {
            m_threads[i].join();
        }
    }

private:
    const int                          m_thread_num;
    int                                m_cur_pos;
    std::vector<std::queue<task_type>> m_ques;
    std::vector<std::thread>           m_threads;
};

inline const char* bench_str =
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";