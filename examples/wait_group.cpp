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
    context ctx[4];
    wg.add(3);

    ctx[0].submit_task(wait());
    ctx[1].submit_task(done());
    ctx[2].submit_task(done());
    ctx[3].submit_task(done());

    ctx[0].start();
    ctx[1].start();
    ctx[2].start();
    ctx[3].start();

    ctx[0].stop();
    ctx[1].stop();
    ctx[2].stop();
    ctx[3].stop();

    return 0;
}
