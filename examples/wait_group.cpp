#include "coro/coro.hpp"

using namespace coro;

wait_group wg{0};

task<> wait()
{
    co_await wg.wait();
    log::info("wait finish");
}

task<> done()
{
    utils::sleep(2);
    wg.done();
    log::info("done...");
    co_return;
}

int main(int argc, char const* argv[])
{
    /* code */
    scheduler::init();
    wg.add(3);

    scheduler::submit(wait());
    scheduler::submit(done());
    scheduler::submit(done());
    scheduler::submit(done());

    scheduler::start();
    scheduler::stop();

    return 0;
}
