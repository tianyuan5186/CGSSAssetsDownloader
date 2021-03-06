
#include <fstream>
#include <string>
#include <Windows.h>
#include "./download.h"
#include "./lz4.h"
#include "./clHCA.h"
#include "./CGSSAssetsDownloader.h"
#include "./ACBExtractor/include/ACBExtractor.h"

using namespace std;

Downloader::Downloader(string v, string t) {
  res_ver = v;
  type = t;
}

void Downloader::set_type(string t) {
  type = t;
}

void Downloader::check_manifest() {
  fstream _file;
  string fileName = "data\\manifest_" + res_ver + ".db";
  _file.open(fileName.c_str(), ios::in);

  if (!_file) {
    printf("Start download manifest.\n");
    download_manifest();
  }
  else {
    printf("Manifest file exists.\n");
  }
}

void Downloader::download_manifest() {
  exec_sync("if not exist \"data\" md data");
  //exec_sync("tool\\wget\\wget -c -O data\\manifest_" + res_ver + " http://storage.game.starlight-stage.jp/dl/" + res_ver + "/manifests/Android_AHigh_SHigh");
  download("http://storage.game.starlight-stage.jp/dl/" + res_ver + "/manifests/Android_AHigh_SHigh", "./data/manifest_" + res_ver + ".");

  string lz4file = "data\\manifest_" + res_ver;
  long size = get_file_size(lz4file.c_str());
  if (size < 1) {
    printf("Failed.\n");
    exec_sync("del data\\manifest_" + res_ver + " /f /s /q");
    exit(0);
  }
  lz4dec(lz4file, "db");
  exec_sync("del data\\manifest_" + res_ver);
  printf("Successfully download manifest.\n");
}

void Downloader::download_asset() {
  sqlite3 *db;
  char *zErrMsg = 0;
  int rc;
  const char* sql = "";
  const char* data = type.c_str();

  string sqlfile = "data\\manifest_" + res_ver + ".db";
  rc = sqlite3_open(sqlfile.c_str(), &db);
  if (rc) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    exit(0);
  }
  else {
    fprintf(stderr, "Successfully open database.\n");
  }

  exec_sync("if not exist \"" + type + "\" md " + type);

  if (type == "bgm") {
    sql = "SELECT 'http://storage.game.starlight-stage.jp/dl/resources/High/Sound/Common/b/'||hash AS url, REPLACE(REPLACE(name,'b/',''),'.acb','') AS filename FROM manifests WHERE name LIKE 'b/%acb'";
    if (Downloader::mp3 != 0) {
      exec_sync("if not exist bgm\\mp3 md bgm\\mp3");
    }
    else {
      exec_sync("if not exist bgm\\wav md bgm\\wav");
    }
  }
  else if (type == "live") {
    sql = "SELECT 'http://storage.game.starlight-stage.jp/dl/resources/High/Sound/Common/l/'||hash AS url, REPLACE(REPLACE(name,'l/',''),'.acb','') AS filename FROM manifests WHERE name LIKE 'l/%acb'";
    if (Downloader::mp3 != 0) {
      exec_sync("if not exist live\\mp3 md live\\mp3");
    }
    else {
      exec_sync("if not exist live\\wav md live\\wav");
    }
  }
  else if (type == "card") {
    sql = "SELECT 'http://storage.game.starlight-stage.jp/dl/resources/High/AssetBundles/Android/'||hash AS url, REPLACE(name,'.unity3d','') AS filename FROM manifests WHERE name LIKE 'card_bg_______.unity3d'";
  }
  else if (type == "icon") {
    sql = "SELECT 'http://storage.game.starlight-stage.jp/dl/resources/High/AssetBundles/Android/'||hash AS url, REPLACE(name,'.unity3d','') AS filename FROM manifests WHERE name LIKE 'card________m.unity3d'";
  }
  else if (type == "score") {
    sql = "SELECT 'http://storage.game.starlight-stage.jp/dl/resources/Generic/'||hash AS url, REPLACE(name,'.bdb','') AS filename FROM manifests WHERE name LIKE 'musicscores%bdb'";
  }
  read_database(db, sql, data, zErrMsg, rc, type);

  sqlite3_close(db);
}

void Downloader::download_single(string file) {
  sqlite3 *db;
  string sql;
  char *zErrMsg = 0;
  int rc;
  const char* data = file.c_str();

  string sqlfile = "data\\manifest_" + res_ver + ".db";
  rc = sqlite3_open(sqlfile.c_str(), &db);
  if (rc) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    exit(0);
  }
  else {
    fprintf(stderr, "Successfully open database.\n");
  }

  string suffixStr = file.substr(file.find_last_of(".") + 1);
  exec_sync("if not exist dl md dl");
  if (suffixStr == "acb") {
    string acb_type = file.substr(0,1);
    sql = "SELECT 'http://storage.game.starlight-stage.jp/dl/resources/High/Sound/Common/" + acb_type + "/'||hash AS url, REPLACE(REPLACE(name,'" + acb_type + "/',''),'.acb','') AS filename FROM manifests WHERE name='" + file + "'";
  }
  else if (suffixStr == "unity3d") {
    sql = "SELECT 'http://storage.game.starlight-stage.jp/dl/resources/High/AssetBundles/Android/'||hash AS url, REPLACE(name,'.unity3d','') AS filename FROM manifests WHERE name='" + file + "'";
  }
  else if (suffixStr == "bdb") {
    sql = "SELECT 'http://storage.game.starlight-stage.jp/dl/resources/Generic/'||hash AS url, REPLACE(name,'.bdb','') AS filename FROM manifests WHERE name='" + file + "'";
  }
  else if (suffixStr == "mdb") {
    sql = "SELECT 'http://storage.game.starlight-stage.jp/dl/resources/Generic/'||hash AS url, REPLACE(name,'.mdb','') AS filename FROM manifests WHERE name='" + file + "'";
  }
  else {
    exec_sync("cls");
    printf("File name error.\n\n");
    return;
  }

  rc = sqlite3_exec(db, sql.c_str(), exist, (void*)data, &zErrMsg);
  if (Downloader::exists != 0) {
    rc = sqlite3_exec(db, sql.c_str(), get_single, (void*)suffixStr.c_str(), &zErrMsg);
  }
  else {
    exec_sync("cls");
    printf("%s not found.\n\n", file.c_str());
  }
  is_db_ok(rc, zErrMsg);
}

void show_introduction() {
  printf("CGSSAssetsDownloader VERSION 1.8\n\n");

  printf("Usage: \n");
  printf("CGSSAssetsDownloader <-v resource_version> [-a] [-u] [-mp3]\n");
  printf("CGSSAssetsDownloader <-v resource_version> [-o option or filename] [-u] [-mp3]\n");
  printf("CGSSAssetsDownloader file1 file2 file3 ...\n\n");

  printf("Example: \n");
  printf("CGSSAssetsDownloader -v 10027700 -o -bgm -u\n");
  printf("CGSSAssetsDownloader -v 10028005 -o gachaselect_30145.unity3d\n");
  printf("CGSSAssetsDownloader -v 10031250 -a -u -mp3\n");
  printf("CGSSAssetsDownloader path\\to\\NoSuffixFile path\\to\\ACBFile.acb path\\to\\HCAFile.hca ...\n\n");

  printf("Arguments: \n");
  printf("<-v resource_version> [NECESSARY] Set the resource version of game and download database.\n");
  printf("[-a] [OPTIONAL] Auto update bgm, live, card, icon, score assets.\n");
  printf("[-o bgm|live|card|icon|score|(filename)] [OPTIONAL] Read the detail below.\n");
  printf("[-u] [OPTIONAL] Copy files to \"dl\\\" folder.\n");
  printf("[-mp3] [OPTIONAL] WAV to MP3.Default: WAV.\n\n");

  printf("If you don't know the <resource_version>, try to visit\nhttps://starlight.kirara.ca/api/v1/info\n\n");

  printf("-o detail: \n");
  printf("  bgm \t all background music will be downloaded.\n");
  printf("  live \t all live music will be downloaded.\n");
  printf("  card \t all unity3d files that contain card background will be downloaded.\n");
  printf("  icon \t all unity3d files that contain 124x124 card icon will be downloaded.\n");
  printf("  score \t all bdb files that contain music score will be downloaded.\n\n");

  printf("You can use \"DB Browser for SQLite\" open the manifest database file in data\\ to browse file names\n\n");

  printf("You can also drag no suffix file (to .unity3d), .acb file or .hca file (to .wav, or to .mp3 if you use -mp3 argument with command line) into the exe\n\n");

  printf("Developed by github@toyobayashi\n\n");

  printf("Powered by:\n");
  printf("HCADecoder\n");
  printf("ffmpeg\n");
  printf("SQLite\n");
  printf("UnityLz4\n\n");

  printf("The copyright of CGSS and its related content is held by BANDAI NAMCO Entertainment Inc.\n\n");

  system("pause>nul");
}

bool extract_acb (string acbFile) {
  try {
    ACBExtractor extractor(acbFile);
    bool result = extractor.extract(nullptr);
    return result;
  } catch (const char* err) {
    printf("ACBExtractor Error: %s\n", err);
    return false;
  }
}

int string_index_of (char* arr[], const char* str, int length) {
  for (int i = 0; i < length; i++) {
    if (strcmp(arr[i], str) == 0) {
      return i;
    }
  }
  return -1;
}

void exec_sync (string cmd) {
  system(cmd.c_str());
}

void is_db_ok (int &r, char *&errmsg) {
  if (r != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", errmsg);
    sqlite3_free(errmsg);
    exit(0);
  }
}

string dir_name () {
  char exeFullPath[MAX_PATH];
  string strPath = "";

  GetModuleFileNameA(NULL, exeFullPath, MAX_PATH);
  strPath = (string)exeFullPath;
  size_t pos = strPath.find_last_of('\\', strPath.length());
  return strPath.substr(0, pos);
}

void hcadec (string hcafile) {
  unsigned int count = 0;
  char *filenameOut = NULL;
  float volume = 1;
  unsigned int ciphKey1 = 0xF27E3B22;
  unsigned int ciphKey2 = 0x00003657;
  int mode = 16;
  int loop = 0;

  char path[MAX_PATH];
  if (!(filenameOut&&filenameOut[0])) {
    strcpy_s(path, sizeof(path), hcafile.c_str());
    char *d1 = strrchr(path, '\\');
    char *d2 = strrchr(path, '/');
    char *e = strrchr(path, '.');
    if (e&&d1<e&&d2<e)*e = '\0';
    strcat_s(path, sizeof(path), ".wav");
    filenameOut = path;
  }

  printf("Decoding %s...\n", hcafile.c_str());
  clHCA hca(ciphKey1, ciphKey2);
  if (!hca.DecodeToWavefile(hcafile.c_str(), filenameOut, volume, mode, loop)) {
    printf("Error: Decoding failed.\n");
  }
}

int exist(void *data, int argc, char **argv, char **azColName) {
  Downloader::exists++;
  return 0;
}

int sum_number(void *data, int argc, char **argv, char **azColName) {
  string name(argv[1]);
  string type = (char*)data;

  fstream _file;
  string fileName;
  if (type == "bgm" || type == "live") {
    if (Downloader::mp3 != 0) {
      fileName = type + "\\mp3\\" + name + ".mp3";
    }
    else {
      fileName = type + "\\wav\\" + name + ".wav";
    }
  }
  else if (type == "card" || type == "icon") {
    fileName = type + "\\" + name + ".unity3d";
  }
  else if (type == "score") {
    fileName = type + "\\" + name + ".bdb";
  }

  _file.open(fileName.c_str(), ios::in);

  if (!_file) {
    Downloader::max++;
  }
  _file.close();
  return 0;
}

int get_asset(void *data, int argc, char **argv, char **azColName) {
  string url(argv[0]);
  string name(argv[1]);

  fstream _file;
  string fileName;

  if (strcmp((char*)data, "bgm") == 0) {
    if (Downloader::mp3 != 0) {
      fileName = "bgm\\mp3\\" + name + ".mp3";
    }
    else {
      fileName = "bgm\\wav\\" + name + ".wav";
    }
  }
  else if (strcmp((char*)data, "live") == 0) {
    if (Downloader::mp3 != 0) {
      fileName = "live\\mp3\\" + name + ".mp3";
    }
    else {
      fileName = "live\\wav\\" + name + ".wav";
    }
  }
  else if (strcmp((char*)data, "card") == 0) {
    fileName = "card\\" + name + ".unity3d";
  }
  else if (strcmp((char*)data, "icon") == 0) {
    fileName = "icon\\" + name + ".unity3d";
  }
  else if (strcmp((char*)data, "score") == 0) {
    fileName = "score\\" + name + ".bdb";
  }
  else {
    fileName = "";
  }

  _file.open(fileName.c_str(), ios::in);

  if (!_file) {
    Downloader::update_list += name;
    Downloader::update_list += "\n";
    exec_sync("cls");
    printf("Downloading: %s\n\n", name.c_str());
    progress(Downloader::current, Downloader::max);
    printf("\n\n");

    if (strcmp((char*)data, "bgm") == 0) {

      download(url, "./bgm/" + name + ".acb");

      extract_acb("bgm\\" + name + ".acb");
      hcadec("bgm\\_acb_" + name + ".acb\\" + name + ".hca");
      exec_sync("move bgm\\_acb_" + name + ".acb\\" + name + ".wav bgm\\");
      exec_sync("rd bgm\\_acb_" + name + ".acb /s /q");
      exec_sync("del bgm\\" + name + ".acb");
      if (Downloader::mp3 != 0) {
        exec_sync("tool\\ffmpeg\\ffmpeg.exe -i bgm\\" + name + ".wav bgm\\mp3\\" + name + ".mp3 -v quiet");
        exec_sync("del bgm\\" + name + ".wav");
      }
      else {
        exec_sync("move bgm\\" + name + ".wav bgm\\wav\\");
      }
      if (Downloader::copy != 0) {
        if (Downloader::mp3 != 0) {
          exec_sync("copy bgm\\mp3\\" + name + ".mp3 dl\\");
        }
        else {
          exec_sync("copy bgm\\wav\\" + name + ".wav dl\\");
        }
      }
    }
    else if (strcmp((char*)data, "live") == 0) {

      download(url, "./live/" + name + ".acb");

      extract_acb("live\\" + name + ".acb");
      hcadec("live\\_acb_" + name + ".acb\\" + name + ".hca");
      exec_sync("move live\\_acb_" + name + ".acb\\" + name + ".wav live\\");
      exec_sync("rd live\\_acb_" + name + ".acb /s /q");
      exec_sync("del live\\" + name + ".acb");
      if (Downloader::mp3 != 0) {
        exec_sync("tool\\ffmpeg\\ffmpeg.exe -i live\\" + name + ".wav live\\mp3\\" + name + ".mp3 -v quiet");
        exec_sync("del live\\" + name + ".wav");
      }
      else {
        exec_sync("move live\\" + name + ".wav live\\wav\\");
      }
      if (Downloader::copy != 0) {
        if (Downloader::mp3 != 0) {
          exec_sync("copy live\\mp3\\" + name + ".mp3 dl\\");
        }
        else {
          exec_sync("copy live\\wav\\" + name + ".wav dl\\");
        }
      }
    }
    else if (strcmp((char*)data, "card") == 0) {

      download(url, "./card/" + name + ".");
      lz4dec("card\\" + name, "unity3d");
      exec_sync("del card\\" + name);
      if (Downloader::copy != 0) {
        exec_sync("copy card\\" + name + ".unity3d dl\\");
      }
    }
    else if (strcmp((char*)data, "icon") == 0) {

      download(url, "./icon/" + name + ".");
      lz4dec("icon\\" + name, "unity3d");
      exec_sync("del icon\\" + name);
      if (Downloader::copy != 0) {
        exec_sync("copy icon\\" + name + ".unity3d dl\\");
      }
    }
    else if (strcmp((char*)data, "score") == 0) {

      download(url, "./score/" + name + ".");
      lz4dec("score\\" + name, "bdb");
      exec_sync("del score\\" + name);
      if (Downloader::copy != 0) {
        exec_sync("copy score\\" + name + ".bdb dl\\");
      }

    }
    Downloader::current++;
  }
  _file.close();
  return 0;
}

int get_single(void *data, int argc, char **argv, char **azColName) {
  string url(argv[0]);
  string name(argv[1]);

  fstream _file;
  string fileName;

  if (strcmp((char*)data, "acb") == 0) {

    if (Downloader::mp3 != 0) {
      fileName = "dl\\" + name + ".mp3";
    }
    else {
      fileName = "dl\\" + name + ".wav";
    }
  }
  else if (strcmp((char*)data, "unity3d") == 0) {
    fileName = "dl\\" + name + ".unity3d";
  }
  else if (strcmp((char*)data, "bdb") == 0) {
    fileName = "dl\\" + name + ".bdb";
  }
  else if (strcmp((char*)data, "mdb") == 0) {
    fileName = "dl\\" + name + ".mdb";
  }
  else {
    fileName = "";
  }

  _file.open(fileName.c_str(), ios::in);

  if (!_file) {
    printf(name.c_str());
    printf("\n\n");

    download(url, "./dl/" + name + ".");

    if (strcmp((char*)data, "acb") == 0) {
      exec_sync("ren dl\\" + name + ". " + name + ".acb");

      extract_acb("dl\\" + name + ".acb");
      exec_sync("dir /a-d /b dl\\_acb_" + name + ".acb\\*.hca>dl\\_acb_" + name + ".acb\\hcafiles.txt");

      string hcaFile[300];
      int i = 0;
      ifstream infile;
      infile.open("dl\\_acb_" + name + ".acb\\hcafiles.txt", ios::in);
      while (!infile.eof()) {
        getline(infile, hcaFile[i], '\n');
        i++;
      }
      infile.close();

      for (int x = 0; x < i - 1; x++) {
        hcadec("dl\\_acb_" + name + ".acb\\" + hcaFile[x]);
        string fn = hcaFile[x].substr(0, hcaFile[x].find_last_of("."));
        if (Downloader::mp3 != 0) {
          exec_sync("tool\\ffmpeg\\ffmpeg.exe -i dl\\_acb_" + name + ".acb\\" + fn + ".wav dl\\" + fn + ".mp3 -v quiet");
        }
        else {
          exec_sync("move dl\\_acb_" + name + ".acb\\" + fn + ".wav dl\\");
        }
      }
      exec_sync("rd dl\\_acb_" + name + ".acb /s /q");
      exec_sync("del dl\\" + name + ".acb");
      /* hcadec("dl\\_acb_" + name + ".acb\\" + name + ".hca");
      exec_sync("move dl\\_acb_" + name + ".acb\\" + name + ".wav dl\\");
      exec_sync("rd dl\\_acb_" + name + ".acb /s /q"); 
      exec_sync("del dl\\" + name + ".acb");
      if (Downloader::mp3 != 0) {
        exec_sync("tool\\ffmpeg\\ffmpeg.exe -i dl\\" + name + ".wav dl\\" + name + ".mp3 -v quiet");
        exec_sync("del dl\\" + name + ".wav");
      }*/
    }
    else if (strcmp((char*)data, "unity3d") == 0) {
      lz4dec("dl\\" + name, "unity3d");
      exec_sync("del dl\\" + name);
    }
    else if (strcmp((char*)data, "bdb") == 0) {
      lz4dec("dl\\" + name, "bdb");
      exec_sync("del dl\\" + name);
    }
    else if (strcmp((char*)data, "mdb") == 0) {
      lz4dec("dl\\" + name, "mdb");
      exec_sync("del dl\\" + name);
    }
    exec_sync("cls");
    printf("%s Completed.\n\n", name.c_str());
  }
  else {
    exec_sync("cls");
    printf("File exists.\n\n");
  }
  _file.close();
  return 0;
}

void read_database(sqlite3 *db, const char *sql, const char* data, char *zErrMsg, int rc, string type) {
  printf("Reading database...\n");
  // Downloader::update_list = "";
  rc = sqlite3_exec(db, sql, sum_number, (void*)data, &zErrMsg);
  rc = sqlite3_exec(db, sql, get_asset, (void*)data, &zErrMsg);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
    exit(0);
  }
  else {
    ofstream log_txt;
    exec_sync("if not exist \"log\" md log");
    exec_sync("cls");
    printf("%d/%d Completed.\n\n", Downloader::current, Downloader::max);
    string head = "";
    if (Downloader::auto_update == 0) {
      log_txt.open("log\\" + type + ".txt");
      log_txt << Downloader::update_list;
      log_txt.close();
      head = "===========  Update " + type + "  ===========\n\n";
    }
    else {
      log_txt.open("log\\log.txt");
      log_txt << Downloader::update_list;
      log_txt.close();
      head = "=============  Update  =============\n\n";
    }
    printf(head.c_str());
    printf(Downloader::update_list.c_str());
    printf("\n====================================\n");
  }
}

int main(int argc, char* argv[]) {
  exec_sync("echo off");
  exec_sync("cls");

  if (argc == 1) {
    show_introduction();
    return 0;
  }

  int v = string_index_of(argv, "-v", argc);
  int o = string_index_of(argv, "-o", argc);
  int u = string_index_of(argv, "-u", argc);
  int mp3 = string_index_of(argv, "-mp3", argc);
  int a = string_index_of(argv, "-a", argc);

  string version;
  string option;

  if (v != -1 && v + 1 < argc) {

    if (atoi(argv[v + 1]) > 10012760) {
      version = argv[v + 1];
    }
    else {
      printf("[ERROR] Please try resource version later than 10012760");
      system("pause>nul");
      return 0;
    }

    if (u != -1) {
      Downloader::copy = 1;
      exec_sync("if not exist dl md dl");
    }

    if (mp3 != -1) {
      Downloader::mp3 = 1;
    }

    if (a != -1) {
      Downloader::auto_update = 1;
      Downloader downloader(version, "");
      downloader.check_manifest();
      downloader.set_type("bgm");
      downloader.download_asset();
      downloader.set_type("live");
      downloader.download_asset();
      downloader.set_type("card");
      downloader.download_asset();
      downloader.set_type("icon");
      downloader.download_asset();
      downloader.set_type("score");
      downloader.download_asset();
      return 0;
    }

    if (o == -1) {
      Downloader downloader(version, "");
      downloader.check_manifest();
    }
    else if (o != -1 && o + 1 < argc) {
      option = argv[o + 1];
      if (option == "bgm" || option == "live" || option == "card" || option == "icon" || option == "score") {
        Downloader downloader(version, option);
        downloader.check_manifest();
        downloader.download_asset();
      }
      else {
        Downloader downloader(version, "");
        downloader.check_manifest();
        downloader.download_single(option);
      }
    }

  }
  else {
    if (mp3 != -1) {
      Downloader::mp3 = 1;
    }

    for (int i = 1; i < argc; i++) {
      string arg = argv[i];
      
      if (arg != "-mp3") {

        string root;
        string fileName;
        if (arg.find_last_of("\\") == string::npos) {
          root = "";
          fileName = arg;
        }
        else {
          root = arg.substr(0, arg.find_last_of("\\"));
          fileName = arg.substr(arg.find_last_of("\\") + 1);
        }
        
        if (fileName.find_last_of(".") == string::npos && get_file_size(fileName.c_str()) != 0) {
          lz4dec(arg, "unity3d");
        }
        else {
          if (fileName.substr(fileName.find_last_of(".") + 1) == "hca") {
            hcadec(arg);
          }
          else if (fileName.substr(fileName.find_last_of(".") + 1) == "acb") {
            extract_acb(arg);
            exec_sync("dir /a-d /b " + root + "\\_acb_" + fileName + "\\*.hca>" + root + "\\_acb_" + fileName + "\\hcafiles.txt");

            string hcaFile[300];
            int i = 0;
            ifstream infile;
            infile.open(root + "\\_acb_" + fileName + "\\hcafiles.txt", ios::in);
            while (!infile.eof()){
              getline(infile, hcaFile[i], '\n');
              i++;
            }
            infile.close();

            for (int x = 0; x < i - 1; x++) {
              hcadec(root + "\\_acb_" + fileName + "\\" + hcaFile[x]);
              string name = hcaFile[x].substr(0, hcaFile[x].find_last_of("."));
              if (Downloader::mp3 != 0) {
                exec_sync(dir_name() + "\\tool\\ffmpeg\\ffmpeg.exe -i " + root + "\\_acb_" + fileName + "\\" + name + ".wav " + root + "\\" + name + ".mp3 -v quiet");
              }
              else {
                exec_sync("move " + root + "\\_acb_" + fileName + "\\" + name + ".wav " + root + "\\");
              }
            }
            exec_sync("rd " + root + "\\_acb_" + fileName + " /s /q");
          }
        }
      }
    }
  }

  return 0;
}
