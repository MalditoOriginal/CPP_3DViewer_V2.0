#include "file_parser.h"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace s21 {
ObjParser &ObjParser::SingleParser() {
  static ObjParser TheObjParser;
  return TheObjParser;
}

Status ObjParser::ParseFile(const std::string &file_name) {
  ClearData();
  std::ifstream file(file_name);
  if (!file.is_open()) return kErrorFileMissing;
  Status status = kOk;
  status = ReserveData(file);
  std::string current_line;
  while (std::getline(file, current_line) && status == kOk) {
    status = ParseLine(current_line);
  }
  vectors_set_.clear();
  status = vertices_.empty() || edges_.empty() ? kErrorFileEmpty : status;
  if (status != kOk) ClearData();
  file.close();
  return status;
}

void ObjParser::ClearData() {
  vectors_set_.clear();
  vertices_.clear();
  edges_.clear();
  vertices_count_ = 0;
  x_coef_ = y_coef_ = z_coef_ = -10;  // The lowest possible value.
}

std::vector<double> ObjParser::GetVertex() { return vertices_; }

std::vector<unsigned> ObjParser::GetEdges() { return edges_; }

float ObjParser::GetNormalizeCoef() {
  return (x_coef_ + y_coef_ + z_coef_) / 3;
}

Status ObjParser::ReserveData(std::ifstream &file) {
  size_t vertex_count = 0, edge_count = 0;
  std::string current_line;
  while (std::getline(file, current_line)) {
    if (current_line.length() <= 2) continue;
    std::istringstream iss(current_line);
    std::string type;
    iss >> type;
    if (type == "v") {
      vertex_count += 3;
    } else if (type == "f") {
      std::string edges_line = current_line.substr(2);
      std::istringstream iss_f(edges_line);
      std::string index_str;
      while (iss_f >> index_str) edge_count += 2;
    }
  }
  if (vertex_count > 0 && edge_count > 0) {
    vertices_.reserve(vertex_count);
    edges_.reserve(edge_count);
  } else
    return kErrorFileEmpty;
  file.clear();
  file.seekg(0, std::ios::beg);
  return kOk;
}

Status ObjParser::ParseLine(const std::string &line) {
  Status status = kOk;
  if (line.length() > 2) {
    std::istringstream iss(line);
    std::string type;
    iss >> type;
    if (type == "v") {
      std::string data = line.substr(2);
      status = ParseVertex(data);
    } else if (type == "f") {
      std::string data = line.substr(2);
      status = ParseFace(data);
    }
  }
  return status;
}

Status ObjParser::ParseVertex(const std::string &data) {
  std::istringstream iss(data);
  double x, y, z;
  if (iss >> x >> y >> z) {
    vertices_.push_back(x);
    vertices_.push_back(y);
    vertices_.push_back(z);
    if (abs(x) > x_coef_) x_coef_ = abs(x);
    if (abs(y) > y_coef_) y_coef_ = abs(y);
    if (abs(z) > z_coef_) z_coef_ = abs(z);
    ++vertices_count_;
  } else {
    ClearData();
    return kErrorIncorrectFile;
  }
  return kOk;
}

Status ObjParser::ParseFace(const std::string &data) {
  std::istringstream iss(data);
  std::string index_str;
  long first_index;
  bool is_first_index = true;
  std::vector<unsigned int> faces_buff;
  while (iss >> index_str) {
    long index;
    try {
      index = std::stoi(index_str);
    } catch (const std::out_of_range &e) {
      return kErrorIncorrectFile;
    }
    if (std::abs(index) > static_cast<long>(vertices_.size()))
      return kErrorIncorrectFile;
    if (index < 0)
      index += vertices_count_;
    else
      index -= 1;
    if (is_first_index) {
      first_index = index;
      faces_buff.push_back(index);
      is_first_index = false;
    } else {
      faces_buff.push_back(index);
      faces_buff.push_back(index);
    }
  }
  faces_buff.push_back(first_index);
  InsertUniqueVector(faces_buff);
  return kOk;
}

void ObjParser::InsertUniqueVector(const std::vector<unsigned> &data) {
  for (auto it = data.begin(); it != data.end() && data.size() % 2 == 0;
       it += 2) {
    std::pair<unsigned, unsigned> vector(*it, *(it + 1));
    std::pair<unsigned, unsigned> inverse_vector(*(it + 1), *it);
    if (vectors_set_.find(vector) == vectors_set_.end() &&
        vectors_set_.find(inverse_vector) == vectors_set_.end()) {
      edges_.push_back(*it);
      edges_.push_back(*(it + 1));
      vectors_set_.insert(vector);
    }
  }
}
}  // namespace s21
