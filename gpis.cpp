#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <filesystem>
#include <functional>
#include <thread>
#include <chrono>
#include <random>

using namespace std;
/**
 * List of GIT command used throughout.
 */
enum GitCommandHead
{
    GET_UNSTAGED_FILES,
    GET_STAGED_FILES,
    GET_UNTRACKED_FILES,
    CREATE_BRANCH_AND_CHECKOUT,
    BRANCH_CHECKOUT,
    COMMIT_WITH_MESSAGE,
    STAGE_FILES,
    PUSH_NEW_BRANCH,
    PUSH,
    PULL,
    CURRENT_BRANCH,
    CHECK_REMOTE_BRANCH
};

/**
 * GIT changes type in git status
 */
enum GitChangeType
{
    ALL,
    UN_STAGED,
    STAGED,
    UN_TRACKED
};

const string directoryPath = "./";
const string all_character_string = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

// GIT commands map
static unordered_map<GitCommandHead, string> git_command = {
    {GET_UNSTAGED_FILES, "git diff --name-only"},
    {GET_STAGED_FILES, "git diff --name-only --staged"},
    {GET_UNTRACKED_FILES, "git ls-files --others"},
    {CREATE_BRANCH_AND_CHECKOUT, "git checkout -b %s"},
    {BRANCH_CHECKOUT, "git checkout %s"},
    {COMMIT_WITH_MESSAGE, "git commit -m \"%s\""},
    {STAGE_FILES, "git add %s"},
    {PUSH_NEW_BRANCH, "git push -u origin %s"},
    {PUSH, "git push"},
    {PULL, "git pull"},
    {CURRENT_BRANCH, "git branch --show-current"},
    {CHECK_REMOTE_BRANCH, "git ls-remote --heads origin %s"}};

static unordered_map<string, string> error_messages = {
    {"INSUFFICIENT_PARAMETER", "Insufficient parameters provided"},
    {"INVALID_PARAMETER", "Invalid parameters provided"}};

/**
 * Class to perform various adhoc actions
 */
class Action
{
public:
    // Executes terminal command and return output string
    static std::string exec(string cmd, bool isReversed = false)
    {
        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
        if (!pipe)
        {
            throw std::runtime_error("popen() failed!");
        }
        while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr)
        {
            result += buffer.data();
        }
        if (pclose(pipe.get()) != 0 && !isReversed)
        {
            cout << result << endl;
            exit(401);
        }
        else if (isReversed)
        {
            cout << result << endl;
            exit(404);
        }
        return result;
    }

    // Update subStr in the string
    static string updateString(const string &inputStr, const string &subStr)
    {
        char buffer[inputStr.size() + subStr.size()];

        snprintf(buffer, sizeof(buffer), inputStr.c_str(), subStr.c_str());
        return buffer;
    }

    // Split the string at every delimeter
    static std::vector<std::string> getSplitString(const std::string &s, const std::string &delimiter)
    {
        std::vector<std::string> tokens;
        size_t start = 0, end, delimiter_length = delimiter.length();

        while ((end = s.find(delimiter, start)) != std::string::npos)
        {
            tokens.push_back(s.substr(start, end - start));
            start = end + delimiter_length;
        }

        // Add the last token
        tokens.push_back(s.substr(start));

        return tokens;
    }

    static int getHcf(int value1, int value2)
    {
        if (value1 > value2)
        {
            value1 = value1 + value2;
            value2 = value1 - value2;
            value1 = value1 - value2;
        }
        if (value1 == 0)
            return value2;
        return Action::getHcf(value2 % value1, value1);
    }

    // Merges two array and returns unique values
    template <typename Data>
    static vector<Data> mergeArray(const vector<Data> &firstArray, const vector<Data> &secondArray)
    {
        vector<Data> finalArray;
        // Copy non-empty elements from array firstArray
        for (const auto &str : firstArray)
        {
            if (!str.empty())
            {
                finalArray.push_back(str);
            }
        }

        // Copy non-empty elements from array secondArray
        for (const auto &str : secondArray)
        {
            if (!str.empty() && find(finalArray.begin(), finalArray.end(), str) == finalArray.end())
            {
                finalArray.push_back(str);
            }
        }

        return finalArray;
    }

    // Returns unique items from first parameter which is not present in second parameter
    template <typename Data>
    static vector<Data> getUniueString(const vector<Data> &combinedArray, const vector<Data> &unwantedArray)
    {
        vector<Data> finalArray;

        // Copy unique items from combinedArray to final array if they are not present in unwantedArray
        for (const auto &str : combinedArray)
        {
            if (!str.empty() && find(unwantedArray.begin(), unwantedArray.end(), str) == unwantedArray.end())
            {
                finalArray.push_back(str);
            }
        }

        return finalArray;
    }

    static string getRandomString(size_t length = 5)
    {
        const string characters = all_character_string;
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dis(0, characters.size() - 1);

        string randomString;
        for (size_t count = 0; count < length; count++)
        {
            randomString += characters[dis(gen)];
        }
        return randomString;
    }

    static bool checkForGitFiles(string path) {
        string gitFolderSeparator = "/.git",
            gitFileSeparator = "/.git/";
        bool isFilePresent = path.find(gitFileSeparator, 0) != std::string::npos,
            isFolderPresent = path.find(gitFolderSeparator) == (path.length() - gitFolderSeparator.length());
        return isFilePresent || isFolderPresent;
    }
};

class FileWatch
{
private:
    unordered_map<string, filesystem::file_time_type> existingFileMap;
    string _folderPath;
    int _timeDelayInMS;
    function<void(vector<string> path)> _action;
    bool pathExists(const string &key)
    {
        return existingFileMap.find(key) != existingFileMap.end();
    }
    void loadFileMapToModifiedDate(string path)
    {
        for (const auto &file : filesystem::recursive_directory_iterator(path))
        {
            if(Action::checkForGitFiles(file.path())) {
                continue;
            }
            existingFileMap[file.path()] = filesystem::last_write_time(file);
        }
    }

public:
    FileWatch(string path, int timeDelayInMS, function<void(vector<string>)> action) : _action{action}, _folderPath{path}, _timeDelayInMS{timeDelayInMS}
    {
        loadFileMapToModifiedDate(_folderPath);
    }

    void check()
    {
        vector<string> filePathList;
        for (auto filePointer = existingFileMap.begin(); filePointer != existingFileMap.end();)
        {
            if (!filesystem::exists(filePointer->first))
            {
                filePathList.push_back(filePointer->first);
                existingFileMap.erase(filePointer);
            }
            else
            {
                filePointer++;
            }
        }
        for (const auto &file : filesystem::recursive_directory_iterator(_folderPath))
        {
            if(Action::checkForGitFiles(file.path())) {
                continue;
            }
            filesystem::file_time_type lastUpdateTime = filesystem::last_write_time(file);
            if (!pathExists(file.path()))
            {
                filePathList.push_back(file.path());
                existingFileMap[file.path()] = lastUpdateTime;
            }
            else if (lastUpdateTime != existingFileMap[file.path()])
            {
                filePathList.push_back(file.path());
                existingFileMap[file.path()] = lastUpdateTime;
            }
        }
        if (filePathList.size() > 0)
        {
            _action(filePathList);
        }
    }
};

class GitHandler
{
public:
    vector<string> getFileWithChanges(GitChangeType type = GitChangeType::ALL)
    {
        std::string cmdResult;
        vector<string> files = {}, tempFiles;
        if (type == ALL || type == UN_STAGED)
        {
            cmdResult = Action::exec(git_command[GitCommandHead::GET_UNSTAGED_FILES]).c_str();
            files = Action::mergeArray(files, Action::getSplitString(cmdResult, "\n"));
        }
        if (type == ALL || type == STAGED)
        {
            cmdResult = Action::exec(git_command[GitCommandHead::GET_STAGED_FILES]).c_str();
            files = Action::mergeArray(files, Action::getSplitString(cmdResult, "\n"));
        }
        if (type == GitChangeType::ALL || type == GitChangeType::UN_TRACKED)
        {

            cmdResult = Action::exec(git_command[GitCommandHead::GET_UNTRACKED_FILES]).c_str();
            files = Action::mergeArray(files, Action::getSplitString(cmdResult, "\n"));
        }
        return files;
    }

    void switchToNewBranch(string branchName)
    {
        Action::exec(Action::updateString(git_command[GitCommandHead::CREATE_BRANCH_AND_CHECKOUT], branchName));
        Action::exec(Action::updateString(git_command[GitCommandHead::PUSH_NEW_BRANCH], branchName));
    }

    void checkout(string branchName)
    {
        Action::exec(Action::updateString(git_command[GitCommandHead::BRANCH_CHECKOUT], branchName));
    }

    void commit(string msg)
    {
        Action::exec(Action::updateString(git_command[GitCommandHead::COMMIT_WITH_MESSAGE], msg));
    }

    void push()
    {
        Action::exec(git_command[GitCommandHead::PUSH]);
    }

    void pull()
    {
        Action::exec(git_command[GitCommandHead::PULL]);
    }

    string getCurrentBranch()
    {
        auto cmdResult = Action::exec(git_command[CURRENT_BRANCH]);
        return Action::getSplitString(cmdResult, "\n")[0];
    }

    bool checkIfBranchExists(string branch)
    {
        Action::exec(Action::updateString(git_command[CHECK_REMOTE_BRANCH], branch));
        return false;
    }

    void stage(vector<string> files)
    {
        stringstream fileList;
        if (files.size() > 0)
        {
            for (auto &file : files)
            {
                fileList << " " << file;
            }
            Action::exec(Action::updateString(git_command[GitCommandHead::STAGE_FILES], fileList.str()));
        }
    }
};

class Session
{
private:
    vector<string> excludedFiles;
    string previousBranch;
    int commitCount = 0;
    FileWatch watcher{directoryPath, 500, [this](vector<string> paths)
                      {
                          this->saveDetected(paths);
                      }};
    string sessionBranchPrefix = "__gpis_";
    string commitPrefix = "Session changes count: ";
    int pullDelayInMS = 1000, fileCheckDelayInMS = 300;
    GitHandler git = GitHandler();

    void watch()
    {
        int hcf = Action::getHcf(pullDelayInMS, fileCheckDelayInMS);
        int countPull = 0, countFileWatch = 0, maxCountPull = pullDelayInMS / hcf, maxCountFileWatch = fileCheckDelayInMS / hcf;

        while (true)
        {
            if (countPull == 0)
            {
                git.pull();
            }
            if (countFileWatch == 0)
            {
                watcher.check();
            }
            countPull = (countPull + 1) % maxCountPull;
            countFileWatch = (countFileWatch + 1) % maxCountFileWatch;
            this_thread::sleep_for(chrono::milliseconds(hcf));
        }
    }

public:
    Session()
    {
        // cout << "Session initialized" << endl;
        setIgnoredFiles();
    }

    void setIgnoredFiles()
    {
        excludedFiles = git.getFileWithChanges();
    }

    void init()
    {
        string newBranch = "";
        previousBranch = git.getCurrentBranch();
        newBranch = (sessionBranchPrefix + Action::getRandomString());
        if (newBranch.size() == 0 || !git.checkIfBranchExists(newBranch))
        {
            cout << (newBranch.size()) << endl;
            newBranch = (sessionBranchPrefix + Action::getRandomString());
        }
        git.switchToNewBranch(newBranch);
        join(newBranch);
    }

    void start()
    {
        this->watch();
    }

    bool isBranchNameValid(string branchName)
    {
        return branchName.size() > sessionBranchPrefix.size() && branchName.substr(0, sessionBranchPrefix.size()) == sessionBranchPrefix;
    }

    void saveDetected(std::vector<std::string> paths)
    {
        auto gitChangedFiles = Action::getUniueString(git.getFileWithChanges(), excludedFiles);
        if (gitChangedFiles.size() == 0)
            return;
        stringstream commitMessage;
        git.stage(gitChangedFiles);
        commitMessage << commitPrefix << ++commitCount;
        git.commit(commitMessage.str());
        git.push();
    }

    void join(string branch)
    {
        if (!isBranchNameValid(branch))
        {
            return;
        }
        if (branch.find(sessionBranchPrefix, 0) != 0)
        {
            branch = sessionBranchPrefix + branch;
        }
        git.checkout(branch);
        this->watch();
        // thread thread2([this](){ watcher.start(); });
        // git.startSync();
        // watcher.check();
    }

    void checkForChanges()
    {
        string result = Action::exec("git status");
    }
};

int main(int argc, char **argv)
{
    Session session = Session();
    if (argc < 2)
    {
        cout << error_messages["INSUFFICIENT_PARAMETER"] << endl;
        return 0;
    }
    try
    {
        string cmd = argv[1];
        if (cmd == "start")
        {
            session.start();
        }
        else if (cmd == "init")
        {
            session.init();
        }
        else if (cmd == "join" && argc == 3 && session.isBranchNameValid(argv[2]))
        {
            session.join(argv[2]);
        }
        else
        {
            cout << error_messages["INVALID_PARAMETER"] << endl;
        }
    }
    catch (string err)
    {
        cout << err << endl;
    }
}