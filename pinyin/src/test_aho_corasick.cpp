#include <cassert>
#include <iostream>
#include <memory>
#include <set>

#include "aho_corasick.hpp"

template <class V> void assert_eq(const V &a, const V &b) {
  assert(a == b && "assert_eq failed");
}

template <class K, class V>
std::set<V> all_values(const AhoCorasick<K, V> &ac, const K &key) {
  std::set<V> result;
  ac.for_all_values(ac.get_node(key), [&](const std::unique_ptr<V> &value) {
    result.insert(*value);
  });
  return result;
}

void test1() {
  AhoCorasick<std::string, u32> ac;
  ac.add("abc", 1);
  ac.add("bc", 2);
  ac.add("cd", 3);
  ac.add("bcd", 4);
  ac.build();

  assert_eq(all_values<std::string>(ac, "abc"), std::set<u32>{1, 2});
  assert_eq(all_values<std::string>(ac, "bc"), std::set<u32>{2});
  assert_eq(all_values<std::string>(ac, "bcd"), std::set<u32>{3, 4});

  std::cerr << "test1 passed\n";
}
void test2() {
  AhoCorasick<std::string, u32> ac;
  ac.add("a", 1);
  ac.add("ab", 2);
  ac.add("bc", 3);
  ac.add("c", 4);
  ac.build();

  assert_eq(all_values<std::string>(ac, "abc"), std::set<u32>{3, 4});

  std::cerr << "test2 passed\n";
}

int main() {
  test1();
  test2();
}