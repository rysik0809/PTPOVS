#include "console_ui.h"
#include "exceptions.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <vector>

ConsoleUI::ConsoleUI(FileWatcher&      watcher,
                     WorkerPool&       pool,
                     AsyncProcessor&   async,
                     ResultsStorage&   results,
                     ThreadSafeQueue<std::string>& queue)
    : watcher_(watcher),
      pool_(pool),
      async_(async),
      results_(results),
      queue_(queue) {}

std::string ConsoleUI::promptLine(const std::string& prompt) const {
    std::lock_guard<std::mutex> lock(ioMutex_);
    std::cout << prompt;
    std::string s;
    std::getline(std::cin, s);
    return s;
}

int ConsoleUI::promptInt(const std::string& prompt) const {
    std::lock_guard<std::mutex> lock(ioMutex_);
    std::cout << prompt;
    int v = 0;
    std::cin >> v;
    if (!std::cin) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return -1;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return v;
}

void ConsoleUI::printMenu() const {
    std::lock_guard<std::mutex> lock(ioMutex_);
    std::cout << "\n=== Лабораторная №5: обработка исключительных ситуаций ===\n";
    std::cout << " 1. Показать состояние очереди\n";
    std::cout << " 2. Показать результат обработки файла\n";
    std::cout << " 3. Изменить каталог отслеживания\n";
    std::cout << " 4. Сбросить результаты обработки\n";
    std::cout << " 5. Сбросить состояние очереди\n";
    std::cout << " 6. Запустить асинхронную обработку каталога\n";
    std::cout << " 7. Отменить асинхронную операцию\n";
    std::cout << " 8. Показать статус асинхронной обработки\n";
    std::cout << " 9. Показать текущий каталог\n";
    std::cout << "10. Показать журнал исключений\n";
    std::cout << " 0. Выход\n";
    std::cout << "Выбор: ";
}

void ConsoleUI::run() {
    while (true) {
        printMenu();
        int choice = promptInt("");

        switch (choice) {
            case 1: handleQueueState();         break;
            case 2: handleFileResult();         break;
            case 3: handleChangeDirectory();    break;
            case 4: handleResetResults();       break;
            case 5: handleResetQueue();         break;
            case 6: handleAsyncStart();         break;
            case 7: handleAsyncCancel();        break;
            case 8: handleAsyncStatus();        break;
            case 9: handleShowDirectory();      break;
            case 10: handleShowExceptionsLog(); break;
            case 0: return;
            default:
                std::cout << "Неизвестный пункт меню.\n";
        }
    }
}

void ConsoleUI::handleQueueState() const {
    const auto items = queue_.snapshot();
    std::lock_guard<std::mutex> lock(ioMutex_);
    std::cout << "\n--- Очередь (" << items.size() << " файлов) ---\n";
    if (items.empty()) {
        std::cout << "(пусто)\n";
        return;
    }
    for (const auto& p : items) std::cout << "  " << p << "\n";
}

void ConsoleUI::handleFileResult() const {
    std::string path = promptLine("Путь к файлу: ");
    if (path.empty()) return;

    const auto result = results_.get(path);
    std::lock_guard<std::mutex> lock(ioMutex_);
    if (!result) {
        std::cout << "Информации по файлу нет.\n";
        return;
    }
    std::cout << "\nФайл:    " << result->path << "\n";
    std::cout << "Статус:  " << statusToString(result->status) << "\n";
    if (result->status == FileStatus::Done) {
        std::cout << "Строк:                    " << result->stats.totalLines   << "\n";
        std::cout << "Заголовков:               " << result->stats.headings     << "\n";
        std::cout << "Абзацев:                  " << result->stats.paragraphs   << "\n";
        std::cout << "Элементов списков:        " << result->stats.listItems    << "\n";
        std::cout << "Строк с чужой разметкой:  " << result->stats.foreignLines << "\n";
        std::cout << "Полный результат:         " << result->resultsFile        << "\n";
    } else if (result->status == FileStatus::Error) {
        // Здесь, в частности, отображаются сообщения из пользовательских исключений:
        //   - слишком большой файл,
        //   - удалённый файл,
        //   - неверный формат и т.п.
        std::cout << "Ошибка: " << result->error << "\n";
    } else if (result->status == FileStatus::Cancelled) {
        std::cout << "Обработка была отменена.\n";
    }
}

void ConsoleUI::handleChangeDirectory() {
    std::string dir = promptLine("Новый каталог отслеживания: ");
    if (dir.empty()) return;
    try {
        watcher_.setDirectory(dir);
        std::cout << "Каталог отслеживания: " << dir << "\n";
    }
    catch (const DirectoryAccessException& e) {
        logException("ConsoleUI::handleChangeDirectory", e);
        std::cout << "Ошибка: " << e.what() << "\n";
    }
    catch (const std::exception& e) {
        logException("ConsoleUI::handleChangeDirectory", e);
        std::cout << "Ошибка: " << e.what() << "\n";
    }
}

void ConsoleUI::handleResetResults() const {
    results_.clear();
    std::cout << "Результаты обработки сброшены.\n";
}

void ConsoleUI::handleResetQueue() {
    queue_.clear();
    watcher_.resetSeen();
    results_.markAllQueuedAsCancelled();
    std::cout << "Состояние очереди сброшено.\n";
}

void ConsoleUI::handleAsyncStart() {
    if (async_.isRunning()) {
        std::cout << "Асинхронная обработка уже идёт. Сначала отмените её.\n";
        return;
    }

    const std::string dir = watcher_.getDirectory();
    if (dir.empty()) {
        std::cout << "Каталог отслеживания не задан.\n";
        return;
    }

    std::vector<std::string> files;
    try {
        std::error_code ec;
        std::filesystem::directory_iterator it(dir, ec);
        if (ec) {
            throw DirectoryAccessException(dir, ec.message());
        }
        for (const auto& e : it) {
            std::error_code statEc;
            if (e.is_regular_file(statEc) && !statEc) {
                files.push_back(e.path().string());
            }
        }
    }
    catch (const std::exception& e) {
        logException("ConsoleUI::handleAsyncStart", e);
        std::cout << "Ошибка чтения каталога: " << e.what() << "\n";
        return;
    }

    if (files.empty()) {
        std::cout << "В каталоге нет файлов: " << dir << "\n";
        return;
    }

    async_.start(files);
    std::cout << "Запущена асинхронная обработка " << files.size()
              << " файлов на " << async_.threadCount() << " потоках.\n";
}

void ConsoleUI::handleAsyncCancel() const {
    if (!async_.isRunning()) {
        std::cout << "Асинхронная обработка не запущена.\n";
        return;
    }
    async_.cancel();
    std::cout << "Отправлен сигнал отмены асинхронной операции.\n";
}

void ConsoleUI::handleAsyncStatus() const {
    const auto snapshot = async_.status();
    std::lock_guard<std::mutex> lock(ioMutex_);

    if (snapshot.empty() && !async_.isRunning()) {
        std::cout << "Асинхронных задач ещё не было.\n";
        return;
    }
    std::cout << "\n--- Статус асинхронной обработки ---\n";
    for (const auto& r : snapshot) {
        std::cout << "  [" << statusToString(r.status) << "] " << r.path;
        if (r.status == FileStatus::Done) {
            std::cout << "   H=" << r.stats.headings
                      << " P=" << r.stats.paragraphs
                      << " L=" << r.stats.listItems
                      << " F=" << r.stats.foreignLines;
        } else if (r.status == FileStatus::Error) {
            std::cout << "   ошибка: " << r.error;
        }
        std::cout << "\n";
    }

    if (async_.isRunning()) {
        std::cout << "Асинхронная операция ещё выполняется (прошло "
                  << async_.elapsedTime().count() << " мс).\n";
    } else if (!snapshot.empty()) {
        std::cout << "Асинхронная операция завершена за "
                  << async_.elapsedTime().count() << " мс.\n";
    }
}

void ConsoleUI::handleShowDirectory() const {
    std::cout << "Каталог отслеживания: " << watcher_.getDirectory() << "\n";
}

void ConsoleUI::handleShowExceptionsLog() const {
    const std::string path = exceptionLogPath();
    std::lock_guard<std::mutex> lock(ioMutex_);
    std::cout << "\nЖурнал исключений: " << path << "\n";
    std::ifstream in(path);
    if (!in.is_open()) {
        std::cout << "(журнал пуст)\n";
        return;
    }
    std::string line;
    bool any = false;
    while (std::getline(in, line)) {
        std::cout << line << "\n";
        any = true;
    }
    if (!any) std::cout << "(журнал пуст)\n";
}