#include "async_processor.h"
#include "console_ui.h"
#include "file_watcher.h"
#include "worker_pool.h"

#include <filesystem>
#include <iostream>

int main(int argc, char** argv) {
    const std::string watchDir = (argc > 1) ? argv[1] : "./watched";

    std::error_code ec;
    std::filesystem::create_directories(watchDir, ec);
    std::filesystem::create_directories("./results", ec);

    std::cout << "Каталог отслеживания: " << watchDir << "\n";
    std::cout << "Создаются файлы результатов в ./results/\n";
    std::cout << "Положите *.md файлы в " << watchDir
              << " — они будут обнаружены автоматически.\n";

    ThreadSafeQueue<std::string> queue;
    ResultsStorage               results;
    FileWatcher                  watcher(queue);
    WorkerPool                   pool(queue, results);
    AsyncProcessor               async;

    watcher.start(watchDir);
    pool.start(0);                      // 0 = auto (hardware_concurrency)

    std::cout << "Фоновых потоков обработки: " << pool.threadCount() << "\n";

    ConsoleUI ui(watcher, pool, async, results, queue);
    ui.run();

    pool.stop();
    watcher.stop();
    async.cancel();
    async.wait();
    return 0;
}