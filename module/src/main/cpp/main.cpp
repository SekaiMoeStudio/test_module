#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <fstream>
#include <vector>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

using namespace std;

static const string IRIS_PATH = "/odm/bin/irisConfig";
static const int SLEEP_DURATION = 6;
static const int MAX_COUNTER = 8;

rapidjson::Document readJson(const string& file_path) {
    string read_path = file_path + "/iris_config.json";
    cout << ">>> Read Config: " << read_path << endl;

    ifstream ifs(read_path);
    if (!ifs.is_open()) {
        cerr << ">>> Could not open " << read_path << endl;
        exit(EXIT_FAILURE);
    }
    rapidjson::IStreamWrapper isw(ifs);
    rapidjson::Document doc;
    doc.ParseStream(isw);
    ifs.close();

    if (doc.HasParseError()) {
        cerr << ">>> Error parsing JSON file " << read_path << endl;
        exit(EXIT_FAILURE);
    }

    return doc;
}

int executeCmd(const string& cmd, string& result) {
    char buffer[128];
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        perror("popen failed");
        return -1;
    }

    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);

    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }

    return 0;
}

void processPackage(rapidjson::Document& document, const string& pkg_name) {
    if (!document.HasMember(pkg_name.c_str())) return;
    const auto& pkg_configs = document[pkg_name.c_str()];
    for (rapidjson::SizeType i = 0; i < pkg_configs.Size(); ++i) {
        string pkg_config = pkg_configs[i].GetString();
        string cmd_str = IRIS_PATH + " " + pkg_config;
        system(cmd_str.c_str());
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <config_path>" << endl;
        return EXIT_FAILURE;
    }

    rapidjson::Document document = readJson(argv[1]);
    cout << ">>> Iris_helper server launching..." << endl;
    sleep(2);

    string current_app;
    string cmd_result;
    int counter = 0;

    while (true) {
        cmd_result.clear();
        if (executeCmd("dumpsys activity a | grep topResumedActivity= | tail -n 1 | cut -d '/' -f1 | cut -d ' ' -f7", cmd_result) != 0) {
            continue;
        }

        if (cmd_result != current_app) {
            if (!document.HasMember(cmd_result.c_str())) {
                counter++;
                cout << "\n>>> Current app: " << cmd_result << ".\n>> Standing by until " << MAX_COUNTER - counter << " times app-switch after." << endl;
                processPackage(document, "off");
            } else {
                cout << "\n>>> Current app: " << cmd_result << ".\n>>> Starting MEMC for " << cmd_result << "..." << endl;
                processPackage(document, cmd_result);
                counter = 0;
            }
            current_app = cmd_result;
        } else {
            cout << ">> Not app switch, sleeping..." << endl;
        }

        if (counter >= MAX_COUNTER) {
            processPackage(document, "off");
            cout << ">> Long time no working, see you next time!" << endl;
            break;
        }
        sleep(SLEEP_DURATION);
    }

    return 0;
}
