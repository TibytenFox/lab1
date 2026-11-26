#pragma once
#include <memory>
#include <stack>
#include <stdexcept>
#include <utility>


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
		std::stack<const Node*> st;
		st.push(root_.get());

		while (!st.empty()) {
			const Node* curr = st.top();
			st.pop();
			if (!curr) continue;
			res = f(curr->value, res);
			for (std::size_t i = 0; i < curr->count; ++i) {
				if (curr->children[i]) {
					st.push(curr->children[i].get());
				}
			}
		}
		return res;
	}

	using MapFunc = T (*)(const T&);
	Tree map(MapFunc f) const {
		if (!root_) return Tree(arity_);
		Tree res(f(root_->value), arity_);
		std::stack<std::pair<const Node*, Node*>> st;
		st.push(std::make_pair(root_.get(), res.root_.get()));

		while (!st.empty()) {
			std::pair<const Node*, Node*> curr = st.top();
			st.pop();
			if (!curr.first) continue;
			for (std::size_t i = 0; i < curr.first->count; ++i) {
				if (curr.first->children[i]) {
					Node* child = res.add_child(curr.second, f(curr.first->children[i]->value));
					st.push(std::make_pair(curr.first->children[i].get(), child));
				}
			}
		}
		return res;
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
		if (start_node == nullptr) return nullptr;
		std::stack<NodeType*> st;
		st.push(start_node);
		while (!st.empty()) {
			NodeType* curr = st.top();
			st.pop();
			if (!curr) continue;
			if (curr->value == val) return curr;
			for (std::size_t i = curr->count; i > 0; --i) {
				st.push(curr->children[i - 1].get());
			}
		}
		return nullptr;
	}

	static std::size_t count_subtree(const Node* start_node) {
		if (start_node == nullptr) return 0;
		std::size_t res = 0;
		std::stack<const Node*> st;
		st.push(start_node);
		while (!st.empty()) {
			const Node* curr = st.top();
			st.pop();
			if (!curr) continue;
			++res;
			for (std::size_t i = curr->count; i > 0; --i) {
				st.push(curr->children[i - 1].get());
			}
		}
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
};