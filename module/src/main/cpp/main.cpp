#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <fstream>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

using namespace std;

static string iris_path = "/odm/bin/irisConfig";

rapidjson::Document readJson(string file_path){
    string read_path = file_path + "/iris_config.json";
    cout << ">>> Read Config: " << read_path << endl;
    ifstream ifs(read_path);
    if (!ifs.is_open()) {
        cout << ">>> Could not open " << read_path << endl;
        ifs.close();
        exit(-1);
    }

    rapidjson::IStreamWrapper isw(ifs);
    rapidjson::Document doc;
    doc.ParseStream(isw);
    ifs.close();

    if (doc.HasParseError()) {
        cerr << ">>> Error JSON file " << read_path << endl;
        exit(-1);
    }

    return doc;
}

int ExecuteCMD(const char *cmd, char *result)
{
    char outBuffer[100];
    FILE * pipeLine = popen(cmd,"r");
    if(!pipeLine){
        perror("Fail to popen\n");
        return -1;
    }
    while(fgets(outBuffer, 59, pipeLine) != NULL){
        strcpy(result, outBuffer);
    }
    pclose(pipeLine);
    return 0;
}

int ihelper_process(rapidjson::Document& document, const char* pkg_name){
    const rapidjson::Value& pkg_configs = document[pkg_name];
    for (rapidjson::SizeType i = 0; i < pkg_configs.Size(); i++){
        string pkg_config = pkg_configs[i].GetString(); // cout << pkg_configs[i].GetString() << endl;
        string cmd_str = iris_path + " " + pkg_config; //cout << cmd_str << endl;
        system(cmd_str.data());
    }
    return 0;
}


int main(int argc, char *argv[]){
    char name_app[50] = {0};
    char cur_app[50] = {0};
    char cmd_result[50] = {0};
    int counter = 0;

    rapidjson::Document document = readJson(argv[1]);
    cout << ">>> Iris_helper server launching..." << endl;
    sleep(2);

    for( ; ; ){
        ExecuteCMD("dumpsys activity a | grep topResumedActivity= | tail -n 1 | cut -d '/' -f1 | cut -d ' ' -f7", cmd_result);
        strcpy(name_app, cmd_result);
        name_app[strlen(name_app)-1]=0;

        if (strcmp(name_app, cur_app) != 0){
            if (!document.HasMember(name_app)){
                counter++;
                cout << "\n>>> Current app: " << name_app << ".\n>> Standing by until " << 8 - counter <<  " times app-switch after." << endl;
                ihelper_process(document, "off");
            }
            else{
                cout << "\n>>> Current app: " << name_app << ".\n>>> Starting MEMC for" << name_app << "..." << endl;
                ihelper_process(document, name_app);
    		    counter = 0;
            }
            strcpy(cur_app, name_app);
        }
        else{
            cout << ">> Not app switch, sleeping..." << endl;
        }
        if (counter >= 8){
            ihelper_process(document, "off");
            cout << ">> Long time no working, see you next time!" << endl;
            exit(0);
        }
        sleep(6);
    }
    return 0;
}
