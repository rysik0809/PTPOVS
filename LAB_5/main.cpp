#include "async_processor.h"
#include "console_ui.h"
#include "exceptions.h"
#include "file_watcher.h"
#include "worker_pool.h"

#include <filesystem>
#include <iostream>

int main(int argc, char** argv) {
    const std::string watchDir = (argc > 1) ? argv[1] : "./watched";

    try {
        // Каталог для результатов и журнала создаём всегда.
        std::error_code ec;
        std::filesystem::create_directories("./results", ec);

        // Проверяем/создаём каталог отслеживания; бросает DirectoryAccessException
        // при отсутствии прав — ловим и выходим.
        try {
            ensureDirectoryReadable(watchDir);
        }
        catch (const DirectoryAccessException& e) {
            logException("main", e);
            std::cerr << "Не удалось открыть каталог отслеживания: "
                      << e.what() << "\n";
            return 1;
        }

        std::cout << "Каталог отслеживания: " << watchDir << "\n";
        std::cout << "Создаются файлы результатов в ./results/\n";
        std::cout << "Журнал исключений:    " << exceptionLogPath() << "\n";
        std::cout << "Положите *.md файлы в " << watchDir
                  << " — они будут обнаружены автоматически.\n";
        std::cout << "Ограничение размера файла: "
                  << (MAX_FILE_SIZE_BYTES / (1024 * 1024)) << " Мб.\n";

        ThreadSafeQueue<std::string> queue;
        ResultsStorage               results;
        FileWatcher                  watcher(queue);
        WorkerPool                   pool(queue, results);
        AsyncProcessor               async;

        try {
            watcher.start(watchDir);
        }
        catch (const DirectoryAccessException& e) {
            logException("main", e);
            std::cerr << "Не удалось запустить наблюдатель: "
                      << e.what() << "\n";
            return 1;
        }

        pool.start(0);

        std::cout << "Фоновых потоков обработки: "
                  << pool.threadCount() << "\n";

        ConsoleUI ui(watcher, pool, async, results, queue);
        ui.run();

        pool.stop();
        watcher.stop();
        async.cancel();
        async.wait();
        return 0;
    }
    catch (const std::exception& e) {
        logException("main", e);
        std::cerr << "Критическая ошибка: " << e.what() << "\n";
        return 1;
    }
}