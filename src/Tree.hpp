#pragma once
#include <memory>
#include <stack>
#include <stdexcept>


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
		if (node->parent) {
			size_ -= count_subtree(node);
			Node* parent = node->parent;
			for (std::size_t i = 0; i < parent->count; ++i) {
				if (parent->children[i].get() == node) {
					if (i == parent->count - 1) {
						parent->children[parent->count - 1].reset();
						break;
					}
					for (std::size_t j = i; j < parent->count - 1; ++j) {
						parent->children[j] = std::move(parent->children[j + 1]);
					}
					break;
				}
			}
			--parent->count;
		} else { 
			root_.reset(); 
			size_ = 0;
		}
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

	std::size_t count_subtree(const Node* start_node) const {
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
};