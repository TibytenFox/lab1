#pragma once
#include <iostream>
#include <memory>
#include <stack>
#include <stdexcept>
#include <utility>
#include <vector>


template <class T>
class Tree {
public:
	struct Node {
			T value;
			std::unique_ptr<std::unique_ptr<Node>[]> children;
			Node* parent;
			std::size_t count;

			Node(const T& val, std::size_t arity, Node* parent_node = nullptr) 
				: value(val)
				, children(std::make_unique<std::unique_ptr<Node>[]>(arity))
				, parent(parent_node)
				, count(0) {}
		};

public:
	explicit Tree(std::size_t arity) : arity_(arity), root_(nullptr), size_(0) {
		if (arity == 0) throw std::invalid_argument("Arity must be greater than 0");
	}

	Tree(const T& val, std::size_t arity) : arity_(arity), root_(std::make_unique<Node>(val, arity)), size_(1) {
		if (arity == 0) throw std::invalid_argument("Arity must be greater than 0");
	}

	~Tree() {
		clear();
	}

	Tree(Tree&&other) noexcept 
		: arity_(other.arity_)
		, root_(std::move(other.root_))
		, size_(other.size_) {
		other.size_ = 0;
	}

	Tree& operator=(Tree&&other) noexcept {
		if (this != &other) {
			clear();
			arity_ = other.arity_;
			root_ = std::move(other.root_);
			size_ = other.size_;
			other.size_ = 0;
		}
		return *this;
	}

	Node* set_root(const T& val) {
		std::unique_ptr<Node> new_root = std::make_unique<Node>(val, arity_);
		if (root_) {
			for (std::size_t ind = 0; ind < root_->count; ++ind) {
				root_->children[ind]->parent = new_root.get();
				new_root->children[ind] = std::move(root_->children[ind]);
			}
			new_root->count = root_->count;
		} else {
			++size_;
		}
		root_ = std::move(new_root);
		return root_.get();
	}

	Node* add_child(Node* parent, const T& val) {
		if (parent == nullptr || parent->count == arity_) return nullptr; 
		parent->children[parent->count++] = std::make_unique<Node>(val, arity_, parent); 
		++size_;
		return parent->children[parent->count - 1].get();
	}

	Node* find(const T& val) {
		return find_impl<Node>(root_.get(), val);
	}
	const Node* find(const T& val) const {
		return find_impl<const Node>(root_.get(), val);
	}

	Node* find_by_path(const std::vector<T>& path) {
		if (path.empty()) throw std::invalid_argument("Path cannot be empty");
		if (!root_) return nullptr;
		if (path[0] != root_->value) return nullptr;

		return walk_path_impl<Node>(root_.get(), path, 1);
	}

	const Node* find_by_path(const std::vector<T>& path) const {
		if (path.empty()) throw std::invalid_argument("Path cannot be empty");
		if (!root_) return nullptr;
		if (path[0] != root_->value) return nullptr;

		return walk_path_impl<const Node>(root_.get(), path, 1);
	}

	Node* find_by_relative_path(Node* start_node, const std::vector<T>& path) {
		return walk_path_impl<Node>(start_node, path, 0);
	}

	const Node* find_by_relative_path(const Node* start_node, const std::vector<T>& path) const  {
		return walk_path_impl<const Node>(start_node, path, 0);
	}

	bool contains_subtree(const Tree& sub) const {
		if (sub.empty()) return true;
		if (empty() || size_ < sub.size_) return false;

		bool found = false;
		traverse_dfs(root_.get(), [&](const Node* curr) {
			if (curr->value == sub.root()->value && is_same_subtree(curr, sub.root())) {
				found = true;
				return false; // поиск завершён
			}
			return true; // поиск продолжается
		});
		return found;
	}

	void remove_subtree(Node* node) {
		if (node == nullptr) return;
		if (node == root_.get()) {
			clear();
			return;
		}
		if (node->parent) {
			size_ -= count_subtree(node);
			Node* parent = node->parent;
			std::unique_ptr<Node> to_delete;

			for (std::size_t i = 0; i < parent->count; ++i) {
				if (parent->children[i].get() == node) {
					to_delete = std::move(parent->children[i]);
					for (std::size_t j = i; j < parent->count - 1; ++j) {
						parent->children[j] = std::move(parent->children[j + 1]);
					}
					--parent->count;
					break;
				}
			}
			destroy_subtree(std::move(to_delete));
		}
	}

	using ReduceFunc = T (*)(const T&, const T&);
	T reduce(ReduceFunc f, T init) const {
		T res = init;
		traverse_dfs(root_.get(), [&](const Node* curr) {
			res = f(curr->value, res);
			return true;
		});
		return res;	
	}

	void save_fixed(std::ostream& out) const {
		if (!root_) return;
		bool first = true;

		traverse_dfs(root_.get(), [&](const Node* curr) {
			if (!first) out << ' ';
			first = false;
			out << curr->value << ' ' << curr->count;
			return true;
		});
	}

	static Tree load_fixed(std::istream& in, std::size_t arity) {
		Tree res(arity);
		T val{};
		std::size_t cnt = 0;

		if (!(in >> val)) {
			return res;
		}
		if (!(in >> cnt) || cnt > arity) {
			throw std::invalid_argument("Invalid root children count in stream");
		}

		res.set_root(val);
		std::stack<std::pair<Node*, std::size_t>> st;
		if (cnt > 0) {
			st.push(std::make_pair(res.root(), cnt));
		}

		while (in >> val) {
			if (st.empty()) {
				throw std::invalid_argument("Extra nodes in input stream");
			}
			if (!(in >> cnt) || cnt > arity) {
				throw std::invalid_argument("Invalid children count in input stream");
			}

			Node* parent = st.top().first;
			Node* child = res.add_child(parent, val);
			--st.top().second;

			if (st.top().second == 0) {
				st.pop();
			}
			if (cnt > 0) {
				st.push(std::make_pair(child, cnt));
			}
		}

		if (!in.eof()) {
			throw std::invalid_argument("Invalid children count in input stream");
		}
		if (!st.empty()) {
			throw std::invalid_argument("Unexpected end of input stream");
		}

		return res;
	}

	using MapFunc = T (*)(const T&);
	Tree map(MapFunc f) const {
		return clone_subtree(root_.get(), arity_, f);
	}

	Tree extract_subtree(const T& val) const {
		return clone_subtree(find(val), arity_, [](const T& x) { return x; });
	}

	bool empty() const {
		return size_ == 0;
	}

	std::size_t size() const {
		return size_;
	}

	Node* root() {
		return root_.get();
	}

	const Node* root() const {
		return root_.get();
	}

	void clear() noexcept {
		destroy_subtree(std::move(root_));
		size_ = 0;
	}
private:	
	std::size_t arity_;
	std::unique_ptr<Node> root_;
	std::size_t size_;

	template <class NodeType>
	static NodeType* find_impl(NodeType* start_node, const T& val) {
		NodeType* found = nullptr;
		traverse_dfs(start_node, [&](NodeType* curr) {
			if (curr->value == val) {
				found = curr;
				return false; // нашли - останавливаем обход
			}
			return true;
		});
		return found;
	}

	static std::size_t count_subtree(const Node* start_node) {
		std::size_t res = 0;
		traverse_dfs(start_node, [&](const Node*) {
			++res;
			return true;
		});
		return res;
	}

	static void destroy_subtree(std::unique_ptr<Node> subroot) noexcept {
		if (!subroot) return;
		std::stack<std::unique_ptr<Node>> st;
		st.push(std::move(subroot));

		while (!st.empty()) {
			std::unique_ptr<Node> curr = std::move(st.top());
			st.pop();

			for (std::size_t i = 0; i < curr->count; ++i) {
				if (curr->children[i]) {
					st.push(std::move(curr->children[i]));
				}
			}
			// curr выйдет из области видимости, при этом children уже будут пустые, поэтому без проблем удалится
		}
	} 

	template <class Func>
	static Tree clone_subtree(const Node* start_node, std::size_t arity, Func transform) {
		if (!start_node) return Tree(arity);
		Tree res(transform(start_node->value), arity);

		std::stack<std::pair<const Node*, Node*>> st;
		st.push({start_node, res.root_.get()});

		while (!st.empty()) {
			auto [src, dst] = st.top();
			st.pop();
			for (std::size_t i = 0; i < src->count; ++i) {
				if (src->children[i]) {
					Node* child = res.add_child(dst, transform(src->children[i]->value));
					st.push({src->children[i].get(), child});
				}
			}
		}
		return res;
	}

	template <class NodeType, class Func>
	static void traverse_dfs(NodeType* start_node, Func visitor) {
		if (!start_node) return;
		std::stack<NodeType*> st;
		st.push(start_node);

		while (!st.empty()) {
			NodeType* curr = st.top();
			st.pop();
			if (!curr) continue;
			if (!visitor(curr)) return; // досрочный выход (для find / contains_subtree)

			for (std::size_t i = curr->count; i > 0; --i) {
				if (curr->children[i - 1]) {
					st.push(curr->children[i - 1].get());
				}
			}
		}
	}

	static bool is_same_subtree(const Node* a, const Node* b) {
		std::stack<std::pair<const Node*, const Node*>> st;
		st.push({a, b});

		while (!st.empty()) {
			auto [curr_a, curr_b] = st.top();
			st.pop();

			if (!curr_a && !curr_b) continue;
			if (!curr_a || !curr_b) return false;
			if (curr_a->value != curr_b->value || curr_a->count != curr_b->count) {
				return false;
			}

			for (std::size_t i = 0; i < curr_a->count; ++i) {
				st.push({curr_a->children[i].get(), curr_b->children[i].get()});
			}
		}
		return true;
	}

	template <class NodeType>
	static NodeType* walk_path_impl(NodeType* start_node, const std::vector<T>& path, std::size_t start_index) {
		if (path.empty()) throw std::invalid_argument("Path cannot be empty");
		if (start_node == nullptr) throw std::invalid_argument("Start node cannot be null");

		bool found;
		NodeType* curr = start_node;
		for (std::size_t i = start_index; i < path.size(); ++i) {
			found = false;
			for (std::size_t j = 0; j < curr->count; ++j) {
				if (curr->children[j]) {
					if (path[i] == curr->children[j]->value) {
						found = true;
						curr = curr->children[j].get();
						break;
					}
				}
			}
			if (!found) return nullptr;
		}
		return curr;
	}
};