module;

#include <array>
#include <cstdio>
#include <expected>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>

#ifdef __APPLE__
#include <crt_externs.h>
#endif
#endif

module craft.util;
import craft.core;

namespace craft::util {

namespace {

Error CannotRun(const std::string& program, int code) {
    return Error("cannot run " + program + ": " +
                 std::error_code(code, std::system_category()).message());
}

#ifdef _WIN32

std::wstring ToWide(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    const int size = MultiByteToWideChar(
        CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring wide(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                        wide.data(), size);
    return wide;
}

// Quotes one argument the way CommandLineToArgvW expects it.
std::wstring QuoteArgument(const std::string& argument) {
    const std::wstring wide = ToWide(argument);
    if (!wide.empty() &&
        wide.find_first_of(L" \t\n\v\"") == std::wstring::npos) {
        return wide;
    }
    std::wstring quoted = L"\"";
    std::size_t backslashes = 0;
    for (const wchar_t c : wide) {
        if (c == L'\\') {
            ++backslashes;
            continue;
        }
        if (c == L'"') {
            quoted.append(backslashes * 2 + 1, L'\\');
        } else {
            quoted.append(backslashes, L'\\');
        }
        backslashes = 0;
        quoted += c;
    }
    quoted.append(backslashes * 2, L'\\');
    quoted += L'"';
    return quoted;
}

std::expected<ProcessResult, Error> Run(const std::vector<std::string>& args,
                                        const ProcessOptions& options) {
    SECURITY_ATTRIBUTES attributes{};
    attributes.nLength = sizeof(attributes);
    attributes.bInheritHandle = TRUE;

    HANDLE read_end = nullptr;
    HANDLE write_end = nullptr;
    if (options.capture_output) {
        if (CreatePipe(&read_end, &write_end, &attributes, 0) == 0) {
            return std::unexpected(
                CannotRun(args[0], static_cast<int>(GetLastError())));
        }
        SetHandleInformation(read_end, HANDLE_FLAG_INHERIT, 0);
    }

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput =
        options.capture_output ? write_end : GetStdHandle(STD_OUTPUT_HANDLE);
    startup.hStdError =
        options.capture_output ? write_end : GetStdHandle(STD_ERROR_HANDLE);

    std::wstring command_line;
    for (const auto& argument : args) {
        if (!command_line.empty()) {
            command_line += L' ';
        }
        command_line += QuoteArgument(argument);
    }

    PROCESS_INFORMATION process{};
    const BOOL started =
        CreateProcessW(nullptr, command_line.data(), nullptr, nullptr, TRUE, 0,
                       nullptr, nullptr, &startup, &process);
    const DWORD start_error = GetLastError();
    if (write_end != nullptr) {
        CloseHandle(write_end);
    }
    if (started == 0) {
        if (read_end != nullptr) {
            CloseHandle(read_end);
        }
        return std::unexpected(
            CannotRun(args[0], static_cast<int>(start_error)));
    }

    ProcessResult result;
    if (read_end != nullptr) {
        char buffer[4096];
        DWORD count = 0;
        while (ReadFile(read_end, buffer, sizeof(buffer), &count, nullptr) !=
                   0 &&
               count > 0) {
            result.output.append(buffer.data(), count);
        }
        CloseHandle(read_end);
    }

    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exit_code = 1;
    GetExitCodeProcess(process.hProcess, &exit_code);
    CloseHandle(process.hProcess);
    CloseHandle(process.hThread);

    result.exit_code = static_cast<int>(exit_code);
    return result;
}

#else

// Closes a file descriptor when it goes out of scope.
class Descriptor {
public:
    explicit Descriptor(int fd = -1) : fd_(fd) {}
    ~Descriptor() { Close(); }
    Descriptor(const Descriptor&) = delete;
    Descriptor& operator=(const Descriptor&) = delete;

    [[nodiscard]] int get() const { return fd_; }

    void Close() {
        if (fd_ >= 0) {
            close(fd_);
            fd_ = -1;
        }
    }

private:
    int fd_;
};

// posix_spawn_file_actions_t that is destroyed when it goes out of scope.
class FileActions {
public:
    FileActions() { status_ = posix_spawn_file_actions_init(&actions_); }
    ~FileActions() {
        if (status_ == 0) {
            posix_spawn_file_actions_destroy(&actions_);
        }
    }
    FileActions(const FileActions&) = delete;
    FileActions& operator=(const FileActions&) = delete;

    [[nodiscard]] int status() const { return status_; }
    [[nodiscard]] posix_spawn_file_actions_t* get() { return &actions_; }

private:
    posix_spawn_file_actions_t actions_{};
    int status_ = 0;
};

char** Environment() {
#ifdef __APPLE__
    return *_NSGetEnviron();
#else
    return environ;
#endif
}

std::string ReadAll(int fd) {
    std::string text;
    std::array<char, 4096> buffer{};
    for (;;) {
        const ssize_t count = read(fd, buffer.data(), buffer.size());
        if (count > 0) {
            text.append(buffer.data(), static_cast<std::size_t>(count));
        } else if (count < 0 && errno == EINTR) {
            continue;
        } else {
            break;
        }
    }
    return text;
}

std::expected<ProcessResult, Error> Run(const std::vector<std::string>& args,
                                        const ProcessOptions& options) {
    std::vector<std::string> storage = args;
    std::vector<char*> argv;
    argv.reserve(storage.size() + 1);
    for (auto& argument : storage) {
        argv.push_back(argument.data());
    }
    argv.push_back(nullptr);

    FileActions actions;
    if (actions.status() != 0) {
        return std::unexpected(CannotRun(args[0], actions.status()));
    }

    // With a pipe, the program writes stdout and stderr into it and Craft
    // reads it until the program closes it.
    std::array<int, 2> fds = {-1, -1};
    if (options.capture_output && pipe(fds.data()) != 0) {
        return std::unexpected(CannotRun(args[0], errno));
    }
    const Descriptor read_end(fds[0]);
    Descriptor write_end(fds[1]);
    if (options.capture_output) {
        posix_spawn_file_actions_adddup2(actions.get(), write_end.get(),
                                         STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(actions.get(), write_end.get(),
                                         STDERR_FILENO);
        posix_spawn_file_actions_addclose(actions.get(), read_end.get());
        posix_spawn_file_actions_addclose(actions.get(), write_end.get());
    }

    // Output of Craft itself must come before the output of the program.
    std::fflush(nullptr);

    pid_t pid = 0;
    const int spawned = posix_spawnp(&pid, argv[0], actions.get(), nullptr,
                                     argv.data(), Environment());
    if (spawned != 0) {
        return std::unexpected(CannotRun(args[0], spawned));
    }
    // Close our copy of the write end, or the read below never ends.
    write_end.Close();

    ProcessResult result;
    if (options.capture_output) {
        result.output = ReadAll(read_end.get());
    }

    int status = 0;
    pid_t waited = 0;
    do {
        waited = waitpid(pid, &status, 0);
    } while (waited < 0 && errno == EINTR);
    if (waited < 0) {
        return std::unexpected(CannotRun(args[0], errno));
    }

    if (WIFEXITED(status)) {
        result.exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        result.exit_code = 128 + WTERMSIG(status);
    } else {
        result.exit_code = 1;
    }
    return result;
}

#endif

}  // namespace

std::expected<ProcessResult, Error> RunProcess(
    const std::vector<std::string>& args, const ProcessOptions& options) {
    if (args.empty()) {
        return std::unexpected(Error("cannot run an empty command"));
    }
    return Run(args, options);
}

}  // namespace craft::util
