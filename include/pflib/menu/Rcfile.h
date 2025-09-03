#ifndef Rcfile_h_included_
#define Rcfile_h_included_

#include <string>
#include <vector>

namespace YAML {
class Node;
}

namespace pflib::menu {

class Rcmap {
 public:
  Rcmap();
  ~Rcmap();

  void addFile(YAML::Node*);

  /** Does this map have the given key ? */
  bool has_key(const std::string& keyname) const;

  /** Is this key a vector? */
  bool is_vector(const std::string& keyname) const;

  /** Is this key a submap? */
  bool is_map(const std::string& keyname) const;

  /** get item as map */
  Rcmap getMap(const std::string& keyname) const;

  /** get item as vector of strings */
  std::vector<std::string> getVString(const std::string& keyname) const;

  /** get item as vector of bools */
  std::vector<bool> getVBool(const std::string& keyname) const;

  /** get item as vector of ints */
  std::vector<int> getVInt(const std::string& keyname) const;

  /** get item as scalar string */
  std::string getString(const std::string& keyname) const;

  /** get item as scalar bool */
  bool getBool(const std::string& keyname) const;

  /** get item as scalar int */
  int getInt(const std::string& keyname) const;

 protected:
  std::vector<YAML::Node*> nodes_;
};

/** YAML-based RC file */
class Rcfile {
 public:
  Rcfile();

  /// first file takes priority
  void load(const std::string& file);

  Rcmap& contents() { return contents_; }
  void help();

  void declareVString(const std::string& key, const std::string& description);
  void declareString(const std::string& key, const std::string& description);
  void declareVBool(const std::string& key, const std::string& description);
  void declareBool(const std::string& key, const std::string& description);
  void declareVInt(const std::string& key, const std::string& description);
  void declareInt(const std::string& key, const std::string& description);

 private:
  struct HelpInfo {
    std::string key, description, type;
  };
  Rcmap contents_;
  std::vector<HelpInfo> helpInfo_;
};

}  // namespace pflib::menu

#endif  // Rcfile_h_included_
