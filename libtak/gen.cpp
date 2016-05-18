#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>

/********************\
*            /   \   *
*            | N |   *
*  Computes  |   |   *
*            | K |   *
*            \   /   *
\********************/
int binomial(int N, int K) {
  int numerator = 1;
  if(K < N/2) K = N-K;
  for(int n = N; n > K; n--) {
    numerator *= n;
  }

  int denominator = 1;
  for(int k = N-K; k > 1; k--) {
    denominator *= k;
  }

  return numerator/denominator;
}

int main(int argc, char** argv) {
  std::ofstream header, source;
  header.open("include/table.hpp");
  source.open("table.cu");
  header << "#pragma once\n";
  header << "#include <iterator>\n";
  header << "#include <array>\n";
  header << "#include <algorithm>\n";
  header << "#include <type_traits>\n\n";
  source << "#include \"table.hpp\"\n";
  header << "#include \"util.hpp\"\n\n";
  header << "/**\n";
  header << " * THIS IS A GENERATED FILE, DO NOT MODIFY\n";
  header << " */\n";
  source << "/**\n";
  source << " * THIS IS A GENERATED FILE, DO NOT MODIFY\n";
  source << " */\n";
  header << "extern const int binomial_coeff_table[];\n";
  header << "#if defined(__CUDACC__)\n";
  header << "extern __constant__ int binomial_coeff_table_dev[];\n";
  header << "#endif\n";
  int BINOMIAL_SIZE = 9;
  for(int i = 0; i < 2; i++) {
    if(i == 0) source << "const int binomial_coeff_table[] = {\n";
    else {
      source << "#if defined(__CUDACC__)\n";
      source << "__constant__ int binomial_coeff_table_dev[] = {\n";
    }
    for(int n = 0; n < BINOMIAL_SIZE; n++) {
      for(int k = 0; k < BINOMIAL_SIZE; k++) {
        if(k <= n)
          source << binomial(n,k) << ", ";
        else 
          source << "0, ";
      }
      source << "\n";
    }
    source << "};\n";
    if(i==0) source << "\n";
    else source << "#endif\n\n";
  }
  header << "CUDA_CALLABLE inline int binomial_coeff(int n, int k) {\n";
  header << "#ifdef __CUDA_ARCH__\n";
  header << "  return binomial_coeff_table_dev[n*"<<BINOMIAL_SIZE<<"+k];\n";
  header << "#else\n";
  header << "  return binomial_coeff_table[n*"<<BINOMIAL_SIZE<<"+k];\n";
  header << "#endif\n";
  header << "}\n\n";

  header << "extern const int binomial_sum_table[];\n";
  header << "#if defined(__CUDACC__)\n";
  header << "extern __constant__ int binomial_sum_table_dev[];\n";
  header << "#endif\n";
  for(int i = 0; i < 2; i++) {
    if(i==0) source << "const int binomial_sum_table[] = {\n";
    else {
      source << "#if defined(__CUDACC__)\n";
      source << "__constant__ int binomial_sum_table_dev[] = {\n";
    }
    for(int n = 0; n < BINOMIAL_SIZE; n++) {
      for(int k = 0; k < BINOMIAL_SIZE; k++) {
        int c = 0;
        if(k <= n) {
          for(int ks = 0; ks <= k; ks++) {
            c += binomial(n,ks);
          }
          c -= 1;
        }
        source << c << ", ";
      }
      source << "\n";
    }
    source << "};\n";
    if(i==0) source << "\n";
    else source << "#endif\n\n";
  }
  header << "CUDA_CALLABLE inline int binomial_sum(int n, int k) {\n";
  header << "#ifdef __CUDA_ARCH__\n";
  header << "  return binomial_sum_table_dev[n*"<<BINOMIAL_SIZE<<"+k];\n";
  header << "#else\n";
  header << "  return binomial_sum_table[n*"<<BINOMIAL_SIZE<<"+k];\n";
  header << "#endif\n";
  header << "}\n\n";

  const int N = 8;
  std::vector<std::vector<int>> tables[N];
  tables[0].push_back(std::vector<int>({1}));


  for(int i = 1; i < N; i++) {
    for(auto row : tables[i-1]) {
      tables[i].emplace_back(row);
      auto& t = tables[i][tables[i].size()-1];
      t.insert(t.begin(), 1);
    }
    for(auto row : tables[i-1]) {
      tables[i].emplace_back(row);
      auto& t = tables[i][tables[i].size()-1];
      t[0]++;
      t.push_back(0);
    }
  }

  for(int i = 0; i < N; i++) {
    std::vector<std::vector<int>> copy;
    copy = tables[i];

    for(auto& row : copy) {
      row.insert(row.begin(), 0);
    }

    for(auto& row : tables[i]) {
      row.push_back(0);
    }

    tables[i].insert(tables[i].begin(), copy.begin(), copy.end());
    if(i==N-1)
      tables[i].erase(tables[i].begin());
    tables[i].erase(tables[i].end()-1);

    for(auto& row : tables[i]) {
      row.resize(N);
    }

    std::sort(tables[i].begin(), tables[i].end(), [](std::vector<int> left, std::vector<int> right) {
      if(left.size() != right.size()) return false;
      for(int i = left.size()-1; i >= 0; i--) {
        if(left[i] < right[i]) return true;
        if(left[i] == right[i]) continue;
        return false;
      }
      return false;
    });
  }

  for(int i = 0; i < N; i++) {
    for(auto& row : tables[i]) {
      int c = 0;
      for(int j = 0; j < row.size(); j++) { if(row[j]) { c = j; } }
      row[0] = c;
    }
  }

  header << "extern const int table_idxs[];\n";
  header << "extern const char table_values[];\n";
  header << "#if defined(__CUDACC__)\n";
  header << "extern __constant__ int table_idxs_dev[];\n";
  header << "extern __constant__ char table_values_dev[];\n";
  header << "#endif\n\n";

  header << "class Table {\n";
  header << "public:\n";
  header << "  class Index { \n";
  header << "  private:\n";
  header << "    friend class Table;\n";
  header << "    CUDA_CALLABLE Index(int idx) : idx(idx) {}\n";
  header << "    int idx;\n";
  header << "  };\n\n";

  header << "  CUDA_CALLABLE static inline int get_table_idxs(int n) {\n";
  header << "  #ifdef __CUDA_ARCH__\n";
  header << "    return table_idxs_dev[n];\n";
  header << "  #else\n";
  header << "    return table_idxs[n];\n";
  header << "  #endif\n";
  header << "  }\n\n";

  header << "  CUDA_CALLABLE static inline char value(Table::Index idx, int offset) {\n";
  header << "  #ifdef __CUDA_ARCH__\n";
  header << "    return table_values_dev[idx.idx+offset];\n";
  header << "  #else\n";
  header << "    return table_values[idx.idx+offset];\n";
  header << "  #endif\n";
  header << "  }\n\n";

  header << "  class iter : public std::iterator<std::input_iterator_tag, Index> {\n";
  header << "  private:\n";
  header << "    int idx;\n";
  header << "  public:\n";
  header << "    friend class Table;\n";
  header << "    CUDA_CALLABLE inline bool operator!=(iter other) { return idx != other.idx; }\n";
  header << "    CUDA_CALLABLE inline iter(int idx) : idx(idx) {}\n";
  header << "    CUDA_CALLABLE inline iter& operator++() {\n";
  header << "      idx++;\n";
  header << "      return *this;\n";
  header << "    }\n";
  header << "    CUDA_CALLABLE inline Index operator*() {\n";
  header << "      return Index(idx*8);\n";
  header << "    }\n";
  header << "  };\n\n";
  header << "  class range {\n";
  header << "  private:\n";
  header << "    CUDA_CALLABLE inline range(int begin_, int end_) : begin_(begin_), end_(end_) {}\n";
  header << "    int begin_;\n";
  header << "    int end_;\n";
  header << "  public:\n";
  header << "    friend class Table;\n";
  header << "    CUDA_CALLABLE inline iter begin() { return iter(begin_); }\n";
  header << "    CUDA_CALLABLE inline iter end() { return iter(end_); }\n";
  header << "    CUDA_CALLABLE inline int size() { return end_ - begin_; }\n";
  header << "  };\n\n";
  header << "  CUDA_CALLABLE static range moves(int stack_height, int max_move) {\n";
  header << "    int start = get_table_idxs(stack_height);\n";
  header << "    int moves = util::min(max_move&0x7F, stack_height);\n";
  header << "    int extra = (max_move&0x80) ? binomial_coeff(stack_height-1, moves) : 0;\n";
  header << "    return range(start, start+binomial_sum(stack_height, moves)+extra);\n";
  header << "  }\n";
  header << "};\n\n";

  for(int k = 0; k<2; k++) {
    if(k==0) source << "const int table_idxs[] = {\n";
    else     {
      source << "#if defined(__CUDACC__)\n";
      source << "__constant__ int table_idxs_dev[] = {\n";
    }
    source << "  ";
    int c = 0;
    source << c << ", ";
    for(int i = 0; i < N; i++) {
      source << c << ", ";
      c += tables[i].size();
    }
    source << c << ",\n";
    source << "};\n";

    if(k==0) {
      source << "\n";
    } else {
      source << "#endif\n\n";
    }
  }

  for(int k = 0; k<2; k++) {
    if(k==0) source << "const char table_values[] = {\n";
    else {
      source << "#if defined(__CUDACC__)\n";
      source << "__constant__ char table_values_dev[] = {\n";
    }
    for(int i = 0; i < N; i++) {
      for(auto row : tables[i]) {
        for(auto v : row) {
          source << v << ", ";
        }
        source << "\n";
      }
    }
    source << "};\n";

    if(k==0) {
      source << "\n";
    } else {
      source << "#endif";
    }
  }
  header.close();
  source.close();
}
