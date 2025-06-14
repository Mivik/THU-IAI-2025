#pragma once

#include <map>
#include <memory>
#include <queue>
#include <vector>

#include "common.hpp"

const u32 INVALID_NODE = -1;

/**
 * Aho-Corasick implementation.
 */
template <class K, class V> class AhoCorasick {
public:
  using E = typename K::value_type;

  DISABLE_COPY(AhoCorasick);

  AhoCorasick() {
    nodes.emplace_back();
    nodes[0].fail = -1;
  }

  /**
   * Transits a state with an entry.
   */
  u32 transit(u32 node, const E &e) const {
    while (node != INVALID_NODE) {
      auto it = nodes[node].children.find(e);
      if (it != nodes[node].children.end())
        return it->second;
      node = nodes[node].fail;
    }
    return 0;
  }

  /**
   * Transits a state with a key (sequence of entries).
   */
  u32 transit_all(u32 node, const K &key) const {
    for (auto &e : key) {
      node = transit(node, e);
    }
    return node;
  }

  u32 get_node(const K &key) const { return transit_all(0, key); }

  /**
   * Inserts a key-value pair into the automaton.
   */
  std::unique_ptr<V> &insert(const K &key) {
    u32 node = 0;
    for (auto &e : key) {
      auto it = nodes[node].children.find(e);
      if (it != nodes[node].children.end()) {
        node = it->second;
      } else {
        nodes[node].children[e] = nodes.size();
        node = nodes.size();
        nodes.emplace_back();
      }
    }
    return nodes[node].value;
  }
  template <class... Args> void add(const K &key, Args &&...args) {
    insert(key) = std::unique_ptr<V>(new V(std::forward<Args>(args)...));
  }

  /**
   * Builds the automaton.
   *
   * Should be called after all insertions.
   */
  void build() {
    std::queue<u32> q;
    q.push(0);
    while (!q.empty()) {
      auto node = q.front();
      q.pop();
      for (auto &p : nodes[node].children) {
        auto child = p.second;
        nodes[child].fail = transit(nodes[node].fail, p.first);
        q.push(child);
      }
    }
  }

  /**
   * Gets the value associated with a node.
   */
  const std::unique_ptr<V> &get(u32 node) const { return nodes[node].value; }
  std::unique_ptr<V> &get(u32 node) { return nodes[node].value; }

  /**
   * For all values associated with a node, invoke the given function.
   *
   * This looks up all values associated with a node and its ancestors.
   */
  template <class F> void for_all_values(u32 node, F &&f) const {
    while (node != INVALID_NODE) {
      if (nodes[node].value) {
        f(nodes[node].value);
      }
      node = nodes[node].fail;
    }
  }

private:
  struct Node {
    std::map<E, u32> children;
    std::unique_ptr<V> value;
    u32 fail;

    Node() : fail(0) {}
  };

  std::vector<Node> nodes;
};
