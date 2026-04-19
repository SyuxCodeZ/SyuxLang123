#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <algorithm>
#include <stdexcept>

using namespace std;

struct SyuxMath {
  static double abs(double x) { return x < 0 ? -x : x; }
  static double round(double x) { return floor(x + 0.5); }
  static double random() { return (double)rand() / RAND_MAX; }
  static double max(double a, double b) { return a > b ? a : b; }
  static double min(double a, double b) { return a < b ? a : b; }
  static double pow(double base, double exp) { return std::pow(base, exp); }
  static double sqrt(double x) { return std::sqrt(x); }
  static double floor(double x) { return std::floor(x); }
  static double ceil(double x) { return std::ceil(x); }
  static double sin(double x) { return std::sin(x); }
  static double cos(double x) { return std::cos(x); }
  static double tan(double x) { return std::tan(x); }
  static double log(double x) { return std::log(x); }
  static double log10(double x) { return std::log10(x); }
  static double exp(double x) { return std::exp(x); }
};

struct SyuxString {
  static string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
  }
  static string toUpper(const string& s) {
    string r = s; transform(r.begin(), r.end(), r.begin(), ::toupper); return r;
  }
  static string toLower(const string& s) {
    string r = s; transform(r.begin(), r.end(), r.begin(), ::tolower); return r;
  }
  static vector<string> split(const string& s, const string& delim) {
    vector<string> r; size_t start = 0;
    while (true) {
      size_t pos = s.find(delim, start);
    if (pos == string::npos) { r.push_back(s.substr(start)); break; }
      r.push_back(s.substr(start, pos - start));
      start = pos + delim.size();
    }
    return r;
  }
  static string join(const vector<string>& v, const string& delim) {
    string r; for (size_t i = 0; i < v.size(); ++i) {
      r += v[i]; if (i < v.size() - 1) r += delim;
    }
    return r;
  }
  static string replace(const string& s, const string& from, const string& to) {
    string r = s; size_t pos = 0;
    while ((pos = r.find(from, pos)) != string::npos) {
      r.replace(pos, from.size(), to);
      pos += to.size();
    }
    return r;
  }
  static int indexOf(const string& s, const string& sub) {
    return (int)s.find(sub);
  }
  static bool contains(const string& s, const string& sub) {
    return s.find(sub) != string::npos;
  }
  static string substring(const string& s, int start, int end) {
    return s.substr(start, end - start);
  }
  static bool startsWith(const string& s, const string& prefix) {
    return s.rfind(prefix, 0) == 0;
  }
  static bool endsWith(const string& s, const string& suffix) {
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
  }
};

struct console {
  template<typename T> static void log(T v) { cout << v << endl; }
  template<typename T> static void error(T v) { cerr << v << endl; }
  template<typename T> static void warn(T v) { cerr << "WARNING: " << v << endl; }
  template<typename T> static void info(T v) { cout << "INFO: " << v << endl; }
};

auto add(auto a, auto b) {
  return (a + b);
}

int x = 10;
int main() {  std::cout.setf(std::ios::boolalpha);
  if ((x > 5)) {
    std::cout << "big" << std::endl;
  }
  std::cout << x << std::endl;
  return 0;
}
