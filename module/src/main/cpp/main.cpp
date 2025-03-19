#include <android/log.h>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <vector>

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>

#define LOG_TAG "IrisHelper"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static const char* IRIS_PATH = "/odm/bin/irisConfig";
static const int SLEEP_DURATION = 6;
static const int MAX_COUNTER = 8;

bool readJson(const std::string& file_path, rapidjson::Document& doc) {
    std::string read_path = file_path + "/iris_config.json";
    LOGI("Read Config: %s", read_path.c_str());

    FILE* fp = fopen(read_path.c_str(), "rb");
    if (!fp) {
        LOGE("Could not open %s", read_path.c_str());
        return false;
    }

    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    doc.ParseStream(is);
    fclose(fp);

    if (doc.HasParseError()) {
        LOGE("Error parsing JSON file %s", read_path.c_str());
        return false;
    }

    return true;
}

bool executeCmd(const std::string& cmd, std::string& result) {
    char buffer[128];
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        LOGE("popen failed for command: %s", cmd.c_str());
        return false;
    }

    result.clear();
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);

    // Remove trailing newline if present
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }

    return true;
}

void processPackage(const rapidjson::Document& document, const char* pkg_name) {
    if (!document.HasMember(pkg_name)) return;
    
    const auto& pkg_configs = document[pkg_name];
    if (!pkg_configs.IsArray()) {
        LOGE("Invalid config format for package %s", pkg_name);
        return;
    }

    for (rapidjson::SizeType i = 0; i < pkg_configs.Size(); ++i) {
        if (!pkg_configs[i].IsString()) continue;
        
        std::string cmd_str = IRIS_PATH;
        cmd_str += " ";
        cmd_str += pkg_configs[i].GetString();
        
        if (system(cmd_str.c_str()) != 0) {
            LOGE("Failed to execute command: %s", cmd_str.c_str());
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        LOGE("Usage: %s <config_path>", argv[0]);
        return EXIT_FAILURE;
    }

    rapidjson::Document document;
    if (!readJson(argv[1], document)) {
        return EXIT_FAILURE;
    }

    LOGI("Iris_helper server launching...");
    sleep(2);

    std::string current_app;
    std::string cmd_result;
    int counter = 0;

    const char* dump_cmd = "dumpsys activity a | grep topResumedActivity= | tail -n 1 | cut -d '/' -f1 | cut -d ' ' -f7";

    while (true) {
        if (!executeCmd(dump_cmd, cmd_result)) {
            sleep(1);
            continue;
        }

        if (cmd_result != current_app) {
            if (!document.HasMember(cmd_result.c_str())) {
                counter++;
                LOGI("Current app: %s.\nStanding by until %d times app-switch after.",
                     cmd_result.c_str(), MAX_COUNTER - counter);
                processPackage(document, "off");
            } else {
                LOGI("Current app: %s.\nStarting MEMC for %s...",
                     cmd_result.c_str(), cmd_result.c_str());
                processPackage(document, cmd_result.c_str());
                counter = 0;
            }
            current_app = cmd_result;
        } else {
            LOGI("Not app switch, sleeping...");
        }

        if (counter >= MAX_COUNTER) {
            processPackage(document, "off");
            LOGI("Long time no working, see you next time!");
            break;
        }
        sleep(SLEEP_DURATION);
    }

    return EXIT_SUCCESS;
}
