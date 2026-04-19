#include "scanner.hpp"
#include "parser.hpp"
#include "codegen.hpp"
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {
const std::string RESET = "\033[0m";
const std::string GREEN = "\033[32m";
const std::string RED = "\033[31m";
const std::string YELLOW = "\033[33m";
const std::string BLUE = "\033[34m";
const std::string CYAN = "\033[36m";

void logInfo(const std::string& msg) {
  std::cout << BLUE << "[Syux] " << RESET << msg << "\n";
}

void logSuccess(const std::string& msg) {
  std::cout << GREEN << "[Syux] " << RESET << msg << "\n";
}

void logError(const std::string& msg) {
  std::cerr << RED << "[Syux Error] " << RESET << msg << "\n";
}

void printUsage() {
  std::cout << "Syux Compiler v0.5.0\n\n";
  std::cout << "Usage:\n";
  std::cout << "  syux <file.syux>        Run Syux file from anywhere\n";
  std::cout << "  syux run <file.syux>    Run Syux file\n";
  std::cout << "  syux build <file.syux>  Build (compile, don't run)\n";
  std::cout << "  syux transpile <file.syux>  Transpile to C++ only\n";
  std::cout << "\nTip: Run from any directory - Syux finds the file automatically!\n";
}

std::string getExtension(const std::string& path) {
  size_t pos = path.find_last_of('.');
  if (pos == std::string::npos) return "";
  return path.substr(pos + 1);
}

bool isSyuxFile(const std::string& path) {
  std::string ext = getExtension(path);
  return ext == "syux" || ext == "sx";
}

std::filesystem::path getExeDir() {
#ifdef _WIN32
  char path[MAX_PATH];
  GetModuleFileNameA(NULL, path, MAX_PATH);
  return std::filesystem::path(path).parent_path();
#else
  return std::filesystem::current_path();
#endif
}

std::filesystem::path findSyuxFile(const std::string& filename) {
  std::filesystem::path p(filename);
  
  if (p.is_absolute() && std::filesystem::exists(p)) {
    return p;
  }
  
  std::filesystem::path current = std::filesystem::current_path();
  std::filesystem::path test = current / filename;
  if (std::filesystem::exists(test)) {
    return test;
  }
  
  std::filesystem::path exeDir = getExeDir();
  test = exeDir / filename;
  if (std::filesystem::exists(test)) {
    return test;
  }
  
  for (const auto& entry : std::filesystem::recursive_directory_iterator(current)) {
    if (entry.is_regular_file() && entry.path().filename().string() == filename) {
      return entry.path();
    }
  }
  
  for (const auto& entry : std::filesystem::recursive_directory_iterator(exeDir)) {
    if (entry.is_regular_file() && entry.path().filename().string() == filename) {
      return entry.path();
    }
  }
  
  return p;
}

bool transpileToCpp(const std::filesystem::path& inputPath) {
  std::ifstream in(inputPath);
  if (!in) {
    logError("Could not open file: " + inputPath.string());
    return false;
  }

  std::string src((std::istreambuf_iterator<char>(in)), {});
  logInfo("Parsing...");
  Scanner sc(src);
  auto tokens = sc.scan();

  Parser p(tokens);
  auto ast = p.parse();

  logInfo("Generating...");
  CodeGen g;
  g.generate(ast.get());
  
  std::filesystem::path outDir = inputPath.parent_path();
  std::filesystem::path outCpp = outDir / "out.cpp";
  std::ofstream out(outCpp);
  out << g.str();
  logSuccess("Transpiled to: " + outCpp.string());
  return true;
}

std::string findInPath(const std::string& exeName) {
  std::string cmd = "where " + exeName + " 2>nul";
  FILE* pipe = _popen(cmd.c_str(), "r");
  if (!pipe) return "";
  char buffer[512];
  if (fgets(buffer, sizeof(buffer), pipe)) {
    _pclose(pipe);
    std::string path(buffer);
    size_t end = path.find('\n');
    if (end != std::string::npos) path = path.substr(0, end);
    if (!path.empty()) {
      size_t lastSlash = path.find_last_of("/\\");
      if (lastSlash != std::string::npos) {
        return path.substr(0, lastSlash);
      }
    }
  }
  _pclose(pipe);
  return "";
}

void checkCompilerExists(const std::filesystem::path& path, const std::string& exe, bool& found, std::string& comp) {
  if (found) return;
  std::filesystem::path test = path / exe;
  if (std::filesystem::exists(test)) {
    found = true;
    comp = test.string();
  }
}

std::string findCompiler() {
  std::filesystem::path exeDir = getExeDir();
  bool gccFound = false, clangFound = false;
  std::string gccPath, clangPath;
  
  checkCompilerExists(exeDir, "g++.exe", gccFound, gccPath);
  checkCompilerExists(exeDir / "bin", "g++.exe", gccFound, gccPath);
  checkCompilerExists(exeDir / "compiler", "g++.exe", gccFound, gccPath);
  
  checkCompilerExists(exeDir, "clang++.exe", clangFound, clangPath);
  checkCompilerExists(exeDir / "bin", "clang++.exe", clangFound, clangPath);
  checkCompilerExists(exeDir / "compiler", "clang++.exe", clangFound, clangPath);
  
  std::string sysClang = findInPath("clang++");
  std::string sysGcc = findInPath("g++");
  
  if (!sysClang.empty()) {
    logInfo("Using Clang++");
    return "clang++";
  }
  if (!sysGcc.empty()) {
    logInfo("Using GCC");
    return "g++";
  }
  if (clangFound) {
    logInfo("Using Clang++");
    return clangPath + "/clang++";
  }
  if (gccFound) {
    logInfo("Using GCC");
    return gccPath + "/g++";
  }
  
  logInfo("Using system g++");
  return "g++";
}

int compileOutput(const std::filesystem::path& workDir) {
  logInfo("Compiling...");
  std::filesystem::path outCpp = workDir / "out.cpp";
  std::string outExe = (workDir / "out").string();
#ifdef _WIN32
  outExe += ".exe";
#endif
  std::string compiler = findCompiler();
  std::string cmd = compiler + " -std=c++20 \"" + outCpp.string() + "\" -o \"" + outExe + "\"";
  return std::system(cmd.c_str());
}

int runOutput(const std::filesystem::path& exePath) {
  logInfo("Running...");
  std::string cmd = "\"" + exePath.string() + "\"";
  return std::system(cmd.c_str());
}
} // namespace

int main(int argc, char** argv){
  std::string command;
  std::string inputFile;

  if (argc < 2) {
    printUsage();
    return 1;
  }

  if (std::string(argv[1]) == "--version" || std::string(argv[1]) == "-v") {
    std::cout << "Syux Compiler v0.5.0\n";
    std::cout << "Transpiles Syux to C++20\n";
    return 0;
  }

  if (argc == 2) {
    inputFile = argv[1];
    command = "run";
  } else if (argc == 3) {
    command = argv[1];
    inputFile = argv[2];
  } else {
    printUsage();
    return 1;
  }

  if (!isSyuxFile(inputFile)) {
    logError("Input file must have .syux extension");
    return 1;
  }

  std::filesystem::path syuxPath = findSyuxFile(inputFile);
  if (!std::filesystem::exists(syuxPath)) {
    logError("File not found: " + inputFile);
    return 1;
  }

  std::filesystem::path originalDir = std::filesystem::current_path();
  std::filesystem::path workDir = syuxPath.parent_path();
  
  logInfo("Working directory: " + workDir.string());
  
  std::filesystem::current_path(workDir);

  if (command != "run" && command != "build" && command != "transpile") {
    logError("Unknown command: " + command);
    printUsage();
    std::filesystem::current_path(originalDir);
    return 1;
  }

  try {
    if (!transpileToCpp(syuxPath)) {
      std::filesystem::current_path(originalDir);
      return 1;
    }
  } catch (const std::exception& e) {
    logError(e.what());
    std::filesystem::current_path(originalDir);
    return 1;
  }

  if (command == "transpile") {
    std::filesystem::current_path(originalDir);
    return 0;
  }

  if (compileOutput(workDir) != 0) {
    logError("C++ compilation failed.");
    std::filesystem::current_path(originalDir);
    return 1;
  }

  if (command == "run") {
    std::filesystem::path outExe = workDir / "out";
#ifdef _WIN32
    outExe += ".exe";
#endif
    if (runOutput(outExe) != 0) {
      logError("Program execution failed.");
      std::filesystem::current_path(originalDir);
      return 1;
    }
  }

  logSuccess("Done!");
  std::filesystem::current_path(originalDir);
  return 0;
}